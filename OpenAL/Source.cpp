#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "Sound.h"
//#include "GCSoundController.h"
//
//class Sound
//{
//private:
//	std::string m_soundPath;
//	SoundInfo m_info;
//public:
//	Sound(const std::string& soundPath) : m_soundPath(soundPath)
//	{
//		SoundFile soundFile = SoundFile();
//		soundFile.LoadFile(soundPath);
//		CSoundController::Get().CreateNewSourceAndBuffer(soundFile, m_info);
//	}
//
//	Sound(const SoundInfo& soundInfo)
//	{
//		m_info = soundInfo;
//		CSoundController::Get().CreateSourceForExistingBuffer(m_info);
//	}
//
//	void Play(bool isLooping = false, bool stop = false, bool reset = false)
//	{
//		CSoundController::Get().PlaySound(m_info, isLooping, stop, reset);
//	}
//	void Stop()
//	{
//		CSoundController::Get().StopSound(m_info);
//	}
//
//	const SoundInfo& GetSoundInfo() const noexcept { return m_info; }
//
//	bool IsPlaying()
//	{
//		return CSoundController::Get().IsSourcePlaying(m_info);
//	}
//
//	void FullStopBuffer()
//	{
//
//	}
//
//};
//
using namespace std::chrono_literals;

int main()
{

	std::vector<std::shared_ptr<Sound>> sounds;
	for (auto i : { 1, 2, 3, 4, 5, 6, 7 ,8 ,9 })
	{
		std::string path = "../Songs/win" + std::to_string(i) + ".wav";
		sounds.push_back(std::make_shared<Sound>(path,false));
	}

	std::deque<std::vector<char>> dividedData;

	sounds[8]->GetDividedData(dividedData, 1000);

	while (!dividedData.empty())
	{

		sounds[8]->PlayWithRowData(dividedData.front(), sounds[8]->GetSoundFile().GetWaveFormat());
		dividedData.pop_front();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 - 10));
	}

	std::cin;
	/*std::vector<std::shared_ptr<Sound>> soundCopies;
	for (const auto& sound : sounds)
	{
		soundCopies.push_back(std::make_shared<Sound>(sound->GetSoundInfo()));
	}

	std::cout << std::boolalpha;
	sounds[3]->Play();
	std::this_thread::sleep_for(1500ms);
	sounds[3]->Play();
	std::cout << "IS PLAYING " << sounds[3]->IsPlaying() << std::endl;

	while (std::any_of(sounds.begin(), sounds.end(), [](const std::shared_ptr<Sound>& sound) {return sound->IsPlaying(); }))
	{
		std::cout << "Playing" << std::endl;
	}*/

}