#include <genesis.h>
#include <string.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            252
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// vitesse scroll intro
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30

#define TEXT_PAL                 PAL2
#define TEXT_COLOR               0xFF0000   // ROUGE vif pour le texte
#define TEXT_BG                  0x000000
#define MAX_COLS                 40

// lignes basses
#define TEXT_FIRST_VISIBLE_ROW   27
#define PRESS_START_ROW          27

// -----------------------------------------------------------------------------
// Etat commun
// -----------------------------------------------------------------------------
static u16 nextTile;

// -----------------------------------------------------------------------------
// Utilitaires
// -----------------------------------------------------------------------------
static void resetScene(void)
{
    VDP_setPlaneSize(64, 64, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);

    // texte sur BG_A
    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(0);
    VDP_setTextPalette(TEXT_PAL);

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    // IMPORTANT: on repart du premier index utilisateur
    nextTile = TILE_USER_INDEX;
}

static void drawFullImageOn(VDPPlane plane, const Image* img, u16 palIndex)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);
    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        0, 0,
        FALSE,
        TRUE
    );
    nextTile += img->tileset->numTile;
}

// Image arbitraire en (xTile,yTile) avec palette / priorité plan
static void drawImageAt(VDPPlane plane, const Image* img, u16 palIndex, u16 xTile, u16 yTile, bool priority)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);
    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(palIndex, priority ? TRUE : FALSE, FALSE, FALSE, nextTile),
        xTile, yTile,
        FALSE,
        TRUE
    );
    nextTile += img->tileset->numTile;
}

// Tronque/centre ≤ 40 colonnes
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

// Wrap simple sur MAX_COLS, écrit ligne par ligne
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

// -----------------------------------------------------------------------------
// Script intro : 3 segments
// -----------------------------------------------------------------------------

// Segment 1 – La ville (intro1)
static const char* intro_seg1[] =
{
    "Reims, la nuit. La ville suffoque.",
    "Les bars ne ferment jamais.",
    "Les terrasses de Drouet-d'Erlon brillent de neon.",
    "Mais derriere les verres d'alcool, la peur rode.",
    "La drogue coule a flot, la corruption est partout.",
    "Les flics regardent ailleurs. Les juges encaissent.",
    "Les habitants survivent dans une prison a ciel ouvert.",
    ""
};
static const u16 INTRO_SEG1_COUNT = sizeof(intro_seg1)/sizeof(intro_seg1[0]);

// Segment 2 – Jimmy + Houcine (intro2)
static const char* intro_seg2[] =
{
    "Jimmy, 35 ans.",
    "Un boxeur taille pour encaisser et rendre coup pour coup.",
    "Son corps sec et nerveux a ete forge dans la rage.",
    "Il a appris a frapper comme on respire.",
    "Sa colere a grandi derriere les murs de la prison.",
    "",
    "Houcine, 40 ans.",
    "Sec, rapide, precis comme une lame.",
    "Ses poings et ses pieds parlent le langage de Bruce Lee.",
    "Dix ans enferme n'ont pas casse son corps.",
    "Ils ont durci son esprit.",
    ""
};
static const u16 INTRO_SEG2_COUNT = sizeof(intro_seg2)/sizeof(intro_seg2[0]);

// Segment 3 – Leur passe + La vengeance (intro3)
static const char* intro_seg3[] =
{
    "Ensemble, ils ont connu la haine.",
    "Ensemble, ils ont paye le prix du sang.",
    "Un meurtre les a condamnes a dix ans de nuit.",
    "Mais derriere les barreaux, leur rage n'a jamais faibli.",
    "",
    "Aujourd'hui, les portes s'ouvrent.",
    "La nuit les attend.",
    "Les gangs, les dealers, les politiciens corrompus.",
    "Tous vont gouter a leur retour.",
    "",
    "Reims est pourrie.",
    "Mais Jimmy et Houcine sont pires.",
    "",
    "Et la ville va saigner.",
    "",
    "REIMS EN RAGE"
};
static const u16 INTRO_SEG3_COUNT = sizeof(intro_seg3)/sizeof(intro_seg3[0]);

// Couleurs texte
static inline void applyTextColors(void)
{
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
    PAL_setColor(TEXT_PAL * 16 + 15, RGB24_TO_VDPCOLOR(TEXT_COLOR));
}

// Un segment d’intro (image + texte scrollé)
static bool runIntroSegment(const Image* img, const char* const* lines, u16 count, u16 frames)
{
    resetScene();

    applyTextColors();

    // image
    drawFullImageOn(BG_B, img, PAL0);

    // texte en bas
    const u16 yStart = TEXT_FIRST_VISIBLE_ROW;
    drawWrappedBlock(yStart, lines, count);

    // scroll vers le haut
    s16 vscroll = 0;
    VDP_setVerticalScroll(BG_A, vscroll);

    for (u16 f = 0; f < frames; f++)
    {
        if ((f % SCROLL_STEP_PERIOD) == 0)
        {
            vscroll += SCROLL_PIX_PER_STEP;
            VDP_setVerticalScroll(BG_A, vscroll);
        }

        if (JOY_readJoypad(JOY_1) & BUTTON_START) return true;

        SYS_doVBlankProcess();
    }
    return false;
}

// Enchaîne les 3 segments
static void playIntro(void)
{
    XGM_startPlay(intro_music);
    const u16 segFrames = INTRO_FRAMES / 3;

    if (runIntroSegment(&intro1, intro_seg1, INTRO_SEG1_COUNT, segFrames)) return;
    if (runIntroSegment(&intro2, intro_seg2, INTRO_SEG2_COUNT, segFrames)) return;
    if (runIntroSegment(&intro3, intro_seg3, INTRO_SEG3_COUNT, segFrames)) return;
}

// -----------------------------------------------------------------------------
// Ecran Titre (fond + compositing Jimmy/Houcine/Logo)
// -----------------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();

    // 1) Fond (BG_B / PAL0)
    drawFullImageOn(BG_B, &title_bg, PAL0);

    // 2) Avant-plan (BG_A)
    const u16 screenTilesW = 40;
    const u16 bottomLine   = 25; // laisse 2 lignes pour "PRESS START"

    // tailles en tuiles (Image -> tilemap w/h)
    const u16 logoW    = logo.tilemap->w;
    const u16 logoH    = logo.tilemap->h;   (void)logoH;
    const u16 jimmyW   = jimmy.tilemap->w;
    const u16 jimmyH   = jimmy.tilemap->h;
    const u16 houcineW = houcine.tilemap->w;
    const u16 houcineH = houcine.tilemap->h;

    // LOGO centré
    u16 x_logo = (screenTilesW - logoW) / 2;
    u16 y_logo = 2;

    // JIMMY à gauche
    u16 x_jimmy = 4;
    u16 y_jimmy = (bottomLine >= jimmyH) ? (bottomLine - jimmyH) : 0;

    // HOUCINE à droite
    u16 x_houcine = (screenTilesW - houcineW - 4);
    u16 y_houcine = (bottomLine >= houcineH) ? (bottomLine - houcineH) : 0;

    // Dessin (priorité TRUE pour passer au-dessus du BG)
    drawImageAt(BG_A, &jimmy,   PAL1, x_jimmy,   y_jimmy,   TRUE);
    drawImageAt(BG_A, &houcine, PAL2, x_houcine, y_houcine, TRUE);
    drawImageAt(BG_A, &logo,    PAL3, x_logo,    y_logo,    TRUE);

    // 3) Texte "PRESS START" par-dessus
    VDP_setTextPriority(1);
    applyTextColors();
    VDP_setVerticalScroll(BG_A, 0);

    const char* pressStart = "PRESS START";
    u16 blink = 0;

    while (TRUE)
    {
        u16 j = JOY_readJoypad(JOY_1);
        if (j & BUTTON_START) break;

        bool on = ((blink / 30) % 2) == 0; // ~0,5 s
        if (on)
        {
            u16 len = strlen(pressStart);
            if (len > MAX_COLS) len = MAX_COLS;
            s16 x = (MAX_COLS - (s16)len) / 2; if (x < 0) x = 0;
            VDP_drawText(pressStart, (u16)x, PRESS_START_ROW);
        }
        else
        {
            VDP_clearTextArea(0, PRESS_START_ROW, MAX_COLS, 1);
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
