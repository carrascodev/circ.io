#include "game_server.hpp"
#include <cmath>

void GameAdapter::OnServerClientConnected(int clientIndex)
{
    if (m_server)
    {
        m_server->ClientConnected(clientIndex);
    }
}

void GameAdapter::OnServerClientDisconnected(int clientIndex)
{
    if (m_server)
    {
        m_server->ClientDisconnected(clientIndex);
    }
}

GameServer::GameServer(const yojimbo::Address &address)
    : m_connectionConfig(),
      m_adapter(std::make_unique<GameAdapter>(this)),
      m_server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, address, m_connectionConfig, *m_adapter, 0.0),
      m_time(0.0),
      m_lastProcessedInput()
{
    m_server.Start(MAX_PLAYERS);
    if (!m_server.IsRunning())
    {
        char buffer[256];
        address.ToString(buffer, sizeof(buffer));
        throw std::runtime_error(std::string("Failed to start server at ") + buffer +
                                 ". Port may be in use or address is invalid.");
    }

    char buffer[256];
    address.ToString(buffer, sizeof(buffer));
    std::cout << "Server started at " << buffer << std::endl;
}

GameServer::~GameServer()
{
    m_server.Stop();
}

void GameServer::ClientConnected(int clientIndex)
{
    std::cout << "Client " << clientIndex << " connected." << std::endl;

    SpawnPlayer(clientIndex);
}

void GameServer::ClientDisconnected(int clientIndex)
{
    std::cout << "Client " << clientIndex << " disconnected." << std::endl;

    if (m_worldState.players.erase(clientIndex) > 0)
    {
        std::cout << "Player " << clientIndex << " removed from world state." << std::endl;
    }
}

void GameServer::Run()
{
    const double tickRate = 1.0 / 60.0;
    m_time = yojimbo_time();

    for (int j = 0; j < MAX_FOOD; j++)
    {
        m_worldState.foodItems[j] = CreateFood();
    }

    while (m_server.IsRunning())
    {
        double currentTime = yojimbo_time();

        if (m_time <= currentTime)
        {
            Update(static_cast<float>(tickRate));
            m_time += tickRate;
        }
        else
        {
            yojimbo_sleep(m_time - currentTime);
        }
    }
}

void GameServer::Update(float dt)
{
    m_worldState.serverTick++;
    m_worldState.timestamp = m_time;

    m_server.AdvanceTime(m_time);
    m_server.ReceivePackets();
    ProcessMessages();

    HandleGameFood();
    HandlePlayerCollisions();

    BroadcastWorldState();
    m_server.SendPackets();
}

void GameServer::ProcessMessages()
{
    for (int clientIndex = 0; clientIndex < MAX_PLAYERS; ++clientIndex)
    {
        if (m_server.IsClientConnected(clientIndex))
        {
            for (int channelIndex = 0; channelIndex < m_connectionConfig.numChannels; ++channelIndex)
            {
                yojimbo::Message *message;
                while ((message = m_server.ReceiveMessage(clientIndex, channelIndex)) != nullptr)
                {
                    ProcessClientMessage(clientIndex, message);
                    m_server.ReleaseMessage(clientIndex, message);
                }
            }
        }
    }
}

void GameServer::ProcessClientMessage(int clientIndex, yojimbo::Message *message)
{
    switch (message->GetType())
    {
    case (int)GameMessageType::PLAYER_INPUT:
    {
        ReceivePlayerInputMessage(clientIndex, static_cast<PlayerInputMessage *>(message));
        break;
    }
    default:
        std::cout << "Unknown message type from client " << clientIndex << std::endl;
        break;
    }
}

void GameServer::ReceivePlayerInputMessage(int clientIndex, PlayerInputMessage *message)
{
    m_lastProcessedInput[clientIndex] = message->sequenceNumber;

    if (m_worldState.players.find(clientIndex) == m_worldState.players.end())
    {
        return;
    }

    float moveX = message->moveX;
    float moveY = message->moveY;

    float length = std::sqrt(moveX * moveX + moveY * moveY);
    if (length > 0.0f)
    {
        moveX /= length;
        moveY /= length;
    }

    const float moveSpeed = 200.0f;
    m_worldState.players[clientIndex].velocity.x = moveX * moveSpeed;
    m_worldState.players[clientIndex].velocity.y = moveY * moveSpeed;

    const float dt = 1.0f / 60.0f;
    m_worldState.players[clientIndex].position.x += m_worldState.players[clientIndex].velocity.x * dt;
    m_worldState.players[clientIndex].position.y += m_worldState.players[clientIndex].velocity.y * dt;

    // Clamp to world bounds
    if (m_worldState.players[clientIndex].position.x < 0.0f)
        m_worldState.players[clientIndex].position.x = 0.0f;
    if (m_worldState.players[clientIndex].position.x > WORLD_WIDTH)
        m_worldState.players[clientIndex].position.x = WORLD_WIDTH;
    if (m_worldState.players[clientIndex].position.y < 0.0f)
        m_worldState.players[clientIndex].position.y = 0.0f;
    if (m_worldState.players[clientIndex].position.y > WORLD_HEIGHT)
        m_worldState.players[clientIndex].position.y = WORLD_HEIGHT;
}

void GameServer::SpawnPlayer(int clientIndex)
{
    uint32_t r = rand() % 256;
    uint32_t g = rand() % 256;
    uint32_t b = rand() % 256;
    uint32_t color = (r << 24) | (g << 16) | (b << 8) | 0xFF; // RGBA: full alpha
    Player player;
    player.id = clientIndex;
    player.position.x = static_cast<float>(rand() % WORLD_WIDTH);  // Random spawn position
    player.position.y = static_cast<float>(rand() % WORLD_HEIGHT); // Random spawn position
    player.size = 10.0f;                                           // Default size
    player.color = color;

    m_worldState.players[clientIndex] = player;

    std::cout << "Player " << player.id << " spawned for client " << clientIndex << std::endl;
}

void GameServer::BroadcastWorldState()
{
    const int maxClients = m_server.GetMaxClients();
    for (int clientIndex = 0; clientIndex < maxClients; ++clientIndex)
    {
        if (!m_server.IsClientConnected(clientIndex))
            continue;

        WorldStateMessage *msg = (WorldStateMessage *)m_server.CreateMessage(clientIndex, (int)GameMessageType::WORLD_STATE);

        if (msg)
        {
            msg->serverTick = m_worldState.serverTick;
            msg->timestamp = m_worldState.timestamp;

            auto it = m_lastProcessedInput.find(clientIndex);
            msg->lastProcessedInputSeq = (it != m_lastProcessedInput.end()) ? it->second : 0;

            msg->numPlayers = 0;
            for (const auto &[id, player] : m_worldState.players)
            {
                if (msg->numPlayers >= MAX_PLAYERS)
                    break;

                int idx = msg->numPlayers;
                msg->playerIds[idx] = player.id;
                msg->playerX[idx] = player.position.x;
                msg->playerY[idx] = player.position.y;
                msg->playerVelX[idx] = player.velocity.x;
                msg->playerVelY[idx] = player.velocity.y;
                msg->playerSize[idx] = player.size;
                msg->playerColor[idx] = player.color;
                msg->numPlayers++;
            }

            msg->numFoodItems = MAX_FOOD;
            for (int i = 0; i < msg->numFoodItems; ++i)
            {
                msg->foodX[i] = m_worldState.foodItems[i].position.x;
                msg->foodY[i] = m_worldState.foodItems[i].position.y;
                msg->foodTier[i] = static_cast<uint8_t>(m_worldState.foodItems[i].tier);
            }

            m_server.SendMessage(clientIndex, (int)GameChannel::UNRELIABLE, msg);
        }
        else
        {
            std::cerr << "ERROR: Failed to create WorldStateMessage for client " << clientIndex
                      << " - message allocator may be out of memory" << std::endl;
        }
    }
}

void GameServer::HandleGameFood()
{
    const float FOOD_SIZE = 5.0f; 

    for (const auto &[id, player] : m_worldState.players)
    {
        float collisionRadius = player.size / 2.0f + FOOD_SIZE / 2.0f;
        float collisionRadiusSquared = collisionRadius * collisionRadius;

        for (int j = 0; j < MAX_FOOD; j++)
        {
            if (m_worldState.foodItems[j].position.distanceSquared(player.position) < collisionRadiusSquared)
            {
                float growthAmount = m_worldState.foodItems[j].value;
                m_worldState.players[id].size += growthAmount;

                m_worldState.foodItems[j] = CreateFood();
            }
        }
    }
}

FoodItem GameServer::CreateFood()
{
    float x = static_cast<float>(rand() % WORLD_WIDTH);
    float y = static_cast<float>(rand() % WORLD_HEIGHT);

    int roll = rand() % 100;
    FoodTier tier;

    if (roll < 60) {
        tier = FoodTier::SMALL;
    } else if (roll < 90) {
        tier = FoodTier::MEDIUM;
    } else {
        tier = FoodTier::LARGE;
    }

    return CreateFoodItemFromTier(x, y, tier);
}

void GameServer::RespawnPlayer(uint32_t playerId)
{
    auto it = m_worldState.players.find(playerId);
    if (it == m_worldState.players.end())
        return;

    Player& player = it->second;

    player.position.x = static_cast<float>(rand() % WORLD_WIDTH);
    player.position.y = static_cast<float>(rand() % WORLD_HEIGHT);
    player.velocity.x = 0.0f;
    player.velocity.y = 0.0f;
    player.size = 10.0f;


    std::cout << "Player " << playerId << " respawned at ("
              << player.position.x << ", " << player.position.y << ")" << std::endl;
}

void GameServer::HandlePlayerCollisions()
{
    eastl::vector<uint32_t> playersToRespawn;

    for (auto it1 = m_worldState.players.begin(); it1 != m_worldState.players.end(); ++it1)
    {
        auto it2 = it1;
        ++it2;
        for (; it2 != m_worldState.players.end(); ++it2)
        {
            Player& player1 = it1->second;
            Player& player2 = it2->second;

            float distSquared = player1.position.distanceSquared(player2.position);

            float collisionRadius = (player1.size + player2.size) / 2.0f;
            float collisionRadiusSquared = collisionRadius * collisionRadius;

            if (distSquared < collisionRadiusSquared)
            {
                const float SIZE_ADVANTAGE = 1.1f;

                if (player1.size > player2.size * SIZE_ADVANTAGE)
                {
                    float growthAmount = player2.size * 0.5f; 

                    std::cout << "Player " << player1.id << " (size " << player1.size - growthAmount
                              << ") ate Player " << player2.id << " (size " << player2.size << ")" << std::endl;

                    playersToRespawn.push_back(player2.id);
                }
                else if (player2.size > player1.size * SIZE_ADVANTAGE)
                {
                    float growthAmount = player1.size * 0.5f;
                    player2.size += growthAmount;

                    std::cout << "Player " << player2.id << " (size " << player2.size - growthAmount
                              << ") ate Player " << player1.id << " (size " << player1.size << ")" << std::endl;

                    playersToRespawn.push_back(player1.id);
                }
            }
        }
    }

    for (uint32_t playerId : playersToRespawn)
    {
        RespawnPlayer(playerId);
    }
}