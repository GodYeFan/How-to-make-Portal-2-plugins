#include "autobhop.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdarg.h>

#include "Modules/Client.hpp"
#include "Modules/Server.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Module.hpp"
#include "Modules/Tier1.hpp"
#include "Modules/Surface.hpp"

#include "Offsets.hpp"
#include "Command.hpp"
#include "Game.hpp"
#include "Utils.hpp"
#include "Utils/Memory.hpp"

SMSM smsm;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(SMSM, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, smsm);

BEGIN_SCRIPTDESC_ROOT(SMSM, "The SMSM instance.")
DEFINE_SCRIPTFUNC(GetMode, "Returns current mode.")
DEFINE_SCRIPTFUNC(SetMode, "Sets the mod mode.")
DEFINE_SCRIPTFUNC(GetModeParam, "Returns mod-specific param with given ID.")
DEFINE_SCRIPTFUNC(SetModeParam, "Sets mod specific param if it's not read-only (i.e. not constantly updated).")
DEFINE_SCRIPTFUNC(IsDialogueEnabled, "Is dialogue enabled in audio settings?")
DEFINE_SCRIPTFUNC(SetPortalGunIndicatorColor, "Sets the color of portal gun indicator. Set to 0,0,0 to use default.")
DEFINE_SCRIPTFUNC(SetScreenCoverColor, "Sets color that covers the whole screen.")
DEFINE_SCRIPTFUNC(PrecacheModel, "Precaches model")
DEFINE_SCRIPTFUNC(GetBackupKey, "Gets currently set key used by script to recover parameters.")
DEFINE_SCRIPTFUNC(SetBackupKey, "Sets backup key used by script to recover parameters.")
DEFINE_SCRIPTFUNC(GetModeParamsNumber, "Maximum number of parameters you can assign.")
DEFINE_SCRIPTFUNC(AreModeParamsChanged, "Used by backup system. Returns true once if change to mode-specifc params was made.")
END_SCRIPTDESC()

SMSM::SMSM()
    : game(Game::CreateNew())
    , plugin(new Plugin())
    , modules(new Modules())
    , cheats(new Cheats())
    , clients()
    , mode(0)
{

}

// Used callbacks
bool SMSM::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    console = new Console();
    if (!console->Init())
        return false;

    if (this->game) {
        this->game->LoadOffsets();

        this->modules->AddModule<Tier1>(&tier1);
        this->modules->InitAll();//可能就只有Tier1 tier1注册 目的检测 sv_lan 是否为1 检查是否启动游戏并运行地图

        if (tier1 && tier1->hasLoaded) {
            this->cheats->Init();//自定义的指令注册（sm_mode和smsm_param） 自定义的指令一些信息保存在另一个类的对象中

            this->modules->AddModule<Engine>(&engine);//引擎操作初始化 (获取)
            this->modules->AddModule<Client>(&client);//本地客户端的初始化 (获取)
            this->modules->AddModule<Server>(&server);//本地客户端的初始化 (获取)
            this->modules->AddModule<Surface>(&surface);//surface (获取)
            this->modules->InitAll();//所有模块

            if (engine && client && engine->hasLoaded && client->hasLoaded && server->hasLoaded) {

                this->StartMainThread();

                console->Print("[AutoBhop] Loaded Success [Version: %s]\n", this->Version());
                return true;
            } else {
                console->Warning("[AutoBhop] Failed to load engine and client modules!\n");
            }
        } else {
            console->Warning("[AutoBhop] Failed to load tier1 module!\n");
        }
    } else {
        console->Warning("[AutoBhop] Game not supported!\n");
    }

    console->Warning("[AutoBhop] Plugin failed to load!\n");
    return false;
}
void SMSM::Unload() {
    this->Cleanup();
}
const char* SMSM::GetPluginDescription() {
    return SMSM_SIGNATURE;
}
void SMSM::LevelShutdown() {
    console->DevMsg("[AutoBhop]LevelShutdown\n");

    this->SetBackupKey("0");

    // Make sure to clear the list after sending any client-side shutdown commands
    this->clients.clear();
}

void SMSM::ClientActive(void* pEntity) {
    console->DevMsg("[AutoBhop]ClientActive -> pEntity: %p\n", pEntity);
    client->SetPortalGunIndicatorColor(Vector(0, 0, 0));

    if (!this->clients.empty() && this->clients.at(0) == pEntity) {

    }
}

void SMSM::ClientFullyConnect(void* pEdict) {
    this->clients.push_back(pEdict);
}

void SMSM::Cleanup() {
    if (console)
        console->Print("[AutoBhop] Unloaded success \n");

    if (this->cheats)
        this->cheats->Shutdown();

    if (this->modules)
        this->modules->ShutdownAll();

    SAFE_UNLOAD(this->cheats);
    SAFE_UNLOAD(this->game);
    SAFE_UNLOAD(this->plugin);
    SAFE_UNLOAD(this->modules);
    SAFE_UNLOAD(console);
}

//adds console line parameter in order to override background
void SMSM::ForceAct5MenuBackground() {
    console->CommandLine()->AppendParm("-background", "5");
}


void SMSM::SetMode(int mode) {
    this->mode = mode;
    //reset param table and other stuff when switching modes
    this->ResetModeVariables();
    if (this->mode != mode)this->paramsChanged = true;
}

void SMSM::ResetModeVariables() {
    for (int i = 0; i < MAX_MODE_PARAMETERS; i++) {
        this->modeParams[i] = 0.0f;
    }
}

void SMSM::PrecacheModel(const char* pName, bool bPreload) {
    engine->PrecacheModel(pName, bPreload);
}

void SMSM::SetBackupKey(const char* key) {
    backupKey = key;
    this->paramsChanged = true;
}

const char* SMSM::GetBackupKey() {
    return this->backupKey;
}

float SMSM::GetModeParam(int id) {
    if (id < 0 || id >= MAX_MODE_PARAMETERS)return -1.0f;
    return this->modeParams[id];
}

bool SMSM::SetModeParam(int id, float value) {
    if (id < 0 || id >= MAX_MODE_PARAMETERS)return false;
    if (this->modeParams[id] != value)this->paramsChanged = true;
    this->modeParams[id] = value;
    return true;
}

bool SMSM::AreModeParamsChanged() {
    bool changed = this->paramsChanged;
    if(changed)this->paramsChanged = false;
    return changed;
}

void SMSM::SetPortalGunIndicatorColor(Vector color) {
    client->SetPortalGunIndicatorColor(color);
}

void SMSM::SetScreenCoverColor(int r, int g, int b, int a) {
}

bool SMSM::IsDialogueEnabled() {
    return puzzlemaker_play_sounds.GetBool();
}

void SMSM::StartMainThread() {
    this->ForceAct5MenuBackground();
}

// Might fix potential deadlock
#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_DETACH) {
        smsm.Cleanup();
    }
    return TRUE;
}
#endif

#pragma region Unused callbacks
void SMSM::LevelInit(char const* pMapName)
{
}
void SMSM::Pause() 
{
}
void SMSM::UnPause() 
{
}
void SMSM::GameFrame(bool simulating)
{
    isPaused = !simulating;
}
void SMSM::ClientDisconnect(void* pEntity)
{
}
void SMSM::ClientPutInServer(void* pEntity, char const* playername)
{
}
void SMSM::SetCommandClient(int index)
{
}
void SMSM::ClientSettingsChanged(void* pEdict)
{
}
int SMSM::ClientCommand(void* pEntity, const void*& args)
{
    return 0;
}
int SMSM::NetworkIDValidated(const char* pszUserName, const char* pszNetworkID)
{
    return 0;
}
void SMSM::OnQueryCvarValueFinished(int iCookie, void* pPlayerEntity, int eStatus, const char* pCvarName, const char* pCvarValue)
{
}
void SMSM::OnEdictAllocated(void* edict)
{
}
void SMSM::OnEdictFreed(const void* edict)
{
}
int SMSM::ClientConnect(bool* bAllowConnect, void* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen) {
    return 0;
}
void SMSM::ServerActivate(void* pEdictList, int edictCount, int clientMax) {

}
#pragma endregion
