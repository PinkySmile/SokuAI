#include <Windows.h>
#include <Shlwapi.h>
#include <SokuLib.hpp>
#include <optional>
#include <map>
#include <fstream>
#include <Packet.hpp>
#include <GameInstance.hpp>
#include <GeneticAI.hpp>
#include <random>
#include <process.h>
#include <ctime>
#include "Network/Socket.hpp"
#include "Exceptions.hpp"

#define CHECK_PACKET_SIZE(requested_type, size) if (size != sizeof(requested_type)) return sendError(Trainer::ERROR_INVALID_PACKET)
#define MIDDLE_LAYER_SIZE 500
#define GENE_COUNT 5000
//#define HOSTMSG "SokuAI made by PinkySmile (it is bad). Remirror only. First match is delay test.\n"\
//                "Please read this to understand how to control the AI https://docs.google.com/document/d/1fAVbnKXYcuNFYGt7hUs7i9qIlebGTo8xHrl5XCMabOk/edit?usp=sharing."
#define HOSTMSG "Reserved"

static int (SokuLib::Logo::*s_origCLogo_OnProcess)();
static int (SokuLib::Title::*s_origCTitle_OnProcess)();
static int (SokuLib::BattleClient::*s_origCBattleCL_OnProcess)();
static int (SokuLib::SelectClient::*s_origCSelectCL_OnProcess)();
static int (SokuLib::BattleServer::*s_origCBattleSV_OnProcess)();
static int (SokuLib::SelectServer::*s_origCSelectSV_OnProcess)();
static void (SokuLib::KeymapManager::*s_origKeymapManager_SetInputs)();

static int delay = 25 - 4;
static std::list<Trainer::Input> inputs;
static std::unique_ptr<Trainer::GeneticAI> ai;
static bool game = false;
static bool stop = false;
static bool gameFinished = false;
static long port;
static bool delay_test = true;
static bool last = false;

static SokuLib::KeyInput lastInput;
static const std::map<SokuLib::Character, std::array<unsigned short, 20>> characterSpellCards{
	{SokuLib::CHARACTER_ALICE,     {}},
	{SokuLib::CHARACTER_AYA,       {}},
	{SokuLib::CHARACTER_CIRNO,     {}},
	{SokuLib::CHARACTER_CLOWNPIECE,{}},
	{SokuLib::CHARACTER_FLANDRE,   {}},
	{SokuLib::CHARACTER_IKU,       {}},
	{SokuLib::CHARACTER_KAGUYA,    {}},
	{SokuLib::CHARACTER_KOMACHI,   {}},
	{SokuLib::CHARACTER_MARISA,    {}},
	{SokuLib::CHARACTER_MEILING,   {}},
	{SokuLib::CHARACTER_MIMA,      {}},
	{SokuLib::CHARACTER_MOKOU,     {}},
	{SokuLib::CHARACTER_MOMIJI,    {}},
	{SokuLib::CHARACTER_MURASA,    {}},
	{SokuLib::CHARACTER_ORIN,      {}},
	{SokuLib::CHARACTER_PATCHOULI, {}},
	{SokuLib::CHARACTER_REIMU,     {}},
	{SokuLib::CHARACTER_REMILIA,   {100, 100, 100, 100, 102, 102, 105, 105, 105, 105, 203, 203, 204, 204, 205, 205, 206, 206, 209, 209}},
	{SokuLib::CHARACTER_SAKUYA,    {}},
	{SokuLib::CHARACTER_SANAE,     {}},
	{SokuLib::CHARACTER_SATORI,    {}},
	{SokuLib::CHARACTER_SEKIBANKI, {}},
	{SokuLib::CHARACTER_SHINKI,    {}},
	{SokuLib::CHARACTER_SHOU,      {}},
	{SokuLib::CHARACTER_SUIKA,     {}},
	{SokuLib::CHARACTER_SUWAKO,    {}},
	{SokuLib::CHARACTER_TENSHI,    {}},
	{SokuLib::CHARACTER_REISEN,    {}},
	{SokuLib::CHARACTER_UTSUHO,    {}},
	{SokuLib::CHARACTER_YOUMU,     {}},
	{SokuLib::CHARACTER_YUKARI,    {}},
	{SokuLib::CHARACTER_YUUKA,     {}},
	{SokuLib::CHARACTER_YUYUKO,    {}},
};

std::mt19937 random;

void updateInput(SokuLib::KeyInput &old, const Trainer::Input &n) {
	old.a = n.A ? (old.a + 1) : 0;
	old.b = n.B ? (old.b + 1) : 0;
	old.c = n.C ? (old.c + 1) : 0;
	old.d = n.D ? (old.d + 1) : 0;
	old.changeCard = n.SW ? (old.changeCard + 1) : 0;
	old.spellcard = n.SC ? (old.spellcard + 1) : 0;

	if (n.H == 0)
		old.horizontalAxis = 0;
	else if (n.H > 0)
		old.horizontalAxis = max(0, old.horizontalAxis) + 1;
	else
		old.horizontalAxis = min(0, old.horizontalAxis) - 1;

	if (n.V == 0)
		old.verticalAxis = 0;
	else if (n.V > 0)
		old.verticalAxis = max(0, old.verticalAxis) + 1;
	else
		old.verticalAxis = min(0, old.verticalAxis) - 1;
}

static void fillState(const SokuLib::CharacterManager &source, const SokuLib::CharacterManager &opponent, Trainer::CharacterState &destination)
{
	destination.direction = source.objectBase.direction;
	destination.opponentRelativePos.y = source.objectBase.position.y - opponent.objectBase.position.y;
	if (source.objectBase.direction == SokuLib::LEFT) {
		destination.opponentRelativePos.x = source.objectBase.position.x - opponent.objectBase.position.x;
		destination.distToBackCorner = 1240 - source.objectBase.position.x;
		destination.distToFrontCorner = source.objectBase.position.x - 40;
	} else {
		destination.opponentRelativePos.x = opponent.objectBase.position.x - source.objectBase.position.x;
		destination.distToBackCorner = source.objectBase.position.x - 40;
		destination.distToFrontCorner = 1240 - source.objectBase.position.x;
	}
	destination.distToGround = source.objectBase.position.y;
	destination.action = source.objectBase.action;
	destination.actionBlockId = source.objectBase.actionBlockId;
	destination.animationCounter = source.objectBase.animationCounter;
	destination.animationSubFrame = source.objectBase.animationSubFrame;
	destination.frameCount = source.objectBase.frameCount;
	destination.comboDamage = source.combo.damages;
	destination.comboLimit = source.combo.limit;
	destination.airBorne = source.objectBase.frameData->frameFlags.airborne;
	destination.hp = source.objectBase.hp;
	destination.airDashCount = source.airdashCount;
	destination.spirit = source.currentSpirit;
	destination.maxSpirit = source.maxSpirit;
	destination.untech = source.untech;
	destination.healingCharm = source.healingCharmTimeLeft;
	destination.swordOfRapture = source.swordOfRaptureDebuffTimeLeft;
	destination.score = source.score;
	memset(destination.hand, 0xFF, sizeof(destination.hand));
	if (SokuLib::activeWeather != SokuLib::WEATHER_MOUNTAIN_VAPOR)
		for (int i = 0; i < source.hand.size; i++)
			destination.hand[i] = source.hand[i].id;
	destination.cardGauge = (SokuLib::activeWeather != SokuLib::WEATHER_MOUNTAIN_VAPOR ? source.cardGauge : -1);
	memcpy(destination.skills, source.skillMap, sizeof(destination.skills));
	destination.fanLevel = source.tenguFans;
	destination.dropInvulTimeLeft = source.dropInvulTimeLeft;
	destination.superArmorTimeLeft = 0;//TODO: source.superArmorTimeLeft;
	destination.superArmorHp = 0;//TODO: source.superArmorHp;
	destination.milleniumVampireTimeLeft = source.milleniumVampireTime;
	destination.philosoferStoneTimeLeft = source.philosophersStoneTime;
	destination.sakuyasWorldTimeLeft = source.sakuyasWorldTime;
	destination.privateSquareTimeLeft = source.privateSquare;
	destination.orreriesTimeLeft = source.orreriesTimeLeft;
	destination.mppTimeLeft = source.missingPurplePowerTimeLeft;
	destination.kanakoCooldown = source.kanakoTimeLeft;
	destination.suwakoCooldown = source.suwakoTimeLeft;
	destination.objectCount = source.objects.list.size;
}

static void fillState(const SokuLib::ObjectManager &object, const SokuLib::CharacterManager &source, const SokuLib::CharacterManager &opponent, Trainer::Object &destination)
{
	destination.direction = object.direction;
	destination.relativePosMe.x = object.position.x - source.objectBase.position.x;
	destination.relativePosMe.y = object.position.y - source.objectBase.position.y;
	destination.relativePosOpponent.x = object.position.x - opponent.objectBase.position.x;
	destination.relativePosOpponent.y = object.position.y - opponent.objectBase.position.y;
	destination.action = object.action;
	destination.imageID = object.image->number;
}
struct In {
	unsigned toggle;
	unsigned i;
};

std::map<SokuLib::KeymapManager *, In> keym;
bool isFrame = false;

#define TTTTTT 10
#define TTTTT2 10

void __fastcall KeymapManagerSetInputs(SokuLib::KeymapManager *This)
{
	(This->*s_origKeymapManager_SetInputs)();
	memset(&This->input, 0, sizeof(This->input));
	if (SokuLib::sceneId != SokuLib::newSceneId)
		return;
	keym[This].toggle = (keym[This].toggle + 1) % 8;
	if (SokuLib::sceneId == SokuLib::SCENE_TITLE && !SokuLib::MenuConnect::isInNetworkMenu()) {
		memset(&This->input, 0, sizeof(This->input));
		keym[This].i++;
		if (keym[This].i % TTTTTT == 0)
			This->input.a = 1;
		return;
	}
	if (SokuLib::sceneId == SokuLib::SCENE_SELECTSV || SokuLib::sceneId == SokuLib::SCENE_SELECTCL) {
		if (SokuLib::leftChar != SokuLib::CHARACTER_REMILIA) {
			This->input.horizontalAxis = !keym[This].toggle;
			This->input.b = !keym[This].toggle && SokuLib::currentScene->to<SokuLib::Select>().leftSelectionStage != 0;
		} else if (SokuLib::rightChar != SokuLib::CHARACTER_REMILIA || SokuLib::currentScene->to<SokuLib::Select>().rightSelectionStage < 2)
			This->input.b = !keym[This].toggle && SokuLib::currentScene->to<SokuLib::Select>().leftSelectionStage != 0;
		else
			This->input.a = !keym[This].toggle && SokuLib::currentScene->to<SokuLib::Select>().leftSelectionStage != 3;
		return;
	}
	if (gameFinished) {
		memset(&This->input, 0, sizeof(This->input));
		This->input.a = 1;
		return;
	}
	if (isFrame) {
		if (inputs.size() == delay + 1) {
			updateInput(lastInput, inputs.front());
			inputs.pop_front();
		}
		memcpy(&This->input, &lastInput, sizeof(lastInput));
		return;
	}
}

int __fastcall CLogo_OnProcess(SokuLib::Logo *This)
{
	int ret = (This->*s_origCLogo_OnProcess)();

	if (ret == SokuLib::SCENE_TITLE) {
		if (__argc <= 1) {
			MessageBoxA(SokuLib::window, "No port provided. Please provide a port in the command line.", "Port not given", MB_ICONERROR);
			return -1;
		}
		char *arg = __argv[__argc - 1];
		char *end;

		port = strtol(arg, &end, 0);
		SetWindowTextA(SokuLib::window, ("SokuAI automated hosting system (On port " + std::to_string(port) + ")").c_str());
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
		//SokuLib::profile1.name = "SokuAI";
		//SokuLib::profile1.file = "SokuAI.pf";
	}
	return ret;
}

static int getLatestGen(SokuLib::Character lchr, SokuLib::Character rchr)
{
	int gen = -1;
	WIN32_FIND_DATAA data;
	HANDLE handle = FindFirstFileA((TRAINING_FOLDER "\\GeneticAI_" + std::to_string(MIDDLE_LAYER_SIZE) + "_" + std::to_string(GENE_COUNT) + "\\" + std::to_string(lchr) + " vs " + std::to_string(rchr) + "\\*").c_str(), &data);

	if (handle == INVALID_HANDLE_VALUE)
		return -1;
	do {
		try {
			auto ptr = strchr(data.cFileName, '_');

			if (!ptr)
				continue;
			*ptr = '\0';
			gen = max(gen, std::stoi(data.cFileName));
		} catch (...) {}
	} while (FindNextFileA(handle, &data));
	return gen;
}

static Trainer::GeneticAI *getBestAI()
{
	int gen = getLatestGen(SokuLib::CHARACTER_REMILIA, SokuLib::CHARACTER_REMILIA);
	std::string basePath = TRAINING_FOLDER "\\"
		"GeneticAI_" + std::to_string(MIDDLE_LAYER_SIZE) + "_" + std::to_string(GENE_COUNT) + "\\" +
		std::to_string(SokuLib::CHARACTER_REMILIA) + " vs " + std::to_string(SokuLib::CHARACTER_REMILIA) + "\\";
	std::ifstream stream{basePath + std::to_string(gen) + "_results.txt"};
	std::vector<std::pair<unsigned, unsigned>> ais;
	std::string id;
	std::string generation;

	if (stream.fail())
		throw std::invalid_argument("Fail");

	printf("Loading tournament results for generation %i\n", gen - 1);
	while (std::getline(stream, id, ',') && std::getline(stream, generation))
		ais.emplace_back(std::stoul(id), std::stoul(generation));
	if (ais.empty())
		throw std::invalid_argument("Fail");

	std::uniform_int_distribution<size_t> d(0, ais.size());
	auto a = ais[d(random)];

	return new Trainer::GeneticAI(a.second, a.first, MIDDLE_LAYER_SIZE, GENE_COUNT, basePath + "ais\\" + std::to_string(a.second) + "_" + std::to_string(a.first) + ".ai");
}

static void loadAI()
{
	if (!ai)
		try {
			ai.reset(getBestAI());
		} catch (std::exception &e) {
			puts(e.what());
			ai = std::make_unique<Trainer::GeneticAI>(0, 0, MIDDLE_LAYER_SIZE, GENE_COUNT);
		}

	auto &scene = SokuLib::currentScene->to<SokuLib::Select>();

	//effectiveDeck.clear();
	//for (auto &card : characterSpellCards.at(SokuLib::CHARACTER_REMILIA))
	//	SokuLib::leftPlayerInfo.effectiveDeck.push_back(card);
	scene.leftSelect.palette = ai->getParams().palette;
	//SokuLib::leftPlayerInfo.palette = ai->getParams().palette;
	if (scene.leftCursor.cursorPos == scene.rightCursor.cursorPos && scene.leftSelect.palette == scene.rightSelect.palette)
		scene.rightSelect.palette = (scene.rightSelect.palette + 1) % 8;
	memset(&lastInput, 0, sizeof(lastInput));
	inputs.clear();
}

void selectCommon()
{
	auto &scene = SokuLib::currentScene->to<SokuLib::Select>();

	gameFinished = false;
	if (game) {
		ai.reset();
		game = false;
	}
	if (SokuLib::leftPlayerInfo.character == SokuLib::CHARACTER_REMILIA)
		loadAI();
}

int __fastcall CSelectSV_OnProcess(SokuLib::SelectServer *This)
{
	int ret = (This->*s_origCSelectSV_OnProcess)();

	if (stop)
		return SokuLib::SCENE_TITLE;
	selectCommon();
	return ret;
}

int __fastcall CSelectCL_OnProcess(SokuLib::SelectClient *This)
{
	int ret = (This->*s_origCSelectCL_OnProcess)();

	if (stop)
		return SokuLib::SCENE_TITLE;
	selectCommon();
	return ret;
}

void battleCommon()
{
	auto &battle = SokuLib::getBattleMgr();

	game = true;
	if (!gameFinished) {
		Trainer::GameInstance::GameFrame frame;
		int current = 0;

		fillState(battle.leftCharacterManager, battle.rightCharacterManager, frame.left);
		fillState(battle.rightCharacterManager, battle.leftCharacterManager, frame.right);
		frame.weatherTimer = SokuLib::weatherCounter;
		frame.displayedWeather = SokuLib::displayedWeather;
		if (SokuLib::activeWeather != SokuLib::WEATHER_CLEAR && SokuLib::displayedWeather == SokuLib::WEATHER_AURORA)
			frame.activeWeather = SokuLib::WEATHER_AURORA;
		else
			frame.activeWeather = SokuLib::activeWeather;

		frame.leftObjects.resize(battle.leftCharacterManager.objects.list.size);
		for (auto obj : battle.leftCharacterManager.objects.list.vector())
			fillState(*obj, battle.leftCharacterManager, battle.rightCharacterManager, frame.leftObjects[current++]);

		current = 0;
		frame.rightObjects.resize(battle.rightCharacterManager.objects.list.size);
		for (auto obj : battle.rightCharacterManager.objects.list.vector())
			fillState(*obj, battle.rightCharacterManager, battle.leftCharacterManager, frame.rightObjects[current++]);
		gameFinished |= battle.leftCharacterManager.score == 2 || battle.rightCharacterManager.score == 2;
		inputs.push_back(ai->getInputs(frame, true));
	}
}

int __fastcall CBattleCL_OnProcess(SokuLib::BattleClient *This)
{
	battleCommon();
	isFrame = true;
	int ret = (This->*s_origCBattleCL_OnProcess)();
	isFrame = false;
	if (stop)
		return SokuLib::SCENE_TITLE;
	if (gameFinished)
		return SokuLib::SCENE_SELECTCL;
	return ret;
}

int __fastcall CBattleSV_OnProcess(SokuLib::BattleServer *This)
{
	battleCommon();
	isFrame = true;
	int ret = (This->*s_origCBattleSV_OnProcess)();
	isFrame = false;
	if (stop)
		return SokuLib::SCENE_TITLE;
	if (gameFinished)
		return SokuLib::SCENE_SELECTSV;
	return ret;
}

void addToHostList(void *)
{
	Socket sock;
	Socket::HttpRequest requ;

	requ.portno = 14762;
	requ.path = "/games";
	requ.host = "delthas.fr";
	requ.method = "PUT";
	requ.body = "{"
		R"("profile_name": "SokuAI",)"
		R"("message": ")" HOSTMSG R"(",)"
		R"("port": )" + std::to_string(port) +
	"}";
	requ.header["Content-Length"] = std::to_string(requ.body.size());
	requ.header["Content-Type"] = "application/json; charset=ascii";
	try {
		auto result = sock.makeHttpRequest(requ);
		printf("%i %s\n", result.returnCode, result.codeName.c_str());
		puts(result.body.c_str());
	} catch (HTTPErrorException &e) {
		puts(e.what());
		puts(e.getResponse().codeName.c_str());
		puts(e.getResponse().body.c_str());
	} catch (std::exception &e) {
		puts(e.what());
	}
}

int __fastcall CTitle_OnProcess(SokuLib::Title *This)
{
	static int kl = 0;
	int ret = (This->*s_origCTitle_OnProcess)();
	auto menuObj = SokuLib::getMenuObj<SokuLib::MenuConnect>();

	delay_test = true;
	SokuLib::currentScene->to<SokuLib::Title>().menuInputHandler.pos = 4;
	if (!SokuLib::MenuConnect::isInNetworkMenu())
		return ret;
	if (menuObj->choice == SokuLib::MenuConnect::CHOICE_HOST && menuObj->subchoice == 2)
		return ret;
	kl++;
	if (kl % TTTTT2 == 0) {
		puts("Start host");
		menuObj->setupHost(port, true);
		_beginthread(addToHostList, 0, nullptr);
	}
	return ret;
}

// 設定ロード
void LoadSettings(LPCSTR profilePath)
{
	// 自動シャットダウン
#ifdef _DEBUG
	FILE *_;
	AllocConsole();
	freopen_s(&_, "CONOUT$", "w", stdout);
	freopen_s(&_, "CONOUT$", "w", stderr);
	puts("Console");
#endif
}

extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16])
{
	return ::memcmp(SokuLib::targetHash, hash, sizeof SokuLib::targetHash) == 0;
}

#define PAYLOAD_ADDRESS_GET_INPUTS 0x40A45E
#define PAYLOAD_NEXT_INSTR_GET_INPUTS (PAYLOAD_ADDRESS_GET_INPUTS + 4)

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
	char profilePath[1024 + MAX_PATH];

	GetModuleFileName(hMyModule, profilePath, 1024);
	PathRemoveFileSpec(profilePath);
	PathAppend(profilePath, "SokuOnlinePlayer.ini");
	LoadSettings(profilePath);

	DWORD old;
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	s_origCLogo_OnProcess = SokuLib::TamperDword(&SokuLib::VTable_Logo.onProcess, CLogo_OnProcess);
	s_origCTitle_OnProcess = SokuLib::TamperDword(&SokuLib::VTable_Title.onProcess, CTitle_OnProcess);
	s_origCBattleCL_OnProcess = SokuLib::TamperDword(&SokuLib::VTable_BattleClient.onProcess, CBattleCL_OnProcess);
	s_origCSelectCL_OnProcess = SokuLib::TamperDword(&SokuLib::VTable_SelectClient.onProcess, CSelectCL_OnProcess);
	s_origCBattleSV_OnProcess = SokuLib::TamperDword(&SokuLib::VTable_BattleServer.onProcess, CBattleSV_OnProcess);
	s_origCSelectSV_OnProcess = SokuLib::TamperDword(&SokuLib::VTable_SelectServer.onProcess, CSelectSV_OnProcess);
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	int newOffset = (int)KeymapManagerSetInputs - PAYLOAD_NEXT_INSTR_GET_INPUTS;
	s_origKeymapManager_SetInputs = SokuLib::union_cast<void (SokuLib::KeymapManager::*)()>(*(int *)PAYLOAD_ADDRESS_GET_INPUTS + PAYLOAD_NEXT_INSTR_GET_INPUTS);
	*(int *)PAYLOAD_ADDRESS_GET_INPUTS = newOffset;
	//*(int *)0x47d7a0 = 0xC3;
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);
	::FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
	return true;
}

extern "C" int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_DETACH)
		stop = true;
	random.seed(time(nullptr));
	return TRUE;
}
