#include <genesis.h>
#include "resources.h"

// ----------------------
// Etats du jeu
// ----------------------
typedef enum { ST_INTRO, ST_TITLE } State;
static State g_state = ST_INTRO;

// ----------------------
// Globals
// ----------------------
static u16 g_tileIndex;
static u16 g_joy;

// Texte d'intro (≈ 1 min 30 au total)
static const char* INTRO[] = {
    "Reims, 2025. La nuit tombe sur une ville en ruines.",
    "Depuis des annees, les trafics et la corruption",
    "ont devore chaque rue, chaque quartier.",
    "",
    "Jimmy et Houcine, justiciers des faubourgs,",
    "avaient nettoye les rues... avant d'etre arretes.",
    "",
    "Dix longues annees derriere les barreaux.",
    "Dix ans pendant lesquels les gangs ont prospere.",
    "",
    "Les dealers controlent les ecoles et les quartiers.",
    "Les flics ferment les yeux, achetes ou terrorises.",
    "Les politiciens sont complices jusqu'au sommet.",
    "",
    "La population vit dans la peur et la misere.",
    "Les cris resonnent dans les caves et les ruelles.",
    "",
    "Mais ce soir, tout va changer.",
    "",
    "Jimmy et Houcine sont libres.",
    "Leurs corps sont marques, mais leurs coeurs sont d'acier.",
    "Leur colere est intacte.",
    "",
    "Les rues de Reims vont trembler.",
    "Les gangs vont payer le prix du sang.",
    "",
    "REIMS EN RAGE"
};
#define INTRO_LINES (sizeof(INTRO)/sizeof(INTRO[0]))

// ----------------------
// Utils
// ----------------------
static void drawImageToPlane(const Image* img, VDPPlane plane, u16 pal, u16 x, u16 y)
{
    PAL_setPalette(pal, img->palette->data, DMA);
    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(pal, FALSE, FALSE, FALSE, g_tileIndex),
        x, y,
        FALSE,           // don't use DMA queue for map (we already DMA tiles)
        TRUE             // transfer tiles immediately (DMA)
    );
    g_tileIndex += img->tileset->numTile;
}

static void drawScene_3layers(const Image* sky, const Image* city, const Image* street)
{
    // Reset affichage
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    g_tileIndex = TILE_USER_INDEX;

    // On réserve une palette par calque (PAL0 / PAL1 / PAL2)
    drawImageToPlane(sky,    BG_B, PAL0, 0, 0);   // fond
    drawImageToPlane(city,   BG_A, PAL1, 0, 0);   // immeubles/murs
    drawImageToPlane(street, BG_A, PAL2, 0, 0);   // trottoir / 1er plan (transparence index 0)
}

// ----------------------
// Intro
// ----------------------
static void playIntro(void)
{
    u32 frame = 0;
    u16 blink = 0;

    // SCENE 1 (≈45 s)
    drawScene_3layers(&scene1_sky, &scene1_city, &scene1_street);
    for (frame = 0; frame < 45 * 60; frame++)
    {
        g_joy = JOY_readJoypad(JOY_1);
        if (g_joy & BUTTON_START) break;

        // Scroll du texte : 1 ligne toutes ~0.5 s (30 frames)
        if ((frame % 30) == 0)
        {
            s16 shift = -(frame / 30);      // nb de lignes remontees
            s16 baseY = 22 + shift;         // ligne de base

            // Efface zone texte
            for (u16 y = 16; y < 28; y++) VDP_clearText(0, y, 40);

            for (u16 i = 0; i < INTRO_LINES; i++)
            {
                s16 y = baseY + (s16)i * 2;
                if (y >= 0 && y < 28) VDP_drawText(INTRO[i], 2, y);
            }
        }

        // Petit clignotement "PRESS START"
        blink++;
        if ((blink & 31) == 0)
            VDP_drawText(((blink & 32) ? "            " : "PRESS START"), 12, 26);

        SYS_doVBlankProcess();
    }

    // START => skip directement la suite
    if (g_joy & BUTTON_START) return;

    // SCENE 2 (≈45 s)
    drawScene_3layers(&scene2_sky, &scene2_city, &scene2_street);
    for (frame = 0; frame < 45 * 60; frame++)
    {
        g_joy = JOY_readJoypad(JOY_1);
        if (g_joy & BUTTON_START) break;

        // Le texte continue de défiler (on repart du haut)
        if ((frame % 30) == 0)
        {
            s16 shift = -(frame / 30);
            s16 baseY = 22 + shift;
            for (u16 y = 16; y < 28; y++) VDP_clearText(0, y, 40);

            for (u16 i = 0; i < INTRO_LINES; i++)
            {
                s16 y = baseY + (s16)i * 2;
                if (y >= 0 && y < 28) VDP_drawText(INTRO[i], 2, y);
            }
        }

        blink++;
        if ((blink & 31) == 0)
            VDP_drawText(((blink & 32) ? "            " : "PRESS START"), 12, 26);

        SYS_doVBlankProcess();
    }
}

static void showTitle(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    VDP_drawText("REIMS EN RAGE", 10, 12);
    VDP_drawText("PRESS START",   12, 20);
}

// ----------------------
// Input
// ----------------------
static void joyHandler(u16 joyId, u16 changed, u16 state)
{
    if (joyId == JOY_1) g_joy = state;
}

// ----------------------
// main
// ----------------------
int main(bool hard)
{
    // Setup de base
    JOY_init();
    JOY_setEventHandler(joyHandler);
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    PAL_setColors(0, (u16*)palette_black, 64, DMA);

    // Intro
    playIntro();

    // Ecran titre
    g_state = ST_TITLE;
    showTitle();

    while (1)
    {
        // (pour l’instant, juste rester sur le titre jusqu’a START)
        if (JOY_readJoypad(JOY_1) & BUTTON_START)
        {
            VDP_drawText("TODO: START GAME", 10, 22);
        }
        SYS_doVBlankProcess();
    }
    return 0;
}
