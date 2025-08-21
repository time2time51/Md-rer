#include <genesis.h>
#include <string.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            252
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Scroll intro (inchangé)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30

// Texte ROUGE
#define TEXT_PAL                 PAL3          // <-- texte sur la même banque que le logo
#define TEXT_COLOR               0xFF0000
#define TEXT_BG                  0x000000
#define MAX_COLS                 40

// lignes basses
#define TEXT_FIRST_VISIBLE_ROW   27
#define PRESS_START_ROW          27

// ---------------------- Titre : placements (en tuiles 8x8) -------------------
#define JIMMY_X   4
#define JIMMY_Y   9
#define HOUCINE_X 24
#define HOUCINE_Y 9
#define LOGO_Y    2

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

    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(0);
    VDP_setTextPalette(TEXT_PAL);

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    nextTile = TILE_USER_INDEX;
}

static void applyTextColors(void)
{
    // Assure fond noir et lettres rouges pour la police SGDK
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));     // transparence
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));  // couleur texte
    PAL_setColor(TEXT_PAL * 16 + 15, RGB24_TO_VDPCOLOR(TEXT_COLOR)); // parfois utilisé
}

// Dessine une image n'importe où (coordonnées en tuiles)
static void drawImageAt(VDPPlane plane, const Image* img, u16 palIndex, u16 x, u16 y)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);
    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        x, y,
        FALSE,
        TRUE
    );
    nextTile += img->tileset->numTile;
}

// Largeur en tuiles (pour centrer)
static u16 imageWidthTiles(const Image* img)
{
    // tilemap->w est la largeur en tuiles dans SGDK
    return img->tilemap->w;
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

// Wrap simple sur MAX_COLS
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
// Script en 3 segments
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Segment d'intro
// -----------------------------------------------------------------------------
static bool runIntroSegment(const Image* img, const char* const* lines, u16 count, u16 frames)
{
    resetScene();

    applyTextColors();

    // fond segment (image plein ecran) sur PAL0
    drawImageAt(BG_B, img, PAL0, 0, 0);

    // texte : première ligne en bas
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

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    XGM_startPlay(intro_music);
    const u16 segFrames = INTRO_FRAMES / 3;

    if (runIntroSegment(&intro1, intro_seg1, INTRO_SEG1_COUNT, segFrames)) return;
    if (runIntroSegment(&intro2, intro_seg2, INTRO_SEG2_COUNT, segFrames)) return;
    if (runIntroSegment(&intro3, intro_seg3, INTRO_SEG3_COUNT, segFrames)) return;
}

// -----------------------------------------------------------------------------
// Ecran titre composite (4 palettes distinctes)
// -----------------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();

    // 1) Fond dans PAL0 sur BG_B (derriere tout)
    drawImageAt(BG_B, &title_bg, PAL0, 0, 0);

    // 2) Personnages et logo sur BG_A (par-dessus) avec leurs palettes dédiées
    drawImageAt(BG_A, &jimmy,   PAL1, JIMMY_X,   JIMMY_Y);
    drawImageAt(BG_A, &houcine, PAL2, HOUCINE_X, HOUCINE_Y);

    // Centrage horizontal du logo
    u16 wLogo = imageWidthTiles(&logo);
    s16 logoX = (40 - (s16)wLogo) / 2;
    if (logoX < 0) logoX = 0;
    drawImageAt(BG_A, &logo, PAL3, (u16)logoX, LOGO_Y);

    // 3) Texte "PRESS START" au-dessus
    VDP_setTextPriority(1);       // texte au-dessus des images BG_A/B
    VDP_setTextPalette(TEXT_PAL); // même banque que le logo
    applyTextColors();
    VDP_setVerticalScroll(BG_A, 0);

    const char* pressStart = "PRESS START";
    u16 blink = 0;

    while (TRUE)
    {
        u16 j = JOY_readJoypad(JOY_1);
        if (j & BUTTON_START) break;

        bool on = ((blink / 30) % 2) == 0;
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
