//
// Created by PinkySmile on 22/05/2021.
//

#include <numeric>
#include <iostream>
#include <functional>
#include "TournamentManager.hpp"
#include "Exceptions.hpp"

#ifdef _DEBUG
#define DISPLAY_PARAM true
#define SOUND_PARAM 10, 10
#define GAME_TPS 60
#else
#define DISPLAY_PARAM false
#define SOUND_PARAM 0, 0
#define GAME_TPS 60000
#endif

namespace Trainer
{
	WinnerSide GameThread::_getMatchWinner(const std::vector<GameManager::GameResult> &results)
	{
		float leftPoints = 0;
		float rightPoints = 0;

		for (auto &result : results) {
			leftPoints  += (result.winner == 1) * 1000000;
			rightPoints += (result.winner == 2) * 1000000;
			leftPoints  += (result.score.first - result.score.second) * 20000;
			rightPoints += (result.score.second - result.score.first) * 20000;
			leftPoints  += result.hpLeft.first - result.hpLeft.second;
			rightPoints += result.hpLeft.second - result.hpLeft.first;
		}
		return static_cast<WinnerSide>((leftPoints != rightPoints) + (leftPoints < rightPoints));
	}

	void GameThread::_updateMatchAis(Match &match, WinnerSide winner, int side)
	{
		std::vector<std::string> t{
			"aborted",
			"a draw",
			match.ai1.ai->toString(),
			match.ai2.ai->toString()
		};

		printf("Match %s vs %s %s is %s\n", t[2].c_str(), t[3].c_str(), winner <= 0 ? "result" : "winner", t[static_cast<char>(winner) + 1].c_str());
		fflush(stdout);
		match.ai1.wins <<= 1;
		match.ai2.wins <<= 1;
		match.ai1.sides.push_back(side == 1 ? 0 : 1);
		match.ai2.sides.push_back(side == 0 ? 1 : 0);
		switch (winner) {
		case WINNER_SIDE_DRAW:
			match.ai1.score += 0.5;
			match.ai2.score += 0.5;
			return;
		case WINNER_SIDE_LEFT:
			match.ai1.score += 1;
			match.ai1.wins |= 1;
			return;
		case WINNER_SIDE_RIGHT:
			match.ai2.score += 1;
			match.ai2.wins |= 1;
		}
	}

	GameThread::GameThread(
		unsigned generation,
		unsigned round,
		unsigned maxRound,
		std::mutex &mutex,
		std::vector<Match> &matches,
		GameManager &game,
		std::pair<unsigned char, unsigned char> stageInfo,
		unsigned int nb,
		unsigned int frameTimeout,
		unsigned int inputDelay
	) :
		_matchMutex(mutex),
		_matches(matches),
		_game(game),
		_stageInfo(stageInfo),
		_nb(nb),
		_frameTimeout(frameTimeout),
		_inputDelay(inputDelay),
		_generation(generation),
		_round(round),
		_maxRound(maxRound)
	{
	}

	void GameThread::run()
	{
		while (true) {
			this->_matchMutex.lock();
			if (this->_matches.empty()) {
				this->_matchMutex.unlock();
				return;
			}

			auto &match = this->_matches.back();

			this->_playing = this->_matches.size();
			this->_matches.pop_back();
			this->_matchMutex.unlock();

			auto side = true;

			if (!match.ai1.sides.empty()) {
				auto leftSum  = std::accumulate(match.ai1.sides.begin(), match.ai1.sides.end(), 0);
				auto rightSum = std::accumulate(match.ai2.sides.begin(), match.ai2.sides.end(), 0);

				side = leftSum <= rightSum;
			}

			this->_game.leftAi  = (side ? match.ai1 : match.ai2).ai;
			this->_game.rightAi = (side ? match.ai2 : match.ai1).ai;
			printf("[gen%u-round%u/%u][%i] Playing match %s vs %s\n", this->_generation, this->_round, this->_maxRound, this->_playing, this->_game.leftAi->toString().c_str(), this->_game.rightAi->toString().c_str());
			fflush(stdout);
			try {
				this->_updateMatchAis(match, this->_getMatchWinner(
					this->_game.run(
						{
							this->_game.leftAi->getParams(),
							this->_game.rightAi->getParams(),
							0,
							0
						},
						this->_nb,
						this->_frameTimeout,
						this->_inputDelay,
						true
					)
				), side);
			} catch (const ConnectionResetException &) {
				puts("Match was aborted");
				this->_updateMatchAis(match, static_cast<WinnerSide>(-1), -1);
			}
#ifndef _DEBUG
			catch (std::exception &e) {
				std::cerr << e.what() << std::endl;
				throw;
			}
#endif
		}
	}

	void GameThread::join()
	{
		if (this->_thread.joinable())
			this->_thread.join();
	}

	void GameThread::start()
	{
		this->join();
		this->_thread = std::thread([this]{
			this->run();
		});
	}

	GameOpenThread::GameOpenThread(
		std::mutex &mutex,
		std::vector<std::unique_ptr<GameManager>> &container,
		const std::string &path,
		unsigned short port,
		unsigned int tps,
		bool hasDisplay,
		unsigned char bgmVolume,
		unsigned char sfxVolume,
		const char *iniPath,
		bool justConnect
	) :
		_mutex(mutex),
		_container(container),
		_path(path),
		_port(port),
		_tps(tps),
		_hasDisplay(hasDisplay),
		_bgmVolume(bgmVolume),
		_sfxVolume(sfxVolume),
		_iniPath(iniPath),
		_justConnect(justConnect)
	{
	}

	void GameOpenThread::run()
	{
		auto instance = new GameManager(this->_path, this->_port, this->_tps, this->_hasDisplay, this->_bgmVolume, this->_sfxVolume, this->_iniPath, this->_justConnect);

		this->_mutex.lock();
		this->_container.emplace_back(instance);
		this->_mutex.unlock();
	}

	void GameOpenThread::join()
	{
		if (this->_thread.joinable())
			this->_thread.join();
	}

	void GameOpenThread::start()
	{
		this->join();
		this->_thread = std::thread([this]{
			this->run();
		});
	}

	void TournamentManager::_makeMatches(std::vector<PlayerEntry> &ais, std::vector<Match> &matches)
	{
		std::vector<std::reference_wrapper<PlayerEntry>> _ais;

		_ais.reserve(ais.size());
		for (auto &ai : ais)
			_ais.emplace_back(ai);
		this->_makeMatches(_ais, matches);
	}

	void TournamentManager::_makeMatches(std::vector<std::reference_wrapper<PlayerEntry>> &ais, std::vector<Match> &matches)
	{
		std::map<float, std::vector<PlayerEntry *>> pools;
		PlayerEntry *leftover = nullptr;

		matches.reserve(matches.size() + ais.size() / 2);
		puts("Creating match list");
		for (auto &ai : ais)
			pools[ai.get().score].push_back(&ai.get());
		for (auto &pool : pools) {
			if (leftover)
				pool.second.push_back(leftover);
			leftover = nullptr;
			printf("Pool %.1f contains %i ais\n", pool.first, pool.second.size());
			// TODO: Try to uniform the sides each AI played and match AIs with the same number of wins in a row together
			// TODO: Don't match AIs together twice
			for (int i = 0; i < pool.second.size() - 1; i += 2)
				matches.push_back({*pool.second[i], *pool.second[i + 1]});
			if (pool.second.size() % 2 != 0)
				leftover = pool.second.back();
		}
		if (leftover) {
			leftover->wins <<= 1;
			leftover->wins |= 1;
			leftover->score += 1;
			leftover->sides.push_back(0);
		}
	}

	void TournamentManager::_playMatches(unsigned generation, unsigned round, unsigned maxRound, std::vector<Match> &matches)
	{
		std::vector<GameThread> threads;
		std::mutex mutex;

		threads.reserve(this->_gameManagers.size());
		for (auto &game : this->_gameManagers) {
			threads.emplace_back(generation, round, maxRound, mutex, matches, *game, std::pair<unsigned char, unsigned char>{0, 0}, this->_firstTo, this->_timeout, this->_inputDelay);
			threads.back().start();
		}
		for (auto &thread : threads)
			thread.join();
	}

	TournamentManager::TournamentManager(
		unsigned int gamePool,
		unsigned short portStart,
		const std::string &gamePath,
		unsigned int inputDelay,
		unsigned int timeLimit,
		unsigned int firstTo,
		const char *iniPath
	) :
		_firstTo(firstTo),
		_inputDelay(inputDelay),
		_timeout(timeLimit)
	{
		std::vector<GameOpenThread> openThread;
		std::mutex mutex;

		openThread.reserve(gamePool);
		printf("Opening %u games\n", gamePool);
		for (int i = 0; i < gamePool; i++) {
			openThread.emplace_back(mutex, this->_gameManagers, gamePath + "/" + std::to_string(i) + "/th123.exe", portStart + i, GAME_TPS, DISPLAY_PARAM, SOUND_PARAM, iniPath);
			openThread.back().start();
		}
		for (auto &thread : openThread)
			thread.join();
	}

	std::vector<PlayerEntry> TournamentManager::playTournament(unsigned generation, const std::vector<BaseAI *> &ais, unsigned int nbRounds)
	{
		std::vector<PlayerEntry> allEntries;

		allEntries.reserve(ais.size());
		for (auto ai : ais)
			allEntries.push_back({ai, 0, 0, {}});
		// AI, win score, score, sides (True is left)
		puts("Starting tournament...");
		printf("There are %u players and %u rounds\n", allEntries.size(), nbRounds);
		for (unsigned i = 0; i < nbRounds; i++) {
			std::vector<Match> matches;

			printf("Round %i start !\n", i + 1);
			this->_makeMatches(allEntries, matches);
			this->_playMatches(generation, i + 1, nbRounds, matches);
		}
		return allEntries;
	}

	std::vector<PlayerEntry> TournamentManager::playGroupTournament(unsigned int generation, const std::vector<BaseAI *> &ais, unsigned int poolSize)
	{
		auto nb = std::ceil(std::log2(poolSize));
		auto total = std::log(ais.size()) / std::log(4) * nb;
		std::vector<PlayerEntry> allEntries;
		std::vector<std::vector<std::reference_wrapper<PlayerEntry>>> pools;
		std::vector<std::vector<std::reference_wrapper<PlayerEntry>>> newPools;
		int i = 0;

		allEntries.reserve(ais.size());
		for (auto ai : ais)
			allEntries.push_back({ai, 0, 0, {}});
		printf("There are %u players and %f rounds with pools of size %u\n", allEntries.size(), total, poolSize);
		while (pools.size() != 1) {
			std::vector<Match> matches;

			printf("Round %i start !\n", i + 1);
			this->_makeMatches(allEntries, matches);
			this->_playMatches(generation, i + 1, total, matches);
			i++;
		}
		return allEntries;
	}
}