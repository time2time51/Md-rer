#include <genesis.h>
#include "resources.h"

// --------- Texte d'intro ---------
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

static void drawFullImage(const Image *img)
{
    VDP_clearPlane(BG_B, TRUE);
    PAL_setPalette(PAL1, img->palette->data, DMA);
    // Toujours un tile index >= 1 (0 est reserve par le VDP)
    VDP_drawImageEx(BG_B, img,
        TILE_ATTR_FULL(PAL1, 0, 0, 0, 1),
        0, 0, FALSE, TRUE);
}

static void showTitle(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    drawFullImage(&title);

    while (1)
    {
        if (JOY_readJoypad(JOY_1) & BUTTON_START)
        {
            // TODO: aller vers le menu / jeu
        }
        SYS_doVBlankProcess();
    }
}

static void playIntro(void)
{
    VDP_resetScreen();
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32);   // tables larges = scroll texte safe
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    // Police / texte en BLANC sur PAL0
    VDP_setTextPalette(PAL0);
    PAL_setPaletteColors(0, (u16*)palette_black, 64, DMA); // tout noir de base
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xFFFFFF));         // blanc pour la fonte SGDK

    drawFullImage(&intro1);

    // Musique XGM : boucle infinie
    XGM_setLoopNumber(-1);
    XGM_startPlay(intro_music);

    s16 scrollY = 0;      // scroll vertical du plan A (negatif => monte)
    u16 nextLine = 0;
    u16 lineTimer = 0;
    const u16 lineEvery = 150; // ~2.5s par ligne (60 FPS)
    s16 writeY = 28;      // on ecrit sous l'ecran
    u32 frames = 0;

    while (1)
    {
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        // apparition des lignes
        if ((nextLine < INTRO_LINES) && (++lineTimer >= lineEvery))
        {
            lineTimer = 0;
            VDP_drawText(intro_lines[nextLine], 2, writeY);
            writeY += 2;
            nextLine++;
        }

        // scroll doux (remonte le texte)
        if ((frames & 3) == 0) scrollY--;
        VDP_setVerticalScroll(BG_A, scrollY);

        // switch d'image toutes ~30s (3 x 30s = 90s)
        if (frames == 1800)       drawFullImage(&intro2);
        else if (frames == 3600)  drawFullImage(&intro3);
        else if (frames >= 5400)  break;

        frames++;
        SYS_doVBlankProcess();
    }

    showTitle(); // la musique continue
}

int main(void)
{
    JOY_init();
    SYS_disableInts();

    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32);

    SYS_enableInts();

    playIntro();
    return 0;
}
