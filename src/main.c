#include <genesis.h>
#include "resources.h"

Sprite *p1, *p2;
fix16 p1x = FIX16(40), p1y = FIX16(180);
fix16 p2x = FIX16(200), p2y = FIX16(180);
s16 vx = 0, vy = 0;
u16 tileIndex;
u8 gameStarted = 0;

// Fonction utilitaire pour afficher une image
static void drawImage(Image *img, u16 x, u16 y) {
    VDP_drawImage(BG_B, img, x, y);
    PAL_setPalette(PAL1, img->palette->data, DMA); // corrigé
}

// Intro
static void showIntro() {
    tileIndex = TILE_USER_INDEX; // corrigé (ex-TILE_USERINDEX)
    VDP_drawImageEx(BG_A, &bg_intro, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, tileIndex), 0, 0, FALSE, TRUE);
    PAL_setPalette(PAL1, bg_intro.palette->data, DMA);
}

static void updateIntro() {
    if (JOY_readJoypad(JOY_1) & BUTTON_START) {
        gameStarted = 1;
    }
}

// Démarrage du jeu
static void startGame() {
    tileIndex = TILE_USER_INDEX;
    VDP_drawImageEx(BG_A, &stage1, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, tileIndex), 0, 0, FALSE, TRUE);

    // Charger les palettes
    PAL_setPalette(PAL2, hero1.palette->data, DMA); // corrigé
    PAL_setPalette(PAL3, hero2.palette->data, DMA); // corrigé

    // Ajouter les sprites
    p1 = SPR_addSprite(&hero1, F16_toInt(p1x), F16_toInt(p1y), TILE_ATTR(PAL2, 0, FALSE, FALSE));
    p2 = SPR_addSprite(&hero2, F16_toInt(p2x), F16_toInt(p2y), TILE_ATTR(PAL3, 0, FALSE, FALSE));
    SPR_update();
}

// Update du jeu
static void updateGame() {
    u16 value = JOY_readJoypad(JOY_1);
    vx = 0;

    if (value & BUTTON_LEFT) vx = -2;
    else if (value & BUTTON_RIGHT) vx = 2;

    p1x = FIX16(F16_toInt(p1x) + vx); // corrigé
    SPR_setPosition(p1, F16_toInt(p1x), F16_toInt(p1y)); // corrigé

    u16 frame = 0;
    if (vx != 0) frame = 2 + ((getFrameCount() >> 2) & 3); // getFrameCount = remplace systemFrameCounter
    SPR_setFrame(p1, frame);

    SPR_update();
}

// Main
int main(bool hard) {
    // Effacer palettes
    PAL_setColors(0, (u16*)palette_black, 64, DMA); // corrigé

    SPR_init();

    showIntro();

    while(1) {
        if (!gameStarted) updateIntro();
        else updateGame();

        SPR_update();
        SYS_doVBlankProcess();
    }

    return 0;
}
