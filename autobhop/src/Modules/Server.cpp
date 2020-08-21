#include "Server.hpp"

#include "Command.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"

#include "Console.hpp"
#include "autobhop.hpp"


// CGameMovement::CheckJumpButton
REDECL(Server::CheckJumpButton);
DETOUR_T(bool, Server::CheckJumpButton)
{
	auto jumped = false;

	if (true/*erver->AllowsMovementChanges()*/) {
		auto mv = *reinterpret_cast<CHLMoveData**>((uintptr_t)thisptr + Offsets::mv);

		if (AutoBhop.GetBool() && !server->jumpedLastTime) {
			mv->m_nOldButtons &= ~IN_JUMP;
		}

		server->jumpedLastTime = false;
		server->savedVerticalVelocity = mv->m_vecVelocity[2];

		server->callFromCheckJumpButton = true;
		jumped = Server::CheckJumpButton(thisptr);
		server->callFromCheckJumpButton = false;

		if (jumped) {
			server->jumpedLastTime = true;
		}
	}
	else {
		jumped = Server::CheckJumpButton(thisptr);
	}

	if (jumped) {
		//auto player = *reinterpret_cast<void**>((uintptr_t)thisptr + Offsets::player);
		//auto stat = stats->Get(server->GetSplitScreenPlayerSlot(player));
		//++stat->jumps->total;
		//++stat->steps->total;
		//stat->jumps->StartTrace(server->GetAbsOrigin(player));
	}

	return jumped;
}

bool Server::AllowsMovementChanges()
{
	return sv_cheats.GetBool();
}

void* Server::GetPlayer(int index) {
    return this->UTIL_PlayerByIndex(index);
}

void* Server::GetEntityHandleByIndex(int index) {
    auto size = sizeof(CEntInfo2);
    CEntInfo* info = reinterpret_cast<CEntInfo*>((uintptr_t)server->m_EntPtrArray + size * index);
    return info->m_pEntity;
}

Server::Server()
    : Module() {
}
bool Server::Init() {

    if (auto g_ServerTools = Interface::Create(this->Name(), "VSERVERTOOLS0")) {
        auto GetIServerEntity = g_ServerTools->Original(Offsets::GetIServerEntity);
        Memory::Deref(GetIServerEntity + Offsets::m_EntPtrArray, &this->m_EntPtrArray);
        Interface::Delete(g_ServerTools);
    }

    this->g_GameMovement = Interface::Create(this->Name(), "GameMovement0");
    if (this->g_GameMovement) {
        this->g_GameMovement->Hook(Server::CheckJumpButton_Hook, Server::CheckJumpButton, Offsets::CheckJumpButton);
    }

    this->g_ServerGameDLL = Interface::Create(this->Name(), "ServerGameDLL0");

    if (this->g_ServerGameDLL) {
        auto Think = this->g_ServerGameDLL->Original(Offsets::Think);
        Memory::Read<_UTIL_PlayerByIndex>(Think + Offsets::UTIL_PlayerByIndex, &this->UTIL_PlayerByIndex);
        Memory::DerefDeref<CGlobalVars*>((uintptr_t)this->UTIL_PlayerByIndex + Offsets::gpGlobals, &this->gpGlobals);
    }

    return this->hasLoaded = this->g_GameMovement && this->g_ServerGameDLL;
}
void Server::Shutdown() {
    if (this->g_GameMovement) {
        Interface::Delete(this->g_GameMovement);
    }

    if (this->g_ServerGameDLL) {
        Interface::Delete(this->g_ServerGameDLL);
    }
}

Server* server;
