#include <iostream>
#include "game_client.hpp"
#include <cmath>

GameClient::GameClient(const yojimbo::Address &address)
    : m_localPlayer(),
      m_predictedPlayer(),
      m_isLocalPlayerCreated(false),
      m_otherPlayers(),
      m_foodItems(),
      m_camera(),
      m_inputSequence(0),
      m_inputHistory(),
      m_clientTime(0.0),
      m_snapshotBuffer(),
      m_interpolationTime(0.0),
      m_adapter(),
      m_client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), m_connectionConfig, m_adapter, 0.0)
{
    uint64_t clientId;
    yojimbo_random_bytes((uint8_t *)&clientId, 8);

    char buffer[256];
    address.ToString(buffer, sizeof(buffer));
    std::cout << "Connecting to server at " << buffer << "..." << std::endl;

    m_client.InsecureConnect(DEFAULT_PRIVATE_KEY, clientId, address);

    m_camera.target = {WORLD_WIDTH / 2.0f, WORLD_HEIGHT / 2.0f};
    m_camera.offset = {1280 / 2.0f, 720 / 2.0f}; 
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;
}

GameClient::~GameClient()
{
    m_client.Disconnect();

    if (IsWindowReady())
    {
        CloseWindow();
    }
}

void GameClient::ProcessServerMessages()
{
    for (int i = 0; i < (int)GameMessageType::COUNT; ++i)
    {
        yojimbo::Message *message = m_client.ReceiveMessage(i);
        while (message != NULL)
        {
            switch (message->GetType())
            {
            case (int)GameMessageType::WORLD_STATE:
            {
                ReceiveWorldState(static_cast<WorldStateMessage *>(message));
                break;
            }
            default:
                std::cout << "Unknown message type from server" << std::endl;
                break;
            }
            m_client.ReleaseMessage(message);
            message = m_client.ReceiveMessage(i);
        }
    }
}

void GameClient::Update(float dt)
{
    m_clientTime += dt;
    m_client.AdvanceTime(m_client.GetTime() + dt);
    m_client.ReceivePackets();

    if(m_client.IsConnected()) {
        ProcessServerMessages();

        if (IsWindowReady())
        {
            SendInput();
        }

        if (!m_snapshotBuffer.empty())
        {
            const Snapshot& latestSnapshot = m_snapshotBuffer.back();
            m_otherPlayers.clear();
            for (const auto& [id, player] : latestSnapshot.players)
            {
                m_otherPlayers[id] = player;
            }
        }

        if (IsWindowReady())
        {
            UpdateCamera(dt);

            Render();
        }
    }

    m_client.SendPackets();
}

void GameClient::ReceiveWorldState(WorldStateMessage *message)
{
    Snapshot snapshot;
    snapshot.serverTick = message->serverTick;
    snapshot.timestamp = message->timestamp;

    for (int i = 0; i < message->numPlayers; ++i)
    {
        Player player;
        player.id = message->playerIds[i];
        player.position.x = message->playerX[i];
        player.position.y = message->playerY[i];
        player.velocity.x = message->playerVelX[i];
        player.velocity.y = message->playerVelY[i];
        player.size = message->playerSize[i];
        player.color = message->playerColor[i];

        if (message->playerIds[i] == m_client.GetClientIndex())
        {
            if (!m_isLocalPlayerCreated)
            {
                m_localPlayer = player;
                m_predictedPlayer = player;
                m_isLocalPlayerCreated = true;
            }
            else
            {
                m_localPlayer = player;
                ReconcileWithServer(player, message->lastProcessedInputSeq);
            }
        }
        else
        {
            snapshot.players[player.id] = player;
        }
    }

    m_snapshotBuffer.push_back(snapshot);

    while (m_snapshotBuffer.size() > MAX_SNAPSHOTS)
    {
        m_snapshotBuffer.pop_front();
    }

    m_foodItems.clear();
    for (int i = 0; i < message->numFoodItems; ++i)
    {
        float x = message->foodX[i];
        float y = message->foodY[i];
        FoodTier tier = static_cast<FoodTier>(message->foodTier[i]);

        FoodItem food = CreateFoodItemFromTier(x, y, tier);
        m_foodItems.push_back(food);
    }
}

void GameClient::SendInput()
{
    if (!m_isLocalPlayerCreated)
        return;

    float moveX = 0.0f;
    float moveY = 0.0f;
    if (IsKeyDown(KEY_W)) moveY -= 1.0f;
    if (IsKeyDown(KEY_S)) moveY += 1.0f;
    if (IsKeyDown(KEY_A)) moveX -= 1.0f;
    if (IsKeyDown(KEY_D)) moveX += 1.0f;

    bool hasInput = (moveX != 0.0f || moveY != 0.0f);

    if (hasInput)
    {
        m_inputSequence++;

        StoredInput storedInput(m_inputSequence, m_clientTime, moveX, moveY);
        m_inputHistory.push_back(storedInput);

        while (m_inputHistory.size() > MAX_INPUT_HISTORY)
        {
            m_inputHistory.pop_front();
        }

        const float dt = 1.0f / 60.0f;
        PredictMovement(m_predictedPlayer, moveX, moveY, dt);

        PlayerInputMessage *inputMessage = (PlayerInputMessage *)m_client.CreateMessage((int)GameMessageType::PLAYER_INPUT);
        if (inputMessage)
        {
            inputMessage->sequenceNumber = m_inputSequence;
            inputMessage->timestamp = m_clientTime;
            inputMessage->moveX = moveX;
            inputMessage->moveY = moveY;
            m_client.SendMessage((int)GameChannel::UNRELIABLE, inputMessage);
        }
        else
        {
            std::cerr << "ERROR: Failed to create PlayerInputMessage - message allocator may be out of memory" << std::endl;
        }
    }
}

void GameClient::PredictMovement(Player& player, float moveX, float moveY, float dt)
{
    float length = std::sqrt(moveX * moveX + moveY * moveY);
    if (length > 0.0f)
    {
        moveX /= length;
        moveY /= length;
    }

    const float moveSpeed = 200.0f;
    player.velocity.x = moveX * moveSpeed;
    player.velocity.y = moveY * moveSpeed;

    player.position.x += player.velocity.x * dt;
    player.position.y += player.velocity.y * dt;

    if (player.position.x < 0.0f) player.position.x = 0.0f;
    if (player.position.x > WORLD_WIDTH) player.position.x = WORLD_WIDTH;
    if (player.position.y < 0.0f) player.position.y = 0.0f;
    if (player.position.y > WORLD_HEIGHT) player.position.y = WORLD_HEIGHT;
}

void GameClient::ReconcileWithServer(const Player& serverPlayer, uint32_t lastProcessedInput)
{
    while (!m_inputHistory.empty() && m_inputHistory.front().sequenceNumber <= lastProcessedInput)
    {
        m_inputHistory.pop_front();
    }

    m_predictedPlayer = serverPlayer;

    const float dt = 1.0f / 60.0f;
    for (const auto& input : m_inputHistory)
    {
        PredictMovement(m_predictedPlayer, input.moveX, input.moveY, dt);
    }
}

void GameClient::InterpolatePlayerStates(float dt)
{
    if (m_snapshotBuffer.size() < 2)
    {
        if (!m_snapshotBuffer.empty())
        {
            m_otherPlayers.clear();
            for (const auto& [id, player] : m_snapshotBuffer.back().players)
            {
                m_otherPlayers[id] = player;
            }
        }
        return;
    }

    m_interpolationTime = m_clientTime - INTERPOLATION_DELAY;

    Snapshot* from = nullptr;
    Snapshot* to = nullptr;

    for (size_t i = 0; i < m_snapshotBuffer.size() - 1; ++i)
    {
        if (m_snapshotBuffer[i].timestamp <= m_interpolationTime &&
            m_snapshotBuffer[i + 1].timestamp >= m_interpolationTime)
        {
            from = &m_snapshotBuffer[i];
            to = &m_snapshotBuffer[i + 1];
            break;
        }
    }

    if (!from || !to)
    {
        m_otherPlayers.clear();
        for (const auto& [id, player] : m_snapshotBuffer.back().players)
        {
            m_otherPlayers[id] = player;
        }
        return;
    }

    float t = 0.0f;
    float timeDiff = to->timestamp - from->timestamp;
    if (timeDiff > 0.0f)
    {
        t = (m_interpolationTime - from->timestamp) / timeDiff;
        t = std::max(0.0f, std::min(1.0f, t));
    }

    m_otherPlayers.clear();
    for (const auto& [id, toPlayer] : to->players)
    {
        auto fromIt = from->players.find(id);
        if (fromIt != from->players.end())
        {
            Player interpolated = toPlayer;
            const Player& fromPlayer = fromIt->second;

            interpolated.position.x = fromPlayer.position.x + (toPlayer.position.x - fromPlayer.position.x) * t;
            interpolated.position.y = fromPlayer.position.y + (toPlayer.position.y - fromPlayer.position.y) * t;
            interpolated.size = fromPlayer.size + (toPlayer.size - fromPlayer.size) * t;

            m_otherPlayers[id] = interpolated;
        }
        else
        {
            m_otherPlayers[id] = toPlayer;
        }
    }
}

void GameClient::UpdateCamera(float dt)
{
    if (!m_isLocalPlayerCreated)
        return;

    const float CAMERA_SMOOTHNESS = 0.15f;

    Vector2 targetPos = {m_predictedPlayer.position.x, m_predictedPlayer.position.y};

    m_camera.target.x += (targetPos.x - m_camera.target.x) * CAMERA_SMOOTHNESS;
    m_camera.target.y += (targetPos.y - m_camera.target.y) * CAMERA_SMOOTHNESS;

    const float BASE_ZOOM = 1.0f;
    const float ZOOM_FACTOR = 0.015f;
    const float MIN_ZOOM = 0.3f;
    const float MAX_ZOOM = 1.5f;

    float targetZoom = BASE_ZOOM / (1.0f + m_predictedPlayer.size * ZOOM_FACTOR);
    targetZoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, targetZoom));

    m_camera.zoom += (targetZoom - m_camera.zoom) * CAMERA_SMOOTHNESS;
}

void GameClient::RenderGrid()
{
    const float GRID_SPACING = 100.0f;
    const Color GRID_COLOR = {200, 200, 200, 100};

    Vector2 topLeft = GetScreenToWorld2D({0, 0}, m_camera);
    Vector2 bottomRight = GetScreenToWorld2D({1280, 720}, m_camera);

    int startX = (int)(topLeft.x / GRID_SPACING) * GRID_SPACING;
    int startY = (int)(topLeft.y / GRID_SPACING) * GRID_SPACING;
    int endX = (int)(bottomRight.x / GRID_SPACING + 1) * GRID_SPACING;
    int endY = (int)(bottomRight.y / GRID_SPACING + 1) * GRID_SPACING;


    for (int x = startX; x <= endX; x += GRID_SPACING)
    {
        if (x >= 0 && x <= WORLD_WIDTH)
        {
            DrawLineV({(float)x, std::max(0.0f, topLeft.y)},
                     {(float)x, std::min((float)WORLD_HEIGHT, bottomRight.y)},
                     GRID_COLOR);
        }
    }

   
    for (int y = startY; y <= endY; y += GRID_SPACING)
    {
        if (y >= 0 && y <= WORLD_HEIGHT)
        {
            DrawLineV({std::max(0.0f, topLeft.x), (float)y},
                     {std::min((float)WORLD_WIDTH, bottomRight.x), (float)y},
                     GRID_COLOR);
        }
    }

    
    const Color BOUNDARY_COLOR = {100, 100, 100, 255};  // Dark gray
    DrawRectangleLinesEx({0, 0, (float)WORLD_WIDTH, (float)WORLD_HEIGHT}, 3.0f, BOUNDARY_COLOR);
}

void GameClient::Render()
{
    BeginDrawing();

    ClearBackground({240, 240, 245, 255});  // Light blue-gray background
    
    BeginMode2D(m_camera);
    
    RenderGrid();

    for (const auto& food : m_foodItems)
    {
        Color foodColor = GetColor(food.color);
        Vector2 foodPos = {food.position.x, food.position.y};

        float foodSize = 5.0f;
        if (food.tier == FoodTier::MEDIUM) foodSize = 6.0f;
        else if (food.tier == FoodTier::LARGE) foodSize = 8.0f;

        DrawCircleV(foodPos, foodSize, foodColor);
    }

    for (const auto& [id, player] : m_otherPlayers)
    {
        auto color = GetColor(player.color);
        DrawCircleV({player.position.x, player.position.y}, player.size, color);
    }

    if (m_isLocalPlayerCreated)
    {
        auto color = GetColor(m_predictedPlayer.color);
        DrawCircleV({m_predictedPlayer.position.x, m_predictedPlayer.position.y}, m_predictedPlayer.size, color);
        // outline
        DrawCircleLinesV({m_predictedPlayer.position.x, m_predictedPlayer.position.y}, m_predictedPlayer.size + 2, WHITE);
#ifdef DEBUG
        // Debug: Draw server-authoritative position in red
        DrawCircleV({m_localPlayer.position.x, m_localPlayer.position.y}, m_localPlayer.size * 0.5f, RED);
#endif
    }

    EndMode2D();

#ifdef DEBUG
    if (m_isLocalPlayerCreated)
    {
        DrawText(TextFormat("Size: %.1f", m_predictedPlayer.size), 10, 10, 20, BLACK);
        DrawText(TextFormat("Zoom: %.2f", m_camera.zoom), 10, 35, 20, BLACK);
    }
#endif
    EndDrawing();
}
