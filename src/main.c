#include <genesis.h>
#include <string.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            252
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Scroll intro
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30

// Texte
#define TEXT_PAL                 PAL2
#define TEXT_COLOR               0xFF0000   // rouge
#define TEXT_BG                  0x000000   // fond noir
#define MAX_COLS                 40
#define TEXT_FIRST_VISIBLE_ROW   27         // première ligne visible en bas
#define PRESS_START_ROW          27         // "PRESS START" tout en bas

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

    // texte par défaut sur BG_A
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

// Texte
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

// -----------------------------------------------------------------------------
// Script d'intro (3 segments)
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

static inline void applyTextColors(void)
{
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
    PAL_setColor(TEXT_PAL * 16 + 15, RGB24_TO_VDPCOLOR(TEXT_COLOR));
}

// -----------------------------------------------------------------------------
// Lecture d'un segment
// -----------------------------------------------------------------------------
static bool runIntroSegment(const Image* img, const char* const* lines, u16 count, u16 frames)
{
    resetScene();

    applyTextColors();

    // image de fond du segment
    drawFullImageOn(BG_B, img, PAL0);

    // texte : première ligne placée en bas
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
// Intro (enchaîne les 3 segments)
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
// Titre (fond = IMAGE, logo+persos = SPRITES)
// -----------------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();

    // 1) Fond plein écran (PAL0)
    drawFullImageOn(BG_B, &title_bg, PAL0);

    // 2) Système sprite
    SPR_init(0, 0, 0);

    // 3) Palettes sprites
    // Chaque PNG a <=16 couleurs -> on isole par palette pour éviter les conflits
    PAL_setPalette(PAL1, jimmy.palette->data, DMA);
    PAL_setPalette(PAL2, houcine.palette->data, DMA);
    PAL_setPalette(PAL3, logo.palette->data, DMA);

    // 4) Ajout des sprites (+ positions choisies pour tes images actuelles)
    //    - Jimmy à gauche, légèrement en bas
    //    - Houcine à droite
    //    - Logo centré en haut
    // NOTE: adapte si besoin, mais ces valeurs cadrent bien en 320x224.
    const s16 x_jimmy   =  16;
    const s16 y_jimmy   =  84;

    const s16 x_houcine = 188;
    const s16 y_houcine =  76;

    // Centrage simple du logo en supposant ~256 px de large ; ajuste si besoin
    const s16 x_logo    =  (320 - 256) / 2;  // ~32
    const s16 y_logo    =  12;

    Sprite* sprJimmy   = SPR_addSprite(&jimmy,   x_jimmy,   y_jimmy,   TILE_ATTR(PAL1, 0, FALSE, FALSE));
    Sprite* sprHoucine = SPR_addSprite(&houcine, x_houcine, y_houcine, TILE_ATTR(PAL2, 0, FALSE, FALSE));
    Sprite* sprLogo    = SPR_addSprite(&logo,    x_logo,    y_logo,    TILE_ATTR(PAL3, 0, FALSE, FALSE));

    // 5) Texte "PRESS START" par-dessus (BG_A)
    VDP_setTextPriority(1);
    applyTextColors();
    VDP_setVerticalScroll(BG_A, 0);

    const char* pressStart = "PRESS START";
    u16 blink = 0;

    while (TRUE)
    {
        u16 j = JOY_readJoypad(JOY_1);
        if (j & BUTTON_START) break;

        // Blink
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

        // Update sprites (obligatoire à chaque frame)
        SPR_update();

        VDP_waitVSync();
        blink++;
    }

    // Optionnel : nettoyer les sprites
    SPR_releaseSprite(sprJimmy);
    SPR_releaseSprite(sprHoucine);
    SPR_releaseSprite(sprLogo);
    SPR_end();
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
