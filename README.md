# Circ.io

A multiplayer online game inspired by Agar.io, built with modern C++ and real-time networking.

## Overview

Circ.io is a client-server networked game where players control growing circles that consume food and compete against other players in a shared world. The project demonstrates game networking techniques such as client-side prediction, server reconciliation, and snapshot interpolation. Not production ready, made the project just for fun and learning purposes.

## Features

- Real-time multiplayer gameplay (up to 16 players)
- Client-side prediction with lag compensation
- Three-tier food system with dynamic respawning
- Smooth camera and interpolation
- Reliable UDP networking

## Technology Stack

- **Language**: C++17
- **Graphics**: Raylib
- **Networking**: Yojimbo
- **Build System**: CMake

## Building

```bash
make build
make game_server    # Run server
make game_client    # Run client
```

## Credits

This project makes use of the following open-source libraries:

- **[Raylib](https://www.raylib.com/)** - Graphics and game programming library
- **[Yojimbo](https://github.com/networkprotocol/yojimbo)** - Client-server networking library
- **[EASTL](https://github.com/electronicarts/EASTL)** - Electronic Arts Standard Template Library
