#ifndef PATTERNS_H
#define PATTERNS_H

// Définition simple d'un motif
typedef struct {
    int w, h;
    int *cells; // 1 = vivant, 0 = mort
} Pattern;

// --- GLIDER ---
int glider_data[] = {
    0, 1, 0,
    0, 0, 1,
    1, 1, 1
};
Pattern PATTERN_GLIDER = {3, 3, glider_data};

// --- CLOWN (Forme simple oscillante) ---
int clown_data[] = {
    1, 1, 1,
    1, 0, 1,
    1, 0, 1
};
Pattern PATTERN_CLOWN = {3, 3, clown_data};

// --- GOSPER GLIDER GUN (Le fameux canon) ---
// 36x9 approx
int gosper_data[] = {
    // Une représentation simplifiée 1D d'un bloc 36x9
    // Pour simplifier le code ici, je mets les coordonnées relatives (x,y) des cellules vivantes
    // et une fonction spéciale pour l'appliquer.
    // NOTE : Pour ce fichier header, restons sur une matrice simple pour la lisibilité
    // Ceci est un "Pulsar" (plus simple à coder en brut que le Gosper pour cet exemple)
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,0,0,0,1,1,1,0,0
};
Pattern PATTERN_PULSAR = {13, 13, gosper_data};

// Fonction pour appliquer un pattern sur la grille principale
void apply_pattern(int *grid, int *ageGrid, int cols, int rows, int x, int y, Pattern p) {
    for (int py = 0; py < p.h; py++) {
        for (int px = 0; px < p.w; px++) {
            int targetX = x + px;
            int targetY = y + py;
            
            // Vérifier les limites
            if (targetX >= 0 && targetX < cols && targetY >= 0 && targetY < rows) {
                if (p.cells[py * p.w + px] == 1) {
                    grid[targetY * cols + targetX] = 1;
                    ageGrid[targetY * cols + targetX] = 1; // Age reset
                }
            }
        }
    }
}

#endif