#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include <raymath.h>

// =========================================================
// --- CONSTANTES ---
// =========================================================
#define MAX_PLAYERS 8
#define HISTORY_SIZE 200
#define DUEL_TIME_LIMIT 30.0f
#define ZONE_SHRINK_SPEED 2.5f

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
// --- STRUCTURES & CONFIG ---
// =========================================================

typedef enum {
    MENU, GAME,
    DUEL_SETUP, DUEL_RUN, DUEL_END,
    BR_SETUP, BR_RUN, BR_END,
    SATISFYING_RUN // Mode Fleur Géante
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

} GameOfLife;

Color GetPlayerColor(int id) {
    switch (id) {
        case 1: return RED;    // Joueur 1
        case 2: return GREEN;  // Joueur 2
        case 3: return BLUE;
        case 4: return ORANGE;
        case 5: return PURPLE; // Couleur du mode Satisfaisant
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

// Mise à jour classique (Mode Solo & Satisfaisant)
void update_game_classic(GameOfLife *game) {
    save_state(game);
    game->generation++;
    int offsets[8] = {-1-game->cols, -game->cols, 1-game->cols, -1, 1, -1+game->cols, game->cols, 1+game->cols};

    // Détection auto : si on trouve du violet (5) au centre, on considère que c'est le mode fleur
    // Cela permet de garder les couleurs cohérentes
    int centerVal = game->grid[game->cols/2 + (game->rows/2)*game->cols];
    int spawnColor = (centerVal == 5) ? 5 : 1;
    if (game->generation < 5 && centerVal == 0) spawnColor = 5; // Force violet au début du mode fleur

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
                // Si une cellule nait, elle prend la couleur dominante des voisins ou par défaut 1 (ou 5)
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

// =========================================================
// --- UPDATE DUEL ---
// =========================================================
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
                // Survie
                if (total == 2 || total == 3) game->nextGrid[idx] = game->grid[idx];
                else game->nextGrid[idx] = 0;
            } else {
                // Naissance : La couleur majoritaire l'emporte
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

// Mise à jour Battle Royale
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

void clear_grid(GameOfLife *game) {
    int size = game->cols * game->rows;
    memset(game->grid, 0, size * sizeof(int));
    memset(game->ageGrid, 0, size * sizeof(int));
    game->historyCount = 0; game->generation = 0;
}

// =========================================================
// --- SETUP NOUVEAU DUEL (ALÉATOIRE) ---
// =========================================================
void setup_duel_random(GameOfLife *game) {
    clear_grid(game);
    int totalCells = game->cols * game->rows;
    for (int i = 0; i < totalCells; i++) {
        int r = rand() % 100;
        // 20% de remplissage global, réparti équitablement
        if (r < 10) game->grid[i] = 1;      // Joueur 1
        else if (r < 20) game->grid[i] = 2; // Joueur 2
        else game->grid[i] = 0;             // Vide

        if (game->grid[i] != 0) game->ageGrid[i] = 1;
    }
    game->battleTimer = 0.0f; game->generation = 0;
}

// =========================================================
// --- SETUP MODE SATISFAISANT (FLEUR GÉANTE) ---
// =========================================================
void setup_satisfying(GameOfLife *game) {
    clear_grid(game);
    int cx = game->cols / 2;
    int cy = game->rows / 2;

    // Rayon max basé sur la taille de la grille (un peu moins de la moitié de la hauteur)
    float maxRadius = (game->rows < game->cols ? game->rows : game->cols) * 0.45f;

    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            float dx = (float)(x - cx);
            float dy = (float)(y - cy);
            float dist = sqrtf(dx*dx + dy*dy);
            float angle = atan2f(dy, dx);

            // --- MATHÉMATIQUES DE LA FLEUR (ROSE POLAIRE) ---
            // r = cos(k * theta). Ici k=6 pour faire 12 pétales, c'est joli.
            float shape = fabs(cosf(6.0f * angle));

            // On calcule le rayon limite à cet angle précis
            // 0.3f = base du cercle (coeur), 0.7f = longueur des pétales
            float limit = maxRadius * (0.3f + 0.7f * shape);

            if (dist <= limit) {
                // --- L'ASTUCE "DISPARAÎT AU FUR ET À MESURE" ---
                // Au lieu de remplir tout en solide (ce qui tue l'intérieur trop vite),
                // on remplit en DAMIER (Checkerboard).
                // Cela crée une structure "instable mais vivante" qui s'effrite joliment.
                if ((x + y) % 2 == 0) {
                    int idx = y * game->cols + x;
                    game->grid[idx] = 5; // Couleur Violette
                    game->ageGrid[idx] = 1;
                }
            }
        }
    }

    game->generation = 0;
    // On met en pause au début pour laisser l'utilisateur voir la forme parfaite
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
    // En mode solo ou satisfaisant, on fait un dégradé
    if (state == GAME || state == SATISFYING_RUN) {
        if (val == 5) { // Mode Satisfaisant (Violet -> Rose -> Blanc)
            float factor = (float)age / 40.0f; if (factor > 1.0f) factor = 1.0f;
            // Un beau dégradé Violet vers Rose néon
            return (Color){
                140 + (unsigned char)(115*factor), // R augmente
                40 + (unsigned char)(100*factor),  // G augmente un peu
                255,                               // B reste fort
                255
            };
        }
        // Mode Classic (Blanc -> Cyan)
        float factor = (float)age / 50.0f; if (factor > 1.0f) factor = 1.0f;
        return (Color){ 255 - (unsigned char)(25*factor), 255 - (unsigned char)(214*factor), 255 - (unsigned char)(200*factor), 255 };
    }
    // Modes Compétitifs (Couleurs franches)
    return GetPlayerColor(val);
}

void draw_home_button(AppState *state, GameOfLife *game) {
    Rectangle btnHome = { 10, 10, 40, 40 };
    DrawRectangleRec(btnHome, LIGHTGRAY);
    DrawRectangleLinesEx(btnHome, 2, DARKGRAY);
    DrawTriangle((Vector2){10, 30}, (Vector2){50, 30}, (Vector2){30, 10}, DARKGRAY);
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
        else if (state == DUEL_SETUP || state == BR_SETUP) {
            // Choix nombre joueurs (sauf Duel = 2 fixe)
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
                 Rectangle btnLancer = { screenWidth/2 - 150, screenHeight/2 - 30, 300, 60 };
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

                 float updateTarget = 0.05f;
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
            // ... (Code Solo inchangé) ...
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

        if (state == MENU) {
            for(int i=0; i<screenWidth/40+1; i++) DrawLine(i*40+(int)backgroundScroll%40, 0, i*40+(int)backgroundScroll%40, screenHeight, Fade(WHITE, 0.05f));
            int cx = screenWidth/2; int cy = screenHeight/2;
            DrawText("LE JEU DE LA VIE", cx - MeasureText("LE JEU DE LA VIE", 60)/2, cy - 250, 60, SKYBLUE);

            Rectangle rD = { cx-175, cy-120, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), rD) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->currentInput = 1; state = DUEL_SETUP;
            }
            DrawRectangleRec(rD, CheckCollisionPointRec(GetMousePosition(), rD)?RED:DARKGRAY);
            DrawText("DUEL ALEATOIRE (30s)", rD.x+40, rD.y+15, 20, WHITE);

            Rectangle rB = { cx-175, cy-60, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), rB) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                game->currentInput = 1; state = BR_SETUP; game->brPlayerCount = 4;
            }
            DrawRectangleRec(rB, CheckCollisionPointRec(GetMousePosition(), rB)?ORANGE:DARKGRAY);
            DrawText("BATTLE ROYALE", rB.x+100, rB.y+15, 20, BLACK);

            // --- BOUTON SATISFAISANT (FLEUR GEANTE) ---
            Rectangle rOP = { cx-175, cy, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), rOP) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                setup_satisfying(game); state = SATISFYING_RUN;
            }
            DrawRectangleRec(rOP, CheckCollisionPointRec(GetMousePosition(), rOP)?PURPLE:DARKGRAY);
            DrawText("MODE SATISFAISANT", rOP.x+80, rOP.y+15, 20, WHITE);

            Rectangle r1 = { cx-175, cy+60, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), r1) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { clear_grid(game); state = GAME; }
            DrawRectangleRec(r1, CheckCollisionPointRec(GetMousePosition(), r1)?RAYWHITE:DARKGRAY);
            DrawText("Mode Solo (Editeur)", r1.x+80, r1.y+15, 20, BLACK);

            Rectangle r2 = { cx-175, cy+120, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), r2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { random_fill_classic(game); state = GAME; }
            DrawRectangleRec(r2, CheckCollisionPointRec(GetMousePosition(), r2)?RAYWHITE:DARKGRAY);
            DrawText("Mode Solo (Aleatoire)", r2.x+70, r2.y+15, 20, BLACK);

            Rectangle rQ = { cx-175, cy+180, 350, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), rQ) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) CloseWindow();
            DrawRectangleRec(rQ, CheckCollisionPointRec(GetMousePosition(), rQ)?RED:DARKGRAY); DrawText("Quitter", rQ.x+130, rQ.y+15, 20, WHITE);
        }
        else if (state == DUEL_SETUP || state == BR_SETUP) {
            draw_home_button(&state, game);
            int cx = screenWidth/2; int cy = screenHeight/2;
            int maxP = (state == DUEL_SETUP) ? 2 : game->brPlayerCount;

            const char* title = "CONFIGURATION";
            if(state==DUEL_SETUP) title="DUEL DE CELLULES";
            if(state==BR_SETUP) title="BATTLE ROYALE";

            DrawText(title, cx - MeasureText(title, 30)/2, cy - 250, 30, GOLD);

            if (state == BR_SETUP) {
                DrawText("Nombre de Joueurs :", cx - 150, cy - 200, 20, WHITE);
                DrawText("<", cx + 50, cy - 200, 20, (game->brPlayerCount > 2) ? YELLOW : DARKGRAY);
                DrawText(TextFormat("%d", game->brPlayerCount), cx + 70, cy - 200, 20, YELLOW);
                DrawText(">", cx + 90, cy - 200, 20, (game->brPlayerCount < MAX_PLAYERS) ? YELLOW : DARKGRAY);
                DrawText("(Max 8)", cx + 120, cy - 200, 15, GRAY);
            }

            int startY = cy - 150;
            for(int i=1; i<=maxP; i++) {
                int yPos = startY + ((i-1)*50);
                Color pColor = GetPlayerColor(i);
                DrawText(TextFormat("J%d:", i), cx - 300, yPos, 20, pColor);
                Rectangle rBox = { cx - 220, yPos - 5, 300, 30 };
                DrawRectangleRec(rBox, LIGHTGRAY);
                if (game->currentInput == i) DrawRectangleLinesEx(rBox, 2, pColor);
                DrawText(game->playerNames[i], rBox.x + 5, rBox.y + 5, 20, BLACK);
                if (game->currentInput == i) DrawText("_", rBox.x + 5 + MeasureText(game->playerNames[i], 20), rBox.y + 5, 20, pColor);
            }
            Rectangle btnVal = { cx - 100, cy + 220, 200, 50 };
            if (CheckCollisionPointRec(GetMousePosition(), btnVal) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (state == DUEL_SETUP) { setup_duel_random(game); state = DUEL_RUN; }
                else if (state == BR_SETUP) { setup_battleroyale(game); state = BR_RUN; }
                game->isPaused = true;
            }
            DrawRectangleRec(btnVal, GREEN); DrawText("LANCER", btnVal.x + 60, btnVal.y + 15, 20, BLACK);
        }
        else if (state == DUEL_RUN || state == BR_RUN || state == SATISFYING_RUN) {
            BeginMode2D(camera);
            if (state == BR_RUN) {
                float realRadius = game->zoneRadius * CELL_SIZE;
                Vector2 realCenter = { (GRID_W*CELL_SIZE)/2.0f, (GRID_H*CELL_SIZE)/2.0f };
                DrawCircleLines(realCenter.x, realCenter.y, realRadius, Fade(RED, 0.5f));
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
                DrawText("FLEUR GEANTE", screenWidth/2 - 100, 10, 30, PURPLE);
                DrawText("Appuyez sur ESPACE pour lancer la dissolution", screenWidth/2 - 180, 45, 15, WHITE);
            }

            if (game->isPaused) {
                 Rectangle btnLancer = { screenWidth/2 - 150, screenHeight/2 - 30, 300, 60 };
                 DrawRectangleRec(btnLancer, LIME);
                 DrawText(game->generation==0 ? "COMMENCER" : "REPRENDRE", btnLancer.x + 60, btnLancer.y + 20, 20, BLACK);
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
    free(game->grid); free(game->nextGrid); free(game->ageGrid);
    for(int i=0; i<HISTORY_SIZE; i++) { free(game->history[i]); free(game->historyAge[i]); }
    free(game); CloseWindow(); return 0;
}