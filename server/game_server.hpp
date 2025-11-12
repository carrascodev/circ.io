#pragma once
#include <memory>
#include <iostream>
#include <stdexcept>
#include <yojimbo.h>
#include "../common/protocol.hpp"
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

class GameServer
{
public:
    GameServer(const yojimbo::Address &address);
    ~GameServer();

    void Run();
    void Update(float dt);
    void ClientConnected(int clientIndex);
    void ClientDisconnected(int clientIndex);

    bool IsRunning() const { return m_server.IsRunning(); }
    int GetConnectedClientCount() const
    {
        int count = 0;
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (m_server.IsClientConnected(i)) count++;
        }
        return count;
    }

private:
    GameConnectionConfig m_connectionConfig;
    std::unique_ptr<GameAdapter> m_adapter;
    yojimbo::Server m_server;
    double m_time;
    WorldState m_worldState;

    eastl::unordered_map<int, uint32_t> m_lastProcessedInput;

    void ProcessMessages();
    void ProcessClientMessage(int clientIndex, yojimbo::Message *message);
    void ReceivePlayerInputMessage(int clientIndex, PlayerInputMessage *message);
    void SpawnPlayer(int clientIndex);
    void RespawnPlayer(uint32_t playerId);
    void BroadcastWorldState();
    void HandleGameFood();
    void HandlePlayerCollisions();
    FoodItem CreateFood();
};