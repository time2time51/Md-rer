#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            90
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Scroll doux : 1 px / 30 frames ≃ 2 px/s -> beaucoup plus lent (x10)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30       

#define TEXT_PAL                 PAL2     // palette pour le texte
#define TEXT_COLOR               0xFF7840 // orange lisible
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
// Contenu de l'intro
// -----------------------------------------------------------------------------
static const char* intro_lines[] =
{
    "Reims, la nuit. La ville se meurt.",
    "La corruption et la drogue rongent les rues.",
    "Cela fait 10 ans que Jimmy et Houcine",
    "sont tombes et ont disparu derriere les barreaux.",
    "Depuis, les gangs regnent en maitre.",
    "La police est corrompue, les politiciens achetes.",
    "Les dealers font la loi dans les quartiers.",
    "Les habitants n'ont plus d'espoir.",
    "",
    "Mais ce soir...",
    "Jimmy et Houcine sortent de prison.",
    "Leur colere est intacte.",
    "Ils vont faire payer ceux qui ont detruit leur ville.",
    "",
    "REIMS EN RAGE"
};

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    XGM_startPlay(intro_music);

    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));

    drawFullImageOn(BG_B, &intro1, PAL0);

    const u16 yTopMargin = 4;
    drawWrappedBlock(yTopMargin, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));

    s16 vscroll = -32;
    u16 frame = 0;

    while (frame < INTRO_FRAMES)
    {
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

        if ((frame % SCROLL_STEP_PERIOD) == 0) vscroll += SCROLL_PIX_PER_STEP;
        VDP_setVerticalScroll(BG_A, vscroll);

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

        bool on = ((blink / 30) % 2) == 0; // clignote toutes ~0.5s
        if (on)
        {
            u16 len = strlen(pressStart);
            if (len > MAX_COLS) len = MAX_COLS;
            s16 x = (MAX_COLS - (s16)len) / 2; if (x < 0) x = 0;
            VDP_drawText(pressStart, (u16)x, 25);
        }
        else
        {
            VDP_clearTextArea(0, 25, MAX_COLS, 1);
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
