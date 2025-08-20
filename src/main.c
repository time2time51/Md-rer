#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            90
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Scroll : 1 px / 10 frames  => ~6 px/s (3x plus rapide qu’avant)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       10       

#define TEXT_PAL                 PAL2     // palette pour le texte
#define TEXT_COLOR               0xFF0000 // rouge vif
#define TEXT_BG                  0x000000 // fond noir
#define MAX_COLS                 40       // largeur plane en caractères

// -----------------------------------------------------------------------------
// Etat commun
// -----------------------------------------------------------------------------
static u16 nextTile;

// -----------------------------------------------------------------------------
// Utilitaires
// -----------------------------------------------------------------------------
static void waitFrames(u16 n) { while (n--) SYS_doVBlankProcess(); }

static void resetScene(void)
{
    // Large plane pour beaucoup de lignes à scroller
    VDP_setPlaneSize(64, 64, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);

    // Texte sur BG_A, images sur BG_B
    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(0);
    VDP_setTextPalette(TEXT_PAL);

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    nextTile = TILE_USER_INDEX;
}

static void drawFullImageOn(VDPPlane plane, const Image* img, u16 palIndex)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);
    VDP_drawImageEx(plane, img, nextTile, 0, 0, FALSE, TRUE);
    nextTile += img->tileset->numTile;
}

// Tronque et centre une ligne (≤ 40 colonnes), sans dépassement
static void drawCenteredLine(u16 y, const char* s)
{
    char buf[ MAX_COLS + 1 ];
    u16 len = strlen(s);
    if (len > MAX_COLS) len = MAX_COLS;
    memcpy(buf, s, len);
    buf[len] = 0;

    s16 x = (MAX_COLS - (s16)len) / 2;
    if (x < 0) x = 0;
    VDP_drawText(buf, (u16)x, y);
}

// Wrap simple sur espaces à MAX_COLS colonnes, écrit directement sur le plane
static u16 drawWrappedBlock(u16 yStart, const char* const* lines, u16 count)
{
    char out[ MAX_COLS + 1 ];
    u16 y = yStart;

    for (u16 i = 0; i < count; i++)
    {
        const char* p = lines[i];
        while (*p)
        {
            u16 take = 0, lastSpace = 0;
            while (p[take] && take < MAX_COLS)
            {
                if (p[take] == ' ') lastSpace = take;
                take++;
            }
            if (p[take] && lastSpace) take = lastSpace;

            while (*p == ' ') p++;
            u16 real = take;
            while (real && p[real - 1] == ' ') real--;

            memcpy(out, p, real);
            out[real] = 0;

            drawCenteredLine(y++, out);

            p += take;
            while (*p == ' ') p++;
        }

        if (lines[i][0] == 0) y++;
    }
    return y - yStart;
}

// -----------------------------------------------------------------------------
// Contenu de l'intro (version longue condensée pour l’écran 40 colonnes)
// -----------------------------------------------------------------------------
static const char* intro_lines[] =
{
    "Reims, la nuit. La ville se meurt.",
    "La corruption et la drogue rongent les rues.",
    "Dix annees de taule pour Jimmy et Houcine.",
    "Deux meurtres, une seule loyaut e: la survie.",
    "",
    "Jimmy, 35 ans, 1m78 pour 70kg.",
    "Sec, taille en acier. Boxe anglaise.",
    "Poings propres, regard froid.",
    "",
    "Houcine, 40 ans, 1m85 pour 62kg.",
    "Sec comme une lame. Arts martiaux.",
    "Vitesse, precision, impact.",
    "",
    "Dehors, Reims est tombe. La nuit est roi.",
    "Place Drouet d'Erlon: alcool, deals, bagarres.",
    "Flics achetes, elus muets. La rue commande.",
    "",
    "Les dealers font la loi. Les faibles tombent.",
    "Les habitants baissent les yeux. Plus d'espoir.",
    "",
    "Mais ce soir, les portes s'ouvrent.",
    "Jimmy et Houcine sortent. Colere intacte.",
    "Dix ans a ruminer des noms.",
    "Dix ans a imaginer la note a payer.",
    "",
    "Ils ne parlent pas de justice.",
    "Ils parlent de solde, de sang, de dettes.",
    "Rue par rue. Visage par visage.",
    "",
    "Premiere cible: D'Erlon.",
    "La ville va entendre leurs pas.",
    "",
    "REIMS EN RAGE"
};

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    // Musique (XGM)
    XGM_startPlay(intro_music);

    // Couleurs pour le texte (fond + rouge vif)
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));

    // Image 1
    drawFullImageOn(BG_B, &intro1, PAL0);

    // On écrit le bloc dès le début (il sera scrolle)
    const u16 yTopMargin = 4;
    drawWrappedBlock(yTopMargin, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));

    // Scroll: part légerement hors ecran pour que le texte arrive du bas
    s16 vscroll = -32;
    u16 frame = 0;

    while (frame < INTRO_FRAMES)
    {
        // Changement d'image aux tiers
        if (frame == (INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro2, PAL0);
            drawWrappedBlock(yTopMargin, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));
        }
        else if (frame == (2 * INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro3, PAL0);
            drawWrappedBlock(yTopMargin, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));
        }

        // Avance le scroll (3x plus rapide qu’avant)
        if ((frame % SCROLL_STEP_PERIOD) == 0) vscroll += SCROLL_PIX_PER_STEP;
        VDP_setVerticalScroll(BG_A, vscroll);

        // Skip éventuel
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        SYS_doVBlankProcess();
        frame++;
    }
}

// -----------------------------------------------------------------------------
// Titre
// -----------------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();
    drawFullImageOn(BG_B, &title, PAL0);

    const char* pressStart = "PRESS START";
    u16 blink = 0;

    while (TRUE)
    {
        u16 j = JOY_readJoypad(JOY_1);
        if (j & BUTTON_START) break;

        // Clignote toutes ~0.5s
        bool on = ((blink / 30) % 2) == 0;
        if (on)
        {
            u16 len = strlen(pressStart);
            if (len > MAX_COLS) len = MAX_COLS;
            s16 x = (MAX_COLS - (s16)len) / 2; if (x < 0) x = 0;
            VDP_drawText(pressStart, (u16)x, 26);     // plus bas
        }
        else
        {
            VDP_clearTextArea(0, 26, MAX_COLS, 1);
        }

        VDP_waitVSync();
        blink++;
    }
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main(bool hardReset)
{
    (void)hardReset;
    JOY_init();

    playIntro();
    showTitle();

    while (TRUE) SYS_doVBlankProcess();
    return 0;
}
