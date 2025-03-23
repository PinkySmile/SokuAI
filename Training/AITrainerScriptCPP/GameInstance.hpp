//
// Created by PinkySmile on 21/05/2021.
//

#ifndef SOKUAI_GAMEINSTANCE_HPP
#define SOKUAI_GAMEINSTANCE_HPP


#include <vector>
#include <string>
#ifdef _WIN32
#include <SokuLib.hpp>
#include <Windows.h>
#else
#include <Weather.hpp>
#include <Action.hpp>
#include "winDefines.hpp"
#endif
#include "Packet.hpp"

namespace Trainer
{
	class GameInstance {
	public:
		struct PlayerParams {
			SokuLib::Character character;
			unsigned char palette;
			unsigned char deck[20];
			char name[32];
		};

		struct StartGameParams {
			PlayerParams left;
			PlayerParams right;
			unsigned char stage;
			unsigned char music;
		};
		struct GameInputs {
			Input left;
			Input right;
		};
		struct GameFrame {
			CharacterState left;
			CharacterState right;
			std::vector<Object> leftObjects;
			std::vector<Object> rightObjects;
			SokuLib::Weather displayedWeather;
			SokuLib::Weather activeWeather;
			unsigned short weatherTimer;
		};

		GameInstance(unsigned short por);
		void reconnect(const std::string &path, const char *iniPath = nullptr, bool justConnect = false);
		GameFrame startGame(const StartGameParams &params);
		GameFrame tick(GameInputs inputs);
		void quit();
		void setGameSpeed(unsigned tps = 60);
		void endGame();
		void setDisplayMode(bool shown = true);
		void setGameVolume(unsigned char sfx, unsigned char bgm);
		void setPositions(SokuLib::Vector2f left, SokuLib::Vector2f right);
		void setHealth(unsigned short left, unsigned short right);
		void setWeather(SokuLib::Weather weather, int timer, bool freezeTimer = false);
		void restrictMoves(std::vector<SokuLib::Action> blackList);
		int terminate();
	private:
		SOCKET _baseSocket = INVALID_SOCKET;
		SOCKET _socket = INVALID_SOCKET;
		unsigned short _port;
#ifdef _WIN32
		PROCESS_INFORMATION _processInformation;
#else
		pid_t _pid = 0;
#endif

		GameFrame _recvGameFrame(int timeout);
	};
}


#endif //SOKUAI_GAMEINSTANCE_HPP
