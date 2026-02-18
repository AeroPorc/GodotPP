#pragma once

#include <cstdint>

namespace netproto {

constexpr const char *kDefaultServerAddress = "127.0.0.1:9000";
constexpr uint16_t kServerPort = 9000;

enum class PacketKind : uint8_t {
    Hello = 0x00,
    Spawn = 0x01,
};

namespace type {
constexpr uint32_t kPlayer = 1;
}

#pragma pack(push, 1)
struct HelloPacket {
    PacketKind kind{PacketKind::Hello};
    uint32_t requested_type{type::kPlayer};
};

struct SpawnPacket {
    PacketKind kind{PacketKind::Spawn};
    uint32_t type_id{type::kPlayer};
    uint32_t network_id{0};
    float x{0.0f};
    float y{0.0f};
};
#pragma pack(pop)

}

