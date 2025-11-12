#include "catch.hpp"
#include "../common/protocol.hpp"
#include "../server/game_server.hpp"
#include "../client/game_client.hpp"
#include <yojimbo.h>
#include <thread>
#include <chrono>

// Helper function to pump messages between client and server
static void PumpClientServer(GameClient& client, GameServer& server, double deltaTime = 0.016)
{
    server.Update(deltaTime);
    client.Update(deltaTime);
}

// Helper function to wait for connection with timeout
static bool WaitForConnection(GameClient& client, int maxAttempts = 100)
{
    for (int i = 0; i < maxAttempts; ++i)
    {
        if (client.IsConnected())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

TEST_CASE("Connection tests", "[connection]")
{
    REQUIRE(InitializeYojimbo());

    SECTION("Server starts successfully")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40001);

        GameServer server(serverAddress);
        REQUIRE(server.IsRunning());
    }

    SECTION("Client connects to server")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40002);

        GameServer server(serverAddress);
        REQUIRE(server.IsRunning());

        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        GameClient client(serverAddress);

        // Pump messages to allow connection handshake
        for (int i = 0; i < 100; ++i)
        {
            PumpClientServer(client, server);
            if (client.IsConnected())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        REQUIRE(client.IsConnected());
    }

    SECTION("Client disconnects from server")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40003);

        GameServer server(serverAddress);
        REQUIRE(server.IsRunning());

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
            GameClient client(serverAddress);

            // Wait for connection
            for (int i = 0; i < 100; ++i)
            {
                PumpClientServer(client, server);
                if (client.IsConnected())
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            REQUIRE(client.IsConnected());

            // Client destructor will disconnect
        }

        // Give server time to process disconnection
        server.Update(0.016);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Server should still be running
        REQUIRE(server.IsRunning());
    }

    SECTION("Multiple clients can connect")
    {
        const yojimbo::Address serverAddress("127.0.0.1", 40004);

        GameServer server(serverAddress);
        REQUIRE(server.IsRunning());

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        GameClient client1(serverAddress);
        GameClient client2(serverAddress);

        // Wait for both connections
        for (int i = 0; i < 100; ++i)
        {
            server.Update(0.016);
            client1.Update(0.016);
            client2.Update(0.016);

            if (client1.IsConnected() && client2.IsConnected())
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        REQUIRE(client1.IsConnected());
        REQUIRE(client2.IsConnected());
    }

    ShutdownYojimbo();
}
