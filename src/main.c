#include <genesis.h>
#include "resources.h"

typedef enum { STATE_INTRO, STATE_TITLE } GameState;
static GameState state = STATE_INTRO;

static u16 tileIndex;
static u16 joy;

// Texte d'intro (défilement sur ~1m30)
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

static void drawImage(const Image *img, u16 pal)
{
    VDP_setPalette(pal, img->palette->data);
    VDP_drawImageEx(
        BG_A,
        img,
        TILE_ATTR_FULL(pal, FALSE, FALSE, FALSE, tileIndex),
        0, 0, FALSE, TRUE
    );
    tileIndex += img->tileset->numTile;
}

static void showIntro(void)
{
    tileIndex = TILE_USERINDEX;

    // Scene 1 (ciel + ville + rue)
    drawImage(&scene1_sky, PAL1);
    drawImage(&scene1_city, PAL2);
    drawImage(&scene1_street, PAL3);

    // Premier texte affiché
    s16 baseY = 24;
    for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        VDP_drawText(intro_lines[i], 4, baseY + (s16)i*2);
}

static void updateIntro(void)
{
    static u32 frames = 0;
    frames++;

    // Scroll du texte
    if ((frames % 30) == 0)
    {
        VDP_clearPlane(BG_A, TRUE);

        s16 shift = -(frames / 30);
        s16 baseY = 24 + shift;

        for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        {
            s16 y = baseY + (s16)i*2;
            if (y >= 0 && y < 28)
                VDP_drawText(intro_lines[i], 4, y);
        }
    }

    // Changer de scene apres ~45s
    if (frames == 45*60)
    {
        tileIndex = TILE_USERINDEX;
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);

        drawImage(&scene2_sky, PAL1);
        drawImage(&scene2_city, PAL2);
        drawImage(&scene2_street, PAL3);
    }

    // Quitter l'intro apres 90s ou si START
    if (frames > (90*60) || (joy & BUTTON_START))
    {
        state = STATE_TITLE;
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);

        VDP_drawText("REIMS EN RAGE", 10, 10);
        VDP_drawText("PRESS START", 12, 20);
    }
}

static void joyHandler(u16 joyID, u16 changed, u16 state_)
{
    if (joyID == JOY_1) joy = state_;
}

int main(void)
{
    JOY_init();
    JOY_setEventHandler(joyHandler);

    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    PAL_setColors(0, (u16*)palette_black, 64, DMA);

    showIntro();

    while (1)
    {
        switch (state)
        {
            case STATE_INTRO: updateIntro(); break;
            case STATE_TITLE: break;
        }
        SYS_doVBlankProcess();
    }

    return 0;
}
