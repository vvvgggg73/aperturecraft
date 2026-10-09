// PortalCraft Source bridge.
//
// Build modes:
// 1) Standalone (default CMake configuration):
//    - Compiles without Source SDK headers.
//    - Exports CreateInterface + a minimal IServerPluginCallbacks-compatible
//      vtable. The DLL can be loaded by a wrapper host that talks to the
//      shared memory block directly.
// 2) Portal SDK build:
//    - Define PORTALCRAFT_WITH_SOURCESDK and add include paths for
//      `eiface.h`, `igameevents.h`, `tier0/platform.h`, etc.
//    - The plugin hooks IGameEventManager2 and reads Portal-specific event
//      keys (portal_type / hit_x / hit_y / hit_z / normal_x / normal_y / normal_z).
//
// The shared memory contract lives in `include/bridge_shared.hpp` and must be
// byte-identical with the Java side (BridgeLayout). Memory is always opened
// through pure WinAPI: CreateFileMappingW / OpenFileMappingW / MapViewOfFile /
// UnmapViewOfFile / CloseHandle.

#include "bridge_shared.hpp"
#include "shared_memory_win.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <windows.h>

#if defined(PORTALCRAFT_WITH_SOURCESDK)
    #include <igameevents.h>
    #include <eiface.h>
#endif

namespace portalcraft {
namespace {

// -----------------------------------------------------------------------------
// Forward declarations for SDK-free standalone build. When the SDK is present
// these types come from the real headers via the #include block above; the
// forward declarations just keep the vtable signatures aligned.
// -----------------------------------------------------------------------------
#if !defined(PORTALCRAFT_WITH_SOURCESDK)
class CCommand;
class ConCommandBase;
class KeyValues;
class IGameEvent;
class IGameEventManager2;
class IServerPluginCallbacks;

constexpr char INTERFACEVERSION_ISERVERPLUGINCALLBACKS[] =
    "ISERVERPLUGINCALLBACKS_VERSION003";
#endif

// -----------------------------------------------------------------------------
// Helpers that abstract over the SDK/standalone split.
// -----------------------------------------------------------------------------

constexpr int kEventKindBlue = static_cast<int>(EventKind::BluePortalShot);
constexpr int kEventKindOrange = static_cast<int>(EventKind::OrangePortalShot);

// Deduplicate identical portal hits within ~125 ms. Portal Gun events can fire
// rapidly due to trace re-evaluation; a simple cache prevents spamming the Java
// ring buffer with duplicates.
struct PortalHitCache {
    std::uint64_t lastTick = 0;
    std::uint32_t lastKind = 0;
    std::array<double, 3> lastHit{};
    std::array<float, 3> lastNormal{};

    bool shouldDedupe(std::uint32_t kind, const double (&hit)[3], const float (&normal)[3]) {
        const std::uint64_t now = static_cast<std::uint64_t>(::GetTickCount64());
        const bool freshType = lastKind != kind;
        const bool sameHit =
            std::memcmp(lastHit.data(), hit, sizeof(hit)) == 0 &&
            std::memcmp(lastNormal.data(), normal, sizeof(normal)) == 0;

        if (freshType || !sameHit || (now - lastTick) > 125ULL) {
            lastTick = now;
            lastKind = kind;
            lastHit = {hit[0], hit[1], hit[2]};
            lastNormal = {normal[0], normal[1], normal[2]};
            return false;
        }

        return true;
    }
};

struct PluginState {
    SharedMemoryWin bridge;
    std::atomic<std::uint64_t> sequence = 1;
    PortalHitCache hitCache;
    bool initialized = false;

    void writeSourceViewProxy() {
        if (!bridge.isReady()) {
            return;
        }

        const auto* block = bridge.block();
        const PlayerState& host = block->minecraftToSource;

        // Proxy the authoritative host state into the guest slot so any
        // downstream Source camera hook has the latest transform to bind to
        // its local player entity. This intentionally preserves frameId so
        // the Java side can detect stale reads.
        block->sourceToMinecraft = host;
    }

    bool emitPortalHit(
        int rawKind,
        int face,
        double hitX,
        double hitY,
        double hitZ,
        float normalX,
        float normalY,
        float normalZ
    ) {
        if (!bridge.openOrCreate()) {
            return false;
        }

        const std::uint32_t kind = rawKind == kEventKindOrange
            ? static_cast<std::uint32_t>(EventKind::OrangePortalShot)
            : static_cast<std::uint32_t>(EventKind::BluePortalShot);

        const double hit[3] = {hitX, hitY, hitZ};
        const float normal[3] = {normalX, normalY, normalZ};

        if (hitCache.shouldDedupe(kind, hit, normal)) {
            return false;
        }

        PortalEvent event{};
        event.sequence = sequence.fetch_add(1, std::memory_order_relaxed);
        event.kind = kind;
        event.face = static_cast<std::uint32_t>(face < 0 ? 0 : face);
        event.hitX = hitX;
        event.hitY = hitY;
        event.hitZ = hitZ;
        event.normalX = normalX;
        event.normalY = normalY;
        event.normalZ = normalZ;

        return bridge.pushPortalEvent(event);
    }

    void tickFrame() {
        if (!bridge.openOrCreate()) {
            return;
        }

        bridge.touchGuestHeartbeat();
        writeSourceViewProxy();
    }
};

PluginState g_state;
CreateInterfaceFn g_engineFactory = nullptr;
CreateInterfaceFn g_gameServerFactory = nullptr;

}  // namespace

// -----------------------------------------------------------------------------
// Source plugin callbacks. Vtable layout matches IServerPluginCallbacks from
// Source 2007/2013 SDKs; unused entry points default to no-ops.
// -----------------------------------------------------------------------------
class ServerPluginCallbacks {
public:
    virtual ~ServerPluginCallbacks() = default;

    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
        g_engineFactory = interfaceFactory;
        g_gameServerFactory = gameServerFactory;
        g_state.initialized = true;
        const bool ok = g_state.bridge.openOrCreate();
        if (ok) {
            g_state.tickFrame();
        }
        RegisterGameEventListeners();
        return ok;
    }

    virtual void Unload() {
        UnregisterGameEventListeners();
        g_state.bridge.close();
        g_state.initialized = false;
    }

    virtual void Pause() {}
    virtual void UnPause() {}
    virtual const char* GetPluginDescription() {
        return "PortalCraft <-> Minecraft shared memory bridge";
    }

    virtual void LevelInit(char const* /*mapName*/) {
        g_state.bridge.openOrCreate();
    }

    virtual void ServerActivate(
        void* /*edictList*/,
        int /*edictCount*/,
        int /*clientMax*/
    ) {}

    virtual void GameFrame(bool /*simulating*/) {
        g_state.tickFrame();
    }

    virtual void LevelShutdown() {
        UnregisterGameEventListeners();
    }

    virtual void ClientActive(void* /*player*/) {}
    virtual void ClientDisconnect(void* /*player*/) {}
    virtual void ClientPutInServer(void* /*player*/, char const* /*playername*/) {}

    virtual void SetCommandClient(int /*index*/) {}

    virtual void ClientSettingsChanged(void* /*player*/) {}

    virtual int  ClientConnect(
        bool* /*allowConnect*/,
        void* /*player*/,
        char const* /*name*/,
        char const* /*address*/,
        char* /*reject*/,
        int /*maxrejectlen*/
    ) {
        return 0;
    }

    virtual int  ClientCommand(void* /*player*/, const CCommand& /*args*/) {
        return 0;
    }

    virtual int  NetworkIDValidated(const char* /*userName*/, const char* /*networkID*/) {
        return 0;
    }

    virtual void OnQueryCvarValueFinished(
        int /*cookie*/,
        void* /*player*/,
        int /*status*/,
        const char* /*cvarName*/,
        const char* /*cvarValue*/
    ) {}

    virtual void OnEdictAllocated(void* /*edict*/) {}
    virtual void OnEdictFreed(const void* /*edict*/) {}

    // SDK-only helpers. These are defined out-of-line below so the class stays
    // compileable without SDK headers.
    void RegisterGameEventListeners();
    void UnregisterGameEventListeners();
};

namespace {

ServerPluginCallbacks g_serverPluginCallbacks;
constexpr char kServerPluginInterfaceVersion[] =
#if defined(PORTALCRAFT_WITH_SOURCESDK)
    INTERFACEVERSION_ISERVERPLUGINCALLBACKS;
#else
    "ISERVERPLUGINCALLBACKS_VERSION003";
#endif

#if defined(PORTALCRAFT_WITH_SOURCESDK)
IGameEventManager2* FindGameEventManager() {
    if (g_engineFactory == nullptr) {
        return nullptr;
    }

    return static_cast<IGameEventManager2*>(
        g_engineFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr)
    );
}

class PortalGunGameEventListener final : public IGameEventListener2 {
public:
    void FireGameEvent(IGameEvent* event) override {
        if (event == nullptr) {
            return;
        }

        const char* name = event->GetName();
        if (name == nullptr) {
            return;
        }

        const bool isPortalEvent =
            ::strstr(name, "portal") != nullptr ||
            ::strstr(name, "portalgun") != nullptr ||
            ::strcmp(name, "weapon_portalgun_fire") == 0;

        if (!isPortalEvent) {
            return;
        }

        // Prefer the explicit numeric key "portal_type" where 0 == blue,
        // 1 == orange. If not present, fall back to deriving the kind from
        // the event name itself (blue_* / orange_*).
        int kind = kEventKindBlue;
        if (event->FindKey("portal_type") != nullptr) {
            kind = event->GetInt("portal_type", 0) == 0 ? kEventKindBlue : kEventKindOrange;
        } else {
            if (::strstr(name, "orange") != nullptr) {
                kind = kEventKindOrange;
            } else if (::strstr(name, "blue") == nullptr) {
                // Ambiguous generic weapon_fire; treat as blue unless the
                // caller provides a second event with "orange" suffix.
                kind = kEventKindBlue;
            }
        }

        const double hit[3] = {
            event->GetFloat("hit_x", 0.0f),
            event->GetFloat("hit_y", 0.0f),
            event->GetFloat("hit_z", 0.0f),
        };
        const float normal[3] = {
            event->GetFloat("normal_x", 0.0f),
            event->GetFloat("normal_y", 1.0f),
            event->GetFloat("normal_z", 0.0f),
        };
        const int face = event->GetInt("face", 0);

        const bool validHit =
            (normal[0] != 0.0f || normal[1] != 0.0f || normal[2] != 0.0f) &&
            (hit[0] != 0.0 || hit[1] != 0.0 || hit[2] != 0.0);

        if (!validHit) {
            return;
        }

        g_state.emitPortalHit(kind, face, hit[0], hit[1], hit[2], normal[0], normal[1], normal[2]);
    }

    int GetEventDebugID() override {
        return 42;
    }
};

PortalGunGameEventListener g_portalEventListener;
IGameEventManager2* g_gameEventManager = nullptr;

#endif  // PORTALCRAFT_WITH_SOURCESDK

}  // namespace

void ServerPluginCallbacks::RegisterGameEventListeners() {
#if defined(PORTALCRAFT_WITH_SOURCESDK)
    g_gameEventManager = FindGameEventManager();
    if (g_gameEventManager == nullptr) {
        return;
    }

    // Register for the most likely Portal Gun placement/fire events. The
    // listener itself filters by payload so even if a name isn't an exact
    // match we don't crash or misbehave.
    const char* kCandidateEvents[] = {
        "portal_placed",
        "portal_opened",
        "portal_fired",
        "weapon_portalgun_fire",
        "blue_portal_opened",
        "orange_portal_opened",
    };

    for (const char* name : kCandidateEvents) {
        g_gameEventManager->AddListener(&g_portalEventListener, name, true);
    }
#else
    // Without SDK there is no game event system to subscribe to; the fallback
    // export PortalCraft_OnPortalGunHit remains available for manual pushes.
#endif
}

void ServerPluginCallbacks::UnregisterGameEventListeners() {
#if defined(PORTALCRAFT_WITH_SOURCESDK)
    if (g_gameEventManager != nullptr) {
        g_gameEventManager->RemoveListener(&g_portalEventListener);
        g_gameEventManager = nullptr;
    }
#endif
}

// -----------------------------------------------------------------------------
// Source-standard plugin entrypoint: DLL exports CreateInterface and answers
// both the server plugin version string and a short alias ("PortalCraftBridge").
// -----------------------------------------------------------------------------
extern "C" __declspec(dllexport) void* CreateInterface(const char* name, int* returnCode) {
    if (name == nullptr) {
        if (returnCode != nullptr) *returnCode = 0;
        return nullptr;
    }

    void* iface = nullptr;
    if (::strcmp(name, kServerPluginInterfaceVersion) == 0 ||
        ::strcmp(name, "ISERVERPLUGINCALLBACKS_VERSION002") == 0 ||
        ::strcmp(name, "ISERVERPLUGINCALLBACKS_VERSION004") == 0 ||
        ::strcmp(name, "PortalCraftBridge") == 0) {
        iface = &g_serverPluginCallbacks;
    }

    if (returnCode != nullptr) {
        *returnCode = iface != nullptr ? 0 : 1;
    }

    return iface;
}

// -----------------------------------------------------------------------------
// Backward-compatible fallback entrypoints. These exist so integrators can
// pump transforms and events into the bridge without a full SDK hook (for
// example a VSP-style loader, a proxy d3d9 DLL or a manual trace hook).
// -----------------------------------------------------------------------------
extern "C" __declspec(dllexport) bool PortalCraft_Initialize() {
    return g_state.bridge.openOrCreate();
}

extern "C" __declspec(dllexport) void PortalCraft_Shutdown() {
    g_serverPluginCallbacks.UnregisterGameEventListeners();
    g_state.bridge.close();
}

extern "C" __declspec(dllexport) void PortalCraft_OnFrame() {
    g_state.tickFrame();
}

extern "C" __declspec(dllexport) bool PortalCraft_OnPortalGunHit(
    int portalKind,
    int face,
    double hitX,
    double hitY,
    double hitZ,
    float normalX,
    float normalY,
    float normalZ
) {
    return g_state.emitPortalHit(portalKind, face, hitX, hitY, hitZ, normalX, normalY, normalZ);
}

// Exported for test hosts that want to inspect the shared memory block header
// without duplicating the WinAPI shim logic.
extern "C" __declspec(dllexport) bool PortalCraft_GetBridgeSnapshot(
    std::uint32_t* outMagic,
    std::uint32_t* outGuestHeartbeat,
    std::uint64_t* outEventWriteIndex
) {
    if (!g_state.bridge.isReady()) {
        return false;
    }

    const auto* block = g_state.bridge.block();
    if (outMagic != nullptr) *outMagic = block->header.magic;
    if (outGuestHeartbeat != nullptr) *outGuestHeartbeat = block->header.guestHeartbeat;
    if (outEventWriteIndex != nullptr) *outEventWriteIndex = block->header.eventWriteIndex;
    return true;
}

}  // namespace portalcraft
