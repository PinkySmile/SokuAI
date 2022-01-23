//
// Created by PinkySmile on 22/05/2021.
//

#ifndef SOKUAI_SWISSTOURNAMENTMANAGER_HPP
#define SOKUAI_SWISSTOURNAMENTMANAGER_HPP


#include <mutex>
#include <thread>
#include "GameManager.hpp"
#include "BaseAi.hpp"

namespace Trainer
{
	struct PlayerEntry {
		BaseAI *ai;
		unsigned wins;
		float score;
		std::vector<char> sides;
	};

	struct Match {
		PlayerEntry &ai1;
		PlayerEntry &ai2;
	};

	class GameThread {
	private:
		std::thread _thread;
		std::mutex &_matchMutex;
		std::vector<Match> &_matches;
		GameManager &_game;
		std::pair<unsigned char, unsigned char> _stageInfo;
		unsigned _nb;
		unsigned _frameTimeout;
		unsigned _inputDelay;
		unsigned _playing;
		unsigned _generation;
		unsigned _round;
		unsigned _maxRound;

		static WinnerSide _getMatchWinner(const std::vector<GameManager::GameResult> &results);
		static void _updateMatchAis(Match &match, WinnerSide winner, int side);

	public:
		GameThread(
			unsigned generation,
			unsigned round,
			unsigned maxRound,
			std::mutex &mutex,
			std::vector<Match> &matches,
			GameManager &game,
			std::pair<unsigned char, unsigned char> stageInfo = {0, 0},
			unsigned nb = 1,
			unsigned frameTimeout = -1,
			unsigned inputDelay = 0
		);
		void run();
		void join();
		void start();
	};

	class GameOpenThread {
	private:
		std::thread _thread;
		std::mutex &_mutex;
		std::vector<std::unique_ptr<GameManager>> &_container;
		std::string _path;
		unsigned short _port;
		unsigned int _tps;
		bool _hasDisplay;
		unsigned char _bgmVolume;
		unsigned char _sfxVolume;
		const char *_iniPath;
		bool _justConnect;

	public:
		GameOpenThread(
			std::mutex &_mutex,
			std::vector<std::unique_ptr<GameManager>> &_container,
			const std::string &path,
			unsigned short port,
			unsigned int tps = 60,
			bool hasDisplay = true,
			unsigned char bgmVolume = 20,
			unsigned char sfxVolume = 20,
			const char *iniPath = nullptr,
			bool justConnect = false
		);
		void run();
		void join();
		void start();
	};

	class SwissTournamentManager {
	private:
		unsigned _firstTo;
		unsigned _inputDelay;
		unsigned _timeout;
		std::vector<std::unique_ptr<GameManager>> _gameManagers;

		void _makeMatches(std::vector<PlayerEntry> &ais, std::vector<Match> &matches);
		void _playMatches(unsigned generation, unsigned round, unsigned maxRound, std::vector<Match> &matches);

	public:
		SwissTournamentManager(
			unsigned gamePool,
			unsigned short portStart,
			const std::string &gamePath,
			unsigned inputDelay = 0,
			unsigned timeLimit = -1,
			unsigned firstTo = 2,
			const char *iniPath = nullptr
		);

		std::vector<PlayerEntry> playTournament(unsigned generation, const std::vector<BaseAI *> &ais, unsigned nbRounds);
	};
}


#endif //SOKUAI_SWISSTOURNAMENTMANAGER_HPP
