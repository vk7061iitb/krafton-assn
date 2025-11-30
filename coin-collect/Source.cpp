#include <iostream>
#include <conio.h>  // For _getch()
#include <windows.h> // For console cursor manipulation
#include <ctime>    // For random seed

using namespace std;

// Game Settings
const int WIDTH = 20;
const int HEIGHT = 10;

// Entities
struct Point {
    int x, y;
};

// Global Variables
bool gameOver;
Point player;
Point coin;
int score;

// --- Helper: Hide Cursor (Reduces visual noise) ---
void HideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

// --- Helper: Reset Cursor Position (Prevents flickering) ---
void ResetCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD Position;
    Position.X = 0;
    Position.Y = 0;
    SetConsoleCursorPosition(hOut, Position);
}

void Setup() {
    gameOver = false;
    score = 0;

    // Center player
    player.x = WIDTH / 2;
    player.y = HEIGHT / 2;

    // Random coin position
    srand(time(0));
    coin.x = rand() % WIDTH;
    coin.y = rand() % HEIGHT;

    HideCursor();
}

void Draw() {
    ResetCursor(); // Move to top-left instead of clearing screen

    // Top Border
    for (int i = 0; i < WIDTH + 2; i++) cout << "#";
    cout << endl;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == 0) cout << "#"; // Left wall

            if (x == player.x && y == player.y)
                cout << "P"; // Player
            else if (x == coin.x && y == coin.y)
                cout << "$"; // Coin
            else
                cout << " "; // Empty space

            if (x == WIDTH - 1) cout << "#"; // Right wall
        }
        cout << endl;
    }

    // Bottom Border
    for (int i = 0; i < WIDTH + 2; i++) cout << "#";
    cout << endl;

    cout << "Score: " << score << endl;
    cout << "Use WASD to move. 'x' to quit." << endl;
}

void Input() {
    if (_kbhit()) { // Check if a key is pressed
        switch (_getch()) {
        case 'a': player.x--; break;
        case 'd': player.x++; break;
        case 'w': player.y--; break;
        case 's': player.y++; break;
        case 'x': gameOver = true; break;
        }
    }
}

void Logic() {
    // 1. Coin Collection
    if (player.x == coin.x && player.y == coin.y) {
        score += 10;
        // Respawn coin
        coin.x = rand() % WIDTH;
        coin.y = rand() % HEIGHT;
    }

    // 2. Wall Collision (Simple Clamp)
    if (player.x >= WIDTH) player.x = WIDTH - 1;
    if (player.x < 0) player.x = 0;
    if (player.y >= HEIGHT) player.y = HEIGHT - 1;
    if (player.y < 0) player.y = 0;
}

int main() {
    Setup();
    while (!gameOver) {
        Draw();
        Input();
        Logic();
        Sleep(50); // Slow down the loop slightly
    }
    return 0;
}