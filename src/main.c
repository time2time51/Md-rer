#include <genesis.h>
#include <string.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            254
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// vitesse (ta valeur actuelle)
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30

#define TEXT_PAL                 PAL2
#define TEXT_COLOR               0xFF7840
#define TEXT_BG                  0x000000
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
static void waitFrames(u16 n) { while (n--) SYS_doVBlankProcess(); }

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
// Contenu
// -----------------------------------------------------------------------------
static const char* intro_lines[] =
{
    // Bloc 1 – La ville
    "Reims, la nuit. La ville suffoque.",
    "Les bars ne ferment jamais.",
    "Les terrasses de Drouet-d'Erlon brillent de neon.",
    "Mais derriere les verres d'alcool, la peur rode.",
    "La drogue coule a flot, la corruption est partout.",
    "Les flics regardent ailleurs. Les juges encaissent.",
    "Les habitants survivent dans une prison a ciel ouvert.",
    "",

    // Bloc 2 – Jimmy
    "Jimmy, 35 ans.",
    "Un boxeur taille pour encaisser et rendre coup pour coup.",
    "Son corps sec et nerveux a ete forge dans la rage.",
    "Il a appris a frapper comme on respire.",
    "Sa colere a grandi derriere les murs de la prison.",
    "",

    // Bloc 3 – Houcine
    "Houcine, 40 ans.",
    "Sec, rapide, precis comme une lame.",
    "Ses poings et ses pieds parlent le langage de Bruce Lee.",
    "Dix ans enferme n'ont pas casse son corps.",
    "Ils ont durci son esprit.",
    "",

    // Bloc 4 – Leur passe
    "Ensemble, ils ont connu la haine.",
    "Ensemble, ils ont paye le prix du sang.",
    "Un meurtre les a condamnes a dix ans de nuit.",
    "Mais derriere les barreaux, leur rage n'a jamais faibli.",
    "",

    // Bloc 5 – La vengeance
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
static const u16 INTRO_COUNT = sizeof(intro_lines)/sizeof(intro_lines[0]);

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    // musique
    XGM_startPlay(intro_music);

    // couleurs du texte (fond + lettres)
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));

    // première image
    drawFullImageOn(BG_B, &intro1, PAL0);

    // on dessine le bloc de texte tel que la 1re ligne soit en bas
    const u16 TEXT_Y_START = TEXT_FIRST_VISIBLE_ROW; // 27
    drawWrappedBlock(TEXT_Y_START, intro_lines, INTRO_COUNT);

    // *** direction corrigée : on FAIT MONTER le texte ***
    s16 vscroll = 0;                        // visible dès la frame 0 en bas
    VDP_setVerticalScroll(BG_A, vscroll);

    u16 frame = 0;
    while (frame < INTRO_FRAMES)
    {
        if (frame == (INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro2, PAL0);
            // re-couleurs + re-texte + re-scroll identiques
            PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
            PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
            drawWrappedBlock(TEXT_Y_START, intro_lines, INTRO_COUNT);
            VDP_setVerticalScroll(BG_A, vscroll);
        }
        else if (frame == (2 * INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro3, PAL0);
            PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
            PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
            drawWrappedBlock(TEXT_Y_START, intro_lines, INTRO_COUNT);
            VDP_setVerticalScroll(BG_A, vscroll);
        }

        // fait MONTER (contenu qui va vers le haut)
        if ((frame % SCROLL_STEP_PERIOD) == 0)
        {
            vscroll += SCROLL_PIX_PER_STEP;      // <- signe inversé
            VDP_setVerticalScroll(BG_A, vscroll);
        }

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

    // *** rendre le texte visible au-dessus de l'image ***
    VDP_setTextPriority(1); // texte par-dessus BG_B
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
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
