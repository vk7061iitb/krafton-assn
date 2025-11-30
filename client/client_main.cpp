#include <SDL3/SDL.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "SDL3.lib")

using namespace std;
using namespace std::chrono;

const int WIDTH = 20;
const int HEIGHT = 10;
const int CELL_SIZE = 40;
const int WINDOW_WIDTH = WIDTH * CELL_SIZE;
const int WINDOW_HEIGHT = HEIGHT * CELL_SIZE + 80;
const int NETWORK_LATENCY_MS = 200;
const float INTERPOLATION_TIME = 0.2f;

struct Position {
    float x, y;
    Position(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};

struct Entity {
    Position current, target, previous;
    high_resolution_clock::time_point lastUpdateTime;

    Entity() : current(0, 0), target(0, 0), previous(0, 0) {
        lastUpdateTime = high_resolution_clock::now();
    }

    void updateTarget(float newX, float newY) {
        previous = target;
        target = Position(newX, newY);
        lastUpdateTime = high_resolution_clock::now();
    }

    void interpolate() {
        auto now = high_resolution_clock::now();
        float elapsed = duration_cast<milliseconds>(now - lastUpdateTime).count() / 1000.0f;
        float t = min(elapsed / INTERPOLATION_TIME, 1.0f);
        current.x = previous.x + (target.x - previous.x) * t;
        current.y = previous.y + (target.y - previous.y) * t;
    }
};

Entity selfEntity, otherEntity, coinEntity;
int selfScore = 0, otherScore = 0;
bool gameOver = false;
SOCKET clientSocket = INVALID_SOCKET;

void render(SDL_Renderer* renderer, SDL_Window* window) {
    // update title with scores
    string title = "coin collector | you: " + to_string(selfScore) + " | other: " + to_string(otherScore);
    SDL_SetWindowTitle(window, title.c_str());

    // clear screen white
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // draw grid
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for (int i = 0; i <= WIDTH; i++) {
        SDL_RenderLine(renderer, i * CELL_SIZE, 0, i * CELL_SIZE, HEIGHT * CELL_SIZE);
    }
    for (int i = 0; i <= HEIGHT; i++) {
        SDL_RenderLine(renderer, 0, i * CELL_SIZE, WIDTH * CELL_SIZE, i * CELL_SIZE);
    }

    // draw self -> blue
    SDL_FRect selfRect = {
        selfEntity.current.y * CELL_SIZE + 5,
        selfEntity.current.x * CELL_SIZE + 5,
        CELL_SIZE - 10,
        CELL_SIZE - 10
    };
    SDL_SetRenderDrawColor(renderer, 0, 120, 255, 255);
    SDL_RenderFillRect(renderer, &selfRect);

    // draw other player -> red
    SDL_FRect otherRect = {
        otherEntity.current.y * CELL_SIZE + 5,
        otherEntity.current.x * CELL_SIZE + 5,
        CELL_SIZE - 10,
        CELL_SIZE - 10
    };
    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    SDL_RenderFillRect(renderer, &otherRect);

    // draw coin -> gold
    SDL_FRect coinRect = {
        coinEntity.current.y * CELL_SIZE + 10,
        coinEntity.current.x * CELL_SIZE + 10,
        CELL_SIZE - 20,
        CELL_SIZE - 20
    };
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderFillRect(renderer, &coinRect);

    SDL_RenderPresent(renderer);
}

void networkThread() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "wsa startup failed\n";
        return;
    }

    struct addrinfo hints = {}, * res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("127.0.0.1", "55555", &hints, &res) != 0) {
        cout << "getaddrinfo failed\n";
        WSACleanup();
        return;
    }

    clientSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (clientSocket == INVALID_SOCKET) {
        cout << "socket creation failed\n";
        freeaddrinfo(res);
        WSACleanup();
        return;
    }

    if (connect(clientSocket, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        cout << "connection failed\n";
        closesocket(clientSocket);
        freeaddrinfo(res);
        WSACleanup();
        return;
    }

    freeaddrinfo(res);
    cout << "connected to server\n";

    char buf[512];
    int bytesRecv;

    // wait for initial game state
    while (!gameOver) {
        bytesRecv = recv(clientSocket, buf, sizeof(buf) - 1, 0);
        if (bytesRecv <= 0) continue;

        buf[bytesRecv] = '\0';
        stringstream ss(buf);
        int sx, sy, ox, oy, cx, cy, sscr, oscr;
        if (ss >> sx >> sy >> ox >> oy >> cx >> cy >> sscr >> oscr) {
            selfEntity.current = selfEntity.target = selfEntity.previous = Position((float)sx, (float)sy);
            otherEntity.current = otherEntity.target = otherEntity.previous = Position((float)ox, (float)oy);
            coinEntity.current = coinEntity.target = coinEntity.previous = Position((float)cx, (float)cy);
            selfScore = sscr;
            otherScore = oscr;
            cout << "game started\n";
            break;
        }
    }

    //// set non blocking
    //u_long mode = 1;
    //ioctlsocket(clientSocket, FIONBIO, &mode);

    // game loop
    while (!gameOver) {
        bytesRecv = recv(clientSocket, buf, sizeof(buf) - 1, 0);
        if (bytesRecv > 0) {
            buf[bytesRecv] = '\0';
            stringstream ss(buf);
            int sx, sy, ox, oy, cx, cy, sscr, oscr;
            if (ss >> sx >> sy >> ox >> oy >> cx >> cy >> sscr >> oscr) {
                selfEntity.updateTarget((float)sx, (float)sy);
                otherEntity.updateTarget((float)ox, (float)oy);
                coinEntity.updateTarget((float)cx, (float)cy);

                if (sscr != selfScore || oscr != otherScore) {
                    cout << "you: " << sscr; 
                    cout << ", ";
                    cout << "other: " << oscr;
                    cout << "\n";
                }

                selfScore = sscr;
                otherScore = oscr;
            }
        }
        Sleep(10);
    }
}

int main(int argc, char* argv[]) {
    // init sdl
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cout << "sdl init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // create window
    SDL_Window* window = SDL_CreateWindow("coin collector", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        cout << "window creation failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        cout << "renderer creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // start network thread
    thread netThread(networkThread);
    netThread.detach();

    cout << "arrow keys to move, e to quit\n";

    // main loop
    auto lastTime = high_resolution_clock::now();
    SDL_Event event;

    while (!gameOver) {
        // handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                gameOver = true;
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                char cmd = 0;
                switch (event.key.key) {
                case SDLK_UP:    cmd = 'U'; break;
                case SDLK_DOWN:  cmd = 'D'; break;
                case SDLK_LEFT:  cmd = 'L'; break;
                case SDLK_RIGHT: cmd = 'R'; break;
                case SDLK_E:
                    cmd = 'E';
                    gameOver = true;
                    break;
                }
                if (cmd != 0 && clientSocket != INVALID_SOCKET) {
                    send(clientSocket, &cmd, 1, 0);
                }
            }
        }

        if (gameOver) break;

        // interpolate positions
        selfEntity.interpolate();
        otherEntity.interpolate();
        coinEntity.interpolate();

        // render at 60 fps
        auto now = high_resolution_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - lastTime).count();
        if (elapsed >= 16) {
            render(renderer, window);
            lastTime = now;
        }

        SDL_Delay(1);
    }

    // cleanup
    closesocket(clientSocket);
    WSACleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}