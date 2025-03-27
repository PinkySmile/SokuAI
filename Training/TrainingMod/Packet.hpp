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
		/* 0  */ OPCODE_HELLO,
		/* 1  */ OPCODE_GOODBYE,
		/* 2  */ OPCODE_SPEED,
		/* 3  */ OPCODE_DISPLAY,
		/* 4  */ OPCODE_SOUND,
		/* 5  */ OPCODE_VS_PLAYER_START_GAME,
		/* 6  */ OPCODE_FAULT,
		/* 7  */ OPCODE_ERROR,
		/* 8  */ OPCODE_GAME_FRAME,
		/* 9  */ OPCODE_GAME_INPUTS,
		/* 10 */ OPCODE_GAME_CANCEL,
		/* 11 */ OPCODE_GAME_ENDED,
		/* 12 */ OPCODE_OK,
		/* 13 */ OPCODE_SET_HEALTH,
		/* 14 */ OPCODE_SET_POSITION,
		/* 15 */ OPCODE_SET_WEATHER,
		/* 16 */ OPCODE_RESTRICT_MOVES,
		/* 17 */ OPCODE_VS_COM_START_GAME,
	};

	enum Errors : unsigned char {
		/* 0  */ ERROR_INVALID_OPCODE,
		/* 1  */ ERROR_INVALID_PACKET,
		/* 2  */ ERROR_INVALID_LEFT_DECK,
		/* 3  */ ERROR_INVALID_LEFT_CHARACTER,
		/* 4  */ ERROR_INVALID_LEFT_PALETTE,
		/* 5  */ ERROR_INVALID_RIGHT_DECK,
		/* 6  */ ERROR_INVALID_RIGHT_CHARACTER,
		/* 7  */ ERROR_INVALID_RIGHT_PALETTE,
		/* 8  */ ERROR_INVALID_MUSIC,
		/* 9  */ ERROR_INVALID_STAGE,
		/* 10 */ ERROR_INVALID_MAGIC,
		/* 11 */ ERROR_HELLO_NOT_SENT,
		/* 12 */ ERROR_STILL_PLAYING,
		/* 13 */ ERROR_POSITION_OUT_OF_BOUND,
		/* 14 */ ERROR_INVALID_HEALTH,
		/* 15 */ ERROR_INVALID_WEATHER,
		/* 16 */ ERROR_INVALID_WEATHER_TIME,
		/* 17 */ ERROR_NOT_IN_GAME,
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
		char direction;
		SokuLib::Vector2f position;
		unsigned short action;
		unsigned short sequence;
		unsigned short hitStop;
		unsigned short imageID;
	};

	struct CharacterState {
		char direction;
		SokuLib::Vector2f position;
		unsigned short action;
		unsigned short sequence;
		unsigned short pose;
		unsigned short poseFrame;
		unsigned int frameCount;
		unsigned short comboDamage;
		unsigned short comboLimit;
		bool airBorne;
		unsigned short timeStop;
		unsigned short hitStop;
		unsigned short hp;
		unsigned char airDashCount;
		unsigned short spirit;
		unsigned short maxSpirit;
		unsigned short untech;
		unsigned short healingCharm;
		unsigned short confusion;
		unsigned short swordOfRapture;
		unsigned char score;
		std::pair<unsigned char, unsigned char> hand[5];
		unsigned short cardGauge;
		SokuLib::Skill skills[15];
		unsigned char fanLevel;
		unsigned char rodsLevel;
		unsigned char booksLevel;
		unsigned char dollLevel;
		unsigned char dropLevel;
		unsigned short dropInvulTimeLeft;
		unsigned short superArmorHp;
		unsigned short imageID;
		unsigned char characterSpecificData[20];
		unsigned char objectCount;
	};

	struct GameFramePacket {
		Opcode op;
		CharacterState leftState;
		CharacterState rightState;
		char displayedWeather;
		char activeWeather;
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
		unsigned char winner;
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
