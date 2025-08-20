#include <genesis.h>
#include <string.h>
#include "resources.h"

// ----- Texte d'intro -----
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
#define INTRO_LINES (sizeof(intro_lines)/sizeof(intro_lines[0]))

// ----- Utilitaires images / palettes -----
static u16 nextTile = TILE_USER_INDEX;

// dessine une image plein ecran sur un plan, charge sa palette a l'index voulu
static void drawFullImageOn(VDPPlane plan, const Image *img, u16 palIndex)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);
    VDP_drawImageEx(
        plan,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        0, 0,
        FALSE,
        TRUE
    );
    nextTile += img->tileset->numTile;
}

// efface un plan (texte)
static void clearPlane(VDPPlane plan)
{
    VDP_clearPlane(plan, TRUE);
}

// ----- Intro -----
static void playIntro(void)
{
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setBackgroundColor(0);

    // Musique (depuis res: XGM intro_music "intro.vgm")
    XGM_startPlay(&intro_music);   // pas la variante *Resource*

    // plans / palettes
    const u16 palImg = PAL0;
    const u16 palTxt = PAL1;

    // palette texte : uniquement la couleur 15 utilisée par la font
    u16 palTextData[16];
    for (u16 i = 0; i < 16; i++) palTextData[i] = 0;
    palTextData[15] = RGB24_TO_VDPCOLOR(0xD05030);   // orange sombre
    PAL_setPalette(palTxt, palTextData, DMA);
    VDP_setTextPalette(palTxt);

    // image de fond initiale
    clearPlane(BG_A);
    drawFullImageOn(BG_A, &intro1, palImg);

    // texte sur le plan B (scroll vertical doux sur 90 s)
    clearPlane(BG_B);
    const int lineH   = 14;
    const int firstY  = 27;
    const int totalTextPixel = (int)INTRO_LINES * lineH + 224;
    const int durFrames = 90 * 60;     // 90 sec @60 Hz
    int frame = 0;

    for (u16 i = 0; i < INTRO_LINES; i++)
        VDP_drawTextBG(BG_B, intro_lines[i], 3, firstY/8 + (i*(lineH/8)));

    const int swap1 = durFrames/3;       // 30 s
    const int swap2 = (2*durFrames)/3;   // 60 s

    while (frame < durFrames)
    {
        int scrollY = (frame * totalTextPixel) / durFrames;
        VDP_setVerticalScroll(BG_B, -scrollY);

        if (frame == swap1) {
            clearPlane(BG_A);
            drawFullImageOn(BG_A, &intro2, palImg);
        } else if (frame == swap2) {
            clearPlane(BG_A);
            drawFullImageOn(BG_A, &intro3, palImg);
        }

        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        SYS_doVBlankProcess();
        frame++;
    }
}

// ----- Ecran titre -----
static void showTitle(void)
{
    nextTile = TILE_USER_INDEX;
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setBackgroundColor(0);
    clearPlane(BG_A);
    clearPlane(BG_B);

    drawFullImageOn(BG_A, &title, PAL0);

    const char *press = "PRESS  START";
    s16 blink = 0;

    while (1)
    {
        if ((blink >> 4) & 1)
            VDP_drawTextBG(BG_B, press, 16 - 6, 25);
        else
            VDP_clearTextAreaBG(BG_B, 0, 25, 40, 1);

        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        blink++;
        SYS_doVBlankProcess();
    }
}

static void joyCallback(u16 joy, u16 changed, u16 state)
{
    (void)joy; (void)changed; (void)state;
}

int main(bool hard)
{
    (void)hard;

    JOY_init();
    JOY_setEventHandler(joyCallback);

    playIntro();     // 90 s ou START pour skip
    showTitle();     // musique continue

    while (1) SYS_doVBlankProcess();
    return 0;
}
