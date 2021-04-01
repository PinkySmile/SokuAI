#include <windows.h>
#include <shlwapi.h>
#include <SokuLib.hpp>
#include <thread>
#include <optional>
#include <map>
#include "Packet.hpp"
#include "Exceptions.hpp"

#define CHECK_PACKET_SIZE(requested_type, size) if (size != sizeof(requested_type)) return sendError(Trainer::ERROR_INVALID_PACKET);

struct T {};
static DWORD s_origCBattle_OnRenderAddr;
static DWORD s_origCSelect_OnRenderAddr;
static int (T::*s_origCLogo_OnProcess)();
static int (T::*s_origCBattle_OnProcess)();
static int (T::*s_origCSelect_OnProcess)();
void (__stdcall *s_origLoadDeckData)(char *, void *, SokuLib::deckInfo &, int, SokuLib::mVC9Dequeue<short> &);
static void (SokuLib::KeymapManager::*s_origKeymapManager_SetInputs)();

static SOCKET sock;
static std::thread thread;
static bool hello = false;
static bool stop = false;
static bool displayed = true;
static bool player = false;
static bool begin = true;

static bool startRequested = false;

static unsigned counter = 0;
static unsigned tps = 60;
static unsigned char decks[40];
static std::pair<unsigned char, unsigned char> palettes;
static std::optional<Trainer::Input> leftInputs;
static std::optional<Trainer::Input> rightInputs;
static const std::map<SokuLib::Character, std::vector<unsigned short>> characterSpellCards{
	{SokuLib::CHARACTER_ALICE, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211}},
	{SokuLib::CHARACTER_AYA, {200, 201, 202, 203, 205, 206, 207, 208, 211, 212}},
	{SokuLib::CHARACTER_CIRNO, {200, 201, 202, 203, 204, 205, 206, 207, 208, 210, 213}},
	{SokuLib::CHARACTER_IKU, {200, 201, 202, 203, 206, 207, 208, 209, 210, 211}},
	{SokuLib::CHARACTER_KOMACHI, {200, 201, 202, 203, 204, 205, 206, 207, 211}},
	{SokuLib::CHARACTER_MARISA, {200, 202, 203, 204, 205, 206, 207, 208, 209, 211, 212, 214, 215, 219}},
	{SokuLib::CHARACTER_MEILING, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 211}},
	{SokuLib::CHARACTER_PATCHOULI, {200, 201, 202, 203, 204, 205, 206, 207, 210, 211, 212, 213}},
	{SokuLib::CHARACTER_REIMU, {200, 201, 204, 206, 207, 208, 209, 210, 214, 219}},
	{SokuLib::CHARACTER_REMILIA, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209}},
	{SokuLib::CHARACTER_SAKUYA, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212}},
	{SokuLib::CHARACTER_SANAE, {200, 201, 202, 203, 204, 205, 206, 207, 210}},
	{SokuLib::CHARACTER_SUIKA, {200, 201, 202, 203, 204, 205, 206, 207, 208, 212}},
	{SokuLib::CHARACTER_SUWAKO, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 212}},
	{SokuLib::CHARACTER_TENSHI, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209}},
	{SokuLib::CHARACTER_REISEN, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211}},
	{SokuLib::CHARACTER_UTSUHO, {200, 201, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214}},
	{SokuLib::CHARACTER_YOUMU, {200, 201, 202, 203, 204, 205, 206, 207, 208, 212}},
	{SokuLib::CHARACTER_YUKARI, {200, 201, 202, 203, 204, 205, 206, 207, 208, 215}},
	{SokuLib::CHARACTER_YUYUKO, {200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 219}}
};

void __fastcall KeymapManagerSetInputs(SokuLib::KeymapManager *This)
{
	(This->*s_origKeymapManager_SetInputs)();
	if (begin && SokuLib::sceneId == SokuLib::SCENE_TITLE) {
		begin = false;
		memset(&This->input, 0, sizeof(This->input));
		This->input.a = 1;
	}
	if (startRequested) {
		memset(&This->input, 0, sizeof(This->input));
		This->input.a = 1;
		SokuLib::leftPlayerInfo.palette = palettes.first;
		SokuLib::rightPlayerInfo.palette = palettes.second;
	}
	//static bool lasts = false;
	//if (SokuLib::sceneId == SokuLib::SCENE_SELECTCL || SokuLib::sceneId == SokuLib::SCENE_SELECTSV) {
	//	memset(&This->input, 0, sizeof(This->input));
	//	This->input.a = lasts;

	//	lasts = !lasts;

	//	SokuLib::leftPlayerInfo.character = SokuLib::CHARACTER_REMILIA;
	//	SokuLib::leftPlayerInfo.palette = 0;
	//	SokuLib::leftPlayerInfo.deck = 0;
	//	SokuLib::rightPlayerInfo.character = SokuLib::CHARACTER_REMILIA;
	//	SokuLib::rightPlayerInfo.palette = 5;
	//	SokuLib::rightPlayerInfo.deck = 0;
	//	*(unsigned *)(*(unsigned *)0x8A000C + 0x2438) = 13;// Stage
	//	*(unsigned *)(*(unsigned *)0x8A000C + 0x244C) = 13;// Music
	//}
}

void __stdcall loadDeckData(char *charName, void *csvFile, SokuLib::deckInfo &deck, int param4, SokuLib::mVC9Dequeue<short> &newDeck)
{
	for (int i = 0; i < 20; i++)
		newDeck[i] = decks[player * 20 + i];
	player = !player;
	return s_origLoadDeckData(charName, csvFile, deck, param4, newDeck);
}

int dummyFunction()
{
	return 0;
}

static void sendError(Trainer::Errors code)
{
	Trainer::ErrorPacket packet;

	packet.op = Trainer::OPCODE_ERROR;
	packet.error = code;
	::send(sock, reinterpret_cast<const char *>(&packet), sizeof(packet), 0);
}

static void swapDisplay(bool enabled)
{
	if (!enabled == !displayed)
		return;

	DWORD old;

	displayed = enabled;
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	if (!displayed) {
		s_origCBattle_OnRenderAddr = SokuLib::TamperDword(SokuLib::vtbl_CBattle + SokuLib::OFFSET_ON_RENDER, (DWORD)dummyFunction);
		s_origCSelect_OnRenderAddr = SokuLib::TamperDword(SokuLib::vtbl_CSelect + SokuLib::OFFSET_ON_RENDER, (DWORD)dummyFunction);
	} else {
		SokuLib::TamperDword(SokuLib::vtbl_CBattle + SokuLib::OFFSET_ON_RENDER, s_origCBattle_OnRenderAddr);
		SokuLib::TamperDword(SokuLib::vtbl_CSelect + SokuLib::OFFSET_ON_RENDER, s_origCSelect_OnRenderAddr);
	}
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);
}

static void startGame(const Trainer::StartGamePacket &startData)
{
	SokuLib::changeScene(SokuLib::SCENE_SELECT);
	SokuLib::waitForSceneChange();
	SokuLib::setBattleMode(SokuLib::BATTLE_MODE_VSPLAYER, SokuLib::BATTLE_SUBMODE_PLAYING2);

	SokuLib::leftPlayerInfo.character = startData.leftCharacter;
	palettes.first = startData.leftPalette;
	SokuLib::leftPlayerInfo.deck = 0;
	SokuLib::rightPlayerInfo.character = startData.rightCharacter;
	palettes.second = startData.rightPalette;
	SokuLib::rightPlayerInfo.deck = 0;
	*(unsigned *)(*(unsigned *)0x8A000C + 0x2438) = startData.stageId;// Stage
	*(unsigned *)(*(unsigned *)0x8A000C + 0x244C) = startData.musicId;// Music
	((unsigned *)0x00898680)[0] = 0x008986A8;
	((unsigned *)0x00898680)[1] = 0x008986A8;
	memcpy(decks, startData.leftDeck, 20);
	memcpy(decks + 20, startData.rightDeck, 20);

	startRequested = true;
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
	if (packet.leftCharacter >= SokuLib::CHARACTER_RANDOM)
		return sendError(Trainer::ERROR_INVALID_LEFT_CHARACTER), false;
	if (packet.rightCharacter >= SokuLib::CHARACTER_RANDOM)
		return sendError(Trainer::ERROR_INVALID_RIGHT_CHARACTER), false;
	if (packet.leftPalette > 7)
		return sendError(Trainer::ERROR_INVALID_LEFT_PALETTE), false;
	if (packet.rightPalette > 7)
		return sendError(Trainer::ERROR_INVALID_RIGHT_PALETTE), false;
	if (!isDeckValid(packet.leftCharacter, packet.leftDeck))
		return sendError(Trainer::ERROR_INVALID_LEFT_DECK), false;
	if (!isDeckValid(packet.rightCharacter, packet.rightDeck))
		return sendError(Trainer::ERROR_INVALID_RIGHT_DECK), false;
	return true;
}

static void handlePacket(const Trainer::Packet &packet, unsigned int size)
{
	if (!hello && packet.op != Trainer::OPCODE_HELLO)
		return sendError(Trainer::ERROR_HELLO_NOT_SENT);

	switch (packet.op) {
	case Trainer::OPCODE_HELLO:
		CHECK_PACKET_SIZE(Trainer::HelloPacket, size);
		if (packet.hello.magic != PACKET_HELLO_MAGIC_NUMBER) {
			sendError(Trainer::ERROR_INVALID_MAGIC);
			break;
		}
		::send(sock, reinterpret_cast<const char *>(&packet), size, 0);
		hello = true;
		return;
	case Trainer::OPCODE_GOODBYE:
		CHECK_PACKET_SIZE(Trainer::Opcode, size);
		stop = true;
		return;
	case Trainer::OPCODE_SPEED:
		CHECK_PACKET_SIZE(Trainer::SpeedPacket, size);
		tps = packet.speed.ticksPerSecond;
		return;
	case Trainer::OPCODE_START_GAME:
		CHECK_PACKET_SIZE(Trainer::StartGamePacket, size);
		if (!verifyStartData(packet.startGame))
			return;
		return startGame(packet.startGame);
	case Trainer::OPCODE_GAME_INPUTS:
		CHECK_PACKET_SIZE(Trainer::GameInputPacket, size);
		leftInputs  = packet.gameInput.left;
		rightInputs = packet.gameInput.right;
		return;
	case Trainer::OPCODE_DISPLAY:
		CHECK_PACKET_SIZE(Trainer::DisplayPacket, size);
		swapDisplay(packet.display.enabled);
		return;
	case Trainer::OPCODE_GAME_CANCEL:
		CHECK_PACKET_SIZE(Trainer::Opcode, size);
		return;
	case Trainer::OPCODE_SOUND:
		CHECK_PACKET_SIZE(Trainer::SoundPacket, size);
		return;
	case Trainer::OPCODE_ERROR:
	case Trainer::OPCODE_FAULT:
	case Trainer::OPCODE_GAME_FRAME:
	case Trainer::OPCODE_GAME_ENDED:
	default:
		return sendError(Trainer::ERROR_INVALID_OPCODE);
	}
}

static void threadLoop(void *)
{
	while (!stop) {
		Trainer::Packet packet;
		int received = ::recv(sock, reinterpret_cast<char *>(&packet), sizeof(packet), 0);

		if (received == 0) {
			stop = true;
			return;
		}
		handlePacket(packet, received);
	}
}

int __fastcall CLogo_OnProcess(T *This) {
	int ret = (This->*s_origCLogo_OnProcess)();

	if (ret == SokuLib::SCENE_TITLE) {
		char *arg = __argv[__argc - 1];
		WSADATA WSAData;
		struct sockaddr_in serv_addr = {};

		WSAStartup(MAKEWORD(2, 0), &WSAData);

		/* create the socket */
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET)
			throw SocketCreationErrorException(strerror(errno));

		/* fill in the structure */
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(atoi(arg));
		serv_addr.sin_addr.S_un.S_un_b.s_b1 = 127;
		serv_addr.sin_addr.S_un.S_un_b.s_b2 = 0;
		serv_addr.sin_addr.S_un.S_un_b.s_b3 = 0;
		serv_addr.sin_addr.S_un.S_un_b.s_b4 = 1;

		/* connect the socket */
		if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
			throw ConnectException(std::string("Cannot connect to ") + inet_ntoa(serv_addr.sin_addr));
		_beginthread(threadLoop, 0, nullptr);
	}
	return ret;
}

int __fastcall CSelect_OnProcess(T *This) {
	int ret = (This->*s_origCSelect_OnProcess)();

	//*0x8A000C + 0x2438 Stage
	//*0x8A000C + 0x244C Music
	if (stop)
		return -1;
	//if (ret == SokuLib::SCENE_LOADING)
	//	return SokuLib::SCENE_LOADING;
	//return SokuLib::SCENE_SELECT;
	startRequested &= ret == SokuLib::SCENE_SELECT;
	return ret;
}

int __fastcall CBattle_OnProcess(T *This) {
	int ret = (This->*s_origCBattle_OnProcess)();

	if (stop)
		return -1;
	return ret;
}

// 設定ロード
void LoadSettings(LPCSTR profilePath) {
	// 自動シャットダウン
}

extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
	return ::memcmp(SokuLib::targetHash, hash, sizeof SokuLib::targetHash) == 0;
}

#define PAYLOAD_ADDRESS_GET_INPUTS 0x40A45E
#define PAYLOAD_NEXT_INSTR_GET_INPUTS (PAYLOAD_ADDRESS_GET_INPUTS + 4)
#define PAYLOAD_ADDRESS_DECK_INFOS 0x437D24
#define PAYLOAD_NEXT_INSTR_DECK_INFOS (PAYLOAD_ADDRESS_DECK_INFOS + 4)

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
	char profilePath[1024 + MAX_PATH];

	GetModuleFileName(hMyModule, profilePath, 1024);
	PathRemoveFileSpec(profilePath);
	PathAppend(profilePath, "SokuAITrainer.ini");
	LoadSettings(profilePath);

	DWORD old;
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	s_origCLogo_OnProcess   = SokuLib::union_cast<int (T::*)()>(SokuLib::TamperDword(SokuLib::vtbl_CLogo + SokuLib::OFFSET_ON_PROCESS,   (DWORD)CLogo_OnProcess));
	s_origCBattle_OnProcess = SokuLib::union_cast<int (T::*)()>(SokuLib::TamperDword(SokuLib::vtbl_CBattle + SokuLib::OFFSET_ON_PROCESS, (DWORD)CBattle_OnProcess));
	s_origCSelect_OnProcess = SokuLib::union_cast<int (T::*)()>(SokuLib::TamperDword(SokuLib::vtbl_CSelect + SokuLib::OFFSET_ON_PROCESS, (DWORD)CSelect_OnProcess));
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	int newOffset = (int)KeymapManagerSetInputs - PAYLOAD_NEXT_INSTR_GET_INPUTS;
	s_origKeymapManager_SetInputs = SokuLib::union_cast<void (SokuLib::KeymapManager::*)()>(*(int *)PAYLOAD_ADDRESS_GET_INPUTS + PAYLOAD_NEXT_INSTR_GET_INPUTS);
	*(int *)PAYLOAD_ADDRESS_GET_INPUTS = newOffset;

	newOffset = reinterpret_cast<int>(loadDeckData) - PAYLOAD_NEXT_INSTR_DECK_INFOS;
	s_origLoadDeckData = SokuLib::union_cast<void (__stdcall *)(char *, void *, SokuLib::deckInfo &, int, SokuLib::mVC9Dequeue<short> &)>(*(int *)PAYLOAD_ADDRESS_DECK_INFOS + PAYLOAD_NEXT_INSTR_DECK_INFOS);
	*(int *)PAYLOAD_ADDRESS_DECK_INFOS = newOffset;
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);

	::FlushInstructionCache(GetCurrentProcess(), NULL, 0);

	return true;
}

extern "C" int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	if (fdwReason == DLL_PROCESS_DETACH) {
		stop = true;
		if (thread.joinable())
			thread.join();
	}
	return TRUE;
}
