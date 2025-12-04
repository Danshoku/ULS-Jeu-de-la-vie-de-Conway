#ifndef PATTERNS_H
#define PATTERNS_H

typedef struct {
    int w, h;
    int *cells;
} Pattern;

// --- GLIDER (Vaisseau) ---
int glider_data[] = {
    0, 1, 0,
    0, 0, 1,
    1, 1, 1
};
Pattern PATTERN_GLIDER = {3, 3, glider_data};

// --- CLOWN (Oscillateur) ---
int clown_data[] = {
    1, 1, 1,
    1, 0, 1,
    1, 0, 1
};
Pattern PATTERN_CLOWN = {3, 3, clown_data};

// --- GOSPER GLIDER GUN (Le VRAI Canon) ---
// Taille : 36 colonnes x 9 lignes
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

// Fonction d'application
void apply_pattern(int *grid, int *ageGrid, int cols, int rows, int x, int y, Pattern p) {
    for (int py = 0; py < p.h; py++) {
        for (int px = 0; px < p.w; px++) {
            // On ajoute px et py à la position de la souris (x, y)
            int targetX = x + px;
            int targetY = y + py;

            // Vérification stricte des limites pour ne pas planter
            if (targetX >= 0 && targetX < cols && targetY >= 0 && targetY < rows) {
                // On copie le motif
                if (p.cells[py * p.w + px] == 1) {
                    int idx = targetY * cols + targetX;
                    grid[idx] = 1;
                    ageGrid[idx] = 1; // On réinitialise l'âge pour qu'il soit blanc
                }
            }
        }
    }
}

#endif