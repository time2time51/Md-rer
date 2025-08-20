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

// IMPORTANT: on réserve PAL3 pour le TEXTE (et aussi pour le LOGO).
// On ne modifie que les entrées 0/1/15 de PAL3 pour ne pas détruire les couleurs du logo.
#define TEXT_PAL                 PAL3
#define TEXT_COLOR               0xFF0000   // ROUGE vif
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
// Utilitaires de base
// -----------------------------------------------------------------------------
static void resetScene(void)
{
    VDP_setPlaneSize(64, 64, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);

    // texte sur BG_A
    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(0);          // on passera à 1 pour l'écran titre
    VDP_setTextPalette(TEXT_PAL);

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    nextTile = TILE_USER_INDEX;
}

static void drawImageAtTiles(VDPPlane plane, const Image* img, u16 palIndex, u16 tx, u16 ty)
{
    // on charge la palette dédiée (si différente, la suivante l’écrasera si on réutilise le même palIndex)
    PAL_setPalette(palIndex, img->palette->data, DMA);

    // dessine avec attributs (palette + index de tuile)
    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        tx, ty,
        FALSE,      // NO loadpal (on gère nous-mêmes)
        TRUE        // DMA
    );
    nextTile += img->tileset->numTile;
}

static void drawFullImageOn(VDPPlane plane, const Image* img, u16 palIndex)
{
    drawImageAtTiles(plane, img, palIndex, 0, 0);
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
// Script découpé en 3 segments (une image = un segment)
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
// Couleurs texte: on ne touche qu'aux entrées 0/1/15 du TEXT_PAL (PAL3)
// -----------------------------------------------------------------------------
static inline void applyTextColors(void)
{
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));    // fond (transparent pour la fonte)
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR)); // texte
    PAL_setColor(TEXT_PAL * 16 + 15, RGB24_TO_VDPCOLOR(TEXT_COLOR));// variante utilisée par la fonte SGDK
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
    drawFullImageOn(BG_B, img, PAL0); // intro en PAL0

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
// Titre (BG composite: title_bg + jimmy + houcine + logo) + PRESS START rouge
// -----------------------------------------------------------------------------
// Calage générique: calcule x centré en tuiles pour une Image
static u16 centeredXInTiles(const Image* img)
{
    // Largeur plane = 40 tuiles. Largeur image en tuiles = img->tilemap->w
    u16 w = img->tilemap->w;
    s16 x = (40 - (s16)w) / 2;
    return (x < 0) ? 0 : (u16)x;
}

static void showTitle(void)
{
    resetScene();

    // --- Palettes / plan ---
    // PAL0 : background (title_bg)
    // PAL1 : jimmy
    // PAL2 : houcine
    // PAL3 : logo (+ texte, en modifiant uniquement 0/1/15)
    drawFullImageOn(BG_B, &title_bg, PAL0);

    // Dessus du BG : on colle les layers en BG_B aussi
    // Jimmy à gauche (x=3), aligné plutôt bas (y=12)
    drawImageAtTiles(BG_B, &jimmy, PAL1, 3, 12);

    // Houcine à droite, y aligné avec Jimmy, x = bord droit - marge - largeur
    u16 hou_w = houcine.tilemap->w;
    u16 hou_x = (40 >= (3 + hou_w)) ? (40 - 3 - hou_w) : 0;
    drawImageAtTiles(BG_B, &houcine, PAL2, hou_x, 12);

    // Logo centré en haut (y=2)
    u16 logo_x = centeredXInTiles(&logo);
    drawImageAtTiles(BG_B, &logo, PAL3, logo_x, 2);

    // Texte au-dessus (BG_A), priorité 1 pour passer devant BG_B
    VDP_setTextPriority(1);
    VDP_setTextPalette(TEXT_PAL);
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
            VDP_drawText(pressStart, (u16)x, PRESS_START_ROW); // tout en bas
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
