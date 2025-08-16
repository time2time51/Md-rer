#include <genesis.h>
#include "resources.h"

typedef enum { STATE_INTRO, STATE_TITLE, STATE_GAME } GameState;
static GameState state = STATE_INTRO;
static u16 tileIndex;
static u16 joy;
static Sprite *p1, *p2;
static fix16 p1x, p1y, p2x, p2y;

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
    VDP_setPalette(PAL1, img->palette->data);
    VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, tileIndex),
                    0, 0, FALSE, TRUE);
    tileIndex += img->tileset->numTile;
}

static void showIntro()
{
    tileIndex = TILE_USERINDEX;
    drawImage(&bg_intro);
    s16 baseY = 24;
    for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        VDP_drawText(intro_lines[i], 4, baseY + (s16)i*2);
}

static void updateIntro()
{
    static u32 frames = 0; frames++;
    if ((frames % 30) == 0)
    {
        VDP_clearText(0, 0, 40);
        VDP_clearText(0, 2, 40);
        s16 shift = -(frames / 30);
        s16 baseY = 24 + shift;
        for (u16 i = 0; i < sizeof(intro_lines)/sizeof(intro_lines[0]); i++)
        {
            s16 y = baseY + (s16)i*2;
            if (y >= 0 && y < 28) VDP_drawText(intro_lines[i], 4, y);
        }
    }
    if (frames > (90*60) || (joy & BUTTON_START))
    {
        state = STATE_TITLE;
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
        tileIndex = TILE_USERINDEX;
        drawImage(&bg_title);
        VDP_drawText("PRESS START", 12, 20);
    }
}

static void startGame()
{
    state = STATE_GAME;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    tileIndex = TILE_USERINDEX;
    drawImage(&stage1);
    SPR_init();
    VDP_setPalette(PAL2, hero1.palette->data);
    VDP_setPalette(PAL3, hero2.palette->data);
    p1x = FIX16(60); p1y = FIX16(140);
    p2x = FIX16(180); p2y = FIX16(140);
    p1 = SPR_addSprite(&hero1, fix16ToInt(p1x), fix16ToInt(p1y), TILE_ATTR(PAL2, 0, FALSE, FALSE));
    p2 = SPR_addSprite(&hero2, fix16ToInt(p2x), fix16ToInt(p2y), TILE_ATTR(PAL3, 0, FALSE, FALSE));
}

static void updateTitle()
{
    if (joy & BUTTON_START) startGame();
}

static void updateGame()
{
    s16 vx = 0;
    if (joy & BUTTON_LEFT)  { vx = -1; SPR_setHFlip(p1, TRUE); }
    if (joy & BUTTON_RIGHT) { vx = +1; SPR_setHFlip(p1, FALSE); }
    p1x = FIX16(fix16ToInt(p1x) + vx);
    SPR_setPosition(p1, fix16ToInt(p1x), fix16ToInt(p1y));
    static u16 punchTimer = 0;
    if ((joy & BUTTON_B) && (punchTimer == 0)) { punchTimer = 12; }
    if (punchTimer > 0) punchTimer--;
    static u16 frame = 0;
    if (punchTimer > 0) frame = 2 + 4 + ((punchTimer > 6) ? 0 : 1);
    else if (vx != 0)   frame = 2 + ((systemFrameCounter >> 2) & 3);
    else                frame = (systemFrameCounter >> 3) & 1;
    SPR_setFrame(p1, frame);
    SPR_setFrame(p2, (systemFrameCounter >> 3) & 1);
    SPR_update();
}

static void joyHandler(u16 joyID, u16 changed, u16 state_)
{
    if (joyID == JOY_1) joy = state_;
}

int main()
{
    JOY_init();
    JOY_setEventHandler(joyHandler);
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPaletteColors(0, (u16*)palette_black, 64);
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
