#include "game_client.hpp"
#include <iostream>
#include <cstdlib>
#include <yojimbo.h>
#include <thread>

int main(void)
{
    try
    {
        if (!InitializeYojimbo())
        {
            std::cerr << "Failed to initialize Yojimbo!" << std::endl;
            return 1;
        }

        const yojimbo::Address address("127.0.0.1", 40000);
        GameClient client(address);

        // Note: Hack to poll connection to game server, refactor to proper connection handling later.
        for (int i = 0; i < 200 && !client.IsConnected(); ++i)
        {
            client.Update(0.016f); // Pump at ~60fps
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!client.IsConnected())
        {
            std::cerr << "ERROR: Failed to connect to server after timeout!" << std::endl;
            std::cerr << "Make sure the server is running at 127.0.0.1:40000" << std::endl;
            ShutdownYojimbo();
            return 1;
        }

        InitWindow(1280, 720, "Circ.io Client");
        SetTargetFPS(60);
        std::cout << "[DEBUG] Starting Server updates" << std::endl;
        while (!WindowShouldClose())
        {
            client.Update(GetFrameTime());
        }
        std::cout << "[DEBUG] Exiting" << std::endl;

        ShutdownYojimbo();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "Client will now exit." << std::endl;
        ShutdownYojimbo();
        return 1;
    }
}