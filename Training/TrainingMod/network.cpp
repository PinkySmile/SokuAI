//
// Created by PinkySmile on 22/03/25.
//

#include <map>
#include "Packet.hpp"
#include "network.hpp"
#include "sokuData.hpp"
#include "state.hpp"


static bool hello = false;
static bool stop = false;
static SOCKET sock;
static bool displayed = true;
static int p1name = 0;
static int p2name = 0;

static void _sendError(Trainer::Errors code)
{
	Trainer::ErrorPacket packet;

	packet.op = Trainer::OPCODE_ERROR;
	packet.error = code;
	::send(sock, reinterpret_cast<const char *>(&packet), sizeof(packet), 0);
}

#define sendError(code) printf("Send error " #code " in " __FUNCSIG__ " line %i\n", __LINE__), _sendError(code)
#define sendErrorf(code, fmt, ...) printf("Send error " #code " in " __FUNCSIG__ " line %i: " fmt "\n", __LINE__, __VA_ARGS__), _sendError(code)
#define CHECK_PACKET_SIZE(requested_type, size) if (size < sizeof(requested_type)) return sendErrorf(Trainer::ERROR_INVALID_PACKET, "size < sizeof(requested_type) (%zu < %zu)", size, sizeof(requested_type)), size

static void sendOpcode(Trainer::Opcode code)
{
	::send(sock, reinterpret_cast<const char *>(&code), sizeof(code), 0);
}

void stopNetworkThread()
{
	stop = false;
}

static bool isDeckValid(SokuLib::Character chr, const unsigned char (&deck)[20])
{
	std::map<unsigned short, unsigned char> found;
	const auto &validCards = characterSpellCards.at(chr);
	const unsigned char maxSkillId = 112 + (chr == SokuLib::CHARACTER_PATCHOULI) * 3;

	for (auto card : deck) {
		auto &elem = found[card];

		elem++;
		if (elem > 4)
			return false;
		if (card <= 20)
			continue;
		if (card < 100)
			return false;
		if (card <= maxSkillId)
			continue;
		if (std::find(validCards.begin(), validCards.end(), card) == validCards.end())
			return false;
	}
	return true;
}

static bool verifyStartData(const Trainer::StartGamePacket &packet)
{
	if (SokuLib::sceneId != SokuLib::SCENE_TITLE)
		return sendError(Trainer::ERROR_STILL_PLAYING), false;

	if (packet.stageId > 19)
		return sendError(Trainer::ERROR_INVALID_STAGE), false;
	if (packet.musicId > 19)
		return sendError(Trainer::ERROR_INVALID_MUSIC), false;

	// TODO: Actually check the list of enabled characters instead
	if (packet.leftCharacter >= SokuLib::CHARACTER_RANDOM)
		return sendError(Trainer::ERROR_INVALID_LEFT_CHARACTER), false;
	if (packet.rightCharacter >= SokuLib::CHARACTER_RANDOM)
		return sendError(Trainer::ERROR_INVALID_RIGHT_CHARACTER), false;

	if (!isDeckValid(packet.leftCharacter, packet.leftDeck))
		return sendError(Trainer::ERROR_INVALID_LEFT_DECK), false;
	if (!isDeckValid(packet.rightCharacter, packet.rightDeck))
		return sendError(Trainer::ERROR_INVALID_RIGHT_DECK), false;
	return true;
}

static void renderName(SokuLib::Sprite &sprite, const char *name, size_t size, int index)
{
	SokuLib::SWRFont font;
	SokuLib::FontDescription desc;

	desc.r1 = 255;
	desc.r2 = 255;
	desc.g1 = 255;
	desc.g2 = 255;
	desc.b1 = 255;
	desc.b2 = 255;
	desc.weight = FW_NORMAL;
	desc.italic = 0;
	desc.shadow = 1;
	desc.bufferSize = 1000000;
	desc.charSpaceX = 0;
	desc.charSpaceY = 2;
	desc.offsetX = 0;
	desc.offsetY = 0;
	desc.useOffset = 0;

	strcpy(desc.faceName, *(char **)0x453BFA);
	if (index % 2 == 0) {
		desc.r2 = 0xA0;
		desc.g2 = 0xA0;
	} else {
		desc.g2 = 0x80;
		desc.b2 = 0x80;
	}

	int *r = index ? &p2name : &p1name;
	int v = ((int)size - 16) / 2;

	v = v & (v < 0) - 1;
	desc.height = 14 - v;
	desc.offsetY = v / 2;

	font.create();
	font.setIndirect(desc);
	if (*r)
		SokuLib::textureMgr.remove(*r);
	SokuLib::textureMgr.createTextTexture(r, name, font, 0x80, 0x20, nullptr, nullptr);
	sprite.setTexture2(*r, 0, 0, 0x80, 0x20);
	font.destruct();
}

static void startGame(const Trainer::StartGamePacket &startData, bool isCom)
{
	SokuLib::leftPlayerInfo.character = startData.leftCharacter;
	SokuLib::leftPlayerInfo.palette = startData.leftPalette;
	SokuLib::rightPlayerInfo.character = startData.rightCharacter;
	SokuLib::rightPlayerInfo.palette = startData.rightPalette;

	SokuLib::setBattleMode(isCom ? SokuLib::BATTLE_MODE_VSCOM : SokuLib::BATTLE_MODE_VSPLAYER, SokuLib::BATTLE_SUBMODE_PLAYING1);

	size_t left;
	size_t right;

	for (left  = 0; left < 32  && startData.leftPlayerName[left];   left++);
	for (right = 0; right < 32 && startData.rightPlayerName[right]; right++);

	if (displayed) {
		renderName(SokuLib::profile1.sprite, startData.leftPlayerName, left, 0);
		renderName(SokuLib::profile2.sprite, startData.rightPlayerName, right, 1);
	}
	SokuLib::profile1.name = {startData.leftPlayerName, startData.leftPlayerName + left};
	SokuLib::profile1.file = "SokuAI.pf";
	SokuLib::profile2.name = {startData.rightPlayerName, startData.rightPlayerName + right};
	SokuLib::profile2.file = "SokuAI.pf";
	printf("%s is fighting against %s\n", SokuLib::profile1.name.operator char *(), SokuLib::profile2.name.operator char *());

	SokuLib::leftPlayerInfo.character = startData.leftCharacter;
	SokuLib::leftPlayerInfo.palette = startData.leftPalette;
	SokuLib::leftPlayerInfo.isRight = false;
	SokuLib::leftPlayerInfo.effectiveDeck.clear();
	for (auto card : startData.leftDeck)
		SokuLib::leftPlayerInfo.effectiveDeck.push_back(card);
	SokuLib::rightPlayerInfo.character = startData.rightCharacter;
	SokuLib::rightPlayerInfo.palette = startData.rightPalette;
	SokuLib::rightPlayerInfo.isRight = true;
	SokuLib::rightPlayerInfo.effectiveDeck.clear();
	for (auto card : startData.rightDeck)
		SokuLib::rightPlayerInfo.effectiveDeck.push_back(card);

	*(char *)0x899D0C = startData.stageId;
	*(char *)0x899D0D = startData.musicId;
	//Init both inputs
	((unsigned *)0x00898680)[0] = 0x008986A8;
	((unsigned *)0x00898680)[1] = isCom ? 0 : 0x008986A8;
	resetGameState();
	sendOpcode(Trainer::OPCODE_OK);
}


void swapDisplay(bool enabled)
{
	if (!enabled == !displayed)
		return;
	displayed = enabled;
	setDisplayMode(enabled);
}

static size_t handlePacket(const Trainer::Packet &packet, unsigned int size)
{
	if (!hello && packet.op != Trainer::OPCODE_HELLO)
		return sendError(Trainer::ERROR_HELLO_NOT_SENT), size;

	switch (packet.op) {
	case Trainer::OPCODE_HELLO:
		CHECK_PACKET_SIZE(Trainer::HelloPacket, size);
		if (packet.hello.magic != PACKET_HELLO_MAGIC_NUMBER) {
			sendError(Trainer::ERROR_INVALID_MAGIC);
			return sizeof(Trainer::HelloPacket);
		}
		::send(sock, reinterpret_cast<const char *>(&packet), size, 0);
		hello = true;
		return sizeof(Trainer::HelloPacket);
	case Trainer::OPCODE_GOODBYE:
		CHECK_PACKET_SIZE(Trainer::Opcode, size);
		sendOpcode(Trainer::OPCODE_OK);
		stop = true;
		return sizeof(Trainer::Opcode);
	case Trainer::OPCODE_SPEED:
		CHECK_PACKET_SIZE(Trainer::SpeedPacket, size);
		tps = packet.speed.ticksPerSecond;
		sendOpcode(Trainer::OPCODE_OK);
		return sizeof(Trainer::SpeedPacket);
	case Trainer::OPCODE_VS_PLAYER_START_GAME:
		CHECK_PACKET_SIZE(Trainer::StartGamePacket, size);
		if (!verifyStartData(packet.startGame))
			return sizeof(Trainer::StartGamePacket);
		startGame(packet.startGame, false);
		return sizeof(Trainer::StartGamePacket);
	case Trainer::OPCODE_VS_COM_START_GAME:
		CHECK_PACKET_SIZE(Trainer::StartGamePacket, size);
		if (!verifyStartData(packet.startGame))
			return sizeof(Trainer::StartGamePacket);
		startGame(packet.startGame, true);
		return sizeof(Trainer::StartGamePacket);
	case Trainer::OPCODE_GAME_INPUTS:
		CHECK_PACKET_SIZE(Trainer::GameInputPacket, size);
		if (SokuLib::sceneId != SokuLib::SCENE_BATTLE)
			return sendError(Trainer::ERROR_NOT_IN_GAME), sizeof(Trainer::Opcode);
		inputs.first  = packet.gameInput.left;
		inputs.second = packet.gameInput.right;
		inputsReceived = true;
		return sizeof(Trainer::GameInputPacket);
	case Trainer::OPCODE_DISPLAY:
		CHECK_PACKET_SIZE(Trainer::DisplayPacket, size);
		swapDisplay(packet.display.enabled);
		sendOpcode(Trainer::OPCODE_OK);
		return sizeof(Trainer::DisplayPacket);
	case Trainer::OPCODE_GAME_CANCEL:
		CHECK_PACKET_SIZE(Trainer::Opcode, size);
		if (SokuLib::sceneId != SokuLib::SCENE_BATTLE)
			return sendError(Trainer::ERROR_NOT_IN_GAME), sizeof(Trainer::Opcode);
		sendOpcode(Trainer::OPCODE_OK);
		cancel = true;
		return sizeof(Trainer::Opcode);
	case Trainer::OPCODE_SOUND:
		CHECK_PACKET_SIZE(Trainer::SoundPacket, size);
		sendOpcode(Trainer::OPCODE_OK);
		reinterpret_cast<void (*)(int)>(0x43e200)(packet.sound.bgmVolume);
		reinterpret_cast<void (*)(int)>(0x43e230)(packet.sound.sfxVolume);
		return sizeof(Trainer::SoundPacket);
	case Trainer::OPCODE_SET_HEALTH:
		CHECK_PACKET_SIZE(Trainer::SetHealthPacket, size);
		if (SokuLib::sceneId != SokuLib::SCENE_BATTLE)
			return sendError(Trainer::ERROR_NOT_IN_GAME), sizeof(Trainer::SetHealthPacket);
		sendOpcode(Trainer::OPCODE_OK);
		if (packet.setHealth.left > 10000 || packet.setHealth.right > 10000)
			return sendError(Trainer::ERROR_INVALID_HEALTH), sizeof(Trainer::SetHealthPacket);
		SokuLib::getBattleMgr().leftCharacterManager.objectBase.hp = packet.setHealth.left;
		SokuLib::getBattleMgr().rightCharacterManager.objectBase.hp = packet.setHealth.right;
		return sizeof(Trainer::SetHealthPacket);
	case Trainer::OPCODE_SET_POSITION:
		CHECK_PACKET_SIZE(Trainer::SetPositionPacket, size);
		if (SokuLib::sceneId != SokuLib::SCENE_BATTLE)
			return sendError(Trainer::ERROR_NOT_IN_GAME), sizeof(Trainer::SetWeatherPacket);
		if (packet.setPosition.left.y < 0 || packet.setPosition.right.y < 0)
			return sendError(Trainer::ERROR_POSITION_OUT_OF_BOUND), sizeof(Trainer::SetWeatherPacket);
		if (packet.setPosition.left.x < 40 || packet.setPosition.right.x < 40)
			return sendError(Trainer::ERROR_POSITION_OUT_OF_BOUND), sizeof(Trainer::SetWeatherPacket);
		if (packet.setPosition.left.y > 1240 || packet.setPosition.right.y > 1240)
			return sendError(Trainer::ERROR_POSITION_OUT_OF_BOUND), sizeof(Trainer::SetWeatherPacket);
		SokuLib::getBattleMgr().leftCharacterManager.objectBase.position = packet.setPosition.left;
		SokuLib::getBattleMgr().rightCharacterManager.objectBase.position = packet.setPosition.right;
		sendOpcode(Trainer::OPCODE_OK);
		return sizeof(Trainer::SetPositionPacket);
	case Trainer::OPCODE_SET_WEATHER:
		CHECK_PACKET_SIZE(Trainer::SetWeatherPacket, size);
		if (SokuLib::sceneId != SokuLib::SCENE_BATTLE)
			return sendError(Trainer::ERROR_NOT_IN_GAME), sizeof(Trainer::SetWeatherPacket);
		if (packet.setWeather.timer > 999)
			return sendError(Trainer::ERROR_INVALID_WEATHER_TIME), sizeof(Trainer::SetWeatherPacket);
		if (packet.setWeather.weather > SokuLib::WEATHER_CLEAR)
			return sendError(Trainer::ERROR_INVALID_WEATHER), sizeof(Trainer::SetWeatherPacket);
		weatherTimer = packet.setWeather.timer;
		weather = packet.setWeather.weather;
		freezeWeather = packet.setWeather.freeze;
		SokuLib::displayedWeather = weather;
		SokuLib::activeWeather = SokuLib::WEATHER_CLEAR;
		SokuLib::weatherCounter = 999;
		setWeather = true;
		sendOpcode(Trainer::OPCODE_OK);
		return sizeof(Trainer::SetWeatherPacket);
	case Trainer::OPCODE_RESTRICT_MOVES:
		if (size < 2 || size != packet.restrictMoves.nb * 2 + 2)
			return sendError(Trainer::ERROR_INVALID_PACKET), size;
		moveBlacklist.clear();
		moveBlacklist.reserve(packet.restrictMoves.nb);
		moveBlacklist.insert(moveBlacklist.begin(), packet.restrictMoves.moves, packet.restrictMoves.moves + packet.restrictMoves.nb);
		sendOpcode(Trainer::OPCODE_OK);
		return packet.restrictMoves.nb * 2 + 2;
	case Trainer::OPCODE_ERROR:
	case Trainer::OPCODE_FAULT:
	case Trainer::OPCODE_GAME_FRAME:
	case Trainer::OPCODE_GAME_ENDED:
	default:
		return sendError(Trainer::ERROR_INVALID_OPCODE), size;
	}
}

static void networkThreadLoop(void *)
{
	while (!stop) {
		char buffer[1024];
		size_t offset = 0;
		int received = ::recv(sock, buffer, sizeof(buffer), 0);

		if (received <= 0) {
			stop = true;
			return;
		}
		offset = 0;
		while (offset != received)
			offset += handlePacket(*(Trainer::Packet *)&buffer[offset], received);
	}
}

int setupNetwork(int argc, char **argv)
{
	if (__argc <= 1) {
		MessageBoxA(SokuLib::window, "No port provided. Please provide a port in the command line.", "Port not given", MB_ICONERROR);
		return -1;
	}

	char *arg = __argv[__argc - 1];
	WSADATA WSAData;
	struct sockaddr_in serv_addr = {};
	char *end;
	long port = strtol(arg, &end, 0);
	char *s = nullptr;

	SetWindowTextA(SokuLib::window, ("Game on port " + std::to_string(port)).c_str());
	if (*end) {
		MessageBoxA(
			SokuLib::window,
			("Port provided as argument #" + std::to_string(__argc) + " (" + std::string(arg) +
			 ") is not a valid number.\nPlease provide a valid port in the command line.").c_str(),
			"Port invalid",
			MB_ICONERROR
		);
		return -1;
	}
	if (port < 0 || port > 65535) {
		MessageBoxA(
			SokuLib::window,
			("Port provided #" + std::to_string(__argc) + " (" + std::string(arg) +
			 ") is not a valid port (not in range 0-65535).\nPlease provide a valid port in the command line.").c_str(),
			"Port invalid",
			MB_ICONERROR
		);
		return -1;
	}
	if (WSAStartup(MAKEWORD(2, 0), &WSAData) < 0) {
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&s,
			0,
			nullptr
		);
		MessageBoxA(SokuLib::window, (std::string("WSAStartup failed\n\nWSAGetLastError(): ") + std::to_string(WSAGetLastError()) + ": " + s).c_str(), "Init failure", MB_ICONERROR);
		return -1;
	}

	/* create the socket */
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&s,
			0,
			nullptr
		);
		MessageBoxA(SokuLib::window, (std::string("Creating socket failed\n\nWSAGetLastError(): ") + std::to_string(WSAGetLastError()) + ": " + s).c_str(), "Init failure", MB_ICONERROR);
		return -1;
	}

	/* fill in the structure */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.S_un.S_un_b.s_b1 = 127;
	serv_addr.sin_addr.S_un.S_un_b.s_b2 = 0;
	serv_addr.sin_addr.S_un.S_un_b.s_b3 = 0;
	serv_addr.sin_addr.S_un.S_un_b.s_b4 = 1;

	/* connect the socket */
	if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&s,
			0,
			nullptr
		);
		MessageBoxA(SokuLib::window, (
			std::string("Cannot connect to ") + inet_ntoa(serv_addr.sin_addr) + " on port " + std::to_string(port) +
			".\nPort gotten from command line argument #" + std::to_string(__argc) +
			" (" + arg + ").\n\nWSAGetLastError(): " + std::to_string(WSAGetLastError()) + ": " + s
		).c_str(), "Connection failure", MB_ICONERROR);
		return -1;
	}
	_beginthread(networkThreadLoop, 0, nullptr);
	return 0;
}

void sendState(const SokuLib::BattleManager &mgr)
{
	auto &p1 = *(SokuLib::v2::Player *)&mgr.leftCharacterManager;
	auto &p2 = *(SokuLib::v2::Player *)&mgr.rightCharacterManager;
	auto &obj1 = p1.objectList->getList();
	auto &obj2 = p2.objectList->getList();
	size_t allocSize = sizeof(Trainer::GameFramePacket) + (obj1.size() + obj2.size()) * sizeof(Trainer::Object);
	char *buffer = new char[allocSize];
	auto *packet = reinterpret_cast<Trainer::GameFramePacket *>(buffer);
	int current = 0;

	fillState(p1, packet->leftState);
	fillState(p2, packet->rightState);
	packet->op = Trainer::OPCODE_GAME_FRAME;
	packet->weatherTimer = SokuLib::weatherCounter;
	packet->displayedWeather = SokuLib::displayedWeather;
	packet->activeWeather = SokuLib::activeWeather;
	for (auto obj : obj1)
		fillState(*obj, packet->objects[current++]);
	for (auto obj : obj2)
		fillState(*obj, packet->objects[current++]);
	send(sock, buffer, allocSize, 0);
	delete[] buffer;
}

void sendGameResult(const SokuLib::BattleManager &mgr)
{
	Trainer::GameEndedPacket packet;
	auto &p1 = *(SokuLib::v2::Player *)&mgr.leftCharacterManager;
	auto &p2 = *(SokuLib::v2::Player *)&mgr.rightCharacterManager;

	packet.op = Trainer::OPCODE_GAME_ENDED;
	packet.winner = 0 + (p1.score == 2) + (p2.score == 2) * 2;
	packet.leftScore = p1.score;
	packet.rightScore = p2.score;
	send(sock, reinterpret_cast<const char *>(&packet), sizeof(packet), 0);
}

bool isStopped()
{
	return stop;
}

void sendFault(PEXCEPTION_POINTERS ExPtr)
{
	Trainer::FaultPacket packet;

	printf("Caught exception 0x%lX at address 0x%lX\n", ExPtr->ExceptionRecord->ExceptionCode, ExPtr->ExceptionRecord->ExceptionAddress);
	packet.op = Trainer::OPCODE_FAULT;
	packet.faultExceptionCode = ExPtr->ExceptionRecord->ExceptionCode;
	packet.faultAddress = ExPtr->ExceptionRecord->ExceptionAddress;
	send(sock, reinterpret_cast<const char *>(&packet), sizeof(packet), 0);
}