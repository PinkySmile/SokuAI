//
// Created by PinkySmile on 21/05/2021.
//

#ifndef SOKUAI_EXCEPTIONS_HPP
#define SOKUAI_EXCEPTIONS_HPP


#include <string>
#include <exception>
#include "Packet.hpp"

namespace Trainer
{
	class Exception : public std::exception {
	private:
		std::string _msg;

	public:
		Exception(const std::string &&msg) : _msg(msg) {}
		const char *what() const noexcept override { return this->_msg.c_str(); }
	};

	class ConnectionResetException : public Exception {
	public:
		ConnectionResetException() : Exception("The connection was reset") {};
	};

	class InvalidHandshakeError : public Exception {
	public:
		InvalidHandshakeError() : Exception("The handshake was invalid") {};
	};

	class GameEndedException : public Exception {
	private:
		static const std::array<std::string, 3> _winnerName;
		WinnerSide _winner;
		unsigned char _leftScore;
		unsigned char _rightScore;

	public:
		GameEndedException(WinnerSide winner, unsigned char leftScore, unsigned char rightScore) :
			Exception("Winner is " + GameEndedException::_winnerName[winner] + " with a score of " + std::to_string(leftScore) + "-"  + std::to_string(rightScore)),
			_winner(winner),
			_leftScore(leftScore),
			_rightScore(rightScore)
		{
		}

		WinnerSide getWinner() const
		{
			return this->_winner;
		}

		unsigned char getLeftScore() const
		{
			return this->_leftScore;
		}

		unsigned char getRightScore() const
		{
			return this->_rightScore;
		}
	};

	class ProtocolError : public Exception {
	private:
		static const std::array<std::string, 18> _errors;
		Errors _code;

	public:
		ProtocolError(Errors code) :
			Exception("Game responded with error code " + ProtocolError::_errors[code]),
			_code(code)
		{
		}

		Errors getCode() const
		{
			return this->_code;
		}
	};

	class InvalidPacketError : public Exception {
	public:
		InvalidPacketError() : Exception("The received packet is malformed") {};
	};

	class WSAErrorException : public Exception {
	public:
		WSAErrorException(const std::string &&fct) :
			Exception("A call to " + fct + " failed with code " + std::to_string(WSAGetLastError()) + ": " + getErrorString())
		{};

		static std::string getErrorString()
		{
			char *s = nullptr;

			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				WSAGetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPSTR>(&s),
				0,
				nullptr
			);

			std::string result = s;

			LocalFree(s);
			return result;
		}
	};

	class SystemCallFailedException : public Exception {
	public:
		SystemCallFailedException(const std::string &&fct) :
			Exception("A call to " + fct + " failed with code " + std::to_string(GetLastError()) + ": " + getErrorString())
		{};

		static std::string getErrorString()
		{
			char *s = nullptr;

			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPSTR>(&s),
				0,
				nullptr
			);

			std::string result = s;

			LocalFree(s);
			return result;
		}
	};

	class UnexpectedPacketError : public Exception {
	private:
		Packet _packet;

	public:
		UnexpectedPacketError(const Packet &packet) :
			Exception("Opcode " + std::to_string(packet.op) + " was not expected in this context."),
			_packet(packet)
		{
		}

		const Packet &getPacket() const
		{
			return this->_packet;
		}
	};
}


#endif //SOKUAI_EXCEPTIONS_HPP
