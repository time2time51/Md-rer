#include <genesis.h>
#include "resources.h"

// --------- Texte d'intro (sans mentions de START) ---------
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
#define INTRO_LINES (sizeof(intro_lines)/sizeof(intro_lines[0]))

// --------- Helpers ---------
static void drawFullImage(const Image *img)
{
    // Plan B pour l'image, PAL1
    VDP_clearPlane(BG_B, TRUE);
    PAL_setPalette(PAL1, img->palette->data, DMA);
    // Toujours utiliser un tile index >=1 (0 est reserve)
    VDP_drawImageEx(BG_B, img, TILE_ATTR_FULL(PAL1, 0, 0, 0, 1),
                    0, 0, FALSE, TRUE);
}

static void showTitle(void)
{
    // On garde la musique qui tourne
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    drawFullImage(&title);

    // Ici on n'affiche rien en texte (ton PNG contient deja "PRESS START")
    while (1)
    {
        if (JOY_readJoypad(JOY_1) & BUTTON_START)
        {
            // plus tard: aller au menu / jeu
        }
        SYS_doVBlankProcess();
    }
}

static void playIntro(void)
{
    // Setup video propre
    VDP_resetScreen();
    VDP_setScreenWidth320();      // 40 colonnes = 320px
    VDP_setPlanSize(64, 32, TRUE); // grandes tables, safe
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    // Texte sur PLAN_A / PAL0 (blanc)
    PAL_setPalette(PAL0, palette_white, DMA);
    VDP_setTextPalette(PAL0);
    VDP_setTextPriority(1);       // le texte passe devant l'image (PLAN_A prio)

    // L'image 1 en fond
    drawFullImage(&intro1);

    // Démarre la musique XGM (VGZ converti par rescomp)
    XGM_setLoopNumber(-1);        // boucle infinie
    XGM_startPlay(intro_music);

    // Défilement vertical "façon SoR" :
    // on écrit les lignes en dessous de l'écran et on remonte tout le PLAN_A
    s16 scrollY = 0;              // scroll vertical du plan A (negatif => monte)
    u16 nextLine = 0;             // prochaine ligne a "spawn" en bas
    u16 lineTimer = 0;
    const u16 lineEvery = 150;    // ~2.5s par ligne (60 FPS)

    // Position d'apparition des lignes (en tuiles)
    s16 writeY = 28;              // hors de l'écran, en bas
    u16 frames = 0;

    while (1)
    {
        // Skip au START (aucun message affiché)
        if (JOY_readJoypad(JOY_1) & BUTTON_START) break;

        // toutes les ~2.5s on pousse une nouvelle ligne
        if ((nextLine < INTRO_LINES) && (++lineTimer >= lineEvery))
        {
            lineTimer = 0;
            VDP_drawText(intro_lines[nextLine], 2, writeY);
            writeY += 2;  // un peu d'espace
            nextLine++;
        }

        // On fait défiler en remontant doucement
        if ((frames & 3) == 0)    // vitesse du scroll
            scrollY--;

        VDP_setVerticalScroll(BG_A, scrollY);

        // Switch des images d'intro (3 plans de ~30s chacun, total 90s)
        if (frames == 1800)       // ~30s
            drawFullImage(&intro2);
        else if (frames == 3600)  // ~60s
            drawFullImage(&intro3);
        else if (frames >= 5400)  // ~90s => fin
            break;

        frames++;
        SYS_doVBlankProcess();
    }

    showTitle();
}

// --------- main ---------
int main(void)
{
    JOY_init();
    SYS_disableInts();

    VDP_setScreenWidth320();
    VDP_setPlanSize(64, 32, TRUE);

    SYS_enableInts();

    playIntro();     // lance l'intro, se termine sur le title
    return 0;
}
