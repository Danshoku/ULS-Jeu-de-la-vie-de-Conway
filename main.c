#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "raylib.h"
#include <raymath.h>
#include "patterns.h"

#define HISTORY_SIZE 200 // Nombre d'étapes qu'on peut revenir en arrière

// États de l'application
typedef enum { MENU, GAME } AppState;

// Structure pour gérer tout l'état du jeu
typedef struct {
    int rows;
    int cols;
    int *grid;      // 0 ou 1
    int *nextGrid;  // Buffer pour le calcul
    int *ageGrid;   // Stocke l'âge des cellules pour la couleur

    // Historique (Buffer circulaire)
    int *history[HISTORY_SIZE]; // Sauvegarde de la grille à chaque étape
    int historyHead;
    int historyCount;

    bool isPaused;
    float timeAccumulator;
    float updateSpeed; // Temps entre chaque génération
} GameOfLife;

// ---------------------------------------------------------
// Initialisation
// ---------------------------------------------------------
GameOfLife* init_game(int cols, int rows) {
    GameOfLife *game = (GameOfLife*)malloc(sizeof(GameOfLife));
    game->cols = cols;
    game->rows = rows;
    int size = cols * rows;

    game->grid = (int*)calloc(size, sizeof(int));
    game->nextGrid = (int*)calloc(size, sizeof(int));
    game->ageGrid = (int*)calloc(size, sizeof(int));

    // Allocation historique
    for(int i=0; i<HISTORY_SIZE; i++) {
        game->history[i] = (int*)calloc(size, sizeof(int));
    }
    game->historyHead = 0;
    game->historyCount = 0;

    game->isPaused = true;
    game->timeAccumulator = 0.0f;
    game->updateSpeed = 0.1f; // Vitesse par défaut

    return game;
}

// ---------------------------------------------------------
// Sauvegarde l'état actuel dans l'historique
// ---------------------------------------------------------
void save_state(GameOfLife *game) {
    int size = game->cols * game->rows;
    game->historyHead = (game->historyHead + 1) % HISTORY_SIZE;
    memcpy(game->history[game->historyHead], game->grid, size * sizeof(int));
    if (game->historyCount < HISTORY_SIZE) game->historyCount++;
}

// ---------------------------------------------------------
// Restaure l'état précédent (Undo)
// ---------------------------------------------------------
void undo_state(GameOfLife *game) {
    if (game->historyCount > 0) {
        int size = game->cols * game->rows;
        // Copier l'état actuel (head) vers la grille
        memcpy(game->grid, game->history[game->historyHead], size * sizeof(int));

        // Reculer la tête
        game->historyHead--;
        if (game->historyHead < 0) game->historyHead = HISTORY_SIZE - 1;

        game->historyCount--;

        // Reset des ages car on ne stocke pas l'historique des ages pour économiser la RAM
        // (Ou on pourrait faire un historyAge, mais ici on reset visuellement)
        for(int i=0; i<size; i++) if(game->grid[i]) game->ageGrid[i] = 1; else game->ageGrid[i] = 0;
    }
}

// ---------------------------------------------------------
// Logique Jeu de la Vie (Optimisée 1D)
// ---------------------------------------------------------
void update_game(GameOfLife *game) {
    save_state(game); // Sauvegarder avant de modifier

    int size = game->cols * game->rows;

    // Directions pour les voisins (8 directions)
    int neighborsOffsets[8];
    neighborsOffsets[0] = -1 - game->cols; neighborsOffsets[1] = -game->cols; neighborsOffsets[2] = 1 - game->cols;
    neighborsOffsets[3] = -1;                                                 neighborsOffsets[4] = 1;
    neighborsOffsets[5] = -1 + game->cols; neighborsOffsets[6] = game->cols;  neighborsOffsets[7] = 1 + game->cols;

    for (int y = 0; y < game->rows; y++) {
        for (int x = 0; x < game->cols; x++) {
            int idx = y * game->cols + x;
            int count = 0;

            // Compter les voisins (avec gestion des bords simple: on ignore les bords pour éviter les crashs index)
            if (x > 0 && x < game->cols - 1 && y > 0 && y < game->rows - 1) {
                for (int k = 0; k < 8; k++) {
                    if (game->grid[idx + neighborsOffsets[k]] == 1) count++;
                }
            }

            // Règles
            if (game->grid[idx] == 1) { // Cellule vivante
                if (count == 2 || count == 3) {
                    game->nextGrid[idx] = 1;
                    game->ageGrid[idx]++; // Elle vieillit
                } else {
                    game->nextGrid[idx] = 0;
                    game->ageGrid[idx] = 0;
                }
            } else { // Cellule morte
                if (count == 3) {
                    game->nextGrid[idx] = 1;
                    game->ageGrid[idx] = 1; // Nouvelle née
                } else {
                    game->nextGrid[idx] = 0;
                    game->ageGrid[idx] = 0;
                }
            }
        }
    }

    // Swap des buffers
    int *temp = game->grid;
    game->grid = game->nextGrid;
    game->nextGrid = temp;
}

// ---------------------------------------------------------
// Remplissage Aléatoire
// ---------------------------------------------------------
void random_fill(GameOfLife *game) {
    int size = game->cols * game->rows;
    for (int i = 0; i < size; i++) {
        game->grid[i] = (rand() % 100 < 20) ? 1 : 0; // 20% de chance d'être vivant
        game->ageGrid[i] = game->grid[i];
    }
}

// ---------------------------------------------------------
// Reset vide
// ---------------------------------------------------------
void clear_grid(GameOfLife *game) {
    int size = game->cols * game->rows;
    memset(game->grid, 0, size * sizeof(int));
    memset(game->ageGrid, 0, size * sizeof(int));
    game->historyCount = 0; // Reset history
}

// ---------------------------------------------------------
// Calcul de la couleur selon l'âge
// ---------------------------------------------------------
Color get_age_color(int age) {
    // Plus vieux = Plus rouge. Jeune = Blanc.
    // Transition sur, disons, 50 générations.
    if (age <= 0) return BLACK;
    if (age == 1) return RAYWHITE;

    float factor = (float)age / 50.0f;
    if (factor > 1.0f) factor = 1.0f;

    // Interpolation de Blanc vers Rouge
    // Blanc (255,255,255) -> Rouge (230, 41, 55)
    unsigned char r = 255 - (unsigned char)((255 - 230) * factor);
    unsigned char g = 255 - (unsigned char)((255 - 41) * factor);
    unsigned char b = 255 - (unsigned char)((255 - 55) * factor);

    return (Color){r, g, b, 255};
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main(void) {
    srand(time(NULL));

    // Config Raylib
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Jeu de la Vie - Raylib Ultimate");
    SetTargetFPS(60);

    // Initialisation du jeu (Grille assez large)
    int GRID_W = 200;
    int GRID_H = 150;
    int CELL_SIZE = 10;
    GameOfLife *game = init_game(GRID_W, GRID_H);

    // Caméra pour le zoom
    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.target = (Vector2){ (GRID_W*CELL_SIZE)/2.0f, (GRID_H*CELL_SIZE)/2.0f };

    AppState state = MENU;
    int selectedPattern = 0; // 0: Souris, 1: Glider, 2: Pulsar, 3: Clown

    while (!WindowShouldClose()) {
        // --- INPUTS GLOBAUX ---
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        // --- UPDATE ---
        if (state == MENU) {
            // Logique menu simple
        }
        else if (state == GAME) {

            // 1. Contrôle Caméra (Zoom Molette)
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.offset = GetMousePosition();
                camera.target = mouseWorldPos;
                camera.zoom += (wheel * 0.125f);
                if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            }
            // Panoramique (Clic molette ou clic droit maintenu pour bouger)
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                Vector2 delta = GetMouseDelta();
                delta = Vector2Scale(delta, -1.0f/camera.zoom);
                camera.target = Vector2Add(camera.target, delta);
            }

            // 2. Contrôle Jeu
            if (IsKeyPressed(KEY_SPACE)) game->isPaused = !game->isPaused;
            if (IsKeyPressed(KEY_ESCAPE)) state = MENU;

            // Flèches pour manuel
            if (game->isPaused) {
                if (IsKeyPressed(KEY_RIGHT)) update_game(game); // Avancer 1 pas
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) undo_state(game); // Reculer 1 pas
            }

            // Sélection Outil
            if (IsKeyPressed(KEY_ONE)) selectedPattern = 0; // Crayon
            if (IsKeyPressed(KEY_TWO)) selectedPattern = 1; // Glider
            if (IsKeyPressed(KEY_THREE)) selectedPattern = 2; // Pulsar
            if (IsKeyPressed(KEY_FOUR)) selectedPattern = 3; // Clown

            // 3. Interaction Souris (Ajout/Suppression)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
                int gx = (int)(mouseWorld.x / CELL_SIZE);
                int gy = (int)(mouseWorld.y / CELL_SIZE);

                if (gx >= 0 && gx < game->cols && gy >= 0 && gy < game->rows) {
                    if (selectedPattern == 0) {
                        // Mode Crayon simple (Toggle)
                        int idx = gy * game->cols + gx;
                        game->grid[idx] = !game->grid[idx];
                        if(game->grid[idx]) game->ageGrid[idx] = 1; else game->ageGrid[idx] = 0;
                    } else if (selectedPattern == 1) {
                        apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_GLIDER);
                    } else if (selectedPattern == 2) {
                        apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_PULSAR);
                    } else if (selectedPattern == 3) {
                        apply_pattern(game->grid, game->ageGrid, game->cols, game->rows, gx, gy, PATTERN_CLOWN);
                    }
                }
            }
            // Dessin continu (seulement en mode crayon)
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && selectedPattern == 0) {
                 Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
                 int gx = (int)(mouseWorld.x / CELL_SIZE);
                 int gy = (int)(mouseWorld.y / CELL_SIZE);
                 if (gx >= 0 && gx < game->cols && gy >= 0 && gy < game->rows) {
                     int idx = gy * game->cols + gx;
                     game->grid[idx] = 1; // Force vivant
                     game->ageGrid[idx] = 1;
                 }
            }

            // 4. Calcul automatique
            if (!game->isPaused) {
                game->timeAccumulator += GetFrameTime();
                if (game->timeAccumulator >= game->updateSpeed) {
                    update_game(game);
                    game->timeAccumulator = 0.0f;
                }
            }
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF)); // Gris très sombre

        if (state == MENU) {
            // Menu stylisé
            DrawText("JEU DE LA VIE", 400, 100, 60, RAYWHITE);

            // Boutons simples (Zone de détection)
            int mouseX = GetMouseX();
            int mouseY = GetMouseY();

            // Bouton 1
            Color btn1 = LIGHTGRAY;
            if (mouseX > 400 && mouseX < 800 && mouseY > 250 && mouseY < 300) {
                btn1 = WHITE;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { clear_grid(game); state = GAME; }
            }
            DrawRectangle(400, 250, 400, 50, DARKGRAY);
            DrawText("Grille Vide (Editeur)", 480, 260, 30, btn1);

            // Bouton 2
            Color btn2 = LIGHTGRAY;
            if (mouseX > 400 && mouseX < 800 && mouseY > 320 && mouseY < 370) {
                btn2 = WHITE;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { random_fill(game); state = GAME; }
            }
            DrawRectangle(400, 320, 400, 50, DARKGRAY);
            DrawText("Grille Aleatoire", 490, 330, 30, btn2);

            // Bouton Quitter
            Color btn3 = LIGHTGRAY;
            if (mouseX > 400 && mouseX < 800 && mouseY > 450 && mouseY < 500) {
                btn3 = RED;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) CloseWindow();
            }
            DrawRectangle(400, 450, 400, 50, DARKGRAY);
            DrawText("Quitter", 550, 460, 30, btn3);
        }
        else if (state == GAME) {
            BeginMode2D(camera);

            // Dessin Grille
            // Optimisation : Ne dessiner que ce qui est visible à l'écran pourrait être ajouté ici
            // mais pour simplifier, on boucle sur tout.
            for (int y = 0; y < game->rows; y++) {
                for (int x = 0; x < game->cols; x++) {
                    int idx = y * game->cols + x;
                    if (game->grid[idx] == 1) {
                        Color col = get_age_color(game->ageGrid[idx]);
                        DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, col);
                    } else {
                        // Effet grille subtil
                        DrawRectangleLines(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, GetColor(0x333333FF));
                    }
                }
            }
            // Bordure monde
            DrawRectangleLines(0, 0, game->cols*CELL_SIZE, game->rows*CELL_SIZE, BLUE);

            EndMode2D();

            // UI HUD (Interface utilisateur par dessus le jeu)
            DrawRectangle(0, 0, screenWidth, 40, Fade(BLACK, 0.8f));
            DrawText(TextFormat("Gen: %d | Zoom: %.2f | Voisins: %s", 0, camera.zoom, game->isPaused ? "PAUSE" : "PLAY"), 10, 10, 20, GREEN);

            DrawText("[ESPACE] Play/Pause  [F11] Plein Ecran  [ECHAP] Menu  [FLECHES] Reculer/Avancer", 10, 40, 10, GRAY);
            DrawText(TextFormat("Outil: %s (1:Cray, 2:Glid, 3:Puls, 4:Clown)",
                selectedPattern == 0 ? "Crayon" : (selectedPattern == 1 ? "Glider" : "Pattern")), 10, 60, 20, YELLOW);

            // Indication zoom
            DrawText("Molette: Zoom | Clic Droit: Bouger", screenWidth - 250, 10, 15, DARKGRAY);
        }

        EndDrawing();
    }

    // Nettoyage mémoire
    free(game->grid);
    free(game->nextGrid);
    free(game->ageGrid);
    for(int i=0; i<HISTORY_SIZE; i++) free(game->history[i]);
    free(game);

    CloseWindow();
    return 0;
}