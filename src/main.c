#include <genesis.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
// Intro = 3 minutes, 1 minute par image
#define INTRO_SECONDS            180
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// Défilement plus lent (~x3 vs ta version précédente)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       45      // 1 px / 45 frames ≈ 1.33 px/s

#define TEXT_PAL                 PAL2
#define TEXT_COLOR_RGB24         0xCC0000   // ROUGE lisible
#define TEXT_BG_RGB24            0x000000   // noir
#define MAX_COLS                 40         // largeur plane en caractères (320/8)
#define VISIBLE_ROWS             28         // 224px / 8px

// -----------------------------------------------------------------------------
// Etat commun
// -----------------------------------------------------------------------------
static u16 nextTile;

// -----------------------------------------------------------------------------
// Utilitaires
// -----------------------------------------------------------------------------
static void resetScene(void)
{
    // Grandes tables + scroll plan/plan
    VDP_setPlaneSize(64, 64, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);

    // Texte sur A, images sur B
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

// Écrit une ligne centrée, tronquée à MAX_COLS
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

// Wrap simple (coupe aux espaces) et écriture directe sur le plan
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

        if (lines[i][0] == 0) y++; // ligne vide -> saut de ligne
    }
    return y - yStart; // nb de lignes écrites
}

// -----------------------------------------------------------------------------
// Contenu de l'intro (script long validé)
// -----------------------------------------------------------------------------
static const char* intro_lines[] =
{
    "REIMS. NUIT ETERNELLE. LA PLUIE LAVE RIEN.",
    "LES LAMPADAIRES GRÉSILLENT, LA VILLE PUE LA PEUR.",
    "",
    "DEPUIS 10 ANS, REIMS S'EST ENFONCEE.",
    "LA CORRUPTION A POURRI JUSQU'AUX FONDATIONS.",
    "LES DEALERS REGNENT SUR LES PLACES ET LES IMPASSES.",
    "LES FLICS TOURNENT LA TETE. LES POLITICIENS ENCAISSENT.",
    "LES HABITANTS BOIVENT POUR OUBLIER, OU PIQUENT POUR TENIR.",
    "",
    "DERRIERE LES BARREAUX, DEUX NOMS TOMBES DANS L'OUBLI:",
    "JIMMY. 35 ANS. 1M78, 70KG. TRACE. BOXEUR ANGLAIS.",
    "HOUCINE. 40 ANS. 1M85, 62KG. SEC. DISCIPLINE BRUCE LEE.",
    "ILS ONT PRIS 10 ANS POUR DES MEURTRES.",
    "PAS PAR PLAISIR. PAR SURVIE. PAR NECESSITE.",
    "",
    "EN PRISON, ILS ONT TENU TETE AUX ANNEES.",
    "CHAQUE JOUR, ILS ONT LIME LA HAINE JUSQU'AU FIL.",
    "LEURS CORPS SE SONT ENDURCIS. LEURS COEURS SE SONT FERRES.",
    "ET LEURS YEUX ONT APPREND A VOIR DANS LE NOIR.",
    "",
    "CE SOIR, LES PORTES GRINCENT.",
    "LE BETON SUINTE. LES CLEFS CHANTENT.",
    "REIMS RESPIRE UN COUP TROP COURT.",
    "",
    "JIMMY ET HOUCINE SORTENT.",
    "LEURS OMBRES S'ETIRENT SUR LES PAVES MOUILLES.",
    "LES RUES LES RECONNAISSENT.",
    "",
    "PLACE DROUET-D'ERLON. COEUR POURRI DE LA VILLE.",
    "ALCOOL. SERINGUES. COUPS BAS. SOURIRES FALSIFIES.",
    "LES NEO-CAIDS Y FONT LEUR MARCHE.",
    "",
    "CE N'EST PAS UN SAUVETAGE. C'EST UNE CHIRURGIE.",
    "ON COUPE. ON PURGE. ON LAISSE SAIGNER.",
    "",
    "ILS NE PROMETTENT RIEN. ILS AGISSENT.",
    "CEUX QUI ONT VENDU REIMS VONT PAYER.",
    "",
    "REIMS EN RAGE COMMENCE ICI.",
    "ET LA NUIT VA ETRE LONGUE."
};

// -----------------------------------------------------------------------------
// Intro avec défilement depuis le BAS vers le HAUT
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    // Couleurs du texte (rouge sur fond noir)
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG_RGB24));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR_RGB24));

    // Musique
    XGM_startPlay(intro_music);

    // Image 1 + texte posé loin SOUS l’écran (yStartText élevé)
    drawFullImageOn(BG_B, &intro1, PAL0);

    const u16 yStartText = 44; // on écrit très bas dans la plane A (64 lignes)
    const u16 linesCount = sizeof(intro_lines) / sizeof(intro_lines[0]);
    drawWrappedBlock(yStartText, intro_lines, linesCount);

    // On place la plane A de façon à ce que RIEN ne soit visible au début,
    // puis on remonte doucement (le texte émerge par le bas).
    // Formule : on démarre bien en dessous (offset négatif en pixels).
    s16 vscroll = -((yStartText + VISIBLE_ROWS) * 8); // assez négatif pour cacher tout
    u32 frame = 0;

    while (frame < INTRO_FRAMES)
    {
        // Changement d'image à 1 min et 2 min
        if (frame == (INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro2, PAL0);
            drawWrappedBlock(yStartText, intro_lines, linesCount);
        }
        else if (frame == (2 * INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro3, PAL0);
            drawWrappedBlock(yStartText, intro_lines, linesCount);
        }

        // Défilement VERS LE HAUT : on AUGMENTE vscroll vers 0, 1 px par pas
        if ((frame % SCROLL_STEP_PERIOD) == 0)
        {
            if (vscroll < 0) vscroll += SCROLL_PIX_PER_STEP;
        }
        VDP_setVerticalScroll(BG_A, vscroll);

        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        SYS_doVBlankProcess();
        frame++;
    }
}

// -----------------------------------------------------------------------------
// Ecran titre avec "PRESS START" tout en bas
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

        // Clignote ~0.5 s (30 frames ON / 30 OFF)
        bool on = ((blink / 30) % 2) == 0;
        if (on)
        {
            u16 len = strlen(pressStart);
            if (len > MAX_COLS) len = MAX_COLS;
            s16 x = (MAX_COLS - (s16)len) / 2; if (x < 0) x = 0;
            VDP_drawText(pressStart, (u16)x, VISIBLE_ROWS - 2); // ligne 26 (tout en bas)
        }
        else
        {
            VDP_clearTextArea(0, VISIBLE_ROWS - 2, MAX_COLS, 1);
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
