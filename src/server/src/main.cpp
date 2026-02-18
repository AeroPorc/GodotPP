#include <chrono>
#include <cstring>
#include <entt/entt.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#include "snl.h"
#include "network_protocol.h"

struct ObjectContext {
    uint32_t network_id;
    entt::entity local_entity;
    float x{0.0f};
    float y{0.0f};
};

int main() {
    using namespace std::chrono_literals;

    const char *bind_address = "0.0.0.0:9000";
    GameSocket *socket = net_socket_create(bind_address);
    if (!socket) {
        std::cerr << "[Server] Failed to bind to " << bind_address << std::endl;
        return 1;
    }

    std::cout << "[Server] Listening on " << bind_address << std::endl;

    entt::registry registry;
    uint32_t next_network_id = 100;

    std::unordered_map<std::string, ObjectContext> clients;

    char sender_buf[64];
    uint8_t data_buf[1024];

    while (true) {
        const int32_t bytes = net_socket_poll(socket, data_buf, sizeof(data_buf), sender_buf, sizeof(sender_buf));

        if (bytes > 0) {
            std::string sender(sender_buf);

            if (bytes < 1) continue;
            netproto::PacketKind kind = static_cast<netproto::PacketKind>(data_buf[0]);
            if (kind == netproto::PacketKind::Hello) {
                if (clients.find(sender) == clients.end()) {
                    std::cout << "[Server] HELLO from " << sender << std::endl;
                }
            } else if (kind == netproto::PacketKind::Spawn) {
                if (bytes < static_cast<int32_t>(sizeof(netproto::SpawnPacket))) {
                    std::cout << "[Server] SPAWN packet too small from " << sender << std::endl;
                    continue;
                }

                netproto::SpawnPacket incoming{};
                std::memcpy(&incoming, data_buf, sizeof(incoming));

                auto existing = clients.find(sender);
                if (existing == clients.end()) {
                    if (incoming.network_id != 0) {
                        std::cout << "[Server] Ignoring SPAWN with non-zero NetworkID from " << sender << std::endl;
                        continue;
                    }

                    std::cout << "[Server] New client connecting: " << sender << std::endl;

                    ObjectContext ctx{};
                    ctx.network_id = next_network_id++;
                    ctx.local_entity = registry.create();
                    ctx.x = incoming.x;
                    ctx.y = incoming.y;
                    std::cout << "[Server] Client " << sender << " assigned NetworkID " << ctx.network_id
                              << " at position (" << ctx.x << ", " << ctx.y << ")" << std::endl;

                    // Send existing entities to the new client first.
                    for (const auto &[addr, existing_ctx] : clients) {
                        netproto::SpawnPacket spawn{};
                        spawn.kind = netproto::PacketKind::Spawn;
                        spawn.type_id = netproto::type::kPlayer;
                        spawn.network_id = existing_ctx.network_id;
                        spawn.x = existing_ctx.x;
                        spawn.y = existing_ctx.y;

                        net_socket_send(socket, sender.c_str(),
                                        reinterpret_cast<const uint8_t *>(&spawn),
                                        sizeof(spawn));

                        std::cout << "[Server] → Sent SPAWN (NetworkID=" << existing_ctx.network_id
                                  << ") to new client " << sender << std::endl;
                    }

                    clients.emplace(sender, ctx);

                    netproto::SpawnPacket new_spawn{};
                    new_spawn.kind = netproto::PacketKind::Spawn;
                    new_spawn.type_id = netproto::type::kPlayer;
                    new_spawn.network_id = ctx.network_id;
                    new_spawn.x = ctx.x;
                    new_spawn.y = ctx.y;

                    for (const auto &[addr, _] : clients) {
                        net_socket_send(socket, addr.c_str(),
                                        reinterpret_cast<const uint8_t *>(&new_spawn),
                                        sizeof(new_spawn));
                        std::cout << "[Server] → Broadcast new SPAWN (NetworkID=" << ctx.network_id
                                  << ") to " << addr << std::endl;
                    }
                } else {
                    // Optional: update server-side position for existing clients.
                    if (incoming.network_id == existing->second.network_id) {
                        existing->second.x = incoming.x;
                        existing->second.y = incoming.y;
                    }
                }
            }
        }

        std::this_thread::sleep_for(16ms);
    }

    net_socket_destroy(socket);
    return 0;
}