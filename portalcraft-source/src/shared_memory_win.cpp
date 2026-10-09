#include "shared_memory_win.hpp"

#include <cstring>

namespace portalcraft {

SharedMemoryWin::~SharedMemoryWin() {
    close();
}

bool SharedMemoryWin::openOrCreate() {
    if (block_ != nullptr) {
        return true;
    }

    mapping_ = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kMappingName);
    if (mapping_ == nullptr) {
        mapping_ = ::CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(BridgeBlock),
            kMappingName
        );
    }

    if (mapping_ == nullptr) {
        return false;
    }

    void* view = ::MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BridgeBlock));
    if (view == nullptr) {
        close();
        return false;
    }

    block_ = static_cast<BridgeBlock*>(view);

    if (block_->header.magic != kBridgeMagic) {
        std::memset(block_, 0, sizeof(BridgeBlock));
        block_->header.magic = kBridgeMagic;
        block_->header.version = kBridgeVersion;
    }

    return true;
}

void SharedMemoryWin::close() {
    if (block_ != nullptr) {
        ::UnmapViewOfFile(block_);
        block_ = nullptr;
    }

    if (mapping_ != nullptr) {
        ::CloseHandle(mapping_);
        mapping_ = nullptr;
    }
}

bool SharedMemoryWin::isReady() const {
    return block_ != nullptr;
}

BridgeBlock* SharedMemoryWin::block() {
    return block_;
}

void SharedMemoryWin::touchGuestHeartbeat() const {
    if (block_ == nullptr) {
        return;
    }

    block_->header.guestHeartbeat = static_cast<std::uint32_t>(::GetTickCount64() & 0x7fffffff);
}

bool SharedMemoryWin::pushPortalEvent(const PortalEvent& event) const {
    if (block_ == nullptr) {
        return false;
    }

    const std::uint32_t writeIndex = block_->header.eventWriteIndex;
    const std::uint32_t readIndex = block_->header.eventReadIndex;

    if ((writeIndex - readIndex) >= kEventCapacity) {
        return false;
    }

    block_->events[writeIndex % kEventCapacity] = event;
    block_->header.eventWriteIndex = writeIndex + 1;
    return true;
}

}  // namespace portalcraft
