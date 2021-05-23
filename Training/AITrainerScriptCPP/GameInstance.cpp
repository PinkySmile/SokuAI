//
// Created by PinkySmile on 21/05/2021.
//

#include "GameInstance.hpp"
#include "Exceptions.hpp"

namespace Trainer
{
	template<typename T>
	class GuardedPtr {
	private:
		T *_ptr;

	public:
		GuardedPtr() : _ptr(nullptr) {}
		GuardedPtr(T *p) : _ptr(p) {}
		~GuardedPtr() { this->remove(); }
		GuardedPtr<T> &operator=(T *p) { this->remove(); this->_ptr = p; return *this; }
		GuardedPtr<T> &operator=(GuardedPtr<T> *p) = delete;
		void remove() { delete[] reinterpret_cast<char *>(this->_ptr); this->_ptr = nullptr; }
		T &operator*() noexcept { return *this->_ptr; }
		const T &operator*() const noexcept { return *this->_ptr; }
		T *operator->() noexcept { return this->_ptr; }
		const T *operator->() const noexcept { return this->_ptr; }
	};

	inline int _recv(SOCKET sock, void *buffer, size_t size, int flags, int timeout)
	{
		if (timeout >= 0) {
			struct timeval t{
				timeout,
				0
			};
			FD_SET set;

			FD_ZERO(&set);
			FD_SET(sock, &set);
			if (select(FD_SETSIZE, &set, nullptr, nullptr, &t) == 0) {
				printf("Timeout after %i seconds\n", timeout);
				throw ConnectionResetException();
			}
		}

		int received = recv(sock, reinterpret_cast<char *>(buffer), size, flags);

		if (received <= 0)
			throw ConnectionResetException();
		return received;
	}

	inline Packet *_recvPacket(SOCKET sock, int timeout = 30)
	{
		Opcode op;
		int received = _recv(sock, reinterpret_cast<char *>(&op), sizeof(op), 0, timeout);
		size_t expected;
		Packet *buffer;
		Packet *buffer2;

		switch (op) {
		case OPCODE_HELLO:
			expected = sizeof(HelloPacket) - 1;
			break;
		case OPCODE_GOODBYE:
			expected = sizeof(Opcode) - 1;
			break;
		case OPCODE_SPEED:
			expected = sizeof(SpeedPacket) - 1;
			break;
		case OPCODE_START_GAME:
			expected = sizeof(StartGamePacket) - 1;
			break;
		case OPCODE_GAME_INPUTS:
			expected = sizeof(GameInputPacket) - 1;
			break;
		case OPCODE_DISPLAY:
			expected = sizeof(DisplayPacket) - 1;
			break;
		case OPCODE_GAME_CANCEL:
			expected = sizeof(Opcode) - 1;
			break;
		case OPCODE_SOUND:
			expected = sizeof(SoundPacket) - 1;
			break;
		case OPCODE_SET_HEALTH:
			expected = sizeof(SetHealthPacket) - 1;
			break;
		case OPCODE_SET_POSITION:
			expected = sizeof(SetPositionPacket) - 1;
			break;
		case OPCODE_SET_WEATHER:
			expected = sizeof(SetWeatherPacket) - 1;
			break;
		case OPCODE_ERROR:
			expected = sizeof(ErrorPacket) - 1;
			break;
		case OPCODE_FAULT:
			expected = sizeof(FaultPacket) - 1;
			break;
		case OPCODE_GAME_FRAME:
			expected = sizeof(GameFramePacket) - 1;
			break;
		case OPCODE_GAME_ENDED:
			expected = sizeof(GameEndedPacket) - 1;
			break;
		case OPCODE_OK:
			expected = sizeof(Opcode) - 1;
			break;
		default:
			throw InvalidPacketError();
		}

		buffer = reinterpret_cast<Packet *>(new char[expected + 1]);
		buffer->op = op;
		if (expected != 0 && _recv(sock, reinterpret_cast<char *>(buffer) + 1, expected, 0, timeout) != expected) {
			delete[] buffer;
			throw InvalidPacketError();
		}

		if (buffer->op != OPCODE_GAME_FRAME)
			return buffer;

		if (buffer->gameFrame.leftState.objectCount || buffer->gameFrame.rightState.objectCount) {
			auto added = (buffer->gameFrame.leftState.objectCount + buffer->gameFrame.rightState.objectCount) * sizeof(*buffer->gameFrame.objects);

			buffer2 = reinterpret_cast<Packet *>(new char[expected + 1 + added]);
			memcpy(buffer2, buffer, expected + 1);
			delete[] buffer;
			if (added != 0 && _recv(sock, reinterpret_cast<char *>(&buffer2->gameFrame.objects), added, 0, timeout) != added) {
				delete[] buffer2;
				throw InvalidPacketError();
			}
			return buffer2;
		}
		return buffer;
	}

	GameInstance::GameFrame GameInstance::_recvGameFrame(int timeout)
	{
		GuardedPtr<Packet> packet = _recvPacket(this->_socket, timeout);
		GameFrame frame;

		if (packet->op == OPCODE_GAME_ENDED)
			throw GameEndedException(packet->gameEnded.winner, packet->gameEnded.leftScore, packet->gameEnded.rightScore);
		if (packet->op == OPCODE_ERROR)
			throw ProtocolError(packet->error.error);
		if (packet->op != OPCODE_GAME_FRAME)
			throw UnexpectedPacketError(*packet);

		frame.left             = packet->gameFrame.leftState;
		frame.right            = packet->gameFrame.rightState;
		frame.displayedWeather = packet->gameFrame.displayedWeather;
		frame.activeWeather    = packet->gameFrame.activeWeather;
		frame.weatherTimer     = packet->gameFrame.weatherTimer;
		frame.leftObjects     = {packet->gameFrame.objects, &packet->gameFrame.objects[frame.left.objectCount]};
		frame.leftObjects     = {&packet->gameFrame.objects[frame.left.objectCount], &packet->gameFrame.objects[frame.left.objectCount + frame.right.objectCount]};
		return frame;
	}

	GameInstance::GameInstance(unsigned short port) :
		_port(port)
	{
		sockaddr_in sockaddrIn;
		WSADATA WSAData;

		if (WSAStartup(MAKEWORD(2, 2), &WSAData))
			throw WSAErrorException("WSAStartup");

		this->_baseSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		sockaddrIn.sin_family = AF_INET;
		sockaddrIn.sin_port = htons(port);
		sockaddrIn.sin_addr.S_un.S_un_b.s_b1 = 127;
		sockaddrIn.sin_addr.S_un.S_un_b.s_b2 = 0;
		sockaddrIn.sin_addr.S_un.S_un_b.s_b3 = 0;
		sockaddrIn.sin_addr.S_un.S_un_b.s_b4 = 1;
		if (bind(this->_baseSocket, reinterpret_cast<struct sockaddr *>(&sockaddrIn), sizeof(sockaddrIn)) < 0)
			throw WSAErrorException("bind");
		if (listen(this->_baseSocket, 1))
			throw WSAErrorException("listen");
	}

	void GameInstance::reconnect(const std::string &path, const char *iniPath, bool justConnect)
	{
		HelloPacket packet;
		GuardedPtr<Packet> result;

		if (this->_socket != INVALID_SOCKET) {
			this->quit();
			closesocket(this->_socket);
			this->terminate();
		}
		printf("Starting instance %s %s %u %s\n", path.c_str(), iniPath, this->_port, justConnect ? "true" : "false");
		if (!justConnect) {
			std::string cmdLine = "\"" + path + "\" ";
			char buffer[32767];
			STARTUPINFOA startupInfo;

			startupInfo.cb = sizeof(startupInfo);
			startupInfo.lpReserved = nullptr;
			startupInfo.lpDesktop = nullptr;
			startupInfo.lpTitle = nullptr;
			startupInfo.dwX = 0;
			startupInfo.dwY = 0;
			startupInfo.dwXSize = 0;
			startupInfo.dwYSize = 0;
			startupInfo.dwXCountChars = 0;
			startupInfo.dwYCountChars = 0;
			startupInfo.dwFillAttribute = 0;
			startupInfo.dwFlags = 0;
			startupInfo.wShowWindow = SW_HIDE;
			startupInfo.cbReserved2 = 0;
			startupInfo.lpReserved2 = nullptr;
			startupInfo.hStdInput = INVALID_HANDLE_VALUE;
			startupInfo.hStdOutput = INVALID_HANDLE_VALUE;
			startupInfo.hStdError = INVALID_HANDLE_VALUE;
			if (iniPath) {
				cmdLine.reserve(cmdLine.size() + 3 + strlen(iniPath));
				cmdLine += "\"";
				cmdLine += iniPath;
				cmdLine += "\" ";
			}
			cmdLine += std::to_string(this->_port);

			strcpy_s(buffer, cmdLine.c_str());
			if (!CreateProcessA(
				path.c_str(),
				buffer,
				nullptr,
				nullptr,
				false,
				CREATE_NEW_CONSOLE,
				nullptr,
				nullptr,
				&startupInfo,
				&this->_processInformation
			))
				throw SystemCallFailedException("CreateProcessA");
			if (!SetPriorityClass(this->_processInformation.hProcess, HIGH_PRIORITY_CLASS))
				throw SystemCallFailedException("SetPriorityClass");
		}
		this->_socket = ::accept(this->_baseSocket, nullptr, nullptr);
		packet.op = OPCODE_HELLO;
		packet.magic = PACKET_HELLO_MAGIC_NUMBER;
		// Send hello packet
		send(this->_socket, reinterpret_cast<const char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op != OPCODE_HELLO || result->hello.magic != PACKET_HELLO_MAGIC_NUMBER)
			throw InvalidHandshakeError();
	}

	GameInstance::GameFrame GameInstance::startGame(const GameInstance::StartGameParams &params)
	{
		StartGamePacket packet;
		GuardedPtr<Packet> result;

		packet.op = OPCODE_START_GAME;
		packet.leftCharacter = params.left.character;
		memcpy(packet.leftDeck, params.left.deck, sizeof(packet.leftDeck));
		packet.leftPalette = params.left.palette;
		packet.rightCharacter = params.right.character;
		memcpy(packet.rightDeck, params.right.deck, sizeof(packet.leftDeck));
		packet.rightPalette = params.right.palette;
		packet.musicId = params.music;
		packet.stageId = params.stage;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
		if (result->op != OPCODE_OK)
			throw UnexpectedPacketError(*result);
		return this->_recvGameFrame(60);
	}

	GameInstance::GameFrame GameInstance::tick(GameInstance::GameInputs inputs)
	{
		GameInputPacket packet;

		packet.op = OPCODE_GAME_INPUTS;
		packet.left = inputs.left;
		packet.right = inputs.right;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		return this->_recvGameFrame(10);
	}

	void GameInstance::quit()
	{
		Opcode op;
		GuardedPtr<Packet> result;

		op = OPCODE_GOODBYE;
		try {
			send(this->_socket, reinterpret_cast<char *>(&op), sizeof(op), 0);
			result = _recvPacket(this->_socket);
			if (result->op == OPCODE_ERROR)
				throw ProtocolError(result->error.error);
			_recvPacket(this->_socket);
			throw Exception("Wtf ?");
		} catch (ConnectionResetException &) {
			this->_socket = INVALID_SOCKET;
		}
	}

	void GameInstance::setGameSpeed(unsigned int tps)
	{
		SpeedPacket packet;
		GuardedPtr<Packet> result;

		packet.op = OPCODE_SPEED;
		packet.ticksPerSecond = tps;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
	}

	void GameInstance::endGame()
	{
		Opcode packet;
		GuardedPtr<Packet> result;

		packet = OPCODE_GAME_CANCEL;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
	}

	void GameInstance::setDisplayMode(bool shown)
	{
		DisplayPacket packet;
		GuardedPtr<Packet> result;

		packet.op = OPCODE_DISPLAY;
		packet.enabled = shown;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
	}

	void GameInstance::setGameVolume(unsigned char sfx, unsigned char bgm)
	{
		SoundPacket packet;
		GuardedPtr<Packet> result;

		packet.op = OPCODE_SOUND;
		packet.sfxVolume = sfx;
		packet.bgmVolume = bgm;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
	}

	void GameInstance::setPositions(SokuLib::Vector2f left, SokuLib::Vector2f right)
	{
		SetPositionPacket packet;
		GuardedPtr<Packet> result;

		packet.op = OPCODE_SET_POSITION;
		packet.left = left;
		packet.right = right;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
	}

	void GameInstance::setHealth(unsigned short left, unsigned short right)
	{
		SetHealthPacket packet;
		GuardedPtr<Packet> result;

		packet.op = OPCODE_SET_HEALTH;
		packet.left = left;
		packet.right = right;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
	}

	void GameInstance::setWeather(SokuLib::Weather weather, int timer, bool freezeTimer)
	{
		SetWeatherPacket packet;
		GuardedPtr<Packet> result;

		packet.op = OPCODE_SET_WEATHER;
		packet.weather = weather;
		packet.timer = timer;
		packet.freeze = freezeTimer;
		send(this->_socket, reinterpret_cast<char *>(&packet), sizeof(packet), 0);
		result = _recvPacket(this->_socket);
		if (result->op == OPCODE_ERROR)
			throw ProtocolError(result->error.error);
	}

	DWORD GameInstance::terminate()
	{
		if (this->_processInformation.hProcess != INVALID_HANDLE_VALUE) {
			TerminateProcess(this->_processInformation.hProcess, 0);
			WaitForSingleObject(this->_processInformation.hProcess, INFINITE);

			auto code = this->getExitCode();

			CloseHandle(this->_processInformation.hThread);
			this->_processInformation.hProcess = INVALID_HANDLE_VALUE;
			return code;
		}
		return -1;
	}

	DWORD GameInstance::getExitCode()
	{
		DWORD result;

		GetExitCodeProcess(this->_processInformation.hProcess, &result);
		return result;
	}
}