#include "Sound.h"

Sound::Sound(const std::string& soundPath, bool createBuffer) : m_soundPath(soundPath)
{
	SoundFile soundFile = SoundFile();
	soundFile.LoadFile(soundPath);
	m_soundFile = soundFile;
	if (createBuffer)
	{
		CSoundController::Get().CreateNewSourceAndBuffer(soundFile, m_info);
	}
	else
	{
		CSoundController::Get().CreateSource(m_info);
		bufferToPlay = 0;
		m_buffersForQueue = std::deque<ALuint>(MAX_BUFFERS_FOR_QUEUE, 0);
		for (size_t buffers = 0; buffers < MAX_BUFFERS_FOR_QUEUE; buffers++)
		{
			CSoundController::Get().CreateBuffer(m_info.source, m_buffersForQueue[buffers]);
			m_isPlaying[m_buffersForQueue[buffers]] = false;
		}
	}
}

Sound::Sound(bool onlyForStreaming)
{
	bufferToPlay = 0;
	m_buffersForQueue = std::deque<ALuint>(MAX_BUFFERS_FOR_QUEUE, 0);
	CSoundController::Get().CreateSource(m_info);
	for (size_t buffers = 0; buffers < MAX_BUFFERS_FOR_QUEUE; buffers++)
		CSoundController::Get().CreateBuffer(m_info.source, m_buffersForQueue[buffers]);
}

void Sound::Play(bool isLooping, bool stop, bool reset)
{
	CSoundController::Get().PlaySound(m_info, isLooping, stop, reset);
}

void Sound::Stop()
{
	CSoundController::Get().StopSound(m_info);
}

bool Sound::IsPlaying() const noexcept
{
	return CSoundController::Get().IsSourcePlaying(m_info);
}

void Sound::PlayWithRowData(void* data, long size, const MYWAVEFORMATEX& waveFormat)
{
	CSoundController::Get().QueueAndPlayData(data, size, waveFormat, m_buffersForQueue[(bufferToPlay) ], m_info.source, m_isPlaying[bufferToPlay]);
	m_isPlaying[bufferToPlay] = true;
	bufferToPlay++;
	bufferToPlay %= MAX_BUFFERS_FOR_QUEUE;
}

void Sound::GetDividedData(std::deque<std::vector<char>>& dividedData, unsigned int lengthInMiliseconds, int* bytesPerSecond)
{
	unsigned int position = 0;
	std::deque<std::vector<char>> dataToReturn;


	auto waveFormat = m_soundFile.GetWaveFormat();
	unsigned long m_ulSize = m_soundFile.GetSoundData().size();

	unsigned long m_ulLength = 0;
	unsigned long m_ulBytesPerSecond = 0;
	if (waveFormat.nBlockAlign && waveFormat.nSamplesPerSec)
	{
		m_ulBytesPerSecond = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;
		m_ulLength = 1000 * (unsigned long long)m_ulSize / m_ulBytesPerSecond;
	}

	while (position < m_ulSize)
	{
		int bytesToSend = lengthInMiliseconds;

		if (bytesToSend + position > m_ulSize)
			bytesToSend = m_ulSize - position;

		std::vector<char> data = std::vector<char>(m_soundFile.GetSoundData().begin() + position, m_soundFile.GetSoundData().begin() + position + bytesToSend);
		dataToReturn.push_back(data);
		if (position + bytesToSend > m_ulSize)
			position = m_ulSize;
		else
			position += bytesToSend;
	}


	*bytesPerSecond = std::floor(float(lengthInMiliseconds * 1000) / m_ulBytesPerSecond);
	dividedData = decltype(dividedData)(dataToReturn);

}

void Sound::PlaySource() const
{
	CSoundController::Get().PlaySource(m_info.source);
}

std::vector<char> Sound::ConvertMyWaveFormatToData(const MYWAVEFORMATEX& format)
{
	std::vector<char> results; // size of MyWaveFormatEx in bytes

	auto func = [](auto val)
	{
		std::vector<char> resultValue;
		auto resultBitSet = std::bitset<sizeof(val) * 8>(val);
		for (long bit = 0; bit < resultBitSet.size(); bit += 8)
		{
			std::bitset<8> res;
			for (int j = 0; j < 8; j++)
			{
				res[j] = resultBitSet[j + bit];
			}
			resultValue.push_back(static_cast<char>(res.to_ulong()));
		}
		return resultValue;
	};

	auto res1 = func(format.wFormatTag);
	results.insert(results.end(), res1.begin(), res1.end());
	res1.clear();

	res1 = func(format.nChannels);
	results.insert(results.end(), res1.begin(), res1.end());

	res1 = func(format.nSamplesPerSec);
	results.insert(results.end(), res1.begin(), res1.end());

	res1 = func(format.nAvgBytesPerSec);
	results.insert(results.end(), res1.begin(), res1.end());

	res1 = func(format.nBlockAlign);
	results.insert(results.end(), res1.begin(), res1.end());

	res1 = func(format.wBitsPerSample);
	results.insert(results.end(), res1.begin(), res1.end());

	res1 = func(format.cbSize);
	results.insert(results.end(), res1.begin(), res1.end());

	return results;
}

MYWAVEFORMATEX Sound::ConvertDataToFormat(std::vector<char>& data)
{
	MYWAVEFORMATEX newFormat;
	std::bitset<16> b1;
	std::bitset<32> b2;
	
	for (int i = 0; i < 2; i++)
	{
		std::bitset<8> result = std::bitset<8>(data[i]);
		for (int j = 0; j < 8; j++)
		{
			b1[i * 8 + j] = result[j];
		}
	}

	newFormat.wFormatTag = b1.to_ullong();

	for (int i = 2; i < 4; i++)
	{
		std::bitset<8> result = std::bitset<8>(data[i]);
		for (int j = 0; j < 8; j++)
		{
			b1[(i - 2) * 8 + j] = result[j];
		}
	}

	newFormat.nChannels = b1.to_ulong();

	for (int i = 4; i < 8; i++)
	{
		std::bitset<8> result = std::bitset<8>(data[i]);
		for (int j = 0; j < 8; j++)
		{
			b2[(i - 4) * 8 + j] = result[j];
		}
	}

	newFormat.nSamplesPerSec = b2.to_ulong();

	for (int i = 8; i < 12; i++)
	{
		std::bitset<8> result = std::bitset<8>(data[i]);
		for (int j = 0; j < 8; j++)
		{
			b2[(i - 8) * 8 + j] = result[j];
		}
	}
	newFormat.nAvgBytesPerSec = b2.to_ulong();
	for (int i = 12; i < 14; i++)
	{
		std::bitset<8> result = std::bitset<8>(data[i]);
		for (int j = 0; j < 8; j++)
		{
			b1[(i - 12) * 8 + j] = result[j];
		}
	}
	newFormat.nBlockAlign = b1.to_ullong();
	for (int i = 14; i < 16; i++)
	{
		std::bitset<8> result = std::bitset<8>(data[i]);
		for (int j = 0; j < 8; j++)
		{
			b1[(i - 14) * 8 + j] = result[j];
		}
	}
	newFormat.wBitsPerSample = b1.to_ullong();
	for (int i = 16; i < 18; i++)
	{
		std::bitset<8> result = std::bitset<8>(data[i]);
		for (int j = 0; j < 8; j++)
		{
			b1[(i - 16) * 8 + j] = result[j];
		}
	}
	newFormat.cbSize = b1.to_ullong();

	return newFormat;
}

