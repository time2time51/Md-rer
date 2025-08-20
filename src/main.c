#include <genesis.h>
#include "resources.h"

// ===================== Réglages rapides =====================
#define PLANE_W             64
#define PLANE_H             32

// Texte : démarre tout en bas (ligne 27 sur 0..27)
#define TEXT_START_Y        27
// Vitesse du scroll (frames par ligne). 6 ≈ ~10 lignes/sec sur 60Hz.
// Tu peux augmenter pour ralentir, diminuer pour accélérer.
#define TEXT_SCROLL_DELAY   6

// Durée d’affichage de chaque image d’intro (en secondes)
#define INTRO_IMAGE_SEC     30

// Clignotement "PRESS START" (frames)
#define BLINK_ON_FRAMES     30
#define BLINK_OFF_FRAMES    30
// ===========================================================

// Texte d’intro (sans “appuie sur start…” comme demandé)
static const char *intro_lines[] = {
    "Reims, la nuit. La ville se meurt.",
    "La corruption et la drogue rongent les rues.",
    "Cela fait 10 ans que Jimmy et Houcine",
    "sont tombes et ont disparu derriere les barreaux.",
    "Depuis, les gangs regnent en maitre.",
    "La police est corrompue, les politiciens achetes.",
    "Les dealers font la loi dans les quartiers.",
    "Les habitants n'ont plus d'espoir.",
    "Mais ce soir...",
    "Jimmy et Houcine sortent de prison.",
    "Leur colere est intacte.",
    "Ils vont faire payer ceux qui ont detruit leur ville.",
    "",
    "REIMS EN RAGE",
    ""
};
static const u16 INTRO_LINES_COUNT = sizeof(intro_lines) / sizeof(intro_lines[0]);

// Gestion VRAM
static u16 nextTile = TILE_USER_INDEX;

// Raccourcis palettes
#define IMG_PAL     PAL1
#define TEXT_PAL    PAL2
#define UI_PAL      PAL0

// -----------------------------------------------------------
// Utils
static void resetScene(void)
{
    VDP_setPlaneSize(PLANE_W, PLANE_H, TRUE);
    VDP_setTextPlane(BG_A);

    // Nettoie et remet le scrolling à 0
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setVerticalScroll(BG_B, 0);

    nextTile = TILE_USER_INDEX;
}

// Dessine une image 320x224 plein écran sur le plan donné, sans recharger la palette
static void drawFullImageOn(VDPPlane plane, const Image *img, u16 palIndex)
{
    // Charge la palette de l'image dans palIndex (PAL1 pour l'intro, PAL0 pour le title)
    PAL_setPalette(palIndex, img->palette->data, DMA);

    // Place l'image ; basetile = nextTile
    VDP_drawImageEx(
        plane,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        0, 0,
        FALSE,      // ne recharge pas la palette (on l'a déjà fait)
        TRUE        // DMA on
    );

    // Avance les tiles consommés
    nextTile += img->tileset->numTile;
}

// Redessine tout le bloc de texte visible, ancré à TEXT_START_Y - offset
// offset = 0 -> première ligne dessinée sur TEXT_START_Y
static void renderIntroText(s16 offset)
{
    // efface BG_A
    VDP_clearTextArea(0, 0, 40, 28);

    // Nombre de lignes visibles (hauteur de l’écran en tuiles texte)
    const s16 screenLines = 28; // 224/8
    // L’index de la première ligne à afficher
    s16 first = offset;               // ex: 0 au début
    if (first < 0) first = 0;
    // La position Y en plan A où dessiner la première ligne
    s16 y0 = TEXT_START_Y - offset;

    // Pour chaque ligne, si elle tombe dans l'écran, on dessine
    for (s16 i = 0; i < INTRO_LINES_COUNT; i++)
    {
        s16 y = y0 + i;
        if ((y >= 0) && (y < screenLines))
        {
            const char* line = intro_lines[i];
            // Centrage 40 colonnes
            s16 x = 20 - (strlen(line) / 2);
            if (x < 0) x = 0;
            VDP_drawText(line, x, y);
        }
    }
}

// -----------------------------------------------------------
// Intro
static bool playIntro(void)
{
    resetScene();

    // Palettes de base : texte orange sur fond noir
    PAL_setColor(TEXT_PAL * 16 + 0, RGB24_TO_VDPCOLOR(0x000000)); // back
    PAL_setColor(TEXT_PAL * 16 + 1, RGB24_TO_VDPCOLOR(0xFF7840)); // lettres

    // Affiche la 1ere image (plan B), puis musique
    drawFullImageOn(BG_B, &intro1, IMG_PAL);

    XGM_init();                 // init driver
    XGM_startPlay(intro_music); // musique qui continue jusqu’au titre

    // Chrono images (30s chacune)
    const u32 switch1 = INTRO_IMAGE_SEC * 60;
    const u32 switch2 = INTRO_IMAGE_SEC * 2 * 60;
    const u32 endAt   = INTRO_IMAGE_SEC * 3 * 60;

    // Défilement du texte : démarre tout en bas
    s16 textOffset = 0; // 0 => première ligne à TEXT_START_Y
    u16 frame = 0;
    u16 scrollTick = 0;

    // Boucle d’intro
    for (u32 t = 0; t < endAt; t++)
    {
        // Switch des images
        if (t == switch1)
        {
            // nettoie les tiles de l'image précédente si tu veux, ici on empile
            drawFullImageOn(BG_B, &intro2, IMG_PAL);
        }
        else if (t == switch2)
        {
            drawFullImageOn(BG_B, &intro3, IMG_PAL);
        }

        // Rendu texte
        if (scrollTick == 0)
        {
            renderIntroText(textOffset);
        }

        // Lecture START pour skip
        u16 joy = JOY_readJoypad(JOY_1);
        if (joy & BUTTON_START)
        {
            return true; // skip -> passer au titre
        }

        // Tempo scroll
        scrollTick++;
        if (scrollTick >= TEXT_SCROLL_DELAY)
        {
            scrollTick = 0;
            // On fait monter le texte d'UNE ligne (donc la 1ere ligne “sort” par le haut)
            textOffset++;
            // Quand tout le texte a quitté l’écran, on laisse l’intro continuer
            if (textOffset > (TEXT_START_Y + INTRO_LINES_COUNT))
            {
                // plus rien à afficher, mais on laisse tourner jusqu’à la fin de l’intro
                // (tu peux aussi sortir plus tôt si tu préfères)
            }
        }

        VDP_waitVSync();
        frame++;
    }

    return true; // intro terminée -> passer au titre
}

// -----------------------------------------------------------
// Title screen
static void showPressStartBlink(void)
{
    static u16 blink = 0;
    const char *msg = "PRESS START";
    const s16 y = 25;
    const s16 x = 20 - (strlen(msg) / 2);

    blink++;
    u16 phase = blink % (BLINK_ON_FRAMES + BLINK_OFF_FRAMES);
    VDP_clearTextArea(x, y, strlen(msg), 1);
    if (phase < BLINK_ON_FRAMES)
    {
        VDP_drawText(msg, x, y);
    }
}

static void showTitle(void)
{
    resetScene();

    // Le title utilise sa palette propre en PAL0
    drawFullImageOn(BG_B, &title, UI_PAL);

    // Texte clignotant en plan A
    while (1)
    {
        showPressStartBlink();

        // Lire START
        u16 joy = JOY_readJoypad(JOY_1);
        if (joy & BUTTON_START)
        {
            // Ici tu enchaines vers le menu ou le jeu
            // Pour l’instant, on boucle (ou break pour arrêter)
            // break;
        }

        VDP_waitVSync();
    }
}

// -----------------------------------------------------------
// Entrée
int main(bool hard)
{
    (void)hard;

    // Input
    JOY_init();

    // Intro puis titre
    bool goTitle = playIntro();
    if (goTitle) showTitle();

    // Ne jamais sortir (boucle de sécurité)
    while (1) VDP_waitVSync();
    return 0;
}
