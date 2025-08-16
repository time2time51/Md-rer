#include <genesis.h>

int main()
{
    // Affiche du texte à l'écran, ligne 13, colonne 9
    VDP_drawText("Hello, Mega Drive!", 9, 13);

    // Boucle principale du jeu
    while (1)
    {
        SYS_doVBlankProcess(); // synchronisation avec le VBlank
    }

    return 0;
}
