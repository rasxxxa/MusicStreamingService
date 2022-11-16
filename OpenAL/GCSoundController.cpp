#include "GCSoundController.h"
#include <iostream>
#pragma comment(lib, "../lib/OpenAL32.lib")

#ifdef _MHD_UNIT_TESTS
std::unique_ptr<ISoundController> CSoundController::g_testingInstance = nullptr;
#endif
std::recursive_mutex CSoundController::csSoundLock;
ISoundController::SoundStatusChangeCallback CSoundController::m_statusChangeCallback;
std::unordered_map<ALuint, bool> CSoundController::m_shouldUnqueue;

constexpr const char* GetErrorMessage(const uint& error)
{
  switch (error)
  {
    case AL_INVALID_NAME:
      return "AL_INVALID_NAME";
    case AL_INVALID_ENUM:
      return "AL_INVALID_ENUM";
    case AL_INVALID_VALUE:
      return "AL_INVALID_VALUE";
    case AL_INVALID_OPERATION:
      return "AL_INVALID_OPERATION";
    case AL_OUT_OF_MEMORY:
      return "AL_OUT_OF_MEMORY";
    default:
      return "UNKNOWN_AL_ERROR";
  }
}



bool GetALError(uint& error)
{
  error = alGetError();
  return error != AL_NO_ERROR;
}



CSoundController::CSoundController() : m_alcDevice(nullptr), m_alcContext(nullptr), m_initialized(false)
{
  m_alcDevice = alcOpenDevice(nullptr);
  if (!m_alcDevice)
  {
    return;
  }

  m_alcContext = alcCreateContext(m_alcDevice, nullptr);
  if (!m_alcContext || alcMakeContextCurrent(m_alcContext) == ALC_FALSE)
  {
    if (m_alcContext)
      alcDestroyContext(m_alcContext);

    alcCloseDevice(m_alcDevice);
    return;
  }
  RegisterSoundStatusChangeCallback(nullptr);
  m_initialized = true;
}



CSoundController::~CSoundController()
{
  for (auto& sources : m_bufferWithSources)
  {
    for (auto& source : sources.second)
    {
      DeleteSource(source.second, false);
    }
    if (!sources.second.empty())
    {
      DeleteBuffer((*(sources.second.begin())).second, false);
    }
  }

  if (!m_alcContext)
    return;

  alcMakeContextCurrent(nullptr);
  alcDestroyContext(m_alcContext);
  alcCloseDevice(m_alcDevice);
  m_initialized = false;
}

void CSoundController::RegisterSoundStatusChangeCallback(SoundStatusChangeCallback callback)
{
  {
    std::lock_guard lock(csSoundLock);
    m_statusChangeCallback = callback;
  }

  if (!alIsExtensionPresent("AL_SOFT_EVENTS"))
  {
    return;
  }

  std::vector<ALenum> evt_types = { AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT };
  auto alEventCallbackSOFT = reinterpret_cast<LPALEVENTCALLBACKSOFT>(alGetProcAddress("alEventCallbackSOFT"));
  auto alEventControlSOFT = reinterpret_cast<LPALEVENTCONTROLSOFT>(alGetProcAddress("alEventControlSOFT"));
  alEventControlSOFT(evt_types.size(), evt_types.data(), AL_TRUE);
  alEventCallbackSOFT(EventCallBack, nullptr);
}


void AL_APIENTRY CSoundController::EventCallBack(ALenum eventType, ALuint object, ALuint param, ALsizei length, const ALchar* message, void* userParam)
{
  SoundStatusChangeCallback callback = nullptr;
  {
    std::lock_guard lock(csSoundLock);
    callback = m_statusChangeCallback;
  }

  ALint unqueueBuffer = 0;
  alGetSourcei(object, AL_BUFFERS_PROCESSED, &unqueueBuffer);
  if (unqueueBuffer > 0)
  {
      while (unqueueBuffer > 0)
      {
          ALuint bufferUnqueued;
          alSourceUnqueueBuffers(object, 1, &bufferUnqueued);
          m_shouldUnqueue[bufferUnqueued] = false;
          unqueueBuffer--;
      }
  }

  alSourcePlay(object);

  if (!callback)
    return;

  if (param == AL_STOPPED)
  {
    callback(object, StatusChange::Stopped);
  }
  else if (param == AL_PAUSED)
  {
    callback(object, StatusChange::Paused);
  }
}

ISoundController& CSoundController::Get()
{
#ifdef _MHD_UNIT_TESTS
  if (g_testingInstance)
    return *g_testingInstance;
#endif
  static CSoundController instance{};
  return instance;
}



void CSoundController::DeleteSource(const SoundInfo& soundInfo, bool deleteFromList)
{
  if (!m_initialized)
    return;

  if (alIsSource(soundInfo.source))
  {
    alDeleteSources(1, &soundInfo.source);
    if (LogIfOpenALError("Could not delete source", soundInfo))
    {
      return;
    }
    if (deleteFromList)
    {
      std::unique_lock<std::recursive_mutex> lock(csSoundLock);
      auto soundInfoCopy = soundInfo;
      m_bufferWithSources[soundInfo.buffer].erase(soundInfo.source);
      if (m_bufferWithSources[soundInfo.buffer].empty())
        DeleteBuffer(soundInfoCopy);
    }
  }
}



void CSoundController::DeleteBuffer(const SoundInfo& soundInfo, bool deleteFromList)
{
  if (!m_initialized)
    return;

  if (alIsBuffer(soundInfo.buffer))
  {
    alDeleteBuffers(1, &soundInfo.buffer);
    if (LogIfOpenALError("Could not delete buffer", soundInfo))
    {
      //ASSERT(false);
      return;
    }

    if (deleteFromList)
    {
      std::unique_lock<std::recursive_mutex> lock(csSoundLock);
      m_bufferWithSources.erase(soundInfo.buffer);
    }
  }
}



int CSoundController::UnqueueBuffer(const ALuint& source)
{
    int returnResult = 0;
    ALint unqueueBuffer = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &unqueueBuffer);
    if (unqueueBuffer > 0)
    {
        while (unqueueBuffer > 0)
        {
            ALuint bufferUnqueued;
            alSourceUnqueueBuffers(source, 1, &bufferUnqueued);
            m_shouldUnqueue[bufferUnqueued] = false;
            returnResult = bufferUnqueued;
            unqueueBuffer--;
        }
    }
    return returnResult;
}

void CSoundController::CreateBuffer(ALuint& alBuffer, SoundInfo& soundInfo)
{
  if (!m_initialized)
    return;

  alGenBuffers(1, &alBuffer);
  if (LogIfOpenALError("Could not create buffer", soundInfo))
  {
    //ASSERT(false);
    return;
  }
  soundInfo.buffer = alBuffer;
}



void CSoundController::CreateSource(ALuint& alSource, SoundInfo& soundInfo)
{
  if (!m_initialized)
    return;

  alGenSources(1, &alSource);

  if (LogIfOpenALError("Could not create source", soundInfo))
  {
    //ASSERT(false);
    return;
  }

#ifndef OPEN_AL_VIRTUALIZATION
  alSourcei(alSource, AL_DIRECT_CHANNELS_SOFT, AL_REMIX_UNMATCHED_SOFT);

  if (LogIfOpenALError("Could not create source", soundInfo))
  {
    //ASSERT(false);
    return;
  }

#endif

  soundInfo.source = alSource;
  std::unique_lock<std::recursive_mutex> lock(csSoundLock);
  m_bufferWithSources[soundInfo.buffer][alSource] = soundInfo;
}



bool CSoundController::LogIfOpenALError(const char* message, const SoundInfo& soundInfo) const
{
  if (!m_initialized)
    return false;

  uint error = 0; 
  if (GetALError(error))
  {
      std::string soundFullPath = soundInfo.soundPath + "\\" + soundInfo.soundName;
      std::cout << GetErrorMessage(error) << " " << soundFullPath << std::endl;
    //TRACEE("ERROR: %s, CAUSE OF ASSERTION: %s, SOUND: %s", message, GetErrorMessage(error), soundFullPath.c_str());
    return true;
  }
  return false;
}



void CSoundController::StopSound(const SoundInfo& soundInfo)
{
  if (!m_initialized)
    return;

  if (alIsBuffer(soundInfo.buffer))
  {
      std::lock_guard<std::recursive_mutex> lock(csSoundLock);
      std::for_each(m_bufferWithSources[soundInfo.buffer].begin(), m_bufferWithSources[soundInfo.buffer].end(),
          [&](const auto& sound)
          {
              alSourcePause(sound.second.source);
              if (LogIfOpenALError("Problem while trying to stop sound with source", sound.second))
              {
                  return;
              }
          });
  }
}



void CSoundController::PlaySound(const SoundInfo& soundInfo, const bool bIsLooping, const bool bStop, const bool bResetPosition)
{
  if (!m_initialized)
    return;

  if (!alIsSource(soundInfo.source))
  {
    //TRACEE("ERROR: wrong source inserted, for sound %s", (soundInfo.soundPath + "\\" + soundInfo.soundName).c_str());
    return;
  }

  if (bStop)
    StopSound(soundInfo);

  if (bResetPosition)
  {
    alSourceRewind(soundInfo.source);
    if (LogIfOpenALError("Could not reset position of source", soundInfo))
    {
      //ASSERT(false);
    }
  }

  if (bIsLooping)
  {
    alSourcei(soundInfo.source, AL_LOOPING, AL_TRUE);
    if (LogIfOpenALError("Could not set looping of source", soundInfo))
    {
      //ASSERT(false);
    }
  }

  if (!IsSourcePlaying(soundInfo))
  {
    alSourcePlay(soundInfo.source);
  }

  if (LogIfOpenALError("Could not play source", soundInfo))
  {
    //ASSERT(false);
  }
}



void CSoundController::RewindSource(const SoundInfo& soundInfo)
{
  if (!m_initialized)
    return;

  if (alIsSource(soundInfo.source))
  {
    alSourceRewind(soundInfo.source);
    if (LogIfOpenALError("Could not reset position of source", soundInfo))
    {
      //ASSERT(false);
    }
  }
}



bool CSoundController::IsSourcePlaying(const SoundInfo& soundInfo) const
{
  if (!m_initialized)
    return false;

  if (alIsSource(soundInfo.source))
  {
    ALint state;
    alGetSourcei(soundInfo.source, AL_SOURCE_STATE, &state);
    if (LogIfOpenALError("Could not check state of source", soundInfo))
    {
      //ASSERT(false);
      return false;
    }

    if ((state == AL_PLAYING))
      return true;
  }
  return false;
}



void CSoundController::CreateSource(SoundInfo& info)
{
    alGenSources(1, &info.source);
}



void CSoundController::SetSourceVolume(const SoundInfo& soundInfo, const ALfloat alfVolume)
{
  if (!m_initialized)
    return;

  if (alIsSource(soundInfo.source))
  {
    alSourcef(soundInfo.source, AL_GAIN, alfVolume);
    if (LogIfOpenALError("Could not set volume of source", soundInfo))
    {
      //ASSERT(false);
    }
  }
}



bool CSoundController::IsSoundDataLooping(const SoundInfo& soundInfo) const
{
  if (!m_initialized)
    return false;

  if (alIsSource(soundInfo.source))
  {
    ALint state;
    alGetSourcei(soundInfo.source, AL_SOURCE_STATE, &state);
    if (LogIfOpenALError("Could not check state of source", soundInfo))
    {
      //ASSERT(false);
      return false;
    }

    if (state == AL_LOOPING)
      return true;
  }
  return false;
}



void CSoundController::BreakLoop(const SoundInfo& soundInfo)
{
  if (!m_initialized)
    return;

  if (alIsSource(soundInfo.source))
  {
    alSourcef(soundInfo.source, AL_LOOPING, AL_FALSE);
    if (LogIfOpenALError("Could not check state of source", soundInfo))
    {
      //ASSERT(false);
    }
    alSourcePlay(soundInfo.source);
    if (LogIfOpenALError("Could not play source", soundInfo))
    {
      //ASSERT(false);
    }
  }
}



ulong CSoundController::GetCurrentPosition(const SoundInfo& soundInfo) const
{
  if (!m_initialized)
    return 0;

  ALint alPosition;
  alGetSourcei(soundInfo.source, AL_BYTE_OFFSET, &alPosition);
  if (LogIfOpenALError("Could not check byte offset of source", soundInfo))
  {
    //ASSERT(false);
    return 0;
  }
  return alPosition;
}

void CSoundController::FullStopBuffer(const SoundInfo& info)
{

}

void CSoundController::CreateBuffer(ALuint& source, ALuint& buffer)
{
    alGenBuffers(1, &buffer);
    m_buffersWithSources[buffer] = std::pair<ALuint, ALuint>(source, buffer);
    m_shouldUnqueue[buffer] = false;

}



void CSoundController::CreateNewSourceAndBuffer(const SoundFile& soundFile, SoundInfo& soundInfo)
{
  if (!m_initialized)
    return;

  ALenum alDefaultFormat = 0;

  MYWAVEFORMATEX waveFormat=soundFile.GetWaveFormat();
  const std::vector<uchar> &vecSoundData=soundFile.GetSoundData();

  if (waveFormat.nChannels == 1 && waveFormat.wBitsPerSample == 8)
    alDefaultFormat = AL_FORMAT_MONO8;
  else if (waveFormat.nChannels == 1 && waveFormat.wBitsPerSample == 16)
    alDefaultFormat = AL_FORMAT_MONO16;
  else if (waveFormat.nChannels == 2 && waveFormat.wBitsPerSample == 8)
    alDefaultFormat = AL_FORMAT_STEREO8;
  else if (waveFormat.nChannels == 2 && waveFormat.wBitsPerSample == 16)
    alDefaultFormat = AL_FORMAT_STEREO16;
  else if (waveFormat.nChannels == 1 && waveFormat.wBitsPerSample == 32)
    alDefaultFormat = AL_FORMAT_MONO_FLOAT32;
  else if (waveFormat.nChannels == 2 && waveFormat.wBitsPerSample == 32)
    alDefaultFormat = AL_FORMAT_STEREO_FLOAT32;

  ALuint alBuffer;
  CreateBuffer(alBuffer, soundInfo);
  alBufferData(alBuffer, alDefaultFormat, vecSoundData.data(), vecSoundData.size(), waveFormat.nSamplesPerSec);

  if (LogIfOpenALError("Could not bind buffer with data", soundInfo))
  {
    soundInfo.buffer = soundInfo.source = 0;
    return;
  }

  ALuint alSource;
  CreateSource(alSource, soundInfo);
  alSourcei(alSource, AL_BUFFER, alBuffer);

  if (LogIfOpenALError("Could not bind source with buffer", soundInfo))
  {
    soundInfo.buffer = soundInfo.source = 0;
    //ASSERT(false);
    return;
  }
}



void CSoundController::CreateSourceForExistingBuffer(SoundInfo& soundInfo)
{
  if (!m_initialized)
    return;

  ALuint newSource = 0;
  if (alIsBuffer(soundInfo.buffer))
  {
    CreateSource(newSource, soundInfo);
    if (alIsSource(newSource))
    {
      alSourcei(newSource, AL_BUFFER, soundInfo.buffer);
      if (LogIfOpenALError("Could not bind source with buffer", soundInfo))
      {
        //ASSERT(false);
        return;
      }
    }
  }
}

void CSoundController::PlaySource(ALuint source)
{
    if (alIsSource(source))
        alSourcePlay(source);
}



void CSoundController::QueueAndPlayData(void* data, long size, const MYWAVEFORMATEX& waveFormat, ALuint buffer, const ALuint source, bool unqueue)
{

    ALenum alDefaultFormat = 0;

    if (waveFormat.nChannels == 1 && waveFormat.wBitsPerSample == 8)
        alDefaultFormat = AL_FORMAT_MONO8;
    else if (waveFormat.nChannels == 1 && waveFormat.wBitsPerSample == 16)
        alDefaultFormat = AL_FORMAT_MONO16;
    else if (waveFormat.nChannels == 2 && waveFormat.wBitsPerSample == 8)
        alDefaultFormat = AL_FORMAT_STEREO8;
    else if (waveFormat.nChannels == 2 && waveFormat.wBitsPerSample == 16)
        alDefaultFormat = AL_FORMAT_STEREO16;
    else if (waveFormat.nChannels == 1 && waveFormat.wBitsPerSample == 32)
        alDefaultFormat = AL_FORMAT_MONO_FLOAT32;
    else if (waveFormat.nChannels == 2 && waveFormat.wBitsPerSample == 32)
        alDefaultFormat = AL_FORMAT_STEREO_FLOAT32;



    ALuint unqueued = 0;
    if (buffer == 255)
    {
        ALuint unqueued2 = UnqueueBuffer(source);
        m_shouldUnqueue[unqueued2] = true;
        alBufferData(unqueued2, alDefaultFormat, data, size, waveFormat.nSamplesPerSec);
        alSourceQueueBuffers(source, 1, &unqueued2);
        std::cout << "QUEUED BUFFER AFTER UNQEUEING" << unqueued2 << std::endl;
    }
    else
    {
        for (const auto& val : m_shouldUnqueue)
        {
            if (val.second == false)
            {
                unqueued = val.first;
                m_shouldUnqueue[unqueued] = true;
                alBufferData(unqueued, alDefaultFormat, data, size, waveFormat.nSamplesPerSec);
                std::cout << "QUEUED BUFFER" << unqueued << std::endl;
                alSourceQueueBuffers(source, 1, &unqueued);
                break;
            }
        }
    }

    static bool start = false;
    if (!start)
    {
        start = true;
        alSourcePlay(source);
    }
}
