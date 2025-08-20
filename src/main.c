#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// State commun
// -----------------------------------------------------------------------------
static u16 nextTile;                    // base tuile libre en VRAM
static const u16 TEXT_PAL = PAL2;       // palette pour le texte

static void waitFrames(u16 n)
{
    while (n--) SYS_doVBlankProcess();
}

static void drawCentered(u16 y, const char* str)
{
    u16 len = strlen(str);
    if (len > 40) len = 40;
    s16 x = (40 - (s16)len) / 2;
    if (x < 0) x = 0;
    VDP_drawText(str, (u16)x, y);
}

static void resetScene(void)
{
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);

    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(0);
    VDP_setTextPalette(TEXT_PAL);
    VDP_clearTextArea(0, 0, 64, 32);

    // v2.11 : clear **Plane**
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    nextTile = TILE_USER_INDEX;
}

static void drawFullImageOn(VDPPlane plane, const Image* img, u16 x, u16 y, u16 palIndex)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);
    VDP_drawImageEx(plane, img, nextTile, x, y, FALSE, TRUE);
    nextTile += img->tileset->numTile;
}

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    // Musique (pas besoin de XGM_init ici)
    XGM_startPlay(intro_music);

    // Couleur du texte (orange lisible) + fond noir dans la palette texte
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(0xFF7840));
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(0x000000));

    // ---- Ecran 1
    drawFullImageOn(BG_B, &intro1, 0, 0, PAL0);

    const char* lines1[] = {
        "Reims, la nuit. La ville se meurt.",
        "La corruption et la drogue rongent les rues.",
        "Cela fait 10 ans que Jimmy et Houcine",
        "sont tombes, disparus derriere les barreaux.",
        "Depuis, les gangs regnent en maitre.",
        "La police est corrompue, les politiques absents.",
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
        waitFrames(30); // rythme de lecture
    }
    waitFrames(120);

    // ---- Ecran 2
    resetScene();
    drawFullImageOn(BG_B, &intro2, 0, 0, PAL0);
    waitFrames(180);

    // ---- Ecran 3
    resetScene();
    drawFullImageOn(BG_B, &intro3, 0, 0, PAL0);
    waitFrames(180);
}

// -----------------------------------------------------------------------------
// Titre
// -----------------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();
    drawFullImageOn(BG_B, &title, 0, 0, PAL0);

    // Laisse le titre jusqu'au START (si tu ne veux pas d'attente, remplace par waitFrames(300);)
    while(TRUE)
    {
        SYS_doVBlankProcess();
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;
    }
}

// -----------------------------------------------------------------------------
// main (signature attendue par SGDK)
// -----------------------------------------------------------------------------
int main(bool hardReset)
{
    (void)hardReset;
    JOY_init();

    playIntro();
    showTitle();

    while(TRUE) SYS_doVBlankProcess();
    return 0;
}
