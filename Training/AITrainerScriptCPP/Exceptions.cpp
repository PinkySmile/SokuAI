//
// Created by PinkySmile on 23/05/2021.
//

#include "Exceptions.hpp"

namespace Trainer
{
	const std::array<std::string, 18> ProtocolError::_errors = {
		"INVALID_OPCODE",
		"INVALID_PACKET",
		"INVALID_LEFT_DECK",
		"INVALID_LEFT_CHARACTER",
		"INVALID_LEFT_PALETTE",
		"INVALID_RIGHT_DECK",
		"INVALID_RIGHT_CHARACTER",
		"INVALID_RIGHT_PALETTE",
		"INVALID_MUSIC",
		"INVALID_STAGE",
		"INVALID_MAGIC",
		"HELLO_NOT_SENT",
		"STILL_PLAYING",
		"POSITION_OUT_OF_BOUND",
		"INVALID_HEALTH",
		"INVALID_WEATHER",
		"INVALID_WEATHER_TIME",
		"NOT_IN_GAME"
	};

	const std::array<std::string, 3> GameEndedException::_winnerName{
		"no one",
		"left",
		"right"
	};
}