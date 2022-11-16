#pragma once
#include <string>
#include <bitset>
#include "GCSoundController.h"

constexpr unsigned int MAX_BUFFERS_FOR_QUEUE = 256;

struct TransferData
{
	MYWAVEFORMATEX myFormat;
	std::vector<char> myData;
};



class Sound
{
public:
	Sound(const std::string& soundPath, bool createBuffer = true);
	Sound(bool onlyForStreaming);
	void Play(bool isLooping = false, bool stop = false, bool reset = false);
	void Stop();
	const SoundInfo& GetSoundInfo() const noexcept { return m_info; }
	bool IsPlaying() const noexcept;
	const SoundFile& GetSoundFile() const noexcept { return m_soundFile; }
	void PlayWithRowData(void* data, long size, const MYWAVEFORMATEX& waveFormat);
	void GetDividedData(std::deque<std::vector<char>>& dividedData, unsigned int lengthInMiliseconds, int* bytesPerSecond);
	void PlaySource() const;
	static std::vector<char> ConvertMyWaveFormatToData(const MYWAVEFORMATEX& format);
	static MYWAVEFORMATEX ConvertDataToFormat(std::vector<char>& data);

private:
	std::unordered_map<int, bool> m_isPlaying;
	std::string m_soundPath;
	SoundInfo m_info;
	SoundFile m_soundFile;
	std::deque<ALuint> m_buffersForQueue;
	unsigned int bufferToPlay;
};

