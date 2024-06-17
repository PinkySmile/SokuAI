//
// Created by PinkySmile on 30/03/2021.
//

#ifndef SOKUAI_PACKET_HPP
#define SOKUAI_PACKET_HPP


#include <SokuLib.hpp>

#define PACKET_HELLO_MAGIC_NUMBER 0xF56E9D2A

#pragma pack(push, 1)
namespace Trainer
{
	enum Opcode : unsigned char {
		OPCODE_HELLO,
		OPCODE_GOODBYE,
		OPCODE_SPEED,
		OPCODE_DISPLAY,
		OPCODE_SOUND,
		OPCODE_START_GAME,
		OPCODE_FAULT,
		OPCODE_ERROR,
		OPCODE_GAME_FRAME,
		OPCODE_GAME_INPUTS,
		OPCODE_GAME_CANCEL,
		OPCODE_GAME_ENDED,
		OPCODE_OK,
		OPCODE_SET_HEALTH,
		OPCODE_SET_POSITION,
		OPCODE_SET_WEATHER,
		OPCODE_RESTRICT_MOVES,
	};

	enum WinnerSide : unsigned char {
		WINNER_SIDE_DRAW,
		WINNER_SIDE_LEFT,
		WINNER_SIDE_RIGHT,
	};

	enum Errors : unsigned char {
		ERROR_INVALID_OPCODE,
		ERROR_INVALID_PACKET,
		ERROR_INVALID_LEFT_DECK,
		ERROR_INVALID_LEFT_CHARACTER,
		ERROR_INVALID_LEFT_PALETTE,
		ERROR_INVALID_RIGHT_DECK,
		ERROR_INVALID_RIGHT_CHARACTER,
		ERROR_INVALID_RIGHT_PALETTE,
		ERROR_INVALID_MUSIC,
		ERROR_INVALID_STAGE,
		ERROR_INVALID_MAGIC,
		ERROR_HELLO_NOT_SENT,
		ERROR_STILL_PLAYING,
		ERROR_POSITION_OUT_OF_BOUND,
		ERROR_INVALID_HEALTH,
		ERROR_INVALID_WEATHER,
		ERROR_INVALID_WEATHER_TIME,
		ERROR_NOT_IN_GAME,
	};

	struct HelloPacket {
		Opcode op;
		unsigned magic;
	};

	struct SpeedPacket {
		Opcode op;
		unsigned ticksPerSecond;
	};

	struct DisplayPacket {
		Opcode op;
		bool enabled;
	};

	struct SoundPacket {
		Opcode op;
		unsigned char sfxVolume;
		unsigned char bgmVolume;
	};

	struct StartGamePacket {
		Opcode op;
		unsigned char stageId;
		unsigned char musicId;
		SokuLib::Character leftCharacter;
		unsigned char leftDeck[20];
		unsigned char leftPalette;
		char leftPlayerName[32];
		SokuLib::Character rightCharacter;
		unsigned char rightDeck[20];
		unsigned char rightPalette;
		char rightPlayerName[32];
	};

	struct FaultPacket {
		Opcode op;
		void *faultAddress;
		unsigned faultExceptionCode;
	};

	struct ErrorPacket {
		Opcode op;
		Errors error;
	};

	struct Object {
		SokuLib::Direction direction;
		SokuLib::Vector2f relativePosMe;
		SokuLib::Vector2f relativePosOpponent;
		SokuLib::Action action;
		unsigned imageID;
	};

	struct CharacterState {
		SokuLib::Direction direction;
		SokuLib::Vector2f opponentRelativePos;
		float distToBackCorner;
		float distToFrontCorner;
		float distToGround;
		SokuLib::Action action;
		unsigned short actionBlockId;
		unsigned short animationCounter;
		unsigned short animationSubFrame;
		unsigned int frameCount;
		unsigned short comboDamage;
		unsigned short comboLimit;
		bool airBorne;
		unsigned short hp;
		unsigned char airDashCount;
		unsigned short spirit;
		unsigned short maxSpirit;
		unsigned short untech;
		unsigned short healingCharm;
		unsigned short swordOfRapture;
		unsigned char score;
		unsigned char hand[5];
		unsigned short cardGauge;
		SokuLib::Skill skills[15];
		unsigned char fanLevel;
		unsigned short dropInvulTimeLeft;
		unsigned short superArmorTimeLeft;
		unsigned short superArmorHp;
		unsigned short milleniumVampireTimeLeft;
		unsigned short philosoferStoneTimeLeft;
		unsigned short sakuyasWorldTimeLeft;
		unsigned short privateSquareTimeLeft;
		unsigned short orreriesTimeLeft;
		unsigned short mppTimeLeft;
		unsigned short kanakoCooldown;
		unsigned short suwakoCooldown;
		unsigned char objectCount;
	};

	struct GameFramePacket {
		Opcode op;
		CharacterState leftState;
		CharacterState rightState;
		SokuLib::Weather displayedWeather;
		SokuLib::Weather activeWeather;
		unsigned short weatherTimer;
		Object objects[0];
	};

	union Input{
		struct {
			bool A: 1;
			bool B: 1;
			bool C: 1;
			bool D: 1;
			bool SC:1;
			bool SW:1;
			char H: 2;
			char V: 2;
		};
		unsigned short value;
	};

	struct GameInputPacket {
		Opcode op;
		Input left;
		Input right;
	};

	struct GameEndedPacket {
		Opcode op;
		WinnerSide winner;
		unsigned char leftScore;
		unsigned char rightScore;
	};

	struct SetHealthPacket {
		Opcode op;
		unsigned short left;
		unsigned short right;
	};

	struct SetPositionPacket {
		Opcode op;
		SokuLib::Vector2f left;
		SokuLib::Vector2f right;
	};

	struct SetWeatherPacket {
		Opcode op;
		SokuLib::Weather weather;
		unsigned short timer;
		bool freeze;
	};

	struct RestrictMovesPacket {
		Opcode op;
		unsigned char nb;
		unsigned short moves[0];
	};

	union Packet {
		Opcode op;
		HelloPacket hello;
		SpeedPacket speed;
		DisplayPacket display;
		SoundPacket sound;
		StartGamePacket startGame;
		FaultPacket fault;
		ErrorPacket error;
		GameFramePacket gameFrame;
		GameInputPacket gameInput;
		GameEndedPacket gameEnded;
		SetHealthPacket setHealth;
		SetPositionPacket setPosition;
		SetWeatherPacket setWeather;
		RestrictMovesPacket restrictMoves;
	};
#pragma pack(pop)
}


#endif //SOKUAI_PACKET_HPP
