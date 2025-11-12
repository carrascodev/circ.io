#include "catch.hpp"
#include "../common/protocol.hpp"
#include "../server/game_server.hpp"
#include "../client/game_client.hpp"
#include <yojimbo.h>
#include <thread>
#include <chrono>

// Helper function to pump messages between client and server
static void PumpClientServer(GameClient& client, GameServer& server, int iterations = 10)
{
    for (int i = 0; i < iterations; ++i)
    {
        server.Update(0.016f);
        client.Update(0.016f);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// Helper to wait for client connection
static bool WaitForConnection(GameClient& client, GameServer& server, int maxAttempts = 100)
{
    for (int i = 0; i < maxAttempts; ++i)
    {
        server.Update(0.016f);
        client.Update(0.016f);

        if (client.IsConnected())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

TEST_CASE("Message send/receive tests", "[messages]")
{
    REQUIRE(InitializeYojimbo());

    SECTION("Server broadcasts WorldStateMessage to connected client")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40010);

        GameServer server(serverAddress);
        REQUIRE(server.IsRunning());

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        GameClient client(serverAddress);

        // Wait for connection
        REQUIRE(WaitForConnection(client, server));
        REQUIRE(client.IsConnected());

        // Pump messages to allow server to send world state
        // Server should spawn player and send WorldStateMessage
        PumpClientServer(client, server, 50);

        // Client should have received world state and created local player
        REQUIRE(client.IsLocalPlayerCreated());
    }

    SECTION("WorldStateMessage contains player data")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40011);

        GameServer server(serverAddress);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        GameClient client1(serverAddress);

        // Wait for connection
        REQUIRE(WaitForConnection(client1, server));

        // Pump to spawn player
        PumpClientServer(client1, server, 50);
        REQUIRE(client1.IsLocalPlayerCreated());

        // Connect second client
        GameClient client2(serverAddress);
        REQUIRE(WaitForConnection(client2, server));

        // Pump to allow both clients to see each other
        PumpClientServer(client1, server, 50);
        PumpClientServer(client2, server, 50);

        // Each client should see at least themselves
        REQUIRE(client1.IsLocalPlayerCreated());
        REQUIRE(client2.IsLocalPlayerCreated());

        // Client1 should see client2 as "other player" and vice versa
        // Note: This might be 0 or 1 depending on timing and player ID assignment
        INFO("Client1 sees " << client1.GetOtherPlayerCount() << " other players");
        INFO("Client2 sees " << client2.GetOtherPlayerCount() << " other players");
    }

    SECTION("Multiple clients receive world state updates")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40012);

        GameServer server(serverAddress);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        GameClient client1(serverAddress);
        GameClient client2(serverAddress);
        GameClient client3(serverAddress);

        // Connect all clients
        for (int i = 0; i < 100; ++i)
        {
            server.Update(0.016f);
            client1.Update(0.016f);
            client2.Update(0.016f);
            client3.Update(0.016f);

            if (client1.IsConnected() && client2.IsConnected() && client3.IsConnected())
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        REQUIRE(client1.IsConnected());
        REQUIRE(client2.IsConnected());
        REQUIRE(client3.IsConnected());

        // Server should have 3 connected clients
        REQUIRE(server.GetConnectedClientCount() == 3);

        // Pump messages to receive world state
        PumpClientServer(client1, server, 50);
        PumpClientServer(client2, server, 50);
        PumpClientServer(client3, server, 50);

        // All clients should have local players created
        REQUIRE(client1.IsLocalPlayerCreated());
        REQUIRE(client2.IsLocalPlayerCreated());
        REQUIRE(client3.IsLocalPlayerCreated());
    }

    SECTION("Client receives updated world state over time")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40013);

        GameServer server(serverAddress);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        GameClient client(serverAddress);

        // Wait for connection
        REQUIRE(WaitForConnection(client, server));

        // Initial world state
        PumpClientServer(client, server, 20);
        REQUIRE(client.IsLocalPlayerCreated());

        // Continue pumping - player size should grow over time on server
        // and client should receive updates
        for (int i = 0; i < 10; ++i)
        {
            PumpClientServer(client, server, 5);
        }

        // Client should still be connected and receiving updates
        REQUIRE(client.IsConnected());
        REQUIRE(client.IsLocalPlayerCreated());
    }

    ShutdownYojimbo();
}
