#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            180      // 3 minutes
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Vitesse de scroll : 3x plus rapide que l’ancienne (30 -> 10)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       10       // 1 px / 10 frames ≃ 6 px/s

#define TEXT_PAL                 PAL2     // palette pour le texte
#define TEXT_COLOR               0xFF2020 // rouge lisible
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
    char buf[MAX_COLS + 1];
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
// Contenu de l'intro (script long)
// -----------------------------------------------------------------------------
static const char* intro_lines[] =
{
    "REIMS, LA NUIT. LA VILLE SE MEURT.",
    "La corruption et la drogue rongent les rues.",
    "Les neon falots miroitent sur les flaques huileuses.",
    "Les flics se planquent, les dealers paradent.",
    "",
    "DIX ANS PLUS TOT, TOUT A BASCULE.",
    "Jimmy, 35 ans, 1m78 pour 70kg, boxeur anglais sec et taille dans le vif.",
    "Houcine, 40 ans, 1m85 pour 62kg, ecole Bruce Lee: vitesse, precision.",
    "Deux freres d'arme en colere froide.",
    "",
    "Un soir, un clan a frappe la famille.",
    "La reponse fut immediate, fatale.",
    "Le sang a parle. La justice a cingle.",
    "Dix ans. Barreaux. Silence.",
    "",
    "DEHORS, REIMS A POURRI.",
    "Place Drouet d'Erlon n'est plus qu'un couloir d'alcool et de poudre.",
    "Les politiciens se vendent, la police baisse les yeux.",
    "Les habitants ferment portes et coeurs.",
    "",
    "CETTE NUIT, LES PORTES CLAQUENT.",
    "Jimmy et Houcine sortent. Pas de mots de trop.",
    "Leurs poings sont leur promesse.",
    "Leurs pas, un verdict.",
    "",
    "Ils n'ont pas cherche la guerre.",
    "Mais la guerre les attendait.",
    "",
    "REIMS EN RAGE COMMENCE ICI."
};
static const u16 intro_count = sizeof(intro_lines) / sizeof(intro_lines[0]);

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    // Musique (XGM) à partir de la ressource
    XGM_startPlay(intro_music);

    // Couleurs du texte (rouge sur fond noir)
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));

    // Image 1
    drawFullImageOn(BG_B, &intro1, PAL0);

    // Ecrit tout le bloc en haut du plane (y=4), mais on va scroller depuis le bas
    const u16 yTopMargin = 4;
    drawWrappedBlock(yTopMargin, intro_lines, intro_count);

    // Scroll: commence HORS ECRAN **en bas**, puis MONTE vers le haut
    s16 vscroll = 224;  // hauteur ecran MD
    u16 frame = 0;

    while (frame < INTRO_FRAMES)
    {
        // Changement d'image toutes les 60s (1/3 de 180s)
        if (frame == (INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro2, PAL0);
            drawWrappedBlock(yTopMargin, intro_lines, intro_count);
            // Repart du bas pour garder la meme lecture
            vscroll = 224;
        }
        else if (frame == (2 * INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro3, PAL0);
            drawWrappedBlock(yTopMargin, intro_lines, intro_count);
            vscroll = 224;
        }

        // Fait MONTER (de bas -> haut)
        if ((frame % SCROLL_STEP_PERIOD) == 0) vscroll -= SCROLL_PIX_PER_STEP;
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

        // Clignote toutes ~0.5s
        bool on = ((blink / 30) % 2) == 0;
        if (on)
        {
            u16 len = strlen(pressStart);
            if (len > MAX_COLS) len = MAX_COLS;
            s16 x = (MAX_COLS - (s16)len) / 2; if (x < 0) x = 0;
            VDP_drawText(pressStart, (u16)x, 26);   // plus bas (ligne 26)
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
