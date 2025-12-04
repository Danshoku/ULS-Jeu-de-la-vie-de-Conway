#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include <raymath.h>

// =========================================================
// --- DEFINITIONS DES MOTIFS ---
// =========================================================
typedef struct { int w, h; int *cells; } Pattern;

// 1. GLIDER
int glider_data[] = { 0,1,0, 0,0,1, 1,1,1 };
Pattern PATTERN_GLIDER = {3, 3, glider_data};

// 2. CLOWN
int clown_data[] = { 1,1,1, 1,0,1, 1,0,1 };
Pattern PATTERN_CLOWN = {3, 3, clown_data};

// 3. LE "CHEVAL" (R-Pentomino)
int horse_data[] = {
    0, 1, 1,
    1, 1, 0,
    0, 1, 0
};
Pattern PATTERN_HORSE = {3, 3, horse_data};

// 4. DIEHARD
int diehard_data[] = {
    0,0,0,0,0,0,1,0,
    1,1,0,0,0,0,0,0,
    0,1,0,0,0,1,1,1
};
Pattern PATTERN_DIEHARD = {8, 3, diehard_data};

// 5. CANON DE GOSPER
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
                if (p.cells[py * p.w + px] == 1) {
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
                if (p.cells[py * p.w + px] == 1) {
                    int idx = targetY * cols + targetX;
                    grid[idx] = playerVal;
                    ageGrid[idx] = 1;
                }
            }
        }
    }
}

// =========================================================
// --- CONFIGURATION ---
// =========================================================

#define HISTORY_SIZE 200
#define DUEL_TIME_LIMIT 30.0f // 30 secondes strictes pour le duel
#define ZONE_SHRINK_SPEED 2.5f

typedef enum { MENU, GAME, DUEL_SETUP, DUEL_RUN, DUEL_END, BR_SETUP, BR_RUN, BR_END } AppState;

typedef struct {
    int rows; int cols;
    int *grid; int *nextGrid; int *ageGrid;

    int *history[HISTORY_SIZE];
    int *historyAge[HISTORY_SIZE];
    int historyHead; int historyCount;

    long generation;
    bool isPaused;
    float timeAccumulator; float updateSpeed;

    char playerNames[5][20];
    int scores[5];
    int currentInput;
    float battleTimer;
    float zoneRadius;
} GameOfLife;

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

    strcpy(game->playerNames[1], "Rouge");
    strcpy(game->playerNames[2], "Bleu");
    strcpy(game->playerNames[3], "Vert");
    strcpy(game->playerNames[4], "Jaune");

    game->currentInput = 1;
    game->battleTimer = 0.0f;
    game->zoneRadius = (float)cols;

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

void update_game_classic(GameOfLife *game) {
    save_state(game);
    game->generation++;
    int offsets[8] = {-1-game->cols, -game->cols, 1-game->cols, -1, 1, -1+game->cols, game->cols, 1+game->cols};
    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            int idx = y * game->cols + x;
            int count = 0;
            if (x > 0 && x < game->cols - 1 && y > 0 && y < game->rows - 1) {
                for (int k = 0; k < 8; k++) if (game->grid[idx + offsets[k]] > 0) count++;
            }
            if (game->grid[idx] > 0) {
                if (count == 2 || count == 3) { game->nextGrid[idx] = 1; game->ageGrid[idx]++; }
                else { game->nextGrid[idx] = 0; game->ageGrid[idx] = 0; }
            } else {
                if (count == 3) { game->nextGrid[idx] = 1; game->ageGrid[idx] = 1; }
                else { game->nextGrid[idx] = 0; game->ageGrid[idx] = 0; }
            }
        }
    }
    int *temp = game->grid; game->grid = game->nextGrid; game->nextGrid = temp;
}

// LOGIQUE BATAILLE (Avec ou sans zone selon le mode)
int update_game_battle(GameOfLife *game, int maxPlayers, bool useZone) {
    save_state(game);
    game->generation++;
    int offsets[8] = {-1-game->cols, -game->cols, 1-game->cols, -1, 1, -1+game->cols, game->cols, 1+game->cols};

    float cx = game->cols / 2.0f;
    float cy = game->rows / 2.0f;
    float radiusSq = game->zoneRadius * game->zoneRadius;

    for(int i=0; i<=4; i++) game->scores[i] = 0;

    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            int idx = y * game->cols + x;

            // --- VERIFICATION ZONE (Seulement si active) ---
            if (useZone) {
                float dx = x - cx; float dy = y - cy;
                if (dx*dx + dy*dy > radiusSq) { game->nextGrid[idx] = 0; continue; }
            }

            int counts[5] = {0, 0, 0, 0, 0};
            if (x > 0 && x < game->cols - 1 && y > 0 && y < game->rows - 1) {
                for (int k = 0; k < 8; k++) {
                    int val = game->grid[idx + offsets[k]];
                    if (val > 0 && val <= 4) counts[val]++;
                }
            }
            int total = counts[1] + counts[2] + counts[3] + counts[4];

            if (game->grid[idx] > 0) {
                if (total == 2 || total == 3) game->nextGrid[idx] = game->grid[idx];
                else game->nextGrid[idx] = 0;
            } else {
                if (total == 3) {
                    int maxC = 0; int winner = 0;
                    for(int p=1; p<=maxPlayers; p++) {
                        if (counts[p] > maxC) { maxC = counts[p]; winner = p; }
                        else if (counts[p] == maxC && maxC > 0) { if (rand()%2 == 0) winner = p; }
                    }
                    game->nextGrid[idx] = winner;
                } else game->nextGrid[idx] = 0;
            }
            if (game->nextGrid[idx] > 0) game->scores[game->nextGrid[idx]]++;
        }
    }
    int *temp = game->grid; game->grid = game->nextGrid; game->nextGrid = temp;
    int survivors = 0;
    for(int i=1; i<=maxPlayers; i++) { if(game->scores[i] > 0) survivors++; }
    return survivors;
}

void clear_grid(GameOfLife *game) {
    int size = game->cols * game->rows;
    memset(game->grid, 0, size * sizeof(int));
    memset(game->ageGrid, 0, size * sizeof(int));
    game->historyCount = 0; game->generation = 0;
}

void setup_duel(GameOfLife *game) {
    clear_grid(game);
    int totalCells = game->cols * game->rows;
    for (int i = 0; i < totalCells; i++) {
        int r = rand() % 100;
        if (r < 10) game->grid[i] = 1; else if (r < 20) game->grid[i] = 2;
        if (game->grid[i] > 0) game->ageGrid[i] = 1;
    }
    apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, 5, 10, PATTERN_GOSPER, 1);
    apply_pattern_flipped(game->grid, game->ageGrid, game->cols, game->rows, game->cols - 45, 10, PATTERN_GOSPER, 2);
    game->battleTimer = 0.0f; game->generation = 0;
    // Pas de zoneRadius en Duel
}

void setup_battleroyale(GameOfLife *game) {
    clear_grid(game);
    int w = game->cols; int h = game->rows;
    for (int i = 0; i < w * h; i++) {
        int r = rand() % 100;
        if (r < 8) game->grid[i] = 1; else if (r < 16) game->grid[i] = 2;
        else if (r < 24) game->grid[i] = 3; else if (r < 32) game->grid[i] = 4;
        if (game->grid[i] > 0) game->ageGrid[i] = 1;
    }
    apply_pattern(game->grid, game->ageGrid, w, h, 2, 2, PATTERN_GOSPER, 1);
    apply_pattern_flipped(game->grid, game->ageGrid, w, h, w - 40, 2, PATTERN_GOSPER, 2);
    apply_pattern(game->grid, game->ageGrid, w, h, 2, h - 15, PATTERN_GOSPER, 3);
    apply_pattern_flipped(game->grid, game->ageGrid, w, h, w - 40, h - 15, PATTERN_GOSPER, 4);
    apply_pattern(game->grid, game->ageGrid, w, h, w/2 - 20, h/2 - 20, PATTERN_GLIDER, 1);
    apply_pattern(game->grid, game->ageGrid, w, h, w/2 + 20, h/2 - 20, PATTERN_GLIDER, 2);
    apply_pattern(game->grid, game->ageGrid, w, h, w/2 - 20, h/2 + 20, PATTERN_GLIDER, 3);
    apply_pattern(game->grid, game->ageGrid, w, h, w/2 + 20, h/2 + 20, PATTERN_GLIDER, 4);
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
    if (state >= DUEL_SETUP) {
        if (val == 1) return RED; if (val == 2) return BLUE; if (val == 3) return GREEN; if (val == 4) return YELLOW;
    }
    if (age <= 0) return BLACK;
    if (age == 1) return RAYWHITE;
    float factor = (float)age / 50.0f; if (factor > 1.0f) factor = 1.0f;
    return (Color){ 255 - (unsigned char)(25*factor), 255 - (unsigned char)(214*factor), 255 - (unsigned char)(200*factor), 255 };
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
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.target = (Vector2){ (GRID_W*CELL_SIZE)/2.0f, (GRID_H*CELL_SIZE)/2.0f };

    AppState state = MENU;
    float backgroundScroll = 0.0f;
    int selectedPattern = 0;
    const char* toolName = "Crayon";

    while (!WindowShouldClose()) {
        if (IsWindowResized()) { screenWidth = GetScreenWidth(); screenHeight = GetScreenHeight(); camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f }; }
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (state == MENU) {
            backgroundScroll += 0.5f;
        }
        // Pas de galerie
        else if (state == DUEL_SETUP || state == BR_SETUP) {
            int maxP = (state == DUEL_SETUP) ? 2 : 4;
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125)) {
                    int len = strlen(game->playerNames[game->currentInput]);
                    if (len < 19) {
                        game->playerNames[game->currentInput][len] = (char)key;
                        game->playerNames[game->currentInput][len+1] = '\0';
                    }
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = strlen(game->playerNames[game->currentInput]);
                if (len > 0) game->playerNames[game->currentInput][len-1] = '\0';
            }
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ENTER)) {
                game->currentInput++; if (game->currentInput > maxP) game->currentInput = 1;
            }
        }
        else if (state == DUEL_RUN || state == BR_RUN) {
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
                 Rectangle btnLancer = { screenWidth/2 - 150, screenHeight/2 - 30, 300, 60 };
                 if (CheckCollisionPointRec(GetMousePosition(), btnLancer) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) game->isPaused = false;
             }

             if (!game->isPaused) {
                 float dt = GetFrameTime();
                 game->battleTimer += dt;

                 // Zone active seulement en Battle Royale
                 if (state == BR_RUN) {
                     game->zoneRadius -= (ZONE_SHRINK_SPEED * dt);
                     if (game->zoneRadius < 0) game->zoneRadius = 0;
                 }

                 // Conditions de fin
                 if (state == DUEL_RUN && game->battleTimer >= DUEL_TIME_LIMIT) {
                     state = DUEL_END;
                 }

                 game->timeAccumulator += dt;
                 if (game->timeAccumulator >= 0.05f) {
                    bool useZone = (state == BR_RUN);
                    int survivors = update_game_battle(game, (state == DUEL_RUN) ? 2 : 4, useZone);

                    // Fin anticipée pour BR (1 survivant)
                    if (state == BR_RUN && survivors <= 1 && game->battleTimer > 2.0f) {
                        state = BR_END;
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
                         if(selectedPattern==1) apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_GLIDER, 1);
                         else if(selectedPattern==2) apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_CLOWN, 1);
                         else if(selectedPattern==3) apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_GOSPER, 1);
                         else {
                             int idx=gy*game->cols+gx;
                             game->grid[idx]=!game->grid[idx];
                             game->ageGrid[idx]=game->grid[idx]?1:0;
                         }
                    } else if(selectedPattern==0) {
                        int idx=gy*game->cols+gx;
                        game->grid[idx]=1;
                        game->ageGrid[idx]=1;
                    }
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

        if (state == MENU) {
            for(int i=0; i<screenWidth/40+1; i++) DrawLine(i*40+(int)backgroundScroll%40, 0, i*40+(int)backgroundScroll%40, screenHeight, Fade(WHITE, 0.05f));
            int cx = screenWidth/2; int cy = screenHeight/2;
            DrawText("LE JEU DE LA VIE", cx - MeasureText("LE JEU DE LA VIE", 60)/2, cy - 250, 60, SKYBLUE);

            Rectangle rD = { cx-175, cy-120, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), rD) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->currentInput = 1; state = DUEL_SETUP;
            }
            DrawRectangleRec(rD, CheckCollisionPointRec(GetMousePosition(), rD)?GOLD:DARKGRAY);
            DrawText("DUEL (2 Joueurs)", rD.x+90, rD.y+15, 20, BLACK);

            Rectangle rB = { cx-175, cy-60, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), rB) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->currentInput = 1; state = BR_SETUP;
            }
            DrawRectangleRec(rB, CheckCollisionPointRec(GetMousePosition(), rB)?ORANGE:DARKGRAY);
            DrawText("BATTLE ROYALE (4 Joueurs)", rB.x+40, rB.y+15, 20, BLACK);

            Rectangle r1 = { cx-175, cy+10, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), r1) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { clear_grid(game); state = GAME; }
            DrawRectangleRec(r1, CheckCollisionPointRec(GetMousePosition(), r1)?RAYWHITE:DARKGRAY);
            DrawText("Mode Solo (Editeur)", r1.x+80, r1.y+15, 20, BLACK);

            Rectangle r2 = { cx-175, cy+70, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), r2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { random_fill_classic(game); state = GAME; }
            DrawRectangleRec(r2, CheckCollisionPointRec(GetMousePosition(), r2)?RAYWHITE:DARKGRAY);
            DrawText("Mode Solo (Aleatoire)", r2.x+70, r2.y+15, 20, BLACK);

            Rectangle rQ = { cx-175, cy+140, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), rQ) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) CloseWindow();
            DrawRectangleRec(rQ, CheckCollisionPointRec(GetMousePosition(), rQ)?RED:DARKGRAY); DrawText("Quitter", rQ.x+130, rQ.y+15, 20, WHITE);
        }
        // Pas de galerie draw code
        else if (state == DUEL_SETUP || state == BR_SETUP) {
            int cx = screenWidth/2; int cy = screenHeight/2;
            int maxP = (state == DUEL_SETUP) ? 2 : 4;
            DrawText(state==DUEL_SETUP?"CONFIGURATION DUEL":"CONFIGURATION BATTLE ROYALE", cx - 200, cy - 250, 30, GOLD);
            Color colors[] = { BLACK, RED, BLUE, GREEN, YELLOW };
            for(int i=1; i<=maxP; i++) {
                int yPos = cy - 150 + (i*60);
                DrawText(TextFormat("Joueur %d:", i), cx - 300, yPos, 20, colors[i]);
                Rectangle rBox = { cx - 180, yPos - 5, 400, 30 };
                DrawRectangleRec(rBox, LIGHTGRAY);
                if (game->currentInput == i) DrawRectangleLinesEx(rBox, 2, colors[i]);
                DrawText(game->playerNames[i], rBox.x + 5, rBox.y + 5, 20, BLACK);
                if (game->currentInput == i) DrawText("_", rBox.x + 5 + MeasureText(game->playerNames[i], 20), rBox.y + 5, 20, colors[i]);
            }
            Rectangle btnVal = { cx - 100, cy + 200, 200, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), btnVal) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (state == DUEL_SETUP) { setup_duel(game); state = DUEL_RUN; }
                else { setup_battleroyale(game); state = BR_RUN; }
                game->isPaused = true;
            }
            DrawRectangleRec(btnVal, GREEN); DrawText("LANCER", btnVal.x + 60, btnVal.y + 15, 20, BLACK);
        }
        else if (state == DUEL_RUN || state == BR_RUN) {
            BeginMode2D(camera);

            // Dessin de la zone (SEULEMENT EN BR)
            if (state == BR_RUN) {
                float realRadius = game->zoneRadius * CELL_SIZE;
                Vector2 realCenter = { (GRID_W*CELL_SIZE)/2.0f, (GRID_H*CELL_SIZE)/2.0f };
                DrawCircleLines(realCenter.x, realCenter.y, realRadius, Fade(RED, 0.5f));
            }

            for (int y=0; y<game->rows; y++) {
                for (int x=0; x<game->cols; x++) {
                    int idx = y*game->cols+x;
                    if (game->grid[idx] > 0) DrawRectangle(x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE-1, CELL_SIZE-1, get_cell_color(game->grid[idx], 0, state));
                }
            }
            EndMode2D();

            DrawRectangle(0, screenHeight - 100, screenWidth, 100, Fade(BLACK, 0.9f));

            // Affichage différent selon le mode
            if (state == DUEL_RUN) {
                float timeLeft = DUEL_TIME_LIMIT - game->battleTimer;
                if (timeLeft < 0) timeLeft = 0;
                DrawText(TextFormat("TEMPS: %.1f", timeLeft), screenWidth/2 - 50, 10, 30, WHITE);
            } else {
                int survivors = 0;
                for(int i=1; i<=4; i++) if(game->scores[i] > 0) survivors++;
                DrawText(TextFormat("SURVIVANTS: %d", survivors), screenWidth/2 - 80, 10, 30, WHITE);
            }

            if (game->isPaused) {
                 Rectangle btnLancer = { screenWidth/2 - 150, screenHeight/2 - 30, 300, 60 };
                 DrawRectangleRec(btnLancer, LIME);
                 DrawText(game->generation==0 ? "COMMENCER LE COMBAT" : "REPRENDRE", btnLancer.x + 30, btnLancer.y + 20, 20, BLACK);
            }

            // Barres de progression
            int totalCells = 0;
            int maxP = (state == DUEL_RUN) ? 2 : 4;
            for(int i=1; i<=maxP; i++) totalCells += game->scores[i];

            if (totalCells > 0) {
                int barX = 50; int barY = screenHeight - 50; int barW = screenWidth - 100; int barH = 30; int currentX = barX;
                Color colors[] = { BLACK, RED, BLUE, GREEN, YELLOW };
                for(int i=1; i<=maxP; i++) {
                    float pct = (float)game->scores[i] / (float)totalCells;
                    int w = (int)(pct * barW);
                    if (w > 0) {
                        DrawRectangle(currentX, barY, w, barH, colors[i]);
                        if (w > 50) DrawText(TextFormat("%s %.0f%%", game->playerNames[i], pct*100), currentX + 5, barY + 5, 20, (i==4||i==3)?BLACK:WHITE);
                        currentX += w;
                    }
                }
                DrawRectangleLines(barX, barY, barW, barH, WHITE);
            }
        }
        else if (state == DUEL_END || state == BR_END) {
             int cx = screenWidth/2; int cy = screenHeight/2;
             int maxScore = -1; int winner = 0; int maxP = (state == DUEL_END) ? 2 : 4;
             for(int i=1; i<=maxP; i++) { if (game->scores[i] > maxScore) { maxScore = game->scores[i]; winner = i; } }
             Color colors[] = { WHITE, RED, BLUE, GREEN, YELLOW };
             DrawText("FIN DE LA PARTIE", cx - 150, cy - 100, 40, GRAY);
             DrawText("VAINQUEUR :", cx - 100, cy - 40, 30, WHITE);
             DrawText(game->playerNames[winner], cx - MeasureText(game->playerNames[winner], 60)/2, cy + 20, 60, colors[winner]);
             DrawText("Appuyez sur ENTREE pour menu", cx - 150, cy + 100, 20, GRAY);
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
            DrawText(TextFormat("Gen: %ld | Zoom: %.2f", game->generation, camera.zoom), 10, 10, 20, GREEN);
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
    free(game->grid); free(game->nextGrid); free(game->ageGrid);
    for(int i=0; i<HISTORY_SIZE; i++) { free(game->history[i]); free(game->historyAge[i]); }
    free(game); CloseWindow(); return 0;
}