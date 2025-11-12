#pragma once
#include <cstdint>
#include <cmath>
#include <yojimbo.h>
#include <yojimbo_adapter.h>
#include <EASTL/unordered_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/deque.h>

// Forward declaration
class GameServer;

static const int MAX_PLAYERS = 16;
static const int MAX_FOOD = 128;
static const int WORLD_WIDTH = 3200;
static const int WORLD_HEIGHT = 2400;
static const uint8_t DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = {0};

// Networking constants
static const int MAX_INPUT_HISTORY = 128;  // How many inputs to keep for reconciliation
static const int MAX_SNAPSHOTS = 64;       // How many snapshots to keep for interpolation
static const float INTERPOLATION_DELAY = 0.1f;  // 100ms delay for smooth interpolation

struct Position {
    float x;
    float y;

    Position() : x(0), y(0) {}
    Position(float _x, float _y): x(_x), y(_y) {};
    Position(int _x, int _y): x(static_cast<float>(_x)), y(static_cast<float>(_y)) {};

    // Calculate Euclidean distance between two positions
    float distance(const Position& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // Calculate squared distance (faster, avoids sqrt for comparisons)
    float distanceSquared(const Position& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return dx * dx + dy * dy;
    }
};

struct Velocity {
    float x;
    float y;
};

// Food tiers for color-coded value system
enum class FoodTier {
    SMALL = 0,   // Green - low value
    MEDIUM = 1,  // Yellow - medium value
    LARGE = 2    // Red - high value
};

struct FoodItem {
    Position position;
    FoodTier tier;
    uint32_t color;  // RGBA color
    float value;     // Growth amount when eaten

    FoodItem() : position{0, 0}, tier(FoodTier::SMALL), color(0), value(0.3f) {}
    FoodItem(float x, float y, FoodTier t, uint32_t c, float v)
        : position{x, y}, tier(t), color(c), value(v) {}
};

// Get color and value from tier (deterministic, platform-independent)
inline void GetFoodPropertiesFromTier(FoodTier tier, uint32_t& color, float& value) {
    switch (tier) {
        case FoodTier::SMALL:
            color = 0x00FF00FF;  // Green RGBA
            value = 0.3f;
            break;
        case FoodTier::MEDIUM:
            color = 0xFFFF00FF;  // Yellow RGBA
            value = 0.7f;
            break;
        case FoodTier::LARGE:
            color = 0xFF0000FF;  // Red RGBA
            value = 1.5f;
            break;
    }
}

inline FoodItem CreateFoodItemFromTier(float x, float y, FoodTier tier) {
    uint32_t color;
    float value;
    GetFoodPropertiesFromTier(tier, color, value);
    return FoodItem(x, y, tier, color, value);
}

struct Player {
    uint32_t id;
    Position position;
    Velocity velocity;
    float size;
    uint32_t color;

    Player() : id(0), position{0, 0}, velocity{0, 0}, size(0), color(0) {}
};

struct WorldState {
    eastl::unordered_map<uint32_t, Player> players;
    eastl::fixed_vector<FoodItem, 128> foodItems;
    uint32_t serverTick;
    double timestamp;

    WorldState() : serverTick(0), timestamp(0.0) {}
};

enum class GameMessageType {
    WORLD_STATE,
    PLAYER_INPUT,
    COUNT
};

enum class GameChannel {
    RELIABLE,
    UNRELIABLE,
    COUNT
};

struct WorldStateMessage : public yojimbo::Message {

    // Server tick and timestamp for interpolation/prediction
    uint32_t serverTick;
    double timestamp;
    uint32_t lastProcessedInputSeq;  // Last input sequence server processed for this client

    // Fixed arrays (better for games)
    uint16_t numPlayers;
    uint32_t playerIds[MAX_PLAYERS];
    float playerX[MAX_PLAYERS];
    float playerY[MAX_PLAYERS];
    float playerVelX[MAX_PLAYERS];
    float playerVelY[MAX_PLAYERS];
    float playerSize[MAX_PLAYERS];
    uint32_t playerColor[MAX_PLAYERS];

    uint16_t numFoodItems;
    float foodX[MAX_FOOD];
    float foodY[MAX_FOOD];
    uint8_t foodTier[MAX_FOOD];  // Only 2 bits needed: 0=SMALL, 1=MEDIUM, 2=LARGE
    // NOTE: color and value are generated client-side from tier (saves 8 bytes per food!)

    WorldStateMessage() : serverTick(0), timestamp(0.0), lastProcessedInputSeq(0), numPlayers(0), numFoodItems(0) {}

    template <typename Stream>
    bool Serialize(Stream& stream) {
        // Server tick and timestamp
        serialize_bits(stream, serverTick, 32);
        serialize_double(stream, timestamp);
        serialize_bits(stream, lastProcessedInputSeq, 32);

        // Players
        serialize_int(stream, numPlayers, 0, MAX_PLAYERS);
        for (int i = 0; i < numPlayers; ++i) {
            serialize_bits(stream, playerIds[i], 32);
            serialize_float(stream, playerX[i]);
            serialize_float(stream, playerY[i]);
            serialize_float(stream, playerVelX[i]);
            serialize_float(stream, playerVelY[i]);
            serialize_float(stream, playerSize[i]);
            serialize_bits(stream, playerColor[i], 32);
        }

        // Food (position + tier - color/value generated client-side from tier)
        serialize_int(stream, numFoodItems, 0, MAX_FOOD);
        for (int i = 0; i < numFoodItems; ++i) {
            serialize_float(stream, foodX[i]);
            serialize_float(stream, foodY[i]);
            serialize_int(stream, foodTier[i], 0, 2);  // Only 2 bits for 3 tiers (0, 1, 2)
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

struct PlayerInputMessage : public yojimbo::Message {
    uint32_t sequenceNumber;  // Monotonically increasing input sequence
    double timestamp;         // Client timestamp when input was generated
    float moveX, moveY;

    PlayerInputMessage() : sequenceNumber(0), timestamp(0.0), moveX(0.0f), moveY(0.0f) {}

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_bits(stream, sequenceNumber, 32);
        serialize_double(stream, timestamp);
        serialize_float(stream, moveX);
        serialize_float(stream, moveY);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

struct GameConnectionConfig : public yojimbo::ClientServerConfig {
    GameConnectionConfig() {
        numChannels = 2;
        channel[static_cast<int>(GameChannel::RELIABLE)].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[static_cast<int>(GameChannel::UNRELIABLE)].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

// the message factory
YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int)GameMessageType::COUNT);
    YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::WORLD_STATE, WorldStateMessage);
    YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::PLAYER_INPUT, PlayerInputMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();


class GameAdapter : public yojimbo::Adapter {
public:
    explicit GameAdapter(GameServer* server) : m_server(server) {}

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override {
        return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
    }

    void OnServerClientConnected(int clientIndex) override;
    void OnServerClientDisconnected(int clientIndex) override;

private:
    GameServer* m_server;
};

class ClientAdapter : public yojimbo::Adapter {
public:
    ClientAdapter() {}

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override {
        return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
    }
};

// ===========================
// Prediction & Interpolation Structures
// ===========================

// Stored input for client-side prediction and server reconciliation
struct StoredInput {
    uint32_t sequenceNumber;
    double timestamp;
    float moveX;
    float moveY;

    StoredInput() : sequenceNumber(0), timestamp(0.0), moveX(0.0f), moveY(0.0f) {}
    StoredInput(uint32_t seq, double ts, float mx, float my)
        : sequenceNumber(seq), timestamp(ts), moveX(mx), moveY(my) {}
};

// Stored player state for prediction
struct StoredPlayerState {
    Position position;
    Velocity velocity;
    float size;

    StoredPlayerState() : position{0, 0}, velocity{0, 0}, size(0) {}
    StoredPlayerState(const Player& player)
        : position(player.position), velocity(player.velocity), size(player.size) {}
};

// Snapshot for interpolation (other players)
struct Snapshot {
    uint32_t serverTick;
    double timestamp;
    eastl::unordered_map<uint32_t, Player> players;

    Snapshot() : serverTick(0), timestamp(0.0) {}
};