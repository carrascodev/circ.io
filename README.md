# Circ.io

A multiplayer online game inspired by Agar.io, built with modern C++ and real-time networking.

## Overview

Circ.io is a client-server networked game where players control growing circles that consume food and compete against other players in a shared world. The project demonstrates modern game networking techniques including client-side prediction, server reconciliation, and snapshot interpolation.

## Features

- Real-time multiplayer gameplay (up to 16 players)
- Client-side prediction with lag compensation
- Three-tier food system with dynamic respawning
- Smooth camera and interpolation
- Reliable UDP networking with encryption

## Technology Stack

- **Language**: C++17
- **Graphics**: Raylib
- **Networking**: Yojimbo (netcode.io, reliable.c)
- **Build System**: CMake + vcpkg
- **Deployment**: Docker

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
- **[netcode.io](https://github.com/networkprotocol/netcode)** - Secure client/server connection protocol
- **[reliable.io](https://github.com/networkprotocol/reliable)** - Reliable UDP packet transmission
- **[EASTL](https://github.com/electronicarts/EASTL)** - Electronic Arts Standard Template Library
- **[libsodium](https://libsodium.org/)** - Cryptography library for network encryption

Special thanks to Glenn Fiedler for the excellent networking libraries and articles on game networking.
