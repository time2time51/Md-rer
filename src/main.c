#include <genesis.h>
#include "resources.h"

typedef enum { STATE_INTRO, STATE_TITLE, STATE_GAME } GameState;
static GameState state = STATE_INTRO;

static u16 tileIndex;
static u16 joy;
static u32 introFrames = 0;
static u32 gameFrames  = 0;

static Sprite *p1, *p2;
static fix16 p1x, p1y, p2x, p2y;

// Texte d'intro (défilement simple)
static const char *intro_lines[] = {
    "Reims, la nuit. La ville se meurt.",
    "La corruption et la drogue rongent les rues.",
    "Deux hommes decides a tout reprendre...",
    "Ils sortent de l'ombre pour faire justice.",
    "",
    "REIMS EN RAGE",
    "",
    "PRESS START pour continuer."
};

static void drawImage(const Image *img)
{
    PAL_setPalette(PAL1, img->palette->data, DMA);
    VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, tileIndex),
                    0, 0, FALSE, TRUE);
    tileIndex += img->tileset->numTile;
}

static void showIntro(void)
{
    tileIndex = TILE_USER_INDEX;
    introFrames = 0;

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    drawImage(&bg_intro);

    // Texte posé bas, on affichera un scroll simple
    s16 baseY = 22;
    for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        VDP_drawText(intro_lines[i], 3, baseY + (s16)i*2);

    VDP_drawText("PRESS START", 12, 26);
}

static void showTitle(void)
{
    tileIndex = TILE_USER_INDEX;
    VDP_clearPlane(BG_A, TRUE); VDP_clearPlane(BG_B, TRUE);
    drawImage(&bg_title);
    VDP_drawText("PRESS START", 12, 20);
}

static void startGame(void)
{
    state = STATE_GAME;
    gameFrames = 0;

    tileIndex = TILE_USER_INDEX;
    VDP_clearPlane(BG_A, TRUE); VDP_clearPlane(BG_B, TRUE);
    drawImage(&stage1);

    SPR_init();
    PAL_setPalette(PAL2, hero1.palette->data, DMA);
    PAL_setPalette(PAL3, hero2.palette->data, DMA);

    p1x = FIX16(60);  p1y = FIX16(140);
    p2x = FIX16(180); p2y = FIX16(140);

    p1 = SPR_addSprite(&hero1, F16_toInt(p1x), F16_toInt(p1y), TILE_ATTR(PAL2, 0, FALSE, FALSE));
    p2 = SPR_addSprite(&hero2, F16_toInt(p2x), F16_toInt(p2y), TILE_ATTR(PAL3, 0, FALSE, FALSE));
}

static void updateIntro(void)
{
    introFrames++;

    // Scroll text léger (toutes les ~0.5s)
    if ((introFrames % 30) == 0)
    {
        VDP_clearText(0, 0, 40);
        VDP_clearText(0, 2, 40);
        s16 shift = -(introFrames / 30);
        s16 baseY = 22 + shift;
        for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        {
            s16 y = baseY + (s16)i*2;
            if (y >= 0 && y < 28) VDP_drawText(intro_lines[i], 3, y);
        }
        VDP_drawText("PRESS START", 12, 26);
    }

    // START ou timeout auto (3 s pour test ; remets 90*60 plus tard)
    if ((joy & BUTTON_START) || introFrames > (3*60))
    {
        state = STATE_TITLE;
        showTitle();
    }
}

static void updateTitle(void)
{
    if (joy & BUTTON_START)
        startGame();
}

static void updateGame(void)
{
    gameFrames++;

    s16 vx = 0;
    if (joy & BUTTON_LEFT)  { vx = -1; SPR_setHFlip(p1, TRUE); }
    if (joy & BUTTON_RIGHT) { vx = +1; SPR_setHFlip(p1, FALSE); }

    p1x = FIX16(F16_toInt(p1x) + vx);
    SPR_setPosition(p1, F16_toInt(p1x), F16_toInt(p1y));

    // Anim basique: [2 idle][4 walk][2 punch]
    static u16 punchTimer = 0;
    if ((joy & BUTTON_B) && (punchTimer == 0)) punchTimer = 12;
    if (punchTimer) punchTimer--;

    u16 frame;
    if (punchTimer) frame = 2 + 4 + ((punchTimer > 6) ? 0 : 1);
    else if (vx)    frame = 2 + ((gameFrames >> 2) & 3);
    else            frame = (gameFrames >> 3) & 1;

    SPR_setFrame(p1, frame);
    SPR_setFrame(p2, (gameFrames >> 3) & 1);

    SPR_update();
}

static void joyHandler(u16 joyID, u16 changed, u16 state_)
{
    if (joyID == JOY_1) joy = state_;
}

int main(bool hard)
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
            case STATE_TITLE: updateTitle(); break;
            case STATE_GAME:  updateGame();  break;
        }
        SYS_doVBlankProcess();
    }
    return 0;
}
