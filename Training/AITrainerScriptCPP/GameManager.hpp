//
// Created by PinkySmile on 21/05/2021.
//

#ifndef SOKUAI_GAMEMANAGER_HPP
#define SOKUAI_GAMEMANAGER_HPP


#include "GameInstance.hpp"

namespace Trainer
{
	class BaseAI;

	class GameManager {
	private:
		GameInstance _gameInstance;
		std::string _path;
		unsigned short _port;
		unsigned int _tps;
		bool _hasDisplay;
		unsigned char _bgmVolume;
		unsigned char _sfxVolume;
		const char *_iniPath;
		bool _justConnect;

		GameInstance::GameFrame _startGameSequence(const GameInstance::StartGameParams &params);
	public:
		BaseAI *leftAi = nullptr;
		BaseAI *rightAi = nullptr;

		struct GameResult {
			WinnerSide winner;
			std::pair<unsigned char, unsigned char> score;
			std::pair<unsigned short, unsigned short> hpLeft;
		};

		GameManager(
			const std::string &path,
			unsigned short port,
			unsigned int tps = 60,
			bool hasDisplay = true,
			unsigned char bgmVolume = 20,
			unsigned char sfxVolume = 20,
			const char *iniPath = nullptr,
			bool justConnect = false
		);
		void startInstance();
		GameResult runOnce(const GameInstance::StartGameParams &params, unsigned frameTimout, unsigned inputDelay, unsigned maxCrashes=5);
    		std::vector<GameResult> run(GameInstance::StartGameParams params, unsigned nb = 1, unsigned frameTimout = -1, unsigned inputDelay = 0, bool swap = false);
	};
}


#endif //SOKUAI_GAMEMANAGER_HPP
