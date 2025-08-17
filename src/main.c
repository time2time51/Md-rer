#include <genesis.h>
#include "resources.h"

// ------------ Config ------------
#define FPS               60
#define INTRO_TOTAL_SEC   90
#define BG2_AT_SEC        30
#define BG3_AT_SEC        60
#define TICKS_PER_LINE    30   // 0,5 s par “ligne” de scroll

typedef enum { ST_INTRO, ST_TITLE } GameState;
static GameState g_state = ST_INTRO;

static u16 g_tileIndex;
static u16 g_joy;

// --------- Texte intro (scroll vers le bas) ---------
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
    "",
    "Appuie sur START pour passer."
};
#define NB_LINES (sizeof(intro_lines)/sizeof(intro_lines[0]))

// ------------ Utils ------------
static void drawFullImage(const Image* img, u16 pal)
{
    PAL_setPalette(pal, img->palette->data, DMA);
    VDP_drawImageEx(
        BG_A, img,
        TILE_ATTR_FULL(pal, FALSE, FALSE, FALSE, g_tileIndex),
        0, 0, FALSE, TRUE
    );
    g_tileIndex += img->tileset->numTile;
}

static void showTitle(void)
{
    g_tileIndex = TILE_USER_INDEX;
    VDP_clearPlane(BG_A, TRUE);
    drawFullImage(&title, PAL0);          // Title.PNG
}

// ------------ Intro ------------
static void playIntro(void)
{
    u32 frame = 0;
    u16 blink = 0;

    // Musique
    XGM_startPlay(&intro_music);
    XGM_setLoopNumber(-1);

    // Premier fond
    g_tileIndex = TILE_USER_INDEX;
    VDP_clearPlane(BG_A, TRUE);
    drawFullImage(&intro1, PAL0);

    // Texte part au-dessus et descend
    s16 baseY = -(s16)NB_LINES * 2;

    while (frame < INTRO_TOTAL_SEC * FPS)
    {
        // Changement d’image
        if (frame == BG2_AT_SEC * FPS)
        {
            g_tileIndex = TILE_USER_INDEX;
            VDP_clearPlane(BG_A, TRUE);
            drawFullImage(&intro2, PAL0);
        }
        else if (frame == BG3_AT_SEC * FPS)
        {
            g_tileIndex = TILE_USER_INDEX;
            VDP_clearPlane(BG_A, TRUE);
            drawFullImage(&intro3, PAL0);
        }

        // Scroll texte
        if ((frame % TICKS_PER_LINE) == 0)
        {
            for (u16 y = 0; y < 28; y++) VDP_clearText(0, y, 40);

            s16 yStart = baseY + (s16)(frame / TICKS_PER_LINE);
            for (u16 i = 0; i < NB_LINES; i++)
            {
                s16 y = yStart + (s16)i * 2;
                if (y >= 0 && y < 28)
                    VDP_drawText(intro_lines[i], 2, y);
            }
        }

        // Blink "START=SKIP"
        blink++;
        if ((blink & 31) == 0)
            VDP_drawText(((blink & 32) ? "                    " : "START=SKIP"), 12, 26);

        // Skip
        g_joy = JOY_readJoypad(JOY_1);
        if (g_joy & BUTTON_START) break;

        frame++;
        SYS_doVBlankProcess();
    }

    g_state = ST_TITLE;
    showTitle();
}

// ------------ Input ------------
static void joyHandler(u16 joyId, u16 changed, u16 state)
{
    if (joyId == JOY_1) g_joy = state;
}

// ------------ main ------------
int main(bool hard)
{
    JOY_init();
    JOY_setEventHandler(joyHandler);

    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    PAL_setColors(0, (u16*)palette_black, 64, DMA);

    playIntro();

    // Boucle titre
    u16 blink = 0;
    while (1)
    {
        if (g_state == ST_TITLE)
        {
            blink++;
            if ((blink & 31) == 0)
                VDP_drawText(((blink & 32) ? "            " : "PRESS START"), 12, 20);
        }
        SYS_doVBlankProcess();
    }
    return 0;
}
