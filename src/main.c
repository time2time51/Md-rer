#include <genesis.h>
#include "resources.h"   // intro1, intro2, intro3, title, intro_music

// ---------- Config ----------
#define INTRO_DURATION_FRAMES   (90 * 60)     // 90 secondes @60fps
#define INTRO_SEGMENT_FRAMES    (30 * 60)     // 30s par image
#define TEXT_SCROLL_PERIOD      30            // rafraîchit le scroll texte toutes les 30 frames (~0.5s)
#define PRESS_BLINK_PERIOD      20            // clignotement titre

typedef enum { STATE_INTRO, STATE_TITLE } GameState;
static GameState state = STATE_INTRO;

// Input
static u16 joy1_state = 0;

static void onJoy(u16 joy, u16 changed, u16 state)
{
    if (joy == JOY_1) joy1_state = state;
}

// Dessin d'une image plein écran sur un BG donné
static void drawFullImageOn(VDPPlane plane, const Image *img, u16 *ioTileIndex, u16 palIndex)
{
    // Palette de l'image -> palIndex (PAL0..PAL3)
    PAL_setPalette(palIndex, img->palette, DMA);

    // Dessin image (320x224) à (0,0)
    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, *ioTileIndex),
        0, 0,
        FALSE,     // use DMA for tiles
        TRUE       // use DMA for map
    );
    *ioTileIndex += img->tileset->numTile;
}

// Texte d'intro
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
    "REIMS EN RAGE"
};
#define INTRO_LINES_COUNT (sizeof(intro_lines)/sizeof(intro_lines[0]))

// Affiche l'intro (bloquant jusqu'au passage a l'ecran titre)
static void playIntro(void)
{
    // Vidéo setup
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();

    // Grandes tables de scroll pour être large
    VDP_setPlaneSize(64, 32, TRUE);

    // Nettoyage & fond noir
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    PAL_setColors(0, (u16*)palette_black, 64, DMA); // fond noir global au démarrage

    // Lance la musique XGM (intro_music vient des resources)
    XGM_startPlayResource(intro_music);

    // On met les images sur BG_B (fond), le texte sur BG_A (par-dessus)
    u16 tileIndexB = TILE_USER_INDEX;
    u16 palBG = PAL1; // on garde PAL0 pour du texte blanc si besoin

    // Affiche la première image
    drawFullImageOn(BG_B, &intro1, &tileIndexB, palBG);

    // Texte initial (posé en bas de l'écran)
    s16 baseY = 24; // lignes de texte partent du bas et vont remonter
    for (u16 i = 0; i < INTRO_LINES_COUNT; i++)
        VDP_drawText(intro_lines[i], 2, baseY + (s16)i*2);

    // Boucle d'intro
    u32 frames = 0;
    while (frames < INTRO_DURATION_FRAMES)
    {
        // Changement d'image toutes les 30s
        if (frames == INTRO_SEGMENT_FRAMES)
        {
            VDP_clearPlane(BG_B, TRUE);
            tileIndexB = TILE_USER_INDEX;
            drawFullImageOn(BG_B, &intro2, &tileIndexB, palBG);
        }
        else if (frames == (INTRO_SEGMENT_FRAMES*2))
        {
            VDP_clearPlane(BG_B, TRUE);
            tileIndexB = TILE_USER_INDEX;
            drawFullImageOn(BG_B, &intro3, &tileIndexB, palBG);
        }

        // Scroll du texte (toutes ~0.5s)
        if ((frames % TEXT_SCROLL_PERIOD) == 0)
        {
            VDP_clearPlane(BG_A, TRUE); // on réécrit uniquement le texte

            s16 shift = -(frames / TEXT_SCROLL_PERIOD); // 1 "ligne" toutes 0.5s
            s16 y0 = baseY + shift;

            for (u16 i = 0; i < INTRO_LINES_COUNT; i++)
            {
                s16 y = y0 + (s16)i*2;
                if (y >= 0 && y < 28)
                    VDP_drawText(intro_lines[i], 2, y);
            }
        }

        // Skip si START (aucun message affiché à l'écran)
        if (joy1_state & BUTTON_START) break;

        frames++;
        SYS_doVBlankProcess();
    }

    // Transition vers le titre : on ne touche pas à la musique (elle continue)
    state = STATE_TITLE;
}

static void showTitle(void)
{
    // BG clean
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    // Grande table
    VDP_setPlaneSize(64, 32, TRUE);

    // Affiche l'image de titre sur BG_A
    u16 tileIndexA = TILE_USER_INDEX;
    drawFullImageOn(BG_A, &title, &tileIndexA, PAL0);
}

static void runTitle(void)
{
    // “PRESS START” clignotant (on le place vers le bas, centré approximativement)
    static u16 blink = 0;
    blink++;

    if ((blink % (PRESS_BLINK_PERIOD*2)) < PRESS_BLINK_PERIOD)
        VDP_drawText("PRESS START", 12, 21);
    else
        VDP_clearText(12, 21, 12);

    // Quand START est presse, on pourrait enchaîner vers le menu / jeu
    // (placeholder : rien pour l’instant)
}

int main(void)
{
    // Input
    JOY_init();
    JOY_setEventHandler(onJoy);

    // Ecran noir
    PAL_setColors(0, (u16*)palette_black, 64, DMA);

    // Lance l'intro (bloquante, gère les images + défilement)
    playIntro();

    // Affiche l’ecran titre (la musique continue)
    showTitle();

    while (1)
    {
        switch (state)
        {
            case STATE_TITLE:
                runTitle();
                break;
            default:
                break;
        }
        SYS_doVBlankProcess();
    }

    return 0;
}
