# Coin Collector

A realtime multiplayer game demonstrating client-server architecture with network latency simulation and smooth entity interpolation.

## Features

- **Server-authoritative gameplay**: All game logic runs on the server
- **200ms network latency simulation**: Simulates real-world network conditions
- **Smooth interpolation**: Client-side interpolation for fluid movement despite latency
- **2-player competitive**: Collect coins to score points

## Technology Stack

- **Language**: C++
- **Networking**: Raw TCP sockets (WinSock2)
- **Rendering**: SDL3 (client-side)
- **Platform**: Windows

## Architecture

![](/architecture.png)
- Blue -> client 1 Traffic
- Red -> client 2 Traffic

### Server
- Authoritative game state management
- Validates all player movements
- Handles collision detection
- Broadcasts game state to clients with simulated 200ms latency

### Client
- Sends movement intent only (arrow key commands)
- Receives authoritative game state updates
- Implements linear interpolation for smooth rendering
- 60 FPS rendering with SDL3

## Build Instructions

### Prerequisites
- Visual Studio 2019 or later
- SDL3 library ([Download here](https://github.com/libsdl-org/SDL/releases))
   - dowload the visual studio version move it to a folder where keep libraries (i moved it into `C` directory)
   - In Visual studio
      - Inside the C/C++ Additional Include Directories add `C:\SDL\include` (this one renamed to SDL, may not be same for new dowloaded folder)
      - Inside the Linker's Additional Library Directories add `C:\SDL\lib\x64`
      - Additional Dependencies in Linker-> Input : `SDL3.lib;`
      - Move the `SDL3.dll` file to the same folder as `client_main.cpp`
- Windows SDK

### Server Setup
1. Open `server` project in Visual Studio
2. Build configuration: Release or Debug
3. Build the project (Ctrl+Shift+B)

Project Configuration:
![Visual studio project config](/project_config.png)

### Client Setup
1. Download SDL3 and extract to `C:\SDL\` (or update paths in project settings)
2. Open `client` project in Visual Studio
3. Ensure SDL3 include/lib paths are configured:
   - Include: `C:\SDL\include`
   - Library: `C:\SDL\lib\x64`
4. Copy `SDL3.dll` from `C:\SDL\lib\x64\` to client output directory
5. Build the project

## Running the Game

1. **Start the Server**:
   ```
   server.exe
   ```
   Server listens on port `55555`

2. **Start Client 1**:
   ```
   client.exe
   ```
   Connects to `127.0.0.1:55555`

3. **Start Client 2**:
   ```
   client.exe
   ```
   Game starts when both clients are connected

## Controls

- **Arrow Keys**: Move player (Up/Down/Left/Right)
- **E**: Quit game

## Game Rules

- Blue square = Your player
- Red square = Other player
- Gold square = Coin
- Collect coins to score +10 points
- Scores displayed in window title bar

## Technical Implementation

### Network Latency Simulation
Both server and client introduce 200ms delay using `Sleep(NETWORK_LATENCY_MS)` on all network operations.

### Interpolation Algorithm
Client uses linear interpolation (LERP) to smooth entity positions:
```cpp
float t = elapsed / INTERPOLATION_TIME;
current.x = previous.x + (target.x - previous.x) * t;
```

This ensures smooth movement despite receiving discrete position updates every 200ms.

### Server Authority
- Clients send only input commands `(U/D/L/R)`
- Server validates all movements (boundary checks)
- Server detects collisions and updates scores
- Clients cannot spoof positions or scores

## Video Demonstration

Link to video demo : https://youtu.be/OOvI1oGhWeM

## Resources
- [How to use winsock](https://learn.microsoft.com/en-us/windows/win32/winsock/using-winsock)
- [Creating multi-client connection using winsock](https://youtu.be/dquxuXeZXgo?si=zAeoygSZjhWomzpB)
- [SDL3 Docs](https://wiki.libsdl.org/SDL3/FrontPage)