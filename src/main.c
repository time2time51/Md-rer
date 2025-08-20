#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            90
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Scroll doux : 1 px / 3 frames ≃ 20 px/s -> 224px ~ 11.2 s par écran
// On fera défiler bien plus que 224px puisque le texte est long.
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       3        // 1 px toutes les 3 frames

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
// Retourne le nombre total de lignes écrites.
static u16 drawWrappedBlock(u16 yStart, const char* const* lines, u16 count)
{
    char out[ MAX_COLS + 1 ];

    u16 y = yStart;
    for (u16 i = 0; i < count; i++)
    {
        const char* p = lines[i];
        while (*p)
        {
            // cherche un coupure ≤ MAX_COLS
            u16 take = 0, lastSpace = 0;
            while (p[take] && take < MAX_COLS)
            {
                if (p[take] == ' ') lastSpace = take;
                take++;
            }

            if (p[take] && lastSpace) take = lastSpace; // coupe au dernier espace si possible

            // copie le tronçon (skip espace de tête/taille)
            while (*p == ' ') p++;
            u16 real = take;
            while (real && p[real - 1] == ' ') real--;

            memcpy(out, p, real);
            out[real] = 0;

            drawCenteredLine(y++, out);

            p += take;
            while (*p == ' ') p++; // évite espaces doublons
        }

        // ligne vide entre phrases si la source contenait "" :
        if (lines[i][0] == 0) y++;
    }
    return y - yStart;
}

// -----------------------------------------------------------------------------
// Contenu de l'intro (ton texte, identique, en ASCII sans accents)
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
// Intro avec scroll vertical fluide + 3 images
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    // Musique : démarre et laisse tourner
    XGM_startPlay(intro_music);

    // Couleurs du texte (palette TEXT_PAL)
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));

    // Image 1 au début
    drawFullImageOn(BG_B, &intro1, PAL0);

    // On remplit tout le texte wrappe sur BG_A, à partir d’une marge haute
    const u16 yTopMargin = 4;      // espace au-dessus
    u16 total = drawWrappedBlock(yTopMargin, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));
    u16 yBottom = yTopMargin + total + 4; // petite marge après

    // Position de scroll (en pixels). On part avec le texte un peu plus bas que l’ecran
    // pour voir apparaître en douceur.
    s16 vscroll = -32;         // texte commence sous le bord haut
    u16 frame = 0;

    // Durée totale cible
    while (frame < INTRO_FRAMES)
    {
        // Swap d'image aux tiers du temps
        if (frame == (INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro2, PAL0);
            // re-draw texte (resetScene a nettoye BG_A)
            drawWrappedBlock(yTopMargin, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));
        }
        else if (frame == (2 * INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro3, PAL0);
            drawWrappedBlock(yTopMargin, intro_lines, sizeof(intro_lines)/sizeof(intro_lines[0]));
        }

        // Scroll doux (1 px toutes les 3 frames)
        if ((frame % SCROLL_STEP_PERIOD) == 0) vscroll += SCROLL_PIX_PER_STEP;

        VDP_setVerticalScroll(BG_A, vscroll);

        // Skip sur START
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        SYS_doVBlankProcess();
        frame++;
    }
}

// -----------------------------------------------------------------------------
// Titre (image + PRESS START clignotant)
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

        // clignote toutes les ~0.33s
        bool on = ((blink / 20) % 2) == 0;
        if (on)
        {
            // bas de l’ecran (~ligne 25) bien centre
            char buf[40]; buf[0] = 0;
            // on centre sans depasser
            u16 len = strlen(pressStart); if (len > MAX_COLS) len = MAX_COLS;
            s16 x = (MAX_COLS - (s16)len) / 2; if (x < 0) x = 0;
            VDP_drawText(pressStart, (u16)x, 25);
        }
        else
        {
            VDP_clearTextArea(0, 25, 64, 1);
        }

        SYS_doVBlankProcess();
        blink++;
    }
}

// -----------------------------------------------------------------------------
// main SGDK v2.11
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
