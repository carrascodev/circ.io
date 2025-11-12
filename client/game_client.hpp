#pragma once
#include <memory>
#include <yojimbo.h>
#include "../common/protocol.hpp"
#include "raylib.h"
#include <EASTL/deque.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

class GameClient
{
public:
    GameClient(const yojimbo::Address &address);
    ~GameClient();
    void ProcessServerMessages();
    void Update(float dt);

    // Test/utility methods
    bool IsConnected() const { return m_client.IsConnected(); }
    bool IsLocalPlayerCreated() const { return m_isLocalPlayerCreated; }
    int GetOtherPlayerCount() const { return m_otherPlayers.size(); }

private:
    Player m_localPlayer;
    Player m_predictedPlayer;
    bool m_isLocalPlayerCreated = false;

    eastl::unordered_map<int, Player> m_otherPlayers;
    eastl::vector<FoodItem> m_foodItems;

    // Camera
    Camera2D m_camera;
    void UpdateCamera(float dt);
    void RenderGrid();

    uint32_t m_inputSequence;
    eastl::deque<StoredInput> m_inputHistory;
    double m_clientTime;

    eastl::deque<Snapshot> m_snapshotBuffer;
    double m_interpolationTime;

    ClientAdapter m_adapter;
    GameConnectionConfig m_connectionConfig;
    yojimbo::Client m_client;

    void ReceiveWorldState(WorldStateMessage *message);
    void SendInput();
    void PredictMovement(Player& player, float moveX, float moveY, float dt);
    void ReconcileWithServer(const Player& serverPlayer, uint32_t lastProcessedInput);
    void InterpolatePlayerStates(float dt);
    void Render();
};