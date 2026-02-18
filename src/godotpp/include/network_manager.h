#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <unordered_map>
#include <cstdint>

struct GameSocket;

namespace godot {

class NetworkManager : public Node {
    GDCLASS(NetworkManager, Node)

private:
    GameSocket* socket;
    std::unordered_map<uint32_t, Node*> network_to_node;

    bool is_connected;

    uint8_t receive_buffer[1024];
    char sender_buffer[64];

    Node* local_actor{nullptr};
    uint32_t local_network_id{0};
    bool pending_local_spawn{false};
    Vector2 local_spawn_position{0.0f, 0.0f};

protected:
    static void _bind_methods();

public:
    NetworkManager();
    ~NetworkManager();

    void _ready() override;
    void _process(double delta) override;

    void connect_to_server(const String& address);
    void disconnect_from_server();
    void set_local_actor(Node* actor);

    void poll_network();
    void handle_spawn_packet(const uint8_t* data, int32_t size);
};

}

#endif

