
#include <stdio.h>
#include <stdlib.h>

void affichage_matrice(int val, int t[val][val]) {
    int j,i;
    for (i=0;i<val;i++) {
        printf("\n");
        for (j=0;j<val;j++) {
            printf("%i ",t[i][j]);
        }
    }
    printf("\n\n");
}

void remplissage_matrice(int val, int t[val][val]) {
    int j,i;
    for (i=0;i<val;i++) {
        for (j=0;j<val;j++) {
            printf("Saissez votre valeur pour la case [%i][%i]",i,j);
            scanf("%i",&t[i][j]);
        }
    }
}

int menu(int val, int t[val][val]) {
    int rep = 1;
    int var;
    do {
        printf("Voici le menu :\n\n");
        printf("1 - Afficher la matrice\n");
        printf("2- Remplir la matrice\n");
        printf("3- Quitter le menu\n");
        scanf("%i",&rep);
        if (rep<1||rep>3) {
            printf("Erreur de saisie, veuillez saisir une valeur entre 1 et 3\n");
        }
    }while (rep<1||rep>3);
    switch (rep) {
        case 1: affichage_matrice(val,t);
            var = 1;
            break;
        case 2: remplissage_matrice(val,t);
            var = 1;
            break;
        case 3:
            var = 0;
            break;
    }
    return var;

}

int main(void) {
    int val,res;
    printf("Choisissez le nombre de votre matrice \n");
    scanf("%i",&val);
    int t[val][val];
    do {
        res = menu(val,t);
    }while(res = 1);
}



