#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            90
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Vitesse: 1 pixel / 30 frames (x30)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30

#define TEXT_PAL                 PAL2
#define TEXT_COLOR               0xFF7840  // orange
#define TEXT_BG                  0x000000
#define MAX_COLS                 40

static u16 nextTile;

static void waitFrames(u16 n){ while (n--) SYS_doVBlankProcess(); }

static void resetScene(void)
{
    VDP_setPlaneSize(64, 64, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);

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

static void drawCenteredLine(u16 y, const char* s)
{
    char buf[MAX_COLS + 1];
    u16 len = strlen(s);
    if (len > MAX_COLS) len = MAX_COLS;
    memcpy(buf, s, len);
    buf[len] = 0;

    s16 x = (MAX_COLS - (s16)len) / 2;
    if (x < 0) x = 0;
    VDP_drawText(buf, (u16)x, y);
}

static u16 drawWrappedBlock(u16 yStart, const char* const* lines, u16 count)
{
    char out[MAX_COLS + 1];
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

static void playIntro(void)
{
    resetScene();

    // musique
    XGM_startPlay(intro_music);

    // palette texte
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
    VDP_setTextPalette(TEXT_PAL);

    // 1) Fond 1
    drawFullImageOn(BG_B, &intro1, PAL0);

    // 2) On dessine TOUT le bloc plus bas que l'écran...
    //    L'écran fait 28 lignes (224 px / 8). La dernière ligne visible = y=27.
    const u16 yFirstLineOnPlane = 40;                 // on dessine la 1re ligne à y=40 (hors écran)
    drawWrappedBlock(yFirstLineOnPlane, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));

    // ...puis on règle le scroll pour que cette 1re ligne soit pile sur la ligne 27 (bas écran)
    s16 vscroll = -((s16)(yFirstLineOnPlane - 27) * 8);   // 8 px par ligne
    VDP_setVerticalScroll(BG_A, vscroll);

    // Boucle d'intro
    u32 frame = 0;
    while (frame < INTRO_FRAMES)
    {
        // change de fond sans reset pour ne PAS casser le scroll
        if (frame == (INTRO_FRAMES / 3))         drawFullImageOn(BG_B, &intro2, PAL0);
        else if (frame == (2 * INTRO_FRAMES / 3))drawFullImageOn(BG_B, &intro3, PAL0);

        // défilement: 1 px toutes 30 frames
        if ((frame % SCROLL_STEP_PERIOD) == 0)
        {
            vscroll += SCROLL_PIX_PER_STEP;      // contenu monte d'1 px
            VDP_setVerticalScroll(BG_A, vscroll);
        }

        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        SYS_doVBlankProcess();
        frame++;
    }
}

static void showTitle(void)
{
    resetScene();
    drawFullImageOn(BG_B, &title, PAL0);

    // Texte "PRESS START" clignotant (palette texte assurée)
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
    VDP_setTextPalette(TEXT_PAL);

    const char* pressStart = "PRESS START";
    u16 blink = 0;

    while (TRUE)
    {
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        bool on = ((blink / 30) % 2) == 0; // ~0,5 s
        if (on)
        {
            u16 len = strlen(pressStart);
            if (len > MAX_COLS) len = MAX_COLS;
            s16 x = (MAX_COLS - (s16)len) / 2; if (x < 0) x = 0;
            VDP_drawText(pressStart, (u16)x, 25);
        }
        else VDP_clearTextArea(0, 25, MAX_COLS, 1);

        VDP_waitVSync();
        blink++;
    }
}

int main(bool hardReset)
{
    (void)hardReset;
    JOY_init();

    playIntro();
    showTitle();

    while (TRUE) SYS_doVBlankProcess();
    return 0;
}
