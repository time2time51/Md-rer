#include <genesis.h>
#include "resources.h"

// ===================== Réglages rapides =====================
#define PLANE_W             64
#define PLANE_H             32

// Texte : démarre tout en bas (ligne 27 sur 0..27)
#define TEXT_START_Y        27
// Vitesse du scroll (frames par ligne) – ajuste ici.
// 6 = rapide ; augmente pour ralentir.
#define TEXT_SCROLL_DELAY   6

// Durée d’affichage de chaque image d’intro (en secondes)
#define INTRO_IMAGE_SEC     30

// Clignotement "PRESS START" (frames)
#define BLINK_ON_FRAMES     30
#define BLINK_OFF_FRAMES    30
// ===========================================================

// Texte d’intro (sans “appuie sur start…”)
static const char *intro_lines[] = {
    "Reims, la nuit. La ville se meurt.",
    "La corruption et la drogue rongent les rues.",
    "Cela fait 10 ans que Jimmy et Houcine",
    "sont tombes et ont disparu derriere les barreaux.",
    "Depuis, les gangs regnent en maitre.",
    "La police est corrompue, les politiciens achetes.",
    "Les dealers font la loi dans les quartiers.",
    "Les habitants n'ont plus d'espoir.",
    "Mais ce soir...",
    "Jimmy et Houcine sortent de prison.",
    "Leur colere est intacte.",
    "Ils vont faire payer ceux qui ont detruit leur ville.",
    "",
    "REIMS EN RAGE",
    ""
};
static const u16 INTRO_LINES_COUNT = sizeof(intro_lines) / sizeof(intro_lines[0]);

// Gestion VRAM
static u16 nextTile = TILE_USER_INDEX;

// Raccourcis palettes
#define IMG_PAL     PAL1
#define TEXT_PAL    PAL2
#define UI_PAL      PAL0

// -----------------------------------------------------------
// Utils
static void resetScene(void)
{
    VDP_setPlaneSize(PLANE_W, PLANE_H, TRUE);
    VDP_setTextPlane(BG_A);

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setVerticalScroll(BG_B, 0);

    nextTile = TILE_USER_INDEX;
}

// Dessine une image 320x224 plein écran sur le plan donné
static void drawFullImageOn(VDPPlane plane, const Image *img, u16 palIndex)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);

    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        0, 0,
        FALSE,      // palette déjà chargée
        TRUE        // DMA
    );

    nextTile += img->tileset->numTile;
}

// Redessine le bloc de texte visible, ancré à TEXT_START_Y - offset
static void renderIntroText(s16 offset)
{
    VDP_clearTextArea(0, 0, 40, 28);

    const s16 screenLines = 28; // 224/8
    s16 y0 = TEXT_START_Y - offset;

    for (s16 i = 0; i < (s16)INTRO_LINES_COUNT; i++)
    {
        s16 y = y0 + i;
        if ((y >= 0) && (y < screenLines))
        {
            const char* line = intro_lines[i];
            s16 x = 20 - (strlen(line) / 2);
            if (x < 0) x = 0;
            VDP_drawText(line, x, y);
        }
    }
}

// -----------------------------------------------------------
// Intro
static bool playIntro(void)
{
    resetScene();

    // Texte orange sur fond noir
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(0x000000)); // fond
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(0xFF7840)); // lettres

    // Première image (plan B)
    drawFullImageOn(BG_B, &intro1, IMG_PAL);

    // Musique (pas de XGM_init dans ta lib)
    XGM_startPlay(intro_music);

    const u32 switch1 = INTRO_IMAGE_SEC * 60;
    const u32 switch2 = INTRO_IMAGE_SEC * 2 * 60;
    const u32 endAt   = INTRO_IMAGE_SEC * 3 * 60;

    s16 textOffset = 0;   // 0 => première ligne à TEXT_START_Y (tout en bas)
    u16 scrollTick = 0;

    for (u32 t = 0; t < endAt; t++)
    {
        if (t == switch1)      drawFullImageOn(BG_B, &intro2, IMG_PAL);
        else if (t == switch2) drawFullImageOn(BG_B, &intro3, IMG_PAL);

        if (scrollTick == 0) renderIntroText(textOffset);

        // Skip START
        if (JOY_readJoypad(JOY_1) & BUTTON_START) return true;

        // Scroll timing
        scrollTick++;
        if (scrollTick >= TEXT_SCROLL_DELAY)
        {
            scrollTick = 0;
            textOffset++;
            // on laisse finir même si tout est sorti de l’écran
        }

        VDP_waitVSync();
    }

    return true;
}

// -----------------------------------------------------------
// Title screen
static void showPressStartBlink(void)
{
    static u16 blink = 0;
    const char *msg = "PRESS START";
    const s16 y = 25;
    const s16 x = 20 - (strlen(msg) / 2);

    blink++;
    u16 phase = blink % (BLINK_ON_FRAMES + BLINK_OFF_FRAMES);
    VDP_clearTextArea(x, y, strlen(msg), 1);
    if (phase < BLINK_ON_FRAMES) VDP_drawText(msg, x, y);
}

static void showTitle(void)
{
    resetScene();

    // Image titre (utilise sa palette en PAL0)
    drawFullImageOn(BG_B, &title, UI_PAL);

    while (1)
    {
        showPressStartBlink();

        if (JOY_readJoypad(JOY_1) & BUTTON_START)
        {
            // TODO: enchaîner vers menu/jeu
            // break;
        }

        VDP_waitVSync();
    }
}

// -----------------------------------------------------------
// Entrée
int main(bool hard)
{
    (void)hard;
    JOY_init();

    if (playIntro()) showTitle();

    while (1) VDP_waitVSync();
    return 0;
}
