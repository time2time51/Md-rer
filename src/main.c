#include <genesis.h>
#include "resources.h"

typedef enum { STATE_INTRO, STATE_TITLE, STATE_GAME } GameState;

static GameState state = STATE_INTRO;
static u16 tileIndex;
static u16 joy;
static Sprite *p1, *p2;
static fix16 p1x, p1y, p2x, p2y;

// Compteurs locaux
static u32 introFrames = 0;
static u32 gameFrames  = 0;

// Texte d'intro (défilement simple)
static const char *intro_lines[] = {
    "Reims, la nuit. La ville se meurt.",
    "La corruption et la drogue rongent les rues.",
    "Deux hommes decides a tout reprendre...",
    "Ils sortent de l'ombre pour faire justice.",
    "",
    "REIMS EN RAGE",
    "",
    "Appuie sur START pour passer l'intro."
};

static void drawImage(const Image *img)
{
    PAL_setPalette(PAL1, img->palette->data); // (ex-VDP_setPalette)
    VDP_drawImageEx(
        BG_A,
        img,
        TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, tileIndex),
        0, 0, FALSE, TRUE
    );
    tileIndex += img->tileset->numTile;
}

static void showIntro(void)
{
    tileIndex = TILE_USER_INDEX;   // (ex-TILE_USERINDEX)
    drawImage(&bg_intro);

    // Pose le texte d'intro (ligne de base = 24)
    s16 baseY = 24;
    for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        VDP_drawText(intro_lines[i], 4, baseY + (s16)i*2);
}

static void updateIntro(void)
{
    introFrames++;

    // Scroll du texte: toutes les ~0.5s (30 frames) on remonte d'une ligne
    if ((introFrames % 30) == 0)
    {
        VDP_clearText(0, 0, 40);
        VDP_clearText(0, 2, 40);

        s16 shift = -(introFrames / 30);
        s16 baseY = 24 + shift;

        for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        {
            s16 y = baseY + (s16)i*2;
            if (y >= 0 && y < 28) VDP_drawText(intro_lines[i], 4, y);
        }
    }

    // Après 90s ou si START -> ecran titre
    if (introFrames > (90*60) || (joy & BUTTON_START))
    {
        state = STATE_TITLE;
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);

        tileIndex = TILE_USER_INDEX;
        drawImage(&bg_title);
        VDP_drawText("PRESS START", 12, 20);
    }
}

static void startGame(void)
{
    state = STATE_GAME;
    gameFrames = 0;

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    tileIndex = TILE_USER_INDEX;
    drawImage(&stage1);

    SPR_init();

    // Palettes de sprites
    PAL_setPalette(PAL2, hero1.palette->data);  // (ex-VDP_setPalette)
    PAL_setPalette(PAL3, hero2.palette->data);

    // Positions de depart
    p1x = FIX16(60);  p1y = FIX16(140);
    p2x = FIX16(180); p2y = FIX16(140);

    p1 = SPR_addSprite(&hero1, F16_toInt(p1x), F16_toInt(p1y), TILE_ATTR(PAL2, 0, FALSE, FALSE));
    p2 = SPR_addSprite(&hero2, F16_toInt(p2x), F16_toInt(p2y), TILE_ATTR(PAL3, 0, FALSE, FALSE));
}

static void updateTitle(void)
{
    if (joy & BUTTON_START) startGame();
}

static void updateGame(void)
{
    gameFrames++;

    // P1: gauche/droite pour bouger, B pour punch
    s16 vx = 0;
    if (joy & BUTTON_LEFT)  { vx = -1; SPR_setHFlip(p1, TRUE); }
    if (joy & BUTTON_RIGHT) { vx = +1; SPR_setHFlip(p1, FALSE); }

    // p1x += vx (fix16)
    p1x = FIX16(F16_toInt(p1x) + vx);
    SPR_setPosition(p1, F16_toInt(p1x), F16_toInt(p1y));

    // Petit punch visuel en changeant de frame pendant quelques ticks
    static u16 punchTimer = 0;
    if ((joy & BUTTON_B) && (punchTimer == 0)) punchTimer = 12;
    if (punchTimer > 0) punchTimer--;

    // Animation: [2 idle][4 walk][2 punch] sur une seule ligne de spritesheet
    static u16 frame = 0;
    if (punchTimer > 0) frame = 2 + 4 + ((punchTimer > 6) ? 0 : 1);
    else if (vx != 0)   frame = 2 + ((gameFrames >> 2) & 3); // marche
    else                frame = (gameFrames >> 3) & 1;        // idle

    SPR_setFrame(p1, frame);

    // P2: animation idle tranquille
    SPR_setFrame(p2, (gameFrames >> 3) & 1);

    SPR_update();
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
    PAL_setColors(0, (u16*)palette_black, 64);  // (ex-VDP_setPaletteColors)

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
