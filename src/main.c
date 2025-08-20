#include <genesis.h>
#include "resources.h"

// ----- Texte d'intro -----
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

// ----- Utilitaires images / palettes -----
static u16 nextTile = TILE_USERINDEX;

// dessine une image plein ecran sur un plan, charge sa palette a l'index voulu
static void drawFullImageOn(VDPPlane plan, const Image *img, u16 palIndex)
{
    PAL_setPalette(palIndex, img->palette->data, DMA);
    VDP_drawImageEx(
        plan,
        img,
        TILE_ATTR_FULL(palIndex, FALSE, FALSE, FALSE, nextTile),
        0, 0,
        FALSE,
        TRUE
    );
    nextTile += img->tileset->numTile;
}

// efface un plan (texte)
static void clearPlane(VDPPlane plan)
{
    VDP_clearPlane(plan, TRUE);
}

// ----- Intro -----
static void playIntro(void)
{
    // grosses tables pour scroller tranquillement
    VDP_setPlaneSize(64, 32, TRUE);

    // Plan A = images / Plan B = texte
    VDP_setBackgroundColor(0); // noir

    // Musique d'intro – elle continuera jusqu'au titre (pas d'arrêt ici)
    XGM_startPlayResource(&intro_music);

    // images: on laisse ~30 s chacune (90 s / 3), mais le texte défile en continu au-dessus
    const u16 palImg = PAL0;
    const u16 palTxt = PAL1;

    // police lisible (palette texte = blanc/orange doux)
    // on part d'une base blanche, tu peux ajuster si besoin
    u16 palTextData[16];
    memcpy(palTextData, palette_white, 16*2);
    palTextData[2] = RGB24_TO_VDPCOLOR(0xD05030); // orange sombre pour le texte
    PAL_setPalette(palTxt, palTextData, DMA);
    VDP_setTextPalette(palTxt);

    // charge la 1ere image
    clearPlane(BG_A);
    drawFullImageOn(BG_A, &intro1, palImg);

    // prépare le texte sur le plan B
    clearPlane(BG_B);
    const int lineH   = 14;            // hauteur logique par ligne
    const int firstY  = 27;            // position de départ (écran 224 lignes -> 27 donne un bon look)
    const int totalTextPixel = (int)INTRO_LINES * lineH + 224; // +224 pour complètement faire sortir le bloc
    const int durFrames = 90 * 60;     // 90 secondes @60 Hz
    int frame = 0;

    // peint toutes les lignes une seule fois (on scrollera le plan B)
    for (u16 i = 0; i < INTRO_LINES; i++)
    {
        VDP_drawTextBG(BG_B, intro_lines[i], 3, firstY/8 + (i*(lineH/8)));
    }

    // timings de swap d'images
    const int swap1 = durFrames/3;      // après 30 s
    const int swap2 = (2*durFrames)/3;  // après 60 s

    // boucle principale: 90 s ou jusqu'au START
    while (frame < durFrames)
    {
        // scroll Y calculé pour que le bloc se déplace sur tout totalTextPixel
        int scrollY = (frame * totalTextPixel) / durFrames;
        VDP_setVerticalScroll(BG_B, -scrollY); // texte monte doucement

        // change l'image de fond aux jalons
        if (frame == swap1)
        {
            clearPlane(BG_A);
            drawFullImageOn(BG_A, &intro2, palImg);
        }
        else if (frame == swap2)
        {
            clearPlane(BG_A);
            drawFullImageOn(BG_A, &intro3, palImg);
        }

        // skip intro
        u16 keys = JOY_readJoypad(JOY_1);
        if (keys & BUTTON_START) break;

        SYS_doVBlankProcess();
        frame++;
    }
}

// ----- Ecran titre -----
static void showTitle(void)
{
    // on garde la musique qui tourne
    nextTile = TILE_USERINDEX;
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setBackgroundColor(0);
    clearPlane(BG_A);
    clearPlane(BG_B);

    drawFullImageOn(BG_A, &title, PAL0);

    // “PRESS START” clignotant en code (pas dans l'image)
    const char *press = "PRESS  START";
    s16 blink = 0;

    while (1)
    {
        if ((blink >> 4) & 1)
            VDP_drawTextBG(BG_B, press, 16 - 6, 25);  // centré-ish
        else
            VDP_clearTextAreaBG(BG_B, 0, 25, 40, 1);

        u16 keys = JOY_readJoypad(JOY_1);
        if (keys & BUTTON_START) break;

        blink++;
        SYS_doVBlankProcess();
    }

    // ici on pourrait enchaîner vers le menu / jeu…
}

static void joyCallback(u16 joy, u16 changed, u16 state)
{
    (void)joy; (void)changed; (void)state;
}

int main(bool hard)
{
    (void)hard;

    JOY_init();
    JOY_setEventHandler(joyCallback);

    // XGM: active le driver (obligatoire pour la musique)
    XGM_init();

    // Intro 90s scrollee et skippable (sans “appuie sur start” affiché)
    playIntro();

    // Puis titre, musique intacte
    showTitle();

    while (1) SYS_doVBlankProcess();
    return 0;
}
