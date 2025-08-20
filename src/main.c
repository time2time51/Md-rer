#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// State commun
// -----------------------------------------------------------------------------
static u16 nextTile;                    // base tuile libre en VRAM (suite à TILE_USER_INDEX)
static const u16 TEXT_PAL = PAL2;       // palette utilisée pour le texte (évite collision avec images)

// util: attend N VBlanks
static void waitFrames(u16 n)
{
    while (n--) SYS_doVBlankProcess();
}

// util: centre un texte sur 40 colonnes
static void drawCentered(u16 y, const char* str)
{
    u16 len = strlen(str);
    if (len > 40) len = 40;
    s16 x = (40 - (s16)len) / 2;
    if (x < 0) x = 0;
    VDP_drawText(str, (u16)x, y);
}

// clean + configuration sûre avant une scène
static void resetScene(void)
{
    // plans larges, VRAM prête
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);

    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(0);
    VDP_setTextPalette(TEXT_PAL);
    VDP_clearTextArea(0, 0, 64, 32);

    VDP_clearPlan(BG_A, TRUE);
    VDP_clearPlan(BG_B, TRUE);

    // point de départ des tuiles utilisateur
    nextTile = TILE_USER_INDEX;
}

// dessine une image plein écran sur un plan (charge aussi sa palette)
static void drawFullImageOn(VDPPlane plane, const Image* img, u16 x, u16 y, u16 palIndex)
{
    // charge la palette de l'image dans palIndex
    PAL_setPalette(palIndex, img->palette->data, DMA);
    // dessine l'image (charge des tuiles à partir de nextTile)
    VDP_drawImageEx(plane, img, nextTile, x, y, FALSE, TRUE);
    nextTile += img->tileset->numTile;
}

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    // Lance la musique (intro_music vient de resources.res -> XGM)
    XGM_init();
    XGM_startPlay(intro_music);

    // Couleur du texte (orange lisible)
    PAL_setPaletteColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(0xFF7840)); // lettres
    PAL_setPaletteColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(0x000000)); // fond

    // ---- Ecran 1 : ciel + lune
    drawFullImageOn(BG_B, &intro1, 0, 0, PAL0);

    // texte (lignes <= 40 colonnes pour éviter le débord)
    const char* lines1[] = {
        "Reims, la nuit. La ville se meurt.",
        "La corruption et la drogue rongent les rues.",
        "Cela fait 10 ans que Jimmy et Houcine",
        "sont tombes et ont disparu derriere les barreaux.",
        "Depuis, les gangs regnent en maitre.",
        "La police est corrompue, les politiciens absents.",
        "Les dealers font la loi dans les quartiers.",
        "Les habitants n'ont plus d'espoir.",
        "",
        "Mais ce soir...",
        "Jimmy et Houcine sortent de prison.",
        "Leur colere est intacte.",
        "Ils vont faire payer ceux qui ont detruit",
        "REIMS EN RAGE"
    };
    for (u16 i = 0; i < sizeof(lines1)/sizeof(lines1[0]); i++)
    {
        drawCentered(4 + i, lines1[i]);
        waitFrames(30); // ralentit l'apparition des lignes
    }

    // petite pause avant image suivante
    waitFrames(120);

    // ---- Ecran 2 : skyline
    resetScene();
    drawFullImageOn(BG_B, &intro2, 0, 0, PAL0);
    waitFrames(180);

    // ---- Ecran 3 : rue
    resetScene();
    drawFullImageOn(BG_B, &intro3, 0, 0, PAL0);
    waitFrames(180);

    // La musique continue jusqu'au titre
}

// -----------------------------------------------------------------------------
// Titre
// -----------------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();
    drawFullImageOn(BG_B, &title, 0, 0, PAL0);

    // Optionnel : petit "PRESS START" propre (mais tu peux le retirer)
    // drawCentered(26, "PRESS START");

    // Laisse l'ecran titre tant que START n'est pas presse
    // (Si tu ne veux pas de blocage, remplace par un simple waitFrames(300);)
    u16 t = 0;
    while(TRUE)
    {
        SYS_doVBlankProcess();
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        // clignotement facultatif du message (de-commenter si tu remets le texte)
        // if ((t++ & 31) == 0) VDP_clearTextArea(0, 26, 40, 1);
        // if ((t & 63) == 0) drawCentered(26, "PRESS START");
    }
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main(void)
{
    // initial SGDK (joy, interrupts)
    JOY_init();

    playIntro();
    showTitle();

    // boucle vide : titre atteint
    while(TRUE) SYS_doVBlankProcess();
    return 0;
}
