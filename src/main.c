#include <genesis.h>
#include "resources.h"

// ---------- paramètres ----------
#define PLANE_W         64
#define PLANE_H         32
#define FRAMES_PER_PX   6      // 1 pixel de scroll toutes les 6 frames  (~5x plus rapide que 30)
#define LINES_VISIBLE   28     // hauteur écran en lignes de texte (28)
#define TEXT_PAL        PAL2   // palette texte (différente de l'image pour éviter les conflits)

// ---------- texte ----------
static const char* intro_lines[] = {
    "Reims, la nuit. La ville se meurt.",
    "La corruption et la drogue rongent les rues.",
    "Cela fait 10 ans que Jimmy et Houcine",
    "sont tombes et ont disparu derriere les barreaux.",
    "Depuis, les gangs regnent en maitre.",
    "La police est corrompue, les politiciens aussi.",
    "Les dealers font la loi dans les quartiers.",
    "Les habitants n'ont plus d'espoir.",
    "",
    "Mais ce soir...",
    "Jimmy et Houcine sortent de prison.",
    "Leur colere est intacte.",
    "Ils vont faire payer ceux qui ont detruit leur ville.",
    "",
    "REIMS EN RAGE"
};
#define INTRO_LINES  (sizeof(intro_lines)/sizeof(intro_lines[0]))

// ----------- petits helpers ----------
static void clearPlane(VDPPlane p)
{
    VDP_clearPlane(p, TRUE);
}

static void drawCenteredText(u16 y, const char* s)
{
    int len = strlen(s);
    int x = (40 - len) / 2;
    if (x < 0) x = 0;
    VDP_drawText(s, x, y);
}

// Dessine une image plein écran 320x224 sur un plan
static u16 nextTile = TILE_USER_INDEX;
static void drawFullImageOn(VDPPlane plane, const Image* img, bool loadPal)
{
    VDP_drawImageEx(plane, img, nextTile, 0, 0, loadPal, TRUE);
    nextTile += img->tileset->numTile;
}

// Dessine toutes les lignes du bloc, en commençant par la première
// exactement sur la dernière ligne visible (y=27).
static void drawIntroBlockAtBottom(void)
{
    const u16 yStart = 27; // tout en bas
    for (u16 i = 0; i < INTRO_LINES; i++)
    {
        u16 y = yStart + i;               // on dépasse le bas vers "le bas du plan"
        drawCenteredText(y, intro_lines[i]);
    }
}

// ------------- scènes -------------
static void playIntro(void)
{
    // VRAM / plans
    VDP_setPlaneSize(PLANE_W, PLANE_H, TRUE);
    VDP_setTextPlane(BG_A);
    clearPlane(BG_A);
    clearPlane(BG_B);

    // fond image (BG_B)
    nextTile = TILE_USER_INDEX;
    drawFullImageOn(BG_B, &intro1, TRUE);  // charge palette des images

    // texte (BG_A)
    PAL_setPaletteColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(0x000000)); // fond
    PAL_setPaletteColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(0xFF7840)); // lettres
    VDP_setTextPalette(TEXT_PAL);

    drawIntroBlockAtBottom();

    // musique
    XGM_startPlayResource(&intro_music);

    // scrolling + changement d’images
    s16 vscroll = 0;                // on part sans décalage (1re ligne visible en bas)
    u32 frame  = 0;
    u16 imgIdx = 0;

    while (TRUE)
    {
        // Changement d'image toutes ~7 secondes (~420 frames)
        if (frame == 420) { drawFullImageOn(BG_B, &intro2, TRUE); imgIdx = 1; }
        if (frame == 840) { drawFullImageOn(BG_B, &intro3, TRUE); imgIdx = 2; }

        // Scroll vertical (vers le haut) – 1 px toutes FRAMES_PER_PX
        if ((frame % FRAMES_PER_PX) == 0)
        {
            vscroll--;
            VDP_setVerticalScroll(BG_A, vscroll);
        }

        // Skip si START
        u16 joy = JOY_readJoypad(JOY_1);
        if (joy & BUTTON_START) break;

        SYS_doVBlankProcess();
        frame++;

        // quand tout est a peu pres sorti de l'ecran, on quitte
        if (frame > (INTRO_LINES * 16 + 600)) break;
    }

    // stop musique au passage à l’écran titre
    XGM_stopPlay();
}

static void showTitle(void)
{
    // plans
    VDP_setPlaneSize(PLANE_W, PLANE_H, TRUE);
    VDP_setTextPlane(BG_A);
    clearPlane(BG_A);
    clearPlane(BG_B);

    nextTile = TILE_USER_INDEX;
    drawFullImageOn(BG_B, &title, TRUE);

    // texte press start clignotant
    const char* press = "PRESS  START";
    u16 blink = 0;

    while (TRUE)
    {
        // blink toutes 20 frames
        if ((blink / 20) % 2 == 0)
            drawCenteredText(24, press);
        else
            VDP_clearTextLine(24);

        // START pour quitter
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        blink++;
        SYS_doVBlankProcess();
    }
}

// ------------- entrée -------------
int main(void)
{
    // pads
    JOY_init();

    // VDP de base
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();

    playIntro();
    showTitle();

    // boucle vide (placeholder pour le menu / jeu)
    while (TRUE)
    {
        SYS_doVBlankProcess();
    }

    return 0;
}
