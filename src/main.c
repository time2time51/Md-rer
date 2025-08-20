#include <genesis.h>
#include <string.h>
#include "resources.h"

// -----------------------------------------------------------------------------
// Réglages
// -----------------------------------------------------------------------------
#define INTRO_SECONDS            252
#define FPS                      60
#define INTRO_FRAMES             (INTRO_SECONDS * FPS)

// vitesse scroll
#define SCROLL_PIX_PER_STEP      1
#define SCROLL_STEP_PERIOD       30

#define TEXT_PAL                 PAL2
#define TEXT_COLOR               0xFF0000   // Rouge
#define TEXT_BG                  0x000000
#define MAX_COLS                 40

// lignes basses
#define TEXT_FIRST_VISIBLE_ROW   27   // première ligne visible tout en bas
#define PRESS_START_ROW          27   // "PRESS START" tout en bas

#define MAX_VISIBLE_LINES        32   // combien de lignes max visibles en même temps

// -----------------------------------------------------------------------------
// Etat commun
// -----------------------------------------------------------------------------
static u16 nextTile;
static s16 vscroll;
static u16 textLineIndex;  // index dans le script

// -----------------------------------------------------------------------------
// Utilitaires
// -----------------------------------------------------------------------------
static void waitFrames(u16 n) { while (n--) SYS_doVBlankProcess(); }

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

    vscroll = 0;
    textLineIndex = 0;
    VDP_setVerticalScroll(BG_A, vscroll);
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
// Affichage progressif du texte
// -----------------------------------------------------------------------------
static void showNextLine(u16 screenRow)
{
    if (textLineIndex >= INTRO_COUNT) return;
    drawCenteredLine(screenRow, intro_lines[textLineIndex]);
    textLineIndex++;
}

// -----------------------------------------------------------------------------
// Intro
// -----------------------------------------------------------------------------
static void playIntro(void)
{
    resetScene();

    XGM_startPlay(intro_music);

    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));

    drawFullImageOn(BG_B, &intro1, PAL0);

    u16 frame = 0;
    u16 localRow = TEXT_FIRST_VISIBLE_ROW;

    // première ligne
    showNextLine(localRow);

    while (frame < INTRO_FRAMES)
    {
        // changement d’image
        if (frame == (INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro2, PAL0);
            PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
            PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
        }
        else if (frame == (2 * INTRO_FRAMES / 3))
        {
            resetScene();
            drawFullImageOn(BG_B, &intro3, PAL0);
            PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
            PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
        }

        // scroll + ajout de texte
        if ((frame % SCROLL_STEP_PERIOD) == 0)
        {
            vscroll += SCROLL_PIX_PER_STEP;
            VDP_setVerticalScroll(BG_A, vscroll);

            // quand une ligne sort de l’écran → on ajoute la suivante
            if ((vscroll % 8) == 0) // 8px = hauteur d'une ligne
            {
                localRow++;
                if (localRow >= 64) localRow = 0; // wrap safe
                showNextLine(localRow);
            }
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

    VDP_setTextPriority(1);
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(TEXT_BG));
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(TEXT_COLOR));
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
