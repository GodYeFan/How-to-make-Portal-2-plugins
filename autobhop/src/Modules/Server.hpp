#pragma once
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"

class Server : public Module {
public:
    Interface* g_GameMovement = nullptr;
    Interface* g_ServerGameDLL = nullptr;
public:
    int tickBase;
public:
    Server();
    bool Init() override;
    void Shutdown() override;
    const char* Name() override { return MODULE("server"); }

	// CGameMovement::CheckJumpButton
	DECL_DETOUR_T(bool, CheckJumpButton);

    using _UTIL_PlayerByIndex = void* (__cdecl*)(int index);
    _UTIL_PlayerByIndex UTIL_PlayerByIndex = nullptr;

    void* GetPlayer(int index);
    void* GetEntityHandleByIndex(int index);
	bool AllowsMovementChanges();
    CGlobalVars* gpGlobals = nullptr;
    CEntInfo* m_EntPtrArray = nullptr;

private:
	bool jumpedLastTime = false;
	float savedVerticalVelocity = 0.0f;
	bool callFromCheckJumpButton = false;
};

extern Server* server;