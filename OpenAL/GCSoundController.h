#pragma once

#include "../include/AL/al.h"
#include "../include/AL/alc.h"
#include "../include/AL/alext.h"
#include <mutex>
#include <deque>
#include <functional>
#include "SoundFile.h"

using uint = unsigned int;
using ulong = unsigned long;
using uchar = unsigned char;


struct SoundInfo
{
  ALuint source;
  ALuint buffer;
  std::string soundPath;
  std::string soundName;
  SoundInfo() :
    source(0),
    buffer(0)
  {}
};

class ISoundController
{
public:
  virtual ~ISoundController() = default;

  virtual void DeleteSource(const SoundInfo& soundInfo, bool deleteFromList = true) = 0;

  virtual void StopSound(const SoundInfo& soundInfo) = 0;

  virtual void PlaySound(const SoundInfo& soundInfo, const bool bIsLooping, const bool bStop, const bool bResetPosition) = 0;

  virtual void SetSourceVolume(const SoundInfo& soundInfo, const ALfloat alfVolume) = 0;

  virtual void RewindSource(const SoundInfo& soundInfo) = 0;

  virtual bool IsSourcePlaying(const SoundInfo& soundInfo) const = 0;

  virtual bool IsSoundDataLooping(const SoundInfo& soundInfo) const = 0;

  virtual void BreakLoop(const SoundInfo& soundInfo) = 0;

  virtual ulong GetCurrentPosition(const SoundInfo& soundInfo) const = 0;

  virtual void CreateNewSourceAndBuffer(const SoundFile& soundFile, SoundInfo& soundInfo) = 0;

  virtual void CreateSourceForExistingBuffer(SoundInfo& soundInfo) = 0;

  virtual void FullStopBuffer(const SoundInfo& info) = 0;

  virtual void CreateBuffer(ALuint& source, ALuint& buffer) = 0;

  virtual void QueueAndPlayData(void * data, long size, const MYWAVEFORMATEX& waveFormat, ALuint buffer, const ALuint source, bool unqueue) = 0;

  virtual void CreateSource(SoundInfo& info) = 0;

  virtual void PlaySource(ALuint source) = 0;

  enum class StatusChange : uint8_t { Paused, Stopped };

  using SoundStatusChangeCallback = std::function<void (uint object, StatusChange status)>;
  virtual void RegisterSoundStatusChangeCallback(SoundStatusChangeCallback callback) = 0;
};

class CSoundController : public ISoundController
{
public:

  CSoundController(const CSoundController&) = delete;
  CSoundController(const CSoundController&&) = delete;
  CSoundController& operator=(const CSoundController&) = delete;
  CSoundController& operator=(const CSoundController&&) = delete;
  ~CSoundController() override;

  void RegisterSoundStatusChangeCallback(SoundStatusChangeCallback callback);

  void DeleteSource(const SoundInfo& soundInfo, bool deleteFromList = true) override;

  void StopSound(const SoundInfo& soundInfo) override;

  void PlaySound(const SoundInfo& soundInfo, const bool bIsLooping, const bool bStop, const bool bResetPosition) override;

  void SetSourceVolume(const SoundInfo& soundInfo, const ALfloat alfVolume) override;

  void RewindSource(const SoundInfo& soundInfo) override;

  bool IsSourcePlaying(const SoundInfo& soundInfo) const override;

  virtual void CreateSource(SoundInfo& info) override;

  bool IsSoundDataLooping(const SoundInfo& soundInfo) const override;

  void BreakLoop(const SoundInfo& soundInfo) override;

  ulong GetCurrentPosition(const SoundInfo& soundInfo) const override;

  void FullStopBuffer(const SoundInfo& info) override;
  
  void CreateBuffer(ALuint& source, ALuint& buffer) override;
  
  void CreateNewSourceAndBuffer(const SoundFile& soundFile, SoundInfo& soundInfo) override;

  virtual void QueueAndPlayData(void* data, long size, const MYWAVEFORMATEX& waveFormat, ALuint buffer, const ALuint source, bool unqueue) override;

  void CreateSourceForExistingBuffer(SoundInfo& soundInfo) override;

  void PlaySource(ALuint source) override;

  static void AL_APIENTRY EventCallBack(ALenum eventType, ALuint object, ALuint param,
    ALsizei length, const ALchar* message,
    void* userParam);

#ifdef _MHD_UNIT_TESTS
  static std::unique_ptr<ISoundController> g_testingInstance;
  static void SetTestingInstance(std::unique_ptr<ISoundController> testingInstance)
  {
    g_testingInstance = std::move(testingInstance);
  }
#endif
  static ISoundController& Get();

protected:
  CSoundController();

private:
  static std::recursive_mutex csSoundLock;
  static SoundStatusChangeCallback m_statusChangeCallback;
  std::atomic_bool m_initialized;
  ALCdevice* m_alcDevice;
  ALCcontext* m_alcContext;
  std::thread m_unqueuer;
  std::recursive_mutex streamingLock;
  std::unordered_map<ALuint, std::unordered_map<ALuint, SoundInfo>> m_bufferWithSources;

  static std::unordered_map<ALuint, bool> m_shouldUnqueue;
  std::unordered_map<ALuint, std::pair<ALuint, ALuint>> m_buffersWithSources;

  void CreateBuffer(ALuint& alBuffer, SoundInfo& soundInfo);

  void DeleteBuffer(const SoundInfo& soundInfo, bool deleteFromList = true);

  void CreateSource(ALuint& alSource, SoundInfo& soundInfo);

  bool LogIfOpenALError(const char* message, const SoundInfo& soundInfo) const;

  int UnqueueBuffer(const ALuint& source);
};