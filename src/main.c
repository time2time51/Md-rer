#include <genesis.h>
#include <string.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            252
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// vitesse (actuelle)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30

#define TEXT_PAL                 PAL2
#define TEXT_COLOR               0xFF0000   // ROUGE
#define TEXT_BG                  0x000000   // noir
#define MAX_COLS                 40

// lignes basses
#define TEXT_FIRST_VISIBLE_ROW   27   // première ligne visible tout en bas
#define PRESS_START_ROW          27   // "PRESS START" tout en bas

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
    VDP_setTextPriority(0);          // on remettra à 1 pour l'écran titre
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
// Script complet, découpé en 3 segments (une image = un segment)
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

// -----------------------------------------------------------------------------
// Util: applique le fond + rouge aux indices utiles de la palette texte
// -----------------------------------------------------------------------------
static inline void applyTextColors(void)
{
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));     // fond
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));  // glyphes
    PAL_setColor(TEXT_PAL * 16 + 15, RGB24_TO_VDPCOLOR(TEXT_COLOR)); // sécurité
}

// -----------------------------------------------------------------------------
// Lecture d'un segment (image + bloc de texte) avec scroll
// Retourne true si START est pressé (pour skipper toute l'intro)
// -----------------------------------------------------------------------------
static bool runIntroSegment(const Image* img, const char* const* lines, u16 count, u16 frames)
{
    resetScene();

    applyTextColors();

    // image
    drawFullImageOn(BG_B, img, PAL0);

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
// Intro (enchaîne les 3 segments)
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    // musique une seule fois
    XGM_startPlay(intro_music);

    const u16 segFrames = INTRO_FRAMES / 3;

    if (runIntroSegment(&intro1, intro_seg1, INTRO_SEG1_COUNT, segFrames)) return;
    if (runIntroSegment(&intro2, intro_seg2, INTRO_SEG2_COUNT, segFrames)) return;
    if (runIntroSegment(&intro3, intro_seg3, INTRO_SEG3_COUNT, segFrames)) return;
}

// -----------------------------------------------------------------------------
// Titre (BG_B = ruelle ; sprites : Jimmy, Houcine, logo)
// -----------------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();

    // BG : fond ruelle (PAL0)
    drawFullImageOn(BG_B, &title_bg, PAL0);

    // Texte par-dessus tout
    VDP_setTextPriority(1);
    applyTextColors();
    VDP_setVerticalScroll(BG_A, 0);

    // --- SPRITES ---
    // IMPORTANT: on charge explicitement les palettes des sprites
    // (chaque res SPRITE a sa propre palette).
    PAL_setPalette(PAL1, jimmy.palette->data, DMA);
    PAL_setPalette(PAL2, houcine.palette->data, DMA);
    PAL_setPalette(PAL3, logo.palette->data, DMA);

    // Initialiser l’engine sprite (SGDK v2.11 : sans arguments)
    SPR_init();

    // Positions ajustées à partir de tes PNG actuels:
    // (Chaque PNG fait 320x224 avec transparence autour.
    //  On les superpose en x=0, puis on affine avec de petits offsets.)
    s16 x_jimmy   = -6;   s16 y_jimmy   =  12;   // très léger décalage gauche & bas
    s16 x_houcine =  6;   s16 y_houcine =  10;   // très léger décalage droite & bas
    s16 x_logo    =  0;   s16 y_logo    =  -6;   // un chouïa plus haut

    Sprite* sprJimmy   = SPR_addSprite(&jimmy,   x_jimmy,   y_jimmy,   TILE_ATTR(PAL1, 0, FALSE, FALSE));
    Sprite* sprHoucine = SPR_addSprite(&houcine, x_houcine, y_houcine, TILE_ATTR(PAL2, 0, FALSE, FALSE));
    Sprite* sprLogo    = SPR_addSprite(&logo,    x_logo,    y_logo,    TILE_ATTR(PAL3, 0, FALSE, FALSE));

    // On s’assure que le logo passe au-dessus (ordre = priorités logiques)
    // En pratique, l’engine respecte l’ordre d’ajout ; si besoin :
    SPR_setDepth(sprJimmy,   3);  // derrière
    SPR_setDepth(sprHoucine, 2);  // milieu
    SPR_setDepth(sprLogo,    1);  // devant

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
            VDP_drawText(pressStart, (u16)x, PRESS_START_ROW); // tout en bas
        }
        else
        {
            VDP_clearTextArea(0, PRESS_START_ROW, MAX_COLS, 1);
        }

        // Update sprites chaque frame
        SPR_update();

        VDP_waitVSync();
        blink++;
    }

    // Nettoyage sprites (libère VRAM liée aux sprites)
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
