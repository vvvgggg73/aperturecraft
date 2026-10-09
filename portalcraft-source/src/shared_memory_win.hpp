#pragma once

#include "bridge_shared.hpp"

#include <windows.h>

namespace portalcraft {

// Shared type alias used by the plugin vtable, SDK-provided or standalone.
using CreateInterfaceFn = void* (*)(const char* name, int* returnCode);

class SharedMemoryWin {
public:
    SharedMemoryWin() = default;
    ~SharedMemoryWin();

    SharedMemoryWin(const SharedMemoryWin&) = delete;
    SharedMemoryWin& operator=(const SharedMemoryWin&) = delete;

    bool openOrCreate();
    void close();

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] BridgeBlock* block();
    void touchGuestHeartbeat() const;
    bool pushPortalEvent(const PortalEvent& event) const;

private:
    HANDLE mapping_ = nullptr;
    BridgeBlock* block_ = nullptr;
};

}  // namespace portalcraft
