#include <genesis.h>
#include <resources.h>
#include <string.h>
#include <stdbool.h>

// --- Gestion VRAM -----------------------------------------------------------
static u16 nextTile;

// réinitialise l’écran et la VRAM “utilisateur”
static void resetScene(void)
{
    VDP_resetScreen();
    VDP_setPlaneSize(64, 32, TRUE);                 // tables larges
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setTextPlan(BG_A);
    VDP_setTextPriority(1);                         // texte au-dessus
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    nextTile = TILE_USER_INDEX;                     // point de départ SGDK
}

// dessine une image plein écran sur un plan, réutilise PAL{palIndex}
static void drawFullImageOn(VDPPlane plan, const Image* img, u16 palIndex)
{
    // palette de l’image
    PAL_setPalette(palIndex, img->palette->data, DMA);

    // upload & placement du tileset à l’indice courant
    VDP_drawImageEx(
        plan,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        0, 0,
        DMA
    );

    // avance l’indice pour un éventuel ajout
    nextTile += img->tileset->numTile;
}

// --- Texte : word-wrap + centrage ------------------------------------------
static void drawWrappedCentered(const char* text, u16 xMargin, u16 y, u16 widthCols, u16 pal)
{
    VDP_setTextPalette(pal);

    const u16 cols = widthCols;
    const u16 left = xMargin;
    const u16 right = left + cols;

    char line[64];
    u16 cx = left;
    u16 cy = y;

    const char* p = text;
    while (*p)
    {
        // saute espaces de tête
        while (*p == ' ') p++;

        // prend un mot
        const char* start = p;
        while (*p && *p != ' ' && *p != '\n') p++;
        u16 wlen = (u16)(p - start);

        // rupture de ligne dite “manuelle”
        if (*p == '\n')
        {
            line[cx - left] = 0;
            // centrage : recalcule x de départ pour la ligne courante
            u16 used = (cx - left);
            u16 startX = left + (cols > used ? (cols - used)/2 : 0);
            VDP_drawTextBG(BG_A, line, startX, cy);
            cy++;
            cx = left;
            SYS_doVBlankProcess();
            p++; // saute '\n'
            continue;
        }

        // si ça ne tient pas, on pousse la ligne en cours
        if ((cx + wlen) > right)
        {
            line[cx - left] = 0;
            u16 used = (cx - left);
            u16 startX = left + (cols > used ? (cols - used)/2 : 0);
            VDP_drawTextBG(BG_A, line, startX, cy);
            cy++;
            cx = left;

            // petit délai pour la lecture
            for (u16 i=0; i<12; i++) SYS_doVBlankProcess();
        }

        // copie le mot dans le tampon “line”
        for (u16 i=0; i<wlen; i++)
        {
            line[cx - left + i] = start[i];
        }
        cx += wlen;

        // espace si le prochain caractère est un espace
        if (*p == ' ')
        {
            line[cx - left] = ' ';
            cx++;
        }
    }

    // purge le reste
    if (cx > left)
    {
        line[cx - left] = 0;
        u16 used = (cx - left);
        u16 startX = left + (cols > used ? (cols - used)/2 : 0);
        VDP_drawTextBG(BG_A, line, startX, cy);
    }
}

// --- Intro ------------------------------------------------------------------
static const char* INTRO_TXT =
"Reims, la nuit. La ville se meurt.\n"
"La corruption et la drogue rongent les rues.\n"
"Cela fait 10 ans que Jimmy et Houcine sont tombes et ont disparu derriere les barreaux.\n"
"Depuis, les gangs regnent en maitre.\n"
"La police est corrompue, les politiciens aussi.\n"
"Les dealers font la loi dans les quartiers.\n"
"Les habitants n'ont plus d'espoir.\n"
"Mais ce soir...\n"
"Jimmy et Houcine sortent de prison.\n"
"Leur colere est intacte.\n"
"Ils vont faire payer ceux qui ont detruit leurs vies.\n"
"REIMS EN RAGE";

static void playIntro(void)
{
    // Musique
    XGM_setLoopNumber(-1);            // en boucle
    XGM_startPlayResource(&intro_music);

    // 1) Ecran 1 + texte
    resetScene();
    drawFullImageOn(BG_B, &intro1, PAL0);

    // palette texte (PAL2, orange doux)
    PAL_setPaletteColor(PAL2*16 + 1, RGB24_TO_VDPCOLOR(255,120,64));
    PAL_setPaletteColor(PAL2*16 + 2, RGB24_TO_VDPCOLOR(32,16,8));
    VDP_setTextPalette(PAL2);

    // zone de texte : 36 colonnes, marges de 2 colonnes
    drawWrappedCentered(INTRO_TXT, 2, 6, 36, PAL2);

    // laisse 7 secondes de lecture ~ 60 FPS
    for (u16 f=0; f<7*60; f++) SYS_doVBlankProcess();

    // 2) Ecran 2 (reutilise la VRAM → pas de dépassement)
    resetScene();
    drawFullImageOn(BG_B, &intro2, PAL0);
    for (u16 f=0; f<4*60; f++) SYS_doVBlankProcess();

    // 3) Ecran 3
    resetScene();
    drawFullImageOn(BG_B, &intro3, PAL0);
    for (u16 f=0; f<4*60; f++) SYS_doVBlankProcess();

    // Stop musique à la fin de l’intro si tu veux
    // XGM_stopPlay();
}

// --- Titre ------------------------------------------------------------------
static void showTitle(void)
{
    resetScene();
    drawFullImageOn(BG_B, &title, PAL0);

    // Attente du START (ou quelques secondes)
    u16 wait = 10*60;
    while (wait--)
    {
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;
        SYS_doVBlankProcess();
    }
}

int main(bool hard)
{
    (void)hard;
    JOY_init();
    XGM_init();            // init driver XGM

    playIntro();
    showTitle();

    // boucle vide (placeholder jeu)
    while (1) SYS_doVBlankProcess();
    return 0;
}
