#include "game_server.hpp"
#include <yojimbo.h>
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

std::atomic<bool> g_running(true);
std::thread g_serverThread;

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nShutting down server..." << std::endl;
        g_running = false;
    }
}

int main(int argc, char* argv[])
{
    if (!InitializeYojimbo())
    {
        std::cerr << "Failed to initialize yojimbo" << std::endl;
        return 1;
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    const char* serverAddress = "127.0.0.1";
    uint16_t serverPort = 40000;

    if (argc >= 2)
    {
        serverAddress = argv[1];
    }
    if (argc >= 3)
    {
        serverPort = static_cast<uint16_t>(std::atoi(argv[2]));
    }

    std::cout << "Starting Agar.io-like Game Server" << std::endl;
    std::cout << "Address: " << serverAddress << ":" << serverPort << std::endl;
    std::cout << "Max Players: " << MAX_PLAYERS << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;

    try
    {
        yojimbo::Address address(serverAddress, serverPort);

        GameServer server(address);

        std::thread([&server]() {
            server.Run();
        }).detach();
        while (g_running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        ShutdownYojimbo();
        return 1;
    }

    ShutdownYojimbo();
    std::cout << "Server shut down successfully" << std::endl;

    return 0;
}
