#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include <raymath.h>

// =========================================================
// --- CONSTANTES & CONFIG ---
// =========================================================
#define MAX_PLAYERS 8
#define HISTORY_SIZE 200
#define DUEL_TIME_LIMIT 30.0f
#define ZONE_SHRINK_SPEED 2.5f

// Noms des modèles pour le mode Satisfaisant
const char* SATISFYING_NAMES[] = { "FLEUR GEANTE", "GALAXIE", "FRACTALE", "ONDES", "CRISTAL" };
#define SATISFYING_COUNT 5

// =========================================================
// --- DEFINITIONS DES MOTIFS (PATTERNS) ---
// =========================================================
typedef struct { int w, h; int *cells; } Pattern;

// 1. GLIDER (Standard)
int glider_se_data[] = { 0,1,0, 0,0,1, 1,1,1 };
Pattern PATTERN_GLIDER_SE = {3, 3, glider_se_data};

// 2. CLOWN
int clown_data[] = { 1,1,1, 1,0,1, 1,0,1 };
Pattern PATTERN_CLOWN = {3, 3, clown_data};

// 3. CANON DE GOSPER
int gosper_data[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
Pattern PATTERN_GOSPER = {36, 9, gosper_data};

void apply_pattern(int *grid, int *ageGrid, int cols, int rows, int x, int y, Pattern p, int playerVal) {
    for (int py = 0; py < p.h; py++) {
        for (int px = 0; px < p.w; px++) {
            int targetX = x + px;
            int targetY = y + py;
            if (targetX >= 0 && targetX < cols && targetY >= 0 && targetY < rows) {
                if (p.cells[py * p.w + px] >= 1) {
                    int idx = targetY * cols + targetX;
                    grid[idx] = playerVal;
                    ageGrid[idx] = 1;
                }
            }
        }
    }
}

void apply_pattern_flipped(int *grid, int *ageGrid, int cols, int rows, int x, int y, Pattern p, int playerVal) {
    for (int py = 0; py < p.h; py++) {
        for (int px = 0; px < p.w; px++) {
            int targetX = x + (p.w - 1 - px);
            int targetY = y + py;
            if (targetX >= 0 && targetX < cols && targetY >= 0 && targetY < rows) {
                if (p.cells[py * p.w + px] >= 1) {
                    int idx = targetY * cols + targetX;
                    grid[idx] = playerVal;
                    ageGrid[idx] = 1;
                }
            }
        }
    }
}

// =========================================================
// --- STRUCTURES ---
// =========================================================

typedef enum {
    MENU, GAME, RULES,
    DUEL_SETUP, DUEL_RUN, DUEL_END,
    BR_SETUP, BR_RUN, BR_END,
    SATISFYING_SETUP, SATISFYING_RUN
} AppState;

typedef struct {
    int rows; int cols;
    int *grid; int *nextGrid; int *ageGrid;

    int *history[HISTORY_SIZE];
    int *historyAge[HISTORY_SIZE];
    int historyHead; int historyCount;

    long generation;
    bool isPaused;
    float timeAccumulator; float updateSpeed;

    char playerNames[MAX_PLAYERS + 1][20];
    int scores[MAX_PLAYERS + 1];

    int currentInput;
    int brPlayerCount;

    float battleTimer;
    float zoneRadius;

    int satisfyingType;

} GameOfLife;

Color GetPlayerColor(int id) {
    switch (id) {
        // --- COULEURS INVERSÉES ---
        case 1: return BLUE; // Joueur 1 = Bleu
        case 2: return RED;  // Joueur 2 = Rouge
        // --------------------------
        case 3: return GREEN;
        case 4: return ORANGE;
        case 5: return PURPLE;
        case 6: return YELLOW;
        case 7: return PINK;
        case 8: return LIME;
        default: return DARKGRAY;
    }
}

GameOfLife* init_game(int cols, int rows) {
    GameOfLife *game = (GameOfLife*)malloc(sizeof(GameOfLife));
    game->cols = cols; game->rows = rows;
    int size = cols * rows;

    game->grid = (int*)calloc(size, sizeof(int));
    game->nextGrid = (int*)calloc(size, sizeof(int));
    game->ageGrid = (int*)calloc(size, sizeof(int));

    for(int i=0; i<HISTORY_SIZE; i++) {
        game->history[i] = (int*)calloc(size, sizeof(int));
        game->historyAge[i] = (int*)calloc(size, sizeof(int));
    }

    game->historyHead = 0; game->historyCount = 0;
    game->generation = 0;
    game->isPaused = true;
    game->timeAccumulator = 0.0f;
    game->updateSpeed = 0.1f;

    strcpy(game->playerNames[1], "Joueur 1");
    strcpy(game->playerNames[2], "Joueur 2");
    strcpy(game->playerNames[3], "Joueur 3");
    strcpy(game->playerNames[4], "Joueur 4");

    game->currentInput = 1;
    game->brPlayerCount = 4;
    game->battleTimer = 0.0f;
    game->zoneRadius = (float)cols;
    game->satisfyingType = 0;

    return game;
}

void save_state(GameOfLife *game) {
    int size = game->cols * game->rows;
    game->historyHead = (game->historyHead + 1) % HISTORY_SIZE;
    memcpy(game->history[game->historyHead], game->grid, size * sizeof(int));
    memcpy(game->historyAge[game->historyHead], game->ageGrid, size * sizeof(int));
    if (game->historyCount < HISTORY_SIZE) game->historyCount++;
}

void undo_state(GameOfLife *game) {
    if (game->historyCount > 0) {
        int size = game->cols * game->rows;
        memcpy(game->grid, game->history[game->historyHead], size * sizeof(int));
        memcpy(game->ageGrid, game->historyAge[game->historyHead], size * sizeof(int));
        game->historyHead--;
        if (game->historyHead < 0) game->historyHead = HISTORY_SIZE - 1;
        game->historyCount--;
        if (game->generation > 0) game->generation--;
    }
}

void clear_grid(GameOfLife *game) {
    int size = game->cols * game->rows;
    memset(game->grid, 0, size * sizeof(int));
    memset(game->ageGrid, 0, size * sizeof(int));
    game->historyCount = 0; game->generation = 0;
}

// =========================================================
// --- UPDATE LOGIC ---
// =========================================================

void update_game_classic(GameOfLife *game) {
    save_state(game);
    game->generation++;
    int offsets[8] = {-1-game->cols, -game->cols, 1-game->cols, -1, 1, -1+game->cols, game->cols, 1+game->cols};

    int centerVal = game->grid[game->cols/2 + (game->rows/2)*game->cols];
    int spawnColor = (centerVal == 5) ? 5 : 1;
    if (game->generation < 5 && centerVal == 0) spawnColor = 5;

    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            int idx = y * game->cols + x;
            int count = 0;
            if (x > 0 && x < game->cols - 1 && y > 0 && y < game->rows - 1) {
                for (int k = 0; k < 8; k++) if (game->grid[idx + offsets[k]] > 0) count++;
            }

            int myVal = game->grid[idx];
            if (myVal > 0) {
                if (count == 2 || count == 3) { game->nextGrid[idx] = myVal; game->ageGrid[idx]++; }
                else { game->nextGrid[idx] = 0; game->ageGrid[idx] = 0; }
            } else {
                if (count == 3) {
                    game->nextGrid[idx] = (spawnColor == 5) ? 5 : 1;
                    game->ageGrid[idx] = 1;
                }
                else { game->nextGrid[idx] = 0; game->ageGrid[idx] = 0; }
            }
        }
    }
    int *temp = game->grid; game->grid = game->nextGrid; game->nextGrid = temp;
}

void update_duel_simple(GameOfLife *game) {
    save_state(game);
    game->generation++;
    int offsets[8] = {-1-game->cols, -game->cols, 1-game->cols, -1, 1, -1+game->cols, game->cols, 1+game->cols};
    int size = game->rows * game->cols;
    memset(game->nextGrid, 0, size * sizeof(int));
    game->scores[1] = 0; game->scores[2] = 0;

    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            int idx = y * game->cols + x;
            int count1 = 0; int count2 = 0;

            if (x > 0 && x < game->cols - 1 && y > 0 && y < game->rows - 1) {
                for (int k = 0; k < 8; k++) {
                    int nVal = game->grid[idx + offsets[k]];
                    if (nVal == 1) count1++;
                    else if (nVal == 2) count2++;
                }
            }
            int total = count1 + count2;

            if (game->grid[idx] > 0) {
                if (total == 2 || total == 3) game->nextGrid[idx] = game->grid[idx];
                else game->nextGrid[idx] = 0;
            } else {
                if (total == 3) {
                    if (count1 > count2) game->nextGrid[idx] = 1;
                    else game->nextGrid[idx] = 2;
                } else {
                    game->nextGrid[idx] = 0;
                }
            }
            if (game->nextGrid[idx] == 1) game->scores[1]++;
            if (game->nextGrid[idx] == 2) game->scores[2]++;
        }
    }
    int *temp = game->grid; game->grid = game->nextGrid; game->nextGrid = temp;
}

int update_game_battle(GameOfLife *game, int maxPlayers, bool useZone) {
    save_state(game);
    game->generation++;
    int offsets[8] = {-1-game->cols, -game->cols, 1-game->cols, -1, 1, -1+game->cols, game->cols, 1+game->cols};
    float cx = game->cols / 2.0f; float cy = game->rows / 2.0f;
    float radiusSq = game->zoneRadius * game->zoneRadius;
    int size = game->rows * game->cols;
    memset(game->nextGrid, 0, size * sizeof(int));
    for(int i=0; i<=MAX_PLAYERS; i++) game->scores[i] = 0;

    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            int idx = y * game->cols + x;
            float dx = x - cx; float dy = y - cy;
            float distSq = dx*dx + dy*dy;

            if (useZone && distSq > radiusSq && game->grid[idx] > 0) {
                int moveX = (x > cx) ? -1 : (x < cx ? 1 : 0);
                int moveY = (y > cy) ? -1 : (y < cy ? 1 : 0);
                int targetX = x + moveX; int targetY = y + moveY;
                if (targetX >= 0 && targetX < game->cols && targetY >= 0 && targetY < game->rows) {
                    int targetIdx = targetY * game->cols + targetX;
                    if (game->nextGrid[targetIdx] == 0) game->nextGrid[targetIdx] = game->grid[idx];
                }
                continue;
            }

            int counts[MAX_PLAYERS + 1]; for(int k=0; k<=MAX_PLAYERS; k++) counts[k] = 0;
            if (x > 0 && x < game->cols - 1 && y > 0 && y < game->rows - 1) {
                for (int k = 0; k < 8; k++) {
                    int val = game->grid[idx + offsets[k]];
                    if (val > 0 && val <= maxPlayers) counts[val]++;
                }
            }
            int total = 0; for(int p=1; p<=maxPlayers; p++) total += counts[p];

            if (game->grid[idx] > 0) {
                if (total == 2 || total == 3) { if (game->nextGrid[idx] == 0) game->nextGrid[idx] = game->grid[idx]; }
            } else {
                if (total == 3) {
                    int maxC = 0; int winner = 0;
                    for(int p=1; p<=maxPlayers; p++) {
                        if (counts[p] > maxC) { maxC = counts[p]; winner = p; }
                        else if (counts[p] == maxC && maxC > 0) { if (rand()%2 == 0) winner = p; }
                    }
                    if (game->nextGrid[idx] == 0) game->nextGrid[idx] = winner;
                }
            }
            if (game->nextGrid[idx] > 0) game->scores[game->nextGrid[idx]]++;
        }
    }
    int *temp = game->grid; game->grid = game->nextGrid; game->nextGrid = temp;
    int survivors = 0; for(int i=1; i<=maxPlayers; i++) { if(game->scores[i] > 0) survivors++; }
    return survivors;
}


// =========================================================
// --- SETUP & GENERATION ---
// =========================================================

void setup_duel_random(GameOfLife *game) {
    clear_grid(game);
    int totalCells = game->cols * game->rows;
    for (int i = 0; i < totalCells; i++) {
        int r = rand() % 100;
        if (r < 10) game->grid[i] = 1;
        else if (r < 20) game->grid[i] = 2;
        else game->grid[i] = 0;
        if (game->grid[i] != 0) game->ageGrid[i] = 1;
    }
    game->battleTimer = 0.0f; game->generation = 0;
}

// --- SETUP MODE SATISFAISANT (AMÉLIORÉ & CENTRÉ) ---
void setup_satisfying(GameOfLife *game) {
    clear_grid(game);
    int cx = game->cols / 2;
    int cy = game->rows / 2;
    float maxRadius = (game->rows < game->cols ? game->rows : game->cols) * 0.45f;

    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            float dx = (float)(x - cx);
            float dy = (float)(y - cy);
            float dist = sqrtf(dx*dx + dy*dy);
            float angle = atan2f(dy, dx);

            bool fill = false;
            // 4 = Cristal (On traite différemment pour le remplissage)
            bool isCrystal = (game->satisfyingType == 4);

            // --- SELECTION DU MOTIF ---
            switch (game->satisfyingType) {
                case 0: // FLEUR GEANTE
                {
                    float shape = fabs(cosf(6.0f * angle));
                    float limit = maxRadius * (0.3f + 0.7f * shape);
                    if (dist <= limit) fill = true;
                    break;
                }
                case 1: // GALAXIE
                {
                    float spiralOffset = dist * 0.15f;
                    float arm = cosf(3.0f * angle + spiralOffset);
                    if (dist <= maxRadius && arm > 0.2f) fill = true;
                    break;
                }
                case 2: // FRACTALE (RECENTRAGE ICI)
                {
                    // Ajustement des offsets pour centrer le triangle
                    int fx = x - (cx - (int)maxRadius);
                    int fy = y - (cy - (int)maxRadius);
                    if (fx > 0 && fy > 0 && fx < maxRadius*2 && fy < maxRadius*2) {
                        if ((fx & fy) == 0) fill = true;
                    }
                    break;
                }
                case 3: // ONDES
                {
                    float wave = sinf(dist * 0.2f);
                    if (dist <= maxRadius && wave > 0.0f) fill = true;
                    break;
                }
                case 4: // CRISTAL (Manhattan/Chebyshev)
                {
                    float dManhattan = (fabs(dx) + fabs(dy));
                    float dChebyshev = fmaxf(fabs(dx), fabs(dy));
                    if ((dManhattan + dChebyshev) * 0.5f < maxRadius * 0.8f) fill = true;
                    break;
                }
            }

            // --- LOGIQUE DE REMPLISSAGE ---
            if (fill) {
                int idx = y * game->cols + x;

                if (isCrystal) {
                    // POUR LE CRISTAL : Remplissage aléatoire dense (Bruit)
                    if (rand() % 100 < 65) {
                        game->grid[idx] = 5;
                        game->ageGrid[idx] = 1;
                    }
                } else {
                    // POUR LES AUTRES : Damier harmonieux
                    if ((x + y) % 2 == 0) {
                        game->grid[idx] = 5;
                        game->ageGrid[idx] = 1;
                    }
                }
            }
        }
    }

    game->generation = 0;
    game->isPaused = true;
}

void setup_battleroyale(GameOfLife *game) {
    clear_grid(game);
    int w = game->cols; int h = game->rows;
    int playerCount = game->brPlayerCount;
    for (int i = 0; i < w * h; i++) {
        int r = rand() % 100;
        int chancePerPlayer = 6;
        int assigned = 0;
        for (int p = 1; p <= playerCount; p++) {
            if (r < p * chancePerPlayer) { game->grid[i] = p; assigned = 1; break; }
        }
        if (assigned) game->ageGrid[i] = 1;
    }
    float centerX = w / 2.0f; float centerY = h / 2.0f;
    float radius = (w < h ? w : h) * 0.35f;
    for (int p = 1; p <= playerCount; p++) {
        float angle = (2.0f * PI * (p - 1)) / playerCount;
        int spawnX = (int)(centerX + cosf(angle) * radius) - 18;
        int spawnY = (int)(centerY + sinf(angle) * radius) - 4;
        if (spawnX < centerX) apply_pattern(game->grid, game->ageGrid, w, h, spawnX, spawnY, PATTERN_GOSPER, p);
        else apply_pattern_flipped(game->grid, game->ageGrid, w, h, spawnX, spawnY, PATTERN_GOSPER, p);
    }
    game->battleTimer = 0.0f; game->generation = 0; game->zoneRadius = (float)w * 0.75f;
}

void random_fill_classic(GameOfLife *game) {
    int size = game->cols * game->rows;
    for (int i = 0; i < size; i++) {
        game->grid[i] = (rand() % 100 < 20) ? 1 : 0;
        game->ageGrid[i] = game->grid[i];
    }
}

Color get_cell_color(int val, int age, AppState state) {
    if (val == 0) return BLACK;
    if (state == GAME || state == SATISFYING_RUN) {
        if (val == 5) { // Mode Satisfaisant (Violet -> Rose -> Blanc)
            float factor = (float)age / 40.0f; if (factor > 1.0f) factor = 1.0f;
            return (Color){
                140 + (unsigned char)(115*factor),
                40 + (unsigned char)(100*factor),
                255,
                255
            };
        }
        float factor = (float)age / 50.0f; if (factor > 1.0f) factor = 1.0f;
        return (Color){ 255 - (unsigned char)(25*factor), 255 - (unsigned char)(214*factor), 255 - (unsigned char)(200*factor), 255 };
    }
    return GetPlayerColor(val);
}

void draw_home_button(AppState *state, GameOfLife *game) {
    // CORRECTION WARNING : Ajout de .0f pour signifier un float explicite
    Rectangle btnHome = { 10.0f, 10.0f, 40.0f, 40.0f };
    DrawRectangleRec(btnHome, LIGHTGRAY);
    DrawRectangleLinesEx(btnHome, 2, DARKGRAY);

    // CORRECTION WARNING : Vector2 doit utiliser des floats
    DrawTriangle((Vector2){10.0f, 30.0f}, (Vector2){50.0f, 30.0f}, (Vector2){30.0f, 10.0f}, DARKGRAY);

    DrawRectangle(18, 30, 24, 18, DARKGRAY);
    DrawRectangle(26, 38, 8, 10, LIGHTGRAY);
    if (CheckCollisionPointRec(GetMousePosition(), btnHome) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        *state = MENU; game->isPaused = true;
    }
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main(void) {
    srand(time(NULL));
    InitWindow(0, 0, "Jeu de la Vie - Ultimate");
    int monitor = GetCurrentMonitor();
    int screenWidth = GetMonitorWidth(monitor);
    int screenHeight = GetMonitorHeight(monitor);
    ToggleFullscreen();
    SetTargetFPS(60);

    int GRID_W = 200; int GRID_H = 150; int CELL_SIZE = 10;
    GameOfLife *game = init_game(GRID_W, GRID_H);

    Camera2D camera = { 0 };
    camera.zoom = 0.8f;
    // CORRECTION WARNING : Cast explicite (float) pour l'initialisation de Vector2
    camera.offset = (Vector2){ (float)screenWidth/2.0f, (float)screenHeight/2.0f };
    camera.target = (Vector2){ (float)(GRID_W*CELL_SIZE)/2.0f, (float)(GRID_H*CELL_SIZE)/2.0f };

    // Charger les images de fond
    Texture2D duelBackground = LoadTexture("image_2.png");

    AppState state = MENU;
    float backgroundScroll = 0.0f;
    int selectedPattern = 0;
    const char* toolName = "Crayon";

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            // CORRECTION WARNING : Cast explicite
            camera.offset = (Vector2){ (float)screenWidth/2.0f, (float)screenHeight/2.0f };
        }
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (state == MENU) {
            backgroundScroll += 0.5f;
        }
        else if (state == DUEL_SETUP || state == BR_SETUP) {
            if (state == BR_SETUP) {
                 if (IsKeyPressed(KEY_LEFT)) { game->brPlayerCount--; if (game->brPlayerCount < 2) game->brPlayerCount = 2; }
                 if (IsKeyPressed(KEY_RIGHT)) { game->brPlayerCount++; if (game->brPlayerCount > MAX_PLAYERS) game->brPlayerCount = MAX_PLAYERS; }
            }
            int maxP = (state == DUEL_SETUP) ? 2 : game->brPlayerCount;

            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125)) {
                    int len = strlen(game->playerNames[game->currentInput]);
                    if (len < 19) { game->playerNames[game->currentInput][len] = (char)key; game->playerNames[game->currentInput][len+1] = '\0'; }
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = strlen(game->playerNames[game->currentInput]); if (len > 0) game->playerNames[game->currentInput][len-1] = '\0';
            }
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ENTER)) {
                game->currentInput++; if (game->currentInput > maxP) game->currentInput = 1;
            }
            if (game->currentInput > maxP) game->currentInput = maxP;
        }
        else if (state == DUEL_RUN || state == BR_RUN || state == SATISFYING_RUN) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.offset = GetMousePosition(); camera.target = mousePos;
                camera.zoom += (wheel * 0.125f); if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f/camera.zoom);
                camera.target = Vector2Add(camera.target, delta);
            }

             if (IsKeyPressed(KEY_SPACE)) game->isPaused = !game->isPaused;

             if (game->isPaused) {
                 // CORRECTION WARNING : Cast et .0f
                 Rectangle btnLancer = { (float)screenWidth/2.0f - 150.0f, (float)screenHeight/2.0f - 30.0f, 300.0f, 60.0f };
                 if (CheckCollisionPointRec(GetMousePosition(), btnLancer) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) game->isPaused = false;
             }

             if (!game->isPaused) {
                 float dt = GetFrameTime();
                 game->battleTimer += dt;

                 if (state == BR_RUN) {
                     game->zoneRadius -= (ZONE_SHRINK_SPEED * dt);
                     if (game->zoneRadius < 0) game->zoneRadius = 0;
                 }
                 if (state == DUEL_RUN && game->battleTimer >= DUEL_TIME_LIMIT) state = DUEL_END;

                 // --- VITESSE ---
                 float updateTarget = 0.05f;
                 if (state == SATISFYING_RUN) updateTarget = 0.08f;

                 game->timeAccumulator += dt;
                 if (game->timeAccumulator >= updateTarget) {
                    if (state == DUEL_RUN) {
                        update_duel_simple(game);
                    } else if (state == SATISFYING_RUN) {
                        update_game_classic(game);
                    } else {
                        int survivors = update_game_battle(game, game->brPlayerCount, (state == BR_RUN));
                        if (state == BR_RUN && survivors <= 1 && game->battleTimer > 2.0f) state = BR_END;
                    }
                    game->timeAccumulator = 0.0f;
                 }
             }
        }
        else if (state == DUEL_END || state == BR_END) {
             if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) state = MENU;
        }
        else if (state == GAME) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.offset = GetMousePosition(); camera.target = mousePos;
                camera.zoom += (wheel * 0.125f); if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f/camera.zoom);
                camera.target = Vector2Add(camera.target, delta);
            }
            if (IsKeyPressed(KEY_SPACE)) game->isPaused = !game->isPaused;
            if (IsKeyPressed(KEY_ESCAPE)) state = MENU;
            if (IsKeyPressed(KEY_C)) { selectedPattern = 0; toolName = "Crayon"; }
            if (IsKeyPressed(KEY_I)) { selectedPattern = 1; toolName = "Glider"; }
            if (IsKeyPressed(KEY_O)) { selectedPattern = 2; toolName = "Clown"; }
            if (IsKeyPressed(KEY_P)) { selectedPattern = 3; toolName = "Canon Gosper"; }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && selectedPattern == 0)) {
                Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
                int gx = mw.x/CELL_SIZE; int gy = mw.y/CELL_SIZE;
                if(gx>=0 && gx<game->cols && gy>=0 && gy<game->rows) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                         if(selectedPattern==1) apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_GLIDER_SE, 1);
                         else if(selectedPattern==2) apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_CLOWN, 1);
                         else if(selectedPattern==3) apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_GOSPER, 1);
                         else { int idx=gy*game->cols+gx; game->grid[idx]=!game->grid[idx]; game->ageGrid[idx]=game->grid[idx]?1:0; }
                    } else if(selectedPattern==0) { int idx=gy*game->cols+gx; game->grid[idx]=1; game->ageGrid[idx]=1; }
                }
            }
            if (game->isPaused && (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))) undo_state(game);
            if (game->isPaused && (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT))) update_game_classic(game);
            if (!game->isPaused) {
                game->timeAccumulator+=GetFrameTime();
                if(game->timeAccumulator>=game->updateSpeed){ update_game_classic(game); game->timeAccumulator=0.0f;}
            }
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));

        // Fond style menu pour le jeu
        if (state == DUEL_RUN || state == BR_RUN || state == MENU) {
             for(int i=0; i<screenWidth/40+1; i++) DrawLine(i*40+(int)backgroundScroll%40, 0, i*40+(int)backgroundScroll%40, screenHeight, Fade(WHITE, 0.05f));
        }

        // --- NOUVEAU : FOND ARTISTIQUE ANIMÉ POUR LE MODE DESSIN ---
        if (state == SATISFYING_SETUP || state == SATISFYING_RUN) {
            DrawRectangleGradientV(0, 0, screenWidth, screenHeight, (Color){10, 10, 30, 255}, (Color){40, 10, 60, 255});
            float time = (float)GetTime();
            for(int i = 0; i < 15; i++) {
                float offset = (float)i * 123.0f;
                float speed = 0.3f + (float)(i % 3) * 0.1f;
                float x = (float)screenWidth/2.0f + sinf(time * speed + offset) * (float)(screenWidth * 0.4f);
                float y = (float)screenHeight/2.0f + cosf(time * speed * 0.8f + offset) * (float)(screenHeight * 0.4f);
                float radius = 100.0f + sinf(time + offset) * 50.0f;
                float hue = fmodf(time * 10.0f + (float)i * 20.0f, 360.0f);
                Color orbColor = ColorFromHSV(hue, 0.6f, 0.8f);
                orbColor.a = 30;
                DrawCircleGradient((int)x, (int)y, radius, orbColor, Fade(orbColor, 0.0f));
            }
        }
        // -----------------------------------------------------------

        if (state == MENU) {
            int cx = screenWidth/2; int cy = screenHeight/2;
            DrawText("LE JEU DE LA VIE", cx - MeasureText("LE JEU DE LA VIE", 60)/2, cy - 250, 60, SKYBLUE);

            // 1. DUEL (Pos: -140)
            Rectangle rD = { (float)cx - 175.0f, (float)cy - 140.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rD) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->currentInput = 1; state = DUEL_SETUP;
            }
            DrawRectangleRec(rD, CheckCollisionPointRec(GetMousePosition(), rD)?RED:DARKGRAY);
            const char* txtDuel = "MODE DUEL";
            DrawText(txtDuel, (int)(rD.x + (rD.width - MeasureText(txtDuel, 20))/2), (int)rD.y+15, 20, WHITE);

            // 2. BATTLE ROYALE (Pos: -80)
            Rectangle rB = { (float)cx - 175.0f, (float)cy - 80.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rB) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->currentInput = 1; state = BR_SETUP; game->brPlayerCount = 4;
            }
            DrawRectangleRec(rB, CheckCollisionPointRec(GetMousePosition(), rB)?ORANGE:DARKGRAY);
            DrawText("BATTLE ROYALE", (int)rB.x+100, (int)rB.y+15, 20, WHITE);

            // 3. MODE DESSIN (Pos: -20)
            Rectangle rOP = { (float)cx - 175.0f, (float)cy - 20.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rOP) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                state = SATISFYING_SETUP;
            }
            DrawRectangleRec(rOP, CheckCollisionPointRec(GetMousePosition(), rOP)?PURPLE:DARKGRAY);
            DrawText("MODE DESSIN", (int)rOP.x + 110, (int)rOP.y+15, 20, WHITE);

            // 4. REGLES DU JEU (NOUVEAU - Pos: +40)
            Rectangle rRules = { (float)cx - 175.0f, (float)cy + 40.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rRules) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                state = RULES;
            }
            DrawRectangleRec(rRules, CheckCollisionPointRec(GetMousePosition(), rRules)?SKYBLUE:DARKGRAY);
            DrawText("REGLES DU JEU", (int)rRules.x + 105, (int)rRules.y+15, 20, CheckCollisionPointRec(GetMousePosition(), rRules)?BLACK:WHITE);

            // 5. SOLO EDITEUR (Pos: +100)
            Rectangle r1 = { (float)cx - 175.0f, (float)cy + 100.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), r1) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { clear_grid(game); state = GAME; }
            DrawRectangleRec(r1, CheckCollisionPointRec(GetMousePosition(), r1)?RAYWHITE:DARKGRAY);
            DrawText("MODE SOLO (EDITEUR)", (int)r1.x+60, (int)r1.y+15, 20, CheckCollisionPointRec(GetMousePosition(), r1)?BLACK:WHITE);

            // 6. SOLO ALEATOIRE (Pos: +160)
            Rectangle r2 = { (float)cx - 175.0f, (float)cy + 160.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), r2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { random_fill_classic(game); state = GAME; }
            DrawRectangleRec(r2, CheckCollisionPointRec(GetMousePosition(), r2)?RAYWHITE:DARKGRAY);
            DrawText("MODE SOLO (ALEATOIRE)", (int)r2.x+50, (int)r2.y+15, 20, CheckCollisionPointRec(GetMousePosition(), r2)?BLACK:WHITE);

            // 7. QUITTER (Pos: +220)
            Rectangle rQ = { (float)cx - 175.0f, (float)cy + 220.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rQ) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) CloseWindow();
            DrawRectangleRec(rQ, CheckCollisionPointRec(GetMousePosition(), rQ)?RED:DARKGRAY);
            DrawText("QUITTER", (int)rQ.x+130, (int)rQ.y+15, 20, WHITE);
        }
        else if (state == RULES) {
            // --- ECRAN DES REGLES DU JEU ---
            draw_home_button(&state, game);
            int cx = screenWidth/2;
            int cy = screenHeight/2;

            DrawText("LES 4 REGLES DE CONWAY", cx - MeasureText("LES 4 REGLES DE CONWAY", 40)/2, 50, 40, GOLD);

            float time = (float)GetTime();
            int boxH = 120;

            // Recalculer startY pour centrer verticalement les 4 boîtes
            int totalHeight = 4 * (boxH + 20);
            int startY = (screenHeight - totalHeight) / 2 + 40;

            // Regle 1 : Sous-population
            DrawRectangle(cx - 300, startY, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("1. SOUS-POPULATION", cx - 280, startY + 20, 20, ORANGE);
            DrawText("Une cellule vivante avec moins de 2 voisines meurt.", cx - 280, startY + 50, 18, WHITE);
            // Anim: Cellule qui disparait
            float fade1 = (sinf(time * 3.0f) + 1.0f) * 0.5f;
            DrawRectangle(cx + 200, startY + 40, 20, 20, Fade(WHITE, fade1)); // Cellule centrale
            DrawRectangle(cx + 200 + 22, startY + 40, 20, 20, WHITE); // Voisine unique
            DrawText(fade1 > 0.5f ? "VIVANT" : "MORT", cx + 200, startY + 70, 10, fade1 > 0.5f ? GREEN : RED);

            // Regle 2 : Survie
            int y2 = startY + boxH + 20;
            DrawRectangle(cx - 300, y2, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("2. SURVIE", cx - 280, y2 + 20, 20, GREEN);
            DrawText("Une cellule avec 2 ou 3 voisines reste vivante.", cx - 280, y2 + 50, 18, WHITE);
            // Anim: Stable
            DrawRectangle(cx + 200, y2 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 + 22, y2 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 - 22, y2 + 40, 20, 20, WHITE);
            DrawText("STABLE", cx + 200, y2 + 70, 10, GREEN);

            // Regle 3 : Surpopulation
            int y3 = y2 + boxH + 20;
            DrawRectangle(cx - 300, y3, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("3. SURPOPULATION", cx - 280, y3 + 20, 20, RED);
            DrawText("Une cellule avec plus de 3 voisines meurt.", cx - 280, y3 + 50, 18, WHITE);
            // Anim: Devient rouge puis noir
            float fade3 = (sinf(time * 3.0f) + 1.0f) * 0.5f;
            DrawRectangle(cx + 200, y3 + 40, 20, 20, fade3 > 0.5f ? RED : BLACK); // Meurt
            DrawRectangle(cx + 200 + 22, y3 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 - 22, y3 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200, y3 + 40 - 22, 20, 20, WHITE);
            DrawRectangle(cx + 200, y3 + 40 + 22, 20, 20, WHITE);

            // Regle 4 : Reproduction
            int y4 = y3 + boxH + 20;
            DrawRectangle(cx - 300, y4, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("4. NAISSANCE", cx - 280, y4 + 20, 20, SKYBLUE);
            DrawText("Une cellule morte avec exactement 3 voisines nait.", cx - 280, y4 + 50, 18, WHITE);
            // Anim: Apparition
            float fade4 = (sinf(time * 3.0f - 1.5f) + 1.0f) * 0.5f;
            DrawRectangleLines(cx + 200, y4 + 40, 20, 20, GRAY); // Case vide
            if (fade4 > 0.3f) DrawRectangle(cx + 200, y4 + 40, 20, 20, Fade(SKYBLUE, fade4));
            DrawRectangle(cx + 200 + 22, y4 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 - 22, y4 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200, y4 + 40 - 22, 20, 20, WHITE);

        }
        else if (state == SATISFYING_SETUP) {
            draw_home_button(&state, game);
            int cx = screenWidth/2; int cy = screenHeight/2;

            DrawText("CHOISISSEZ VOTRE MOTIF", cx - MeasureText("CHOISISSEZ VOTRE MOTIF", 30)/2, cy - 150, 30, WHITE);

            // CORRECTION WARNING : Rectangle avec float
            Rectangle rLeft = { (float)cx - 200.0f, (float)cy - 30.0f, 50.0f, 60.0f };
            Rectangle rRight = { (float)cx + 150.0f, (float)cy - 30.0f, 50.0f, 60.0f };

            if (CheckCollisionPointRec(GetMousePosition(), rLeft) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->satisfyingType--; if (game->satisfyingType < 0) game->satisfyingType = SATISFYING_COUNT - 1;
            }
            if (CheckCollisionPointRec(GetMousePosition(), rRight) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->satisfyingType++; if (game->satisfyingType >= SATISFYING_COUNT) game->satisfyingType = 0;
            }

            DrawRectangleRec(rLeft, CheckCollisionPointRec(GetMousePosition(), rLeft)?LIGHTGRAY:GRAY);
            DrawText("<", (int)rLeft.x + 15, (int)rLeft.y + 15, 30, WHITE);

            DrawRectangleRec(rRight, CheckCollisionPointRec(GetMousePosition(), rRight)?LIGHTGRAY:GRAY);
            DrawText(">", (int)rRight.x + 15, (int)rRight.y + 15, 30, WHITE);

            const char* currentModelName = SATISFYING_NAMES[game->satisfyingType];
            DrawText(currentModelName, cx - MeasureText(currentModelName, 40)/2, cy - 20, 40, PURPLE);

            Rectangle btnLancer = { (float)cx - 100.0f, (float)cy + 80.0f, 200.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), btnLancer) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                setup_satisfying(game);
                state = SATISFYING_RUN;
            }
            DrawRectangleRec(btnLancer, GREEN);
            DrawText("LANCER", (int)btnLancer.x + 60, (int)btnLancer.y + 15, 20, BLACK);
        }
        else if (state == DUEL_SETUP || state == BR_SETUP) {

            int cx = screenWidth/2; int cy = screenHeight/2;
            int maxP = (state == DUEL_SETUP) ? 2 : game->brPlayerCount;

            // --- FOND D'ECRAN DUEL & BR ---
            if (state == DUEL_SETUP) {
                DrawTexturePro(duelBackground, (Rectangle){0, 0, (float)duelBackground.width, (float)duelBackground.height}, (Rectangle){0, 0, (float)screenWidth, (float)screenHeight}, (Vector2){0, 0}, 0.0f, WHITE);
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.3f)); // Filtre léger
            } else {
                // ANIMATION PROCEDURALE BATTLE ROYALE "ZONE DE GUERRE"
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){20, 20, 40, 255}); // Fond sombre
                float time = (float)GetTime();
                for(int i=0; i<50; i++) {
                    float speed = 1000.0f + (float)(i*50);
                    float offset = (float)i * 1000.0f;
                    float x = fmodf(time * speed + offset, (float)screenWidth + 200.0f) - 100.0f;
                    float y = (float)((i * 30) % screenHeight);
                    DrawRectangle((int)x, (int)y, 40, 4, Fade(ORANGE, 0.7f)); // Traceurs
                }
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.3f)); // Filtre
            }

            // Bouton Home APRES le fond pour être visible
            draw_home_button(&state, game);

            const char* title = "CONFIGURATION";
            if(state==DUEL_SETUP) title="DUEL DE CELLULES";
            if(state==BR_SETUP) title="BATTLE ROYALE";

            DrawText(title, cx - MeasureText(title, 40)/2, cy - 250, 40, GOLD);

            if (state == BR_SETUP) {
                // Design BR simple centré
                DrawText("Nombre de Joueurs :", cx - 150, cy - 180, 20, WHITE);
                DrawText("<", cx + 50, cy - 180, 20, (game->brPlayerCount > 2) ? YELLOW : DARKGRAY);
                DrawText(TextFormat("%d", game->brPlayerCount), cx + 70, cy - 180, 20, YELLOW);
                DrawText(">", cx + 90, cy - 180, 20, (game->brPlayerCount < MAX_PLAYERS) ? YELLOW : DARKGRAY);

                int startY = cy - 120;
                DrawRectangle(cx - 250, startY - 10, 500, (maxP * 40) + 20, Fade(BLACK, 0.6f)); // Panneau de fond

                for(int i=1; i<=maxP; i++) {
                    int yPos = startY + ((i-1)*40);
                    Color pColor = GetPlayerColor(i);
                    DrawText(TextFormat("J%d:", i), cx - 220, yPos + 5, 20, pColor);
                    Rectangle rBox = { (float)cx - 150.0f, (float)yPos, 350.0f, 30.0f };
                    DrawRectangleRec(rBox, Fade(LIGHTGRAY, 0.8f));
                    if (game->currentInput == i) DrawRectangleLinesEx(rBox, 2, pColor);
                    DrawText(game->playerNames[i], (int)rBox.x + 5, (int)rBox.y + 5, 20, BLACK);
                    if (game->currentInput == i) DrawText("_", (int)rBox.x + 5 + MeasureText(game->playerNames[i], 20), (int)rBox.y + 5, 20, pColor);
                }
            }
            else {
                // DESIGN DUEL AMELIORE (Gauche vs Droite)
                int panelY = cy - 50;

                // Joueur 1 (Gauche - Bleu)
                DrawRectangle(cx - 400, panelY, 300, 100, Fade(BLUE, 0.2f));
                DrawRectangleLines(cx - 400, panelY, 300, 100, BLUE);
                DrawText("JOUEUR 1", cx - 400 + 10, panelY - 30, 20, BLUE);

                Rectangle rBox1 = { (float)cx - 380.0f, (float)panelY + 35.0f, 260.0f, 30.0f };
                DrawRectangleRec(rBox1, Fade(LIGHTGRAY, 0.9f));
                if (game->currentInput == 1) DrawRectangleLinesEx(rBox1, 2, BLUE);
                DrawText(game->playerNames[1], (int)rBox1.x + 5, (int)rBox1.y + 5, 20, BLACK);
                if (game->currentInput == 1) DrawText("_", (int)rBox1.x + 5 + MeasureText(game->playerNames[1], 20), (int)rBox1.y + 5, 20, BLUE);

                // VS au milieu
                DrawText("VS", cx - MeasureText("VS", 60)/2, panelY + 20, 60, WHITE);

                // Joueur 2 (Droite - Rouge)
                DrawRectangle(cx + 100, panelY, 300, 100, Fade(RED, 0.2f));
                DrawRectangleLines(cx + 100, panelY, 300, 100, RED);
                DrawText("JOUEUR 2", cx + 100 + 10, panelY - 30, 20, RED);

                Rectangle rBox2 = { (float)cx + 120.0f, (float)panelY + 35.0f, 260.0f, 30.0f };
                DrawRectangleRec(rBox2, Fade(LIGHTGRAY, 0.9f));
                if (game->currentInput == 2) DrawRectangleLinesEx(rBox2, 2, RED);
                DrawText(game->playerNames[2], (int)rBox2.x + 5, (int)rBox2.y + 5, 20, BLACK);
                if (game->currentInput == 2) DrawText("_", (int)rBox2.x + 5 + MeasureText(game->playerNames[2], 20), (int)rBox2.y + 5, 20, RED);
            }

            Rectangle btnVal = { (float)cx - 100.0f, (float)cy + 150.0f, 200.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), btnVal) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (state == DUEL_SETUP) { setup_duel_random(game); state = DUEL_RUN; }
                else if (state == BR_SETUP) { setup_battleroyale(game); state = BR_RUN; }
                game->isPaused = true;
            }
            DrawRectangleRec(btnVal, GREEN); DrawText("LANCER", (int)btnVal.x + 60, (int)btnVal.y + 15, 20, BLACK);
        }
        else if (state == DUEL_RUN || state == BR_RUN || state == SATISFYING_RUN) {
            BeginMode2D(camera);
            if (state == BR_RUN) {
                float realRadius = game->zoneRadius * CELL_SIZE;
                // CORRECTION WARNING : Vector2 avec float
                Vector2 realCenter = { (float)(GRID_W*CELL_SIZE)/2.0f, (float)(GRID_H*CELL_SIZE)/2.0f };
                DrawCircleLines((int)realCenter.x, (int)realCenter.y, realRadius, Fade(RED, 0.5f));
            }
            for (int y=0; y<game->rows; y++) {
                for (int x=0; x<game->cols; x++) {
                    int idx = y*game->cols+x;
                    if (game->grid[idx] > 0) DrawRectangle(x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE-1, CELL_SIZE-1, get_cell_color(game->grid[idx], game->ageGrid[idx], state));
                }
            }
            EndMode2D();
            DrawRectangle(0, screenHeight - 100, screenWidth, 100, Fade(BLACK, 0.9f));
            draw_home_button(&state, game);

            if (state == DUEL_RUN) {
                float timeLeft = DUEL_TIME_LIMIT - game->battleTimer;
                if (timeLeft < 0) timeLeft = 0;
                DrawText(TextFormat("TEMPS: %.1f", timeLeft), screenWidth/2 - 50, 10, 30, WHITE);
            } else if (state == BR_RUN) {
                int survivors = 0; int maxP = game->brPlayerCount;
                for(int i=1; i<=maxP; i++) if(game->scores[i] > 0) survivors++;
                DrawText(TextFormat("SURVIVANTS: %d", survivors), screenWidth/2 - 80, 10, 30, WHITE);
            } else if (state == SATISFYING_RUN) {
                DrawText(SATISFYING_NAMES[game->satisfyingType], screenWidth/2 - MeasureText(SATISFYING_NAMES[game->satisfyingType], 30)/2, 10, 30, PURPLE);
                DrawText("Appuyez sur ESPACE pour lancer la dissolution", screenWidth/2 - 180, 45, 15, WHITE);
            }

            if (game->isPaused) {
                 // CORRECTION WARNING : Rectangle avec float
                 Rectangle btnLancer = { (float)screenWidth/2.0f - 150.0f, (float)screenHeight/2.0f - 30.0f, 300.0f, 60.0f };
                 DrawRectangleRec(btnLancer, LIME);
                 DrawText(game->generation==0 ? "COMMENCER" : "REPRENDRE", (int)btnLancer.x + 60, (int)btnLancer.y + 20, 20, BLACK);
            }
            int totalScore = 0;
            int maxP = (state == DUEL_RUN) ? 2 : game->brPlayerCount;
            for(int i=1; i<=maxP; i++) totalScore += game->scores[i];

            if (totalScore > 0 && state != SATISFYING_RUN) {
                int barX = 50; int barY = screenHeight - 50; int barW = screenWidth - 100; int barH = 30; int currentX = barX;
                for(int i=1; i<=maxP; i++) {
                    float pct = (float)game->scores[i] / (float)totalScore;
                    int w = (int)(pct * barW);
                    if (w > 0) {
                        DrawRectangle(currentX, barY, w, barH, GetPlayerColor(i));
                        if (w > 40) DrawText(TextFormat("%.0f", (float)game->scores[i]), currentX + 5, barY + 5, 20, BLACK);
                        currentX += w;
                    }
                }
                DrawRectangleLines(barX, barY, barW, barH, WHITE);
            }
        }
        else if (state == DUEL_END || state == BR_END) {
             draw_home_button(&state, game);
             int cx = screenWidth/2; int cy = screenHeight/2;
             int winner = 0; int maxScore = -1;
             int maxP = (state == DUEL_END) ? 2 : game->brPlayerCount;
             for(int i=1; i<=maxP; i++) { if (game->scores[i] > maxScore) { maxScore = game->scores[i]; winner = i; } }

             DrawText("FIN DE LA PARTIE", cx - 150, cy - 100, 40, GRAY);
             DrawText("VAINQUEUR :", cx - 100, cy - 40, 30, WHITE);

             DrawText(game->playerNames[winner], cx - MeasureText(game->playerNames[winner], 60)/2, cy + 20, 60, GetPlayerColor(winner));

             DrawText(TextFormat("SCORE: %d", maxScore), cx - 100, cy + 80, 30, GOLD);

             DrawText("Appuyez sur MAISON ou ENTREE", cx - 180, cy + 140, 20, GRAY);
        }
        else if (state == GAME) {
            BeginMode2D(camera);
            for (int y=0; y<game->rows; y++) {
                for (int x=0; x<game->cols; x++) {
                    int idx = y*game->cols+x;
                    if (game->grid[idx]==1) {
                        DrawRectangle(x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE-1, CELL_SIZE-1, get_cell_color(1, game->ageGrid[idx], state));
                    } else if(camera.zoom>0.5f) {
                        DrawRectangleLines(x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE, GetColor(0x333333FF));
                    }
                }
            }
            DrawRectangleLines(0, 0, game->cols*CELL_SIZE, game->rows*CELL_SIZE, BLUE);
            EndMode2D();
            DrawRectangle(0, 0, screenWidth, 80, Fade(BLACK, 0.85f));
            draw_home_button(&state, game);
            DrawText(TextFormat("Gen: %ld | Zoom: %.2f", game->generation, camera.zoom), 60, 10, 20, GREEN);
            if(game->isPaused) DrawText("PAUSE", 250, 10, 20, RED); else DrawText("PLAY", 250, 10, 20, GREEN);
            DrawText("Outil Actuel :", 10, 40, 20, WHITE);
            Color toolColor = YELLOW;
            if (selectedPattern == 1) toolColor = ORANGE;
            if (selectedPattern == 2) toolColor = PINK;
            if (selectedPattern == 3) toolColor = RED;
            DrawText(toolName, 150, 40, 20, toolColor);
            int keysX = screenWidth - 350;
            DrawText("CONTROLES :", keysX, 10, 10, GRAY);
            DrawText("[I/O/P] -> Outils", keysX, 25, 20, WHITE);
            DrawText("[<] Reculer  [>] Avancer", keysX, 50, 20, GREEN);
        }
        EndDrawing();
    }

    // Décharger la texture à la fin du programme
    UnloadTexture(duelBackground);

    free(game->grid); free(game->nextGrid); free(game->ageGrid);
    for(int i=0; i<HISTORY_SIZE; i++) { free(game->history[i]); free(game->historyAge[i]); }
    free(game); CloseWindow(); return 0;
}