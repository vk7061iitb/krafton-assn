#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <string>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int WIDTH = 20;
const int HEIGHT = 10;
const int MAX_CLIENTS = 2;
const char* DEF_PORT = "55555";
const int NETWORK_LATENCY_MS = 200;

enum Direction { LEFT, RIGHT, UP, DOWN };

struct Position {
    int x, y;
    Position(int _x = 0, int _y = 0) : x(_x), y(_y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct Player {
    Position pos;
    int score = 0;
};

struct GameState {
    Player p1, p2;
    Position coin;

    GameState() {
        srand((unsigned)time(0));
        p1.pos = Position(0, 0);
        p2.pos = Position(HEIGHT - 1, WIDTH - 1);
        spawnCoin();
    }

    void spawnCoin() {
        // coin shouldn't appear at players pos
        while (coin == p1.pos || coin == p2.pos) {
            coin = Position(rand() % HEIGHT, rand() % WIDTH);
        }
    }

    void movePlayer(Player& player, Direction dir) {
        Position newPos = player.pos;
        switch (dir) {
        case UP:    newPos.x--; break;
        case DOWN:  newPos.x++; break;
        case LEFT:  newPos.y--; break;
        case RIGHT: newPos.y++; break;
        }

        // boundary check
        bool outofBound = newPos.x < 0 || newPos.x >= HEIGHT || newPos.y < 0 || newPos.y >= WIDTH;
        bool collided = newPos == p1.pos || newPos == p2.pos;
        if (outofBound || collided){
            return; 
        }

        player.pos = newPos;

        // coin collision
        if (player.pos == coin) {
            player.score += 10;
            spawnCoin();
        }
    }

    string serialize(SOCKET currentClient, unordered_map<SOCKET, Player*>& playerMap) {
        Player* self = playerMap[currentClient];
        Player* other = (self == &p1) ? &p2 : &p1;

        return to_string(self->pos.x) + " " + to_string(self->pos.y) + " " +
            to_string(other->pos.x) + " " + to_string(other->pos.y) + " " +
            to_string(coin.x) + " " + to_string(coin.y) + " " +
            to_string(self->score) + " " + to_string(other->score) + "\n";
    }
};

Direction decode(char c) {
    switch (c) {
    case 'U': return UP;
    case 'D': return DOWN;
    case 'L': return LEFT;
    case 'R': return RIGHT;
    }
    return UP;
}

int main() {
    // init winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "wsa startup failed\n";
        return 1;
    }

    // setup address info
    struct addrinfo hints = {}, * res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, DEF_PORT, &hints, &res) != 0) {
        cout << "getaddrinfo failed\n";
        WSACleanup();
        return 1;
    }

    // create listen socket
    SOCKET listenSock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listenSock == INVALID_SOCKET) {
        cout << "socket creation failed\n";
        freeaddrinfo(res);
        WSACleanup();
        return 1;
    }

    if (bind(listenSock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        cout << "bind failed\n";
        closesocket(listenSock);
        freeaddrinfo(res);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(res);

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        cout << "listen failed\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    cout << "server listening on port " << DEF_PORT << endl;

    fd_set masterSet;
    FD_ZERO(&masterSet);
    FD_SET(listenSock, &masterSet);

    vector<SOCKET> clients;
    unordered_map<SOCKET, Player*> playerMap;
    bool gameStarted = false;
    GameState gameState;

    while (true) {
        fd_set copy = masterSet;
        int count = select(0, &copy, nullptr, nullptr, nullptr);

        for (int i = 0; i < count; i++) {
            SOCKET sock = copy.fd_array[i];

            // new connection
            if (sock == listenSock) {
                SOCKET client = accept(listenSock, nullptr, nullptr);

                // server full
                if (clients.size() >= MAX_CLIENTS) {
                    string msg = "server full\n";
                    send(client, msg.c_str(), (int)msg.size(), 0);
                    closesocket(client);
                    cout << "rejected client (server full)\n";
                    continue;
                }

                FD_SET(client, &masterSet);
                clients.push_back(client);
                playerMap[client] = (clients.size() == 1) ? &gameState.p1 : &gameState.p2;

                string msg = "connected! waiting for other player...\n";
                send(client, msg.c_str(), (int)msg.size(), 0);
                cout << "player " << clients.size() << " connected\n";

                // start game when both players connected
                if (clients.size() == MAX_CLIENTS) {
                    gameStarted = true;
                    cout << "game started!\n";

                    for (SOCKET s : clients) {
                        string startMsg = "game start!\n";
                        send(s, startMsg.c_str(), (int)startMsg.size(), 0);

                        string initState = gameState.serialize(s, playerMap);
                        Sleep(NETWORK_LATENCY_MS);
                        send(s, initState.c_str(), (int)initState.size(), 0);
                    }
                }
            }
            // handle client input
            else if (gameStarted) {
                char cmd;
                int bytes = recv(sock, &cmd, 1, 0);

                // client disconnected
                if (bytes <= 0) {
                    cout << "player disconnected\n";
                    FD_CLR(sock, &masterSet);
                    closesocket(sock);
                    clients.erase(find(clients.begin(), clients.end(), sock));
                    playerMap.erase(sock);
                    gameStarted = false;
                    continue;
                }

                // quit command
                if (cmd == 'E' || cmd == 'e') {
                    cout << "game ended by player\n";
                    break;
                }

                // process movement
                Player* player = playerMap[sock];
                gameState.movePlayer(*player, decode(cmd));

                // broadcast update to all clients
                for (SOCKET s : clients) {
                    string update = gameState.serialize(s, playerMap);
                    Sleep(NETWORK_LATENCY_MS);
                    send(s, update.c_str(), (int)update.size(), 0);
                }
            }
        }
    }

    // cleanup
    closesocket(listenSock);
    WSACleanup();
    cout << "server closed\n";
    return 0;
}