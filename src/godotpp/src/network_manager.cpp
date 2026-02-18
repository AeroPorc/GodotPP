#include "network_manager.h"
#include "../../../build/_deps/snl-src/snl/include/snl.h"
#include "../../common/network_protocol.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <cstring>

using namespace godot;

void NetworkManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("connect_to_server", "address"), &NetworkManager::connect_to_server);
    ClassDB::bind_method(D_METHOD("disconnect_from_server"), &NetworkManager::disconnect_from_server);
    ClassDB::bind_method(D_METHOD("set_local_actor", "actor"), &NetworkManager::set_local_actor);
}

NetworkManager::NetworkManager()
    : socket(nullptr), is_connected(false) {
    UtilityFunctions::print("[NetworkManager] Constructor called");
}

NetworkManager::~NetworkManager() {
    if (socket) {
        net_socket_destroy(socket);
        socket = nullptr;
    }
}

void NetworkManager::_ready() {
    UtilityFunctions::print("[NetworkManager] Ready - Network system initialized");

    if (!local_actor) {
        if (Node* parent = get_parent()) {
            if (Node* candidate = parent->get_node_or_null(NodePath("LocalPlayer"))) {
                set_local_actor(candidate);
            }
        }
    }
}

void NetworkManager::_process(double delta) {
    if (is_connected && socket) {
        poll_network();
    }
}

void NetworkManager::connect_to_server(const String& address) {
    if (is_connected) {
        UtilityFunctions::print("[NetworkManager] Already connected");
        return;
    }

    UtilityFunctions::print("[NetworkManager] Connecting to: ", address);

    // Create socket
    socket = net_socket_create("0.0.0.0:0");
    if (!socket) {
        UtilityFunctions::print("[NetworkManager] ERROR: Failed to create socket");
        return;
    }

    is_connected = true;

    // Send HELLO packet
    netproto::HelloPacket hello;
    hello.kind = netproto::PacketKind::Hello;
    hello.requested_type = netproto::type::kPlayer;

    CharString addr_str = address.utf8();
    const char* addr_ptr = addr_str.get_data();

    int32_t sent = net_socket_send(socket, addr_ptr,
                                    reinterpret_cast<const uint8_t*>(&hello),
                                    sizeof(hello));

    if (sent > 0) {
        UtilityFunctions::print("[NetworkManager] HELLO sent (", sent, " bytes)");
    } else {
        UtilityFunctions::print("[NetworkManager] ERROR: Failed to send HELLO");
        disconnect_from_server();
        return;
    }

    // Send initial position
    if (!local_actor) {
        UtilityFunctions::print("[NetworkManager] WARNING: No local actor set");
        return;
    }

    Node2D* node2d = Object::cast_to<Node2D>(local_actor);
    if (!node2d) {
        UtilityFunctions::print("[NetworkManager] WARNING: Local actor is not a Node2D");
        return;
    }

    local_spawn_position = node2d->get_position();

    netproto::SpawnPacket spawn;
    spawn.kind = netproto::PacketKind::Spawn;
    spawn.type_id = netproto::type::kPlayer;
    spawn.network_id = 0;
    spawn.x = local_spawn_position.x;
    spawn.y = local_spawn_position.y;

    int32_t spawn_sent = net_socket_send(socket, addr_ptr,
                                         reinterpret_cast<const uint8_t*>(&spawn),
                                         sizeof(spawn));

    if (spawn_sent > 0) {
        pending_local_spawn = true;
        UtilityFunctions::print("[NetworkManager] Sent position (", spawn.x, ", ", spawn.y, ")");
    } else {
        UtilityFunctions::print("[NetworkManager] ERROR: Failed to send position");
    }
}

void NetworkManager::disconnect_from_server() {
    if (socket) {
        net_socket_destroy(socket);
        socket = nullptr;
    }
    is_connected = false;
    network_to_node.clear();
    local_network_id = 0;
    pending_local_spawn = false;
    UtilityFunctions::print("[NetworkManager] Disconnected from server");
}

void NetworkManager::poll_network() {
    if (!socket) return;

    int32_t bytes_received = net_socket_poll(socket, receive_buffer,
                                              sizeof(receive_buffer),
                                              sender_buffer, sizeof(sender_buffer));

    if (bytes_received > 0) {
        UtilityFunctions::print("[NetworkManager] Received ", bytes_received, " bytes");

        if (bytes_received < 1) return;

        netproto::PacketKind kind = static_cast<netproto::PacketKind>(receive_buffer[0]);

        switch (kind) {
            case netproto::PacketKind::Spawn:
                handle_spawn_packet(receive_buffer, bytes_received);
                break;
            case netproto::PacketKind::Hello:
                UtilityFunctions::print("[NetworkManager] Received HELLO response");
                break;
            default:
                UtilityFunctions::print("[NetworkManager] Unknown packet type: ", (int)kind);
                break;
        }
    } else if (bytes_received < 0) {
        UtilityFunctions::print("[NetworkManager] ERROR: Poll failed: ", bytes_received);
    }
}

void NetworkManager::handle_spawn_packet(const uint8_t* data, int32_t size) {
    if (size < sizeof(netproto::SpawnPacket)) {
        UtilityFunctions::print("[NetworkManager] ERROR: SPAWN packet too small");
        return;
    }

    netproto::SpawnPacket spawn;
    std::memcpy(&spawn, data, sizeof(spawn));

    UtilityFunctions::print("[NetworkManager] SPAWN: ID=", spawn.network_id,
                            " Pos=(", spawn.x, ", ", spawn.y, ")");

    auto it = network_to_node.find(spawn.network_id);
    if (it != network_to_node.end()) {
        if (Node2D* node2d = Object::cast_to<Node2D>(it->second)) {
            node2d->set_position(Vector2(spawn.x, spawn.y));
        }
        return;
    }

    if (pending_local_spawn && local_actor && local_network_id == 0) {
        const float dx = Math::abs(spawn.x - local_spawn_position.x);
        const float dy = Math::abs(spawn.y - local_spawn_position.y);

        if (dx <= 0.5f && dy <= 0.5f) {
            if (Node2D* node2d = Object::cast_to<Node2D>(local_actor)) {
                node2d->set_position(Vector2(spawn.x, spawn.y));
            }
            local_network_id = spawn.network_id;
            network_to_node[spawn.network_id] = local_actor;
            pending_local_spawn = false;
            UtilityFunctions::print("[NetworkManager] ✓ Local actor bound to ID=", spawn.network_id);
            return;
        }
    }

    // Spawn
    if (!local_actor) {
        UtilityFunctions::print("[NetworkManager] ERROR: No local actor template");
        return;
    }

    Node* new_entity = local_actor->duplicate();
    if (!new_entity) {
        UtilityFunctions::print("[NetworkManager] ERROR: Failed to duplicate");
        return;
    }

    if (Node2D* node2d = Object::cast_to<Node2D>(new_entity)) {
        node2d->set_position(Vector2(spawn.x, spawn.y));
    }

    add_child(new_entity);
    network_to_node[spawn.network_id] = new_entity;

    UtilityFunctions::print("[NetworkManager] ✓ Spawned remote entity ID=", spawn.network_id);
}

void NetworkManager::set_local_actor(Node* actor) {
    local_actor = actor;
    if (local_actor) {
        UtilityFunctions::print("[NetworkManager] Local actor set: ", local_actor->get_name());
    } else {
        UtilityFunctions::print("[NetworkManager] Local actor cleared");
    }
}
