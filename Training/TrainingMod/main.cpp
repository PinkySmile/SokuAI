#include <windows.h>
#include <shlwapi.h>
#include <SokuLib.hpp>
#include <thread>
#include <optional>
#include <map>
#include <process.h>
#include "Packet.hpp"
#include "Exceptions.hpp"
#include "network.hpp"
#include "sokuData.hpp"

// #define VSCOM

static int (SokuLib::Battle::*s_origCBattle_OnRenderAddr)();
static int (SokuLib::Select::*s_origCSelect_OnRenderAddr)();
static int (SokuLib::Loading::*s_origCLoading_OnRenderAddr)();
static int (SokuLib::Title::*s_origCTitle_OnRenderAddr)();
static int (SokuLib::Title::*s_origCTitle_OnProcess)();
static int (SokuLib::Logo::*s_origCLogo_OnProcess)();
static int (SokuLib::Battle::*s_origCBattle_OnProcess)();
int (__fastcall *og_loadTexture)(unsigned *, const char *, int **, unsigned *);
static void (SokuLib::KeymapManager::*s_origKeymapManager_SetInputs)();

static float counter = 0;

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

bool isFrame = false;

void __fastcall KeymapManagerSetInputs(SokuLib::KeymapManager *This)
{
	(This->*s_origKeymapManager_SetInputs)();
	if (SokuLib::sceneId != SokuLib::newSceneId)
		return;
	if (gameFinished) {
		memset(&This->input, 0, sizeof(This->input));
		This->input.a = 1;
		return;
	}
	if (isFrame) {
		auto &battle = SokuLib::getBattleMgr();

		if (battle.leftCharacterManager.keyManager && battle.leftCharacterManager.keyManager->keymapManager == This) {
			updateInput(lastInputs.first, inputs.first);
			memcpy(&This->input, &lastInputs.first, sizeof(lastInputs.first));
		} else if (battle.rightCharacterManager.keyManager && battle.rightCharacterManager.keyManager->keymapManager == This) {
			updateInput(lastInputs.second, inputs.second);
			memcpy(&This->input, &lastInputs.second, sizeof(lastInputs.second));
		}
	}
}

int dummyFunction()
{
	return 1;
}

void setDisplayMode(bool enabled)
{
	DWORD oldR;
	DWORD oldT;

	VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &oldR);
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &oldT);
	if (!enabled) {
		//og_loadTexture = reinterpret_cast<int (__fastcall *)(unsigned *, char *, int **, unsigned *)>(SokuLib::TamperNearJmpOpr(0x40505c, fakeLoad));
		s_origCTitle_OnRenderAddr = SokuLib::TamperDword(&SokuLib::VTable_Title.onRender, dummyFunction);
		s_origCBattle_OnRenderAddr = SokuLib::TamperDword(&SokuLib::VTable_Battle.onRender, dummyFunction);
		s_origCSelect_OnRenderAddr = SokuLib::TamperDword(&SokuLib::VTable_Select.onRender, dummyFunction);
		s_origCLoading_OnRenderAddr = SokuLib::TamperDword(&SokuLib::VTable_Loading.onRender, dummyFunction);
	} else {
		//SokuLib::TamperNearJmpOpr(0x40505c, og_loadTexture);
		SokuLib::TamperDword(&SokuLib::VTable_Title.onRender, s_origCTitle_OnRenderAddr);
		SokuLib::TamperDword(&SokuLib::VTable_Battle.onRender, s_origCBattle_OnRenderAddr);
		SokuLib::TamperDword(&SokuLib::VTable_Select.onRender, s_origCSelect_OnRenderAddr);
		SokuLib::TamperDword(&SokuLib::VTable_Loading.onRender, s_origCLoading_OnRenderAddr);
	}
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, oldT, &oldT);
	VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, oldR, &oldR);
}

int __fastcall fakeLoad(unsigned *puParm1, char *pcParm2, int **param_3, unsigned *param_4)
{
	return og_loadTexture(puParm1, "empty.png", param_3, param_4);
}

int __fastcall CTitle_OnProcess(SokuLib::Title *This) {
	int ret = (This->*s_origCTitle_OnProcess)();

	if (isStopped())
		return -1;
	if (ret == SokuLib::SCENE_SELECTSCENARIO)
		return SokuLib::SCENE_TITLE;
	if (startRequested) {
		startRequested = false;
		return SokuLib::SCENE_LOADING;
	}
	return ret;
}

LONG WINAPI UnhandledExFilter(PEXCEPTION_POINTERS ExPtr)
{
	if (!ExPtr || !ExPtr->ExceptionRecord)
		SokuLib::DLL::kernel32.ExitProcess(0);
	sendFault(ExPtr);
	SokuLib::DLL::kernel32.ExitProcess(ExPtr->ExceptionRecord->ExceptionCode);
	return 0;
}

int __fastcall CLogo_OnProcess(SokuLib::Logo *This) {
	int ret = (This->*s_origCLogo_OnProcess)();

	if (ret == SokuLib::SCENE_TITLE) {
		SokuLib::sceneId = SokuLib::SCENE_SELECT;
		SokuLib::fadePtr = nullptr;
	#ifndef _DEBUG
		*(char *)0x89ffbd = true;
	#endif
		SetUnhandledExceptionFilter(UnhandledExFilter);
		if (setupNetwork(__argc, __argv) < 0)
			return -1;
	}
	return ret;
}

int __fastcall CBattle_OnProcess(SokuLib::Battle *This) {
	auto &battle = SokuLib::getBattleMgr();

	if (setWeather || freezeWeather) {
		if (weather == SokuLib::WEATHER_CLEAR)
			SokuLib::weatherCounter = -1;
		else if (SokuLib::activeWeather == weather) {
			SokuLib::weatherCounter = weatherTimer;
			setWeather = false;
		}
	}
	counter += tps / 60.f;
	while (counter >= 1) {
		isFrame = true;
		int ret = (This->*s_origCBattle_OnProcess)();
		isFrame = false;
		if (std::find(moveBlacklist.begin(), moveBlacklist.end(), battle.leftCharacterManager.objectBase.action) != moveBlacklist.end()) {
			battle.leftCharacterManager.objectBase.action = SokuLib::ACTION_IDLE;
			battle.leftCharacterManager.objectBase.animate();
		}
		if (std::find(moveBlacklist.begin(), moveBlacklist.end(), battle.rightCharacterManager.objectBase.action) != moveBlacklist.end()) {
			battle.rightCharacterManager.objectBase.action = SokuLib::ACTION_IDLE;
			battle.rightCharacterManager.objectBase.animate();
		}
		if (!gameFinished) {
			inputsReceived = false;
			sendState(battle);
			gameFinished |= battle.leftCharacterManager.score == 2 || battle.rightCharacterManager.score == 2;
			while (!cancel && !gameFinished && !isStopped() && !inputsReceived);
		}
		if (ret != SokuLib::SCENE_BATTLE || cancel) {
			gameFinished = false;
			sendGameResult(battle);
			return SokuLib::SCENE_TITLE;
		}
		if (isStopped())
			return -1;
		counter--;
	}
	return SokuLib::SCENE_BATTLE;
}

// 設定ロード
void LoadSettings(LPCSTR profilePath) {
	FILE *_;

	/*AllocConsole();
	freopen_s(&_, "CONOUT$", "w", stdout);
	freopen_s(&_, "CONOUT$", "w", stderr);*/
}

int __stdcall _MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	printf("%s: %s\n", lpCaption, lpText);
	return IDOK;
}

extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
	return ::memcmp(SokuLib::targetHash, hash, sizeof SokuLib::targetHash) == 0;
}

#define PAYLOAD_ADDRESS_GET_INPUTS 0x40A45E
#define PAYLOAD_NEXT_INSTR_GET_INPUTS (PAYLOAD_ADDRESS_GET_INPUTS + 4)

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
	char profilePath[1024 + MAX_PATH];

	GetModuleFileName(hMyModule, profilePath, 1024);
	PathRemoveFileSpec(profilePath);
	PathAppend(profilePath, "SokuAITrainer.ini");
	LoadSettings(profilePath);

	DWORD old;
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
	s_origCLogo_OnProcess   = SokuLib::TamperDword(&SokuLib::VTable_Logo.onProcess,   CLogo_OnProcess);
	s_origCTitle_OnProcess  = SokuLib::TamperDword(&SokuLib::VTable_Title.onProcess,  CTitle_OnProcess);
	s_origCBattle_OnProcess = SokuLib::TamperDword(&SokuLib::VTable_Battle.onProcess, CBattle_OnProcess);
	SokuLib::TamperDword(&SokuLib::DLL::user32.MessageBoxA, _MessageBoxA);
	SokuLib::TamperDword(&SokuLib::VTable_Logo.onRender, dummyFunction);
	::VirtualProtect((PVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

#ifndef _DEBUG
	swapDisplay(false);
#endif
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
#ifndef _DEBUG
	og_loadTexture = reinterpret_cast<int (__fastcall *)(unsigned *, const char *, int **, unsigned *)>(SokuLib::TamperNearJmpOpr(0x40505c, fakeLoad));
#endif
	s_origKeymapManager_SetInputs = SokuLib::union_cast<void (SokuLib::KeymapManager::*)()>(SokuLib::TamperNearJmpOpr(0x40A45D, KeymapManagerSetInputs));
	//*(int *)0x47d7a0 = 0xC3;
	VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);
	::FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
	return true;
}

extern "C" int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	if (fdwReason == DLL_PROCESS_DETACH)
		stopNetworkThread();
	return TRUE;
}
