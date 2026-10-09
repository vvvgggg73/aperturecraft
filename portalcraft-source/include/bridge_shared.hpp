#pragma once

#include <array>
#include <cstdint>

namespace portalcraft {

constexpr std::uint32_t kBridgeMagic = 0x42544350;
constexpr std::uint32_t kBridgeVersion = 1;
constexpr std::uint32_t kEventCapacity = 64;
constexpr wchar_t kMappingName[] = L"Local\\PortalCraftBridge";

enum class EventKind : std::uint32_t {
    BluePortalShot = 1,
    OrangePortalShot = 2
};

struct PlayerState {
    double posX;
    double posY;
    double posZ;
    double velX;
    double velY;
    double velZ;
    float yaw;
    float pitch;
    std::uint64_t frameId;
};

static_assert(sizeof(PlayerState) == 64, "PlayerState layout must match Java bridge");

struct PortalEvent {
    std::uint64_t sequence;
    std::uint32_t kind;
    std::uint32_t face;
    double hitX;
    double hitY;
    double hitZ;
    float normalX;
    float normalY;
    float normalZ;
    float reserved;
};

static_assert(sizeof(PortalEvent) == 64, "PortalEvent layout must match Java bridge");

struct BridgeHeader {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t hostHeartbeat;
    std::uint32_t guestHeartbeat;
    std::uint32_t eventWriteIndex;
    std::uint32_t eventReadIndex;
    std::uint8_t reserved[8];
};

static_assert(sizeof(BridgeHeader) == 32, "BridgeHeader layout must match Java bridge");

struct BridgeBlock {
    BridgeHeader header;
    PlayerState minecraftToSource;
    PlayerState sourceToMinecraft;
    std::array<PortalEvent, kEventCapacity> events;
};

static_assert(sizeof(BridgeBlock) == 4192, "BridgeBlock layout changed");

}  // namespace portalcraft
