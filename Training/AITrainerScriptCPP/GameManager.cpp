//
// Created by PinkySmile on 21/05/2021.
//

#include <list>
#include "BaseAi.hpp"
#include "GameManager.hpp"
#include "Exceptions.hpp"

namespace Trainer
{
	GameInstance::GameFrame GameManager::_startGameSequence(const GameInstance::StartGameParams &params)
	{
		while (true) {
			try {
				return this->_gameInstance.startGame(params);
			} catch (const ProtocolError &e) {
				if (e.getCode() != ERROR_STILL_PLAYING)
					throw;
				Sleep(100);
			}
		}
	}

	GameManager::GameManager(const std::string &path, unsigned short port, unsigned int tps, bool hasDisplay, unsigned char bgmVolume, unsigned char sfxVolume, const char *iniPath, bool justConnect) :
		_gameInstance(port),
		_path(path),
		_port(port),
		_tps(tps),
		_hasDisplay(hasDisplay),
		_bgmVolume(bgmVolume),
		_sfxVolume(sfxVolume),
		_iniPath(iniPath),
		_justConnect(justConnect)
	{
		this->startInstance();
	}

	void GameManager::startInstance()
	{
		this->_gameInstance.reconnect(this->_path, this->_iniPath, this->_justConnect);
		this->_gameInstance.setDisplayMode(this->_hasDisplay);
		this->_gameInstance.setGameSpeed(this->_tps);
		this->_gameInstance.setGameVolume(this->_bgmVolume, this->_sfxVolume);
	}

	GameManager::GameResult GameManager::runOnce(const GameInstance::StartGameParams &params, unsigned int frameTimout, unsigned int inputDelay, unsigned int maxCrashes)
	{
		GameInstance::GameFrame state;
		std::list<Input> leftInputs;
		std::list<Input> rightInputs;

		try {
			state = this->_startGameSequence(params);
			this->leftAi->onGameStart(params.left.character, params.right.character, inputDelay);
			this->rightAi->onGameStart(params.right.character, params.left.character, inputDelay);
			leftInputs.resize(inputDelay);
			rightInputs.resize(inputDelay);
			while (true) {
				frameTimout -= 1;
				if (frameTimout <= 0)
					this->_gameInstance.endGame();

				leftInputs.push_back(this->leftAi->getInputs(state, true, frameTimout));
				rightInputs.push_back(this->rightAi->getInputs(state, false, frameTimout));

				state = this->_gameInstance.tick({leftInputs.front(), rightInputs.front()});
				leftInputs.pop_front();
				rightInputs.pop_front();
			}
		} catch (const UnexpectedPacketError &e) {
			puts(e.what());
			puts("Restarting game instance...");
			this->startInstance();
			puts("Restarting game from beginning...");
			return this->runOnce(params, frameTimout, inputDelay, maxCrashes);
		} catch (const ConnectionResetException &) {
			puts("Our list of allies grows thin !");
			printf("Exit code %lX\n", this->_gameInstance.terminate());
			puts("Restarting game instance...");
			this->startInstance();
			if (maxCrashes == 0) {
				puts("Too many crashes, aborting the game");
				throw;
			}
			puts("Restarting game from beginning...");
			return this->runOnce(params, frameTimout, inputDelay, maxCrashes - 1);
		} catch (const GameEndedException &e) {
			auto winner = e.getWinner();
			auto leftScore = e.getLeftScore();
			auto rightScore = e.getRightScore();

			switch (winner) {
				case WINNER_SIDE_LEFT:
					this->leftAi->onWin(leftScore, rightScore);
					this->rightAi->onLose(rightScore, leftScore);
					break;
				case WINNER_SIDE_RIGHT:
					this->leftAi->onLose(leftScore, rightScore);
					this->rightAi->onWin(rightScore, leftScore);
					break;
				case WINNER_SIDE_DRAW:
					this->leftAi->onTimeout(leftScore, rightScore);
					this->rightAi->onTimeout(rightScore, leftScore);
					break;
			}

			return {
				winner,
				{leftScore, rightScore},
				{state.left.hp, state.right.hp}
			};
		}
	}

	std::vector<GameManager::GameResult> GameManager::run(GameInstance::StartGameParams params, unsigned int nb, unsigned int frameTimout, unsigned int inputDelay, bool swap)
	{
		std::vector<GameManager::GameResult> result;

		for (int i = 0; i < nb; i++) {
			result.push_back(this->runOnce(params, frameTimout, inputDelay));
			if (swap) {
				auto old = this->rightAi;
				auto oldParams = params.right;

				this->rightAi = this->leftAi;
				this->leftAi = old;

				params.right = params.left;
				params.left = oldParams;
			}
		}
		return result;
	}
}