/*
 * PIRATE CHEST - Nintendo 3DS
 * Port fidèle du jeu HTML original
 * Compilé avec devkitARM + libctru + citro2d
 */

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ============================================================
 * CONSTANTES
 * ============================================================ */
#define MAX_CHESTS     30
#define MAX_SKINS      21
#define MAX_MAPS        8
#define MAXLVL         50
#define PRICE_DEFAULT 1000
#define RB_BASE       2000   /* Premier rebirth à 2000 (HTML avait 25000) */
#define LCAP          0.35f
#define RP            (1.0f/50.0f)   /* Probabilité coffre rare */

/* Résolution 3DS */
#define RGB(r,g,b) ((u32)(0xFF000000u | ((b)<<16) | ((g)<<8) | (r)))
#define TOP_W  400
#define TOP_H  240
#define BOT_W  320
#define BOT_H  240

/* ============================================================
 * TYPES
 * ============================================================ */
typedef enum {
    SCREEN_LOBBY = 0,
    SCREEN_LOCKER,
    SCREEN_SHOP,
    SCREEN_GAME,
    SCREEN_PAUSE
} GameScreen;

typedef struct {
    float x, y;
    int open;
    int rare;
    float anim;   /* 0=fermé, 1=ouvert, valeur intermédiaire = animation */
} Chest;

typedef struct {
    float x, y;
    float dx, dy;
    float life;
    u32 color;
    float size;
} Particle;

typedef struct {
    int id;
    const char *name;
    u32 color;
    int price;
    float spd;    /* multiplicateur vitesse */
    float cm;     /* multiplicateur pièces  */
    float ch;     /* multiplicateur chance  */
    int legend;
} Skin;

typedef struct {
    const char *name;
    u32 ground;
    u32 sea;
    u32 edge;
    u32 chestBody;
    u32 chestLid;
    u32 chestGold;
    const char *decor[5];
} Map;

typedef struct {
    unsigned long long coins;
    int lvl;
    int skins_owned[MAX_SKINS]; /* 1 = possédé */
    int equipped;               /* index skin équipé */
    int rb;                     /* niveau rebirth */
    int map;                    /* map sélectionnée */
    int mute;
} SaveData;

/* ============================================================
 * DONNÉES DU JEU (fidèles au HTML)
 * ============================================================ */
static const Skin SKINS[MAX_SKINS] = {
    /* id, nom, couleur, prix, spd, cm, ch, legend */
    { 0, "Classique",   0xFFFFFFFF, 0,      1.0f, 1.0f, 1.0f, 0 },
    { 1, "Ninja",       RGB(0x2A,0x2A,0x2A), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 2, "Robo",        RGB(0xB8,0xC4,0xD6), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 3, "Astronaute",  RGB(0xF2,0xF2,0xF2), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 4, "Cow-boy",     RGB(0xE8,0xB7,0x8A), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 5, "Roi",         RGB(0x4D,0xA3,0xFF), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 6, "Chef",        RGB(0xFF,0x9D,0x2E), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 7, "Zombie",      RGB(0x7A,0xC9,0x6B), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 8, "Sorcier",     RGB(0xB0,0x7B,0xFF), 0,    1.0f, 1.0f, 1.0f, 0 },
    { 9, "Pirate",      RGB(0xFF,0x4D,0x4D), 0,    1.0f, 1.0f, 1.0f, 0 },
    {10, "Matelot",     RGB(0x7F,0xD1,0xFF), 100,  1.0f, 1.0f, 1.0f, 0 },
    {11, "Eclair",      RGB(0xFF,0xE1,0x4D), 75000,1.5f, 1.0f, 1.0f, 0 },
    {12, "Turbo",       RGB(0xFF,0x6A,0x2B), 75000,1.5f, 1.0f, 1.0f, 0 },
    {13, "Galaxie",     RGB(0x7A,0x3C,0xFF), 250000,1.0f,2.0f, 1.0f, 0 },
    {14, "Viking",      RGB(0xE8,0xC9,0xA0), 0,    1.0f, 1.0f, 1.0f, 0 },
    {15, "Fantome",     RGB(0xE8,0xF0,0xFF), 0,    1.0f, 1.0f, 1.0f, 0 },
    {16, "Samourai",    RGB(0xC0,0x39,0x2B), 0,    1.0f, 1.0f, 1.0f, 0 },
    {17, "Magma",       RGB(0xFF,0x6A,0x00), 250000,1.0f,3.0f, 1.0f, 0 },
    {18, "Cristal",     RGB(0x7F,0xD6,0xFF), 250000,1.25f,1.0f,1.5f, 0 },
    {19, "Neon",        RGB(0xFF,0x00,0xE5), 250000,1.75f,1.5f,1.0f, 0 },
    {20, "Or hebdo",    RGB(0xFF,0xD4,0x1F), 0,    2.0f, 5.0f, 2.0f, 1 },
};

static const Map MAPS[MAX_MAPS] = {
    { "Ile",         RGB(0x3F,0x9A,0x46), RGB(0x1B,0x5A,0x9C), RGB(0xE9,0xD2,0x8F),
      RGB(0xA0,0x5A,0x1C), RGB(0xC4,0x73,0x1F), RGB(0xFF,0xD4,0x1F),
      {"Palmier","Palmier","Rocher","Plante","Cactus"} },
    { "Desert",      RGB(0xD9,0xB4,0x5A), RGB(0x8A,0x6A,0x2A), RGB(0xF3,0xE2,0xA0),
      RGB(0xB8,0x8A,0x4A), RGB(0xD6,0xA8,0x5C), RGB(0xFF,0xE0,0x8A),
      {"Cactus","Cactus","Rocher","Dune","Chameau"} },
    { "Neige",       RGB(0xE6,0xF1,0xF8), RGB(0x2C,0x6F,0x9C), RGB(0xFF,0xFF,0xFF),
      RGB(0x7E,0xC8,0xF0), RGB(0xA8,0xE0,0xFF), RGB(0xE8,0xF8,0xFF),
      {"Sapin","Sapin","Bonhomme","Rocher","Flocon"} },
    { "Volcan",      RGB(0x4A,0x35,0x35), RGB(0xC2,0x41,0x0C), RGB(0xFF,0x7A,0x1A),
      RGB(0x2A,0x2A,0x33), RGB(0x3A,0x3A,0x46), RGB(0xFF,0x6A,0x1A),
      {"Volcan","Rocher","Feu","Crane","Lave"} },
    { "Jungle",      RGB(0x1F,0x7A,0x3A), RGB(0x0F,0x4A,0x5A), RGB(0x6B,0x4A,0x1A),
      RGB(0x3D,0x6B,0x2A), RGB(0x4F,0x8A,0x35), RGB(0xC9,0xA2,0x27),
      {"Palmier","Plante","Feuille","Perroquet","Fleur"} },
    { "Bonbons",     RGB(0xFF,0x9A,0xC8), RGB(0xB5,0x7B,0xFF), RGB(0xFF,0xFF,0xFF),
      RGB(0xFF,0x5F,0xA2), RGB(0xFF,0x8C,0xC0), RGB(0xFF,0xFF,0xFF),
      {"Sucette","Bonbon","Gateau","Beignet","Glace"} },
    { "Espace",      RGB(0x24,0x1A,0x4A), RGB(0x08,0x05,0x20), RGB(0x7A,0x5C,0xFF),
      RGB(0x4A,0x3A,0x9A), RGB(0x6A,0x52,0xD0), RGB(0x48,0xE0,0xFF),
      {"Etoile","Planete","Comete","Soucoupe","Lune"} },
    { "Tresor d'or", RGB(0xD4,0xA0,0x17), RGB(0x6A,0x4A,0x00), RGB(0xFF,0xF1,0xA0),
      RGB(0xE0,0xA8,0x00), RGB(0xFF,0xD4,0x1F), RGB(0xFF,0xF1,0xA0),
      {"Diamant","Couronne","Tresor","Trophee","Joyau"} },
};

/* Récompenses (fidèles au HTML : c10, c75, c500, c5000, token, gold) */
typedef struct {
    const char *id;
    const char *label;
    float base_prob;
    u32   color;
} Odd;

static const Odd ODDS[] = {
    { "token",  "Jeton gratuit",    1.0f/137500.0f, RGB(0xFF,0x3D,0x7F) },
    { "gold",   "Skin Or",          1.0f/1000000.0f,RGB(0xFF,0xD4,0x1F) },
    { "c5000",  "5000 pieces",      1.0f/7813.0f,   RGB(0x9D,0x4D,0xFF) },
    { "c500",   "500 pieces",       1.0f/568.0f,    RGB(0x2B,0x8C,0xFF) },
    { "c75",    "75 pieces",        1.0f/100.0f,    RGB(0x3E,0xC4,0x3E) },
    { "c10",    "10 pieces",        1.0f/75.0f,     RGB(0xF0,0x8A,0x1C) },
};
#define NUM_ODDS 6

/* ============================================================
 * ÉTAT GLOBAL
 * ============================================================ */
static SaveData S;
static GameScreen curScreen = SCREEN_LOBBY;
static GameScreen prevScreen = SCREEN_LOBBY;

/* Jeu */
static Chest chests[MAX_CHESTS];
static float px = 200.0f, py = 120.0f;
static float pFace = 1.0f; /* 1=droite, -1=gauche */
static float pAnimT = 0.0f;
static int   openedCount = 0;
static int   gameDone = 0;

/* Particules */
#define MAX_PARTS 60
static Particle parts[MAX_PARTS];
static int      numParts = 0;

/* Toast (message flottant) */
static char  toastMsg[64] = "";
static int   toastTimer = 0;
static float toastY = 100.0f;

/* Roue bonus */
static int   wheelActive = 0;
static float wheelAngle = 0.0f;
static float wheelSpeed = 0.0f;
static int   wheelResult = 1;
static int   wheelDone = 0;
static int   pendingReward = 0;
static int   pendingRare = 0;

/* Menu lobby */
static int shopScroll = 0;
static int shopTab = 0; /* 0=skins, 1=upgrades, 2=rebirth */
static int lockerScroll = 0;
static int pauseSelY = 0;

/* Texte */
static C2D_TextBuf tb;
#define TEXT_MAX 8192

/* ============================================================
 * SAUVEGARDE
 * ============================================================ */
static void saveGame(void) {
    FILE *f = fopen("sdmc:/pirate_chest_v2.sav", "wb");
    if (f) {
        fwrite(&S, sizeof(S), 1, f);
        fclose(f);
    }
}

static void loadGame(void) {
    FILE *f = fopen("sdmc:/pirate_chest_v2.sav", "rb");
    if (f) {
        fread(&S, sizeof(S), 1, f);
        fclose(f);
        /* Validation */
        if (S.lvl < 0 || S.lvl > MAXLVL) S.lvl = 0;
        if (S.rb < 0 || S.rb >= MAX_MAPS) S.rb = 0;
        if (S.map < 0 || S.map > S.rb) S.map = 0;
        if (S.equipped < 0 || S.equipped >= MAX_SKINS) S.equipped = 0;
    } else {
        /* Première fois : valeurs par défaut */
        memset(&S, 0, sizeof(S));
        S.skins_owned[0] = 1; /* Classique toujours possédé */
        S.equipped = 0;
    }
    /* Toujours posséder le skin 0 */
    S.skins_owned[0] = 1;
}

/* ============================================================
 * FONCTIONS DE JEU
 * ============================================================ */
static double luckVal(void) {
    double base = pow(1.1, S.lvl);
    base *= SKINS[S.equipped].ch;
    return base;
}

static unsigned long long coinMult(void) {
    double m = pow(2.0, S.rb);
    m *= SKINS[S.equipped].cm;
    return (unsigned long long)m;
}

static int upCost(void) {
    if (S.lvl >= MAXLVL) return 0;
    return (int)(150.0 * pow(1.1, S.lvl));
}

static unsigned long long rbCost(void) {
    return (unsigned long long)(RB_BASE * pow(2.0, S.rb));
}

static void spawnParticles(float x, float y, u32 color, int count) {
    for (int i = 0; i < count && numParts < MAX_PARTS; i++) {
        Particle *p = &parts[numParts++];
        float angle = (rand() % 628) / 100.0f;
        float speed = 1.5f + (rand() % 100) / 40.0f;
        p->x = x;
        p->y = y;
        p->dx = cosf(angle) * speed;
        p->dy = sinf(angle) * speed - 1.0f;
        p->life = 1.0f;
        p->color = color;
        p->size = 2.0f + (rand() % 4);
    }
}

static void newMap(void) {
    openedCount = 0;
    gameDone = 0;
    numParts = 0;
    px = TOP_W / 2.0f;
    py = TOP_H / 2.0f;
    int idx = S.map < S.rb ? S.map : S.rb;
    if (idx < 0) idx = 0;
    if (idx >= MAX_MAPS) idx = MAX_MAPS - 1;

    for (int i = 0; i < MAX_CHESTS; i++) {
        int ok, tries = 0;
        do {
            chests[i].x = 30.0f + rand() % (TOP_W - 60);
            chests[i].y = 40.0f + rand() % (TOP_H - 60);
            chests[i].open = 0;
            chests[i].rare = (rand() % (int)(1.0f/RP + 0.5f)) == 0;
            chests[i].anim = 0.0f;
            ok = 1;
            if (hypotf(chests[i].x - px, chests[i].y - py) < 50.0f) ok = 0;
            for (int j = 0; j < i && ok; j++) {
                if (fabsf(chests[j].x - chests[i].x) < 36.0f &&
                    fabsf(chests[j].y - chests[i].y) < 32.0f)
                    ok = 0;
            }
        } while (!ok && ++tries < 1000);
    }
}

/* Calcule la récompense d'un coffre */
static int openChestReward(int rare) {
    double L = luckVal();
    if (rare) L *= 3.0;

    /* Probabilités brutes */
    double probs[NUM_ODDS];
    for (int i = 0; i < NUM_ODDS; i++) {
        probs[i] = ODDS[i].base_prob * (i < 2 ? fmin(L, 8.0) : L);
    }
    /* Cap */
    double s = 0;
    for (int i = 2; i < NUM_ODDS; i++) s += probs[i];
    if (s > LCAP) {
        double k = LCAP / s;
        for (int i = 2; i < NUM_ODDS; i++) probs[i] *= k;
    }
    /* Tirage */
    double r = rand() / (double)RAND_MAX;
    double acc = 0;
    for (int i = 0; i < NUM_ODDS; i++) {
        acc += probs[i];
        if (r < acc) return i;
    }
    return -1; /* commun */
}

static void applyReward(int oddIdx, int mult) {
    unsigned long long base = 0;
    const char *label = "";

    if (oddIdx < 0) {
        base = 1;
        label = "+1 piece";
    } else {
        switch (oddIdx) {
            case 0: /* token: free shop */
                snprintf(toastMsg, sizeof(toastMsg), "JETON BOUTIQUE!");
                toastTimer = 180;
                toastY = TOP_H / 2.0f - 20.0f;
                saveGame();
                return;
            case 1: /* skin or */
                S.skins_owned[20] = 1;
                snprintf(toastMsg, sizeof(toastMsg), "SKIN OR!");
                toastTimer = 180;
                toastY = TOP_H / 2.0f - 20.0f;
                saveGame();
                return;
            case 2: base = 5000; label = "+5000"; break;
            case 3: base = 500;  label = "+500";  break;
            case 4: base = 75;   label = "+75";   break;
            case 5: base = 10;   label = "+10";   break;
        }
    }

    unsigned long long gained = base * coinMult() * mult;
    S.coins += gained;
    snprintf(toastMsg, sizeof(toastMsg), "+%llu pieces", gained);
    toastTimer = 90;
    toastY = TOP_H / 2.0f - 20.0f;
}

/* ============================================================
 * DESSIN
 * ============================================================ */
static void drawText(float x, float y, float scale, u32 color, const char *str) {
    if (!str || !str[0]) return;
    C2D_Text t;
    C2D_TextParse(&t, tb, str);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, scale, scale, color);
}

static void drawTextCenter(float cx, float y, float scale, u32 color, const char *str) {
    if (!str || !str[0]) return;
    C2D_Text t;
    C2D_TextParse(&t, tb, str);
    C2D_TextOptimize(&t);
    float w = 0, h = 0;
    C2D_TextGetDimensions(&t, scale, scale, &w, &h);
    C2D_DrawText(&t, C2D_WithColor, cx - w/2.0f, y, 0.5f, scale, scale, color);
}

static void drawRect(float x, float y, float w, float h, u32 col) {
    C2D_DrawRectSolid(x, y, 0.3f, w, h, col);
}

static void drawCircle(float x, float y, float r, u32 col) {
    C2D_DrawCircleSolid(x, y, 0.3f, r, col);
}

static void drawButton(float x, float y, float w, float h, u32 bg, u32 shadow, const char *label, float textScale, u32 textCol) {
    /* Ombre */
    drawRect(x + 2, y + 4, w, h, shadow);
    /* Corps */
    drawRect(x, y, w, h, bg);
    /* Texte centré */
    drawTextCenter(x + w/2.0f, y + h/2.0f - textScale * 10.0f, textScale, textCol, label);
}

/* Dessine un coffre stylisé, animé */
static void drawChest(Chest *c, int highlighted) {
    int mapIdx = (S.map < S.rb) ? S.map : S.rb;
    if (mapIdx < 0) mapIdx = 0;
    if (mapIdx >= MAX_MAPS) mapIdx = MAX_MAPS - 1;
    const Map *m = &MAPS[mapIdx];

    u32 brown = m->chestBody;
    u32 lid   = m->chestLid;
    u32 gold  = m->chestGold;
    u32 dark  = C2D_Color32(30, 15, 5, 255);

    float x = c->x, y = c->y;
    float a = c->anim; /* 0=fermé 1=ouvert */

    if (highlighted && !c->open) {
        /* Halo doré */
        drawCircle(x, y, 22.0f, C2D_Color32(255, 240, 80, 60));
        drawCircle(x, y, 18.0f, C2D_Color32(255, 240, 80, 40));
    }

    if (c->rare && !c->open) {
        /* Contour violet pour coffre rare */
        drawRect(x - 17, y - 17, 34, 36, C2D_Color32(180, 60, 255, 200));
    }

    if (c->open || a > 0.5f) {
        /* Coffre ouvert */
        float liftY = -12.0f * fminf(a, 1.0f);
        drawRect(x - 15, y + liftY - 6, 30, 10, lid);
        drawRect(x - 15, y, 30, 18, dark);
        /* Éclat intérieur */
        drawRect(x - 11, y + 2, 22, 7, gold);
    } else {
        /* Coffre fermé */
        drawRect(x - 16, y - 9, 32, 20, dark);
        drawRect(x - 15, y - 8, 30, 18, brown);
        drawRect(x - 15, y - 15, 30, 9, lid);
        /* Serrure dorée */
        drawRect(x - 4, y - 8, 8, 9, gold);
        drawCircle(x, y - 8, 4.0f, gold);
        /* Bandes */
        drawRect(x - 15, y - 8, 30, 3, dark);
        drawRect(x - 15, y + 3, 30, 3, dark);
    }
}

/* Dessine le personnage */
static void drawPlayer(float x, float y, float face, float animT) {
    u32 skinCol = SKINS[S.equipped].color;
    u32 outline = C2D_Color32(20, 10, 5, 255);

    /* Ombre au sol */
    C2D_DrawEllipseSolid(x - 12, y + 22, 0.4f, 24, 7, C2D_Color32(0, 0, 0, 80));

    float legOff = sinf(animT * 8.0f) * 5.0f;
    float armOff = -legOff;

    /* Jambe gauche */
    drawRect(x - 7 + (face < 0 ? legOff : -legOff) * 0.5f, y + 12, 7, 14, skinCol);
    /* Jambe droite */
    drawRect(x + 1 + (face < 0 ? -legOff : legOff) * 0.5f, y + 12, 7, 14, skinCol);

    /* Corps */
    drawRect(x - 11, y - 2, 22, 16, skinCol);

    /* Bras gauche */
    drawRect(x - 15, y + armOff * 0.4f, 5, 12, skinCol);
    /* Bras droit */
    drawRect(x + 10, y - armOff * 0.4f, 5, 12, skinCol);

    /* Tête */
    drawRect(x - 11, y - 18, 22, 18, skinCol);

    /* Yeux */
    u32 eyeCol = C2D_Color32(20, 20, 60, 255);
    if (face >= 0) {
        drawRect(x + 1, y - 13, 4, 4, eyeCol);
        drawRect(x + 6, y - 13, 4, 4, eyeCol);
    } else {
        drawRect(x - 11, y - 13, 4, 4, eyeCol);
        drawRect(x - 5, y - 13, 4, 4, eyeCol);
    }
    /* Bouche */
    drawRect(x - 3, y - 5, 6, 2, C2D_Color32(180, 60, 60, 220));
}

/* Particules */
static void updateParticles(void) {
    for (int i = numParts - 1; i >= 0; i--) {
        parts[i].x += parts[i].dx;
        parts[i].y += parts[i].dy;
        parts[i].dy += 0.12f;
        parts[i].life -= 0.025f;
        if (parts[i].life <= 0.0f) {
            parts[i] = parts[--numParts];
        }
    }
}

static void drawParticles(void) {
    for (int i = 0; i < numParts; i++) {
        float a = parts[i].life;
        u32 col = (parts[i].color & 0xFFFFFF00) | (u8)(a * 255);
        drawCircle(parts[i].x, parts[i].y, parts[i].size * a, col);
    }
}

/* ============================================================
 * ÉCRAN LOBBY (écran du haut = décor, bas = menu)
 * ============================================================ */
static void drawLobby(C3D_RenderTarget *top, C3D_RenderTarget *bot) {
    int mapIdx = (S.map < MAX_MAPS) ? S.map : MAX_MAPS - 1;
    const Map *m = &MAPS[mapIdx];

    /* --- ÉCRAN DU HAUT : décor lobby --- */
    C2D_TargetClear(top, m->sea);
    C2D_SceneBegin(top);

    /* Sol */
    drawRect(0, TOP_H * 0.55f, TOP_W, TOP_H * 0.45f, m->ground);

    /* Logo */
    u32 gold = C2D_Color32(255, 212, 31, 255);
    u32 darkBrown = C2D_Color32(58, 21, 0, 255);
    drawTextCenter(TOP_W/2.0f, 15.0f, 0.95f, gold, "PIRATE CHEST");

    /* Coffre décoratif animé (bobbing) */
    static float lobbyT = 0;
    lobbyT += 0.03f;
    float chY = TOP_H * 0.52f + sinf(lobbyT) * 4.0f;
    Chest fakeChest = { TOP_W * 0.72f, chY, 0, 0, 0.0f };
    drawChest(&fakeChest, 0);

    /* Personnage animé sur le podium */
    static float heroT = 0;
    heroT += 0.015f;
    drawPlayer(TOP_W * 0.38f, TOP_H * 0.55f + sinf(heroT * 2.5f) * 3.0f - 20.0f,
               1.0f, heroT);

    /* Info rebirth + multiplicateur */
    char buf[80];
    snprintf(buf, sizeof(buf), "Rebirth %d | x%llu pieces | x%.2f chance",
             S.rb, coinMult(), luckVal());
    drawTextCenter(TOP_W/2.0f, TOP_H - 18.0f, 0.42f,
                   C2D_Color32(200, 230, 255, 220), buf);

    /* --- ÉCRAN DU BAS : panneau de navigation --- */
    C2D_TargetClear(bot, C2D_Color32(10, 20, 66, 255));
    C2D_SceneBegin(bot);

    /* Titre de la map */
    char mapTitle[48];
    snprintf(mapTitle, sizeof(mapTitle), "MAP : %s (%d/%d)",
             MAPS[mapIdx].name, S.map + 1, S.rb + 1);
    drawTextCenter(BOT_W/2.0f, 8.0f, 0.52f, C2D_Color32(200, 220, 255, 255), mapTitle);

    /* Flèches de map */
    drawButton(10, 30, 36, 30, C2D_Color32(50, 70, 180, 255),
               C2D_Color32(20, 30, 100, 255), "<", 0.65f, 0xFFFFFFFF);
    drawButton(BOT_W - 46, 30, 36, 30, C2D_Color32(50, 70, 180, 255),
               C2D_Color32(20, 30, 100, 255), ">", 0.65f, 0xFFFFFFFF);

    /* Bouton JOUER */
    drawButton(50, 25, BOT_W - 100, 40,
               C2D_Color32(240, 220, 20, 255),
               C2D_Color32(140, 150, 0, 255),
               "JOUER !", 0.8f, C2D_Color32(20, 10, 0, 255));

    /* Pièces */
    char coinsBuf[48];
    snprintf(coinsBuf, sizeof(coinsBuf), "Pieces: %llu", S.coins);
    drawTextCenter(BOT_W/2.0f, 76.0f, 0.58f, gold, coinsBuf);

    /* Onglets */
    u32 tabOn  = C2D_Color32(255, 255, 255, 255);
    u32 tabOff = C2D_Color32(80, 100, 200, 255);
    u32 tabTxt = C2D_Color32(20, 20, 20, 255);
    u32 tabTOff= C2D_Color32(200, 210, 255, 255);

    const char *tabs[] = {"ACCUEIL", "CASIER", "BOUTIQUE"};
    for (int i = 0; i < 3; i++) {
        int active = (curScreen == (GameScreen)i);
        float tx = 8.0f + i * 102.0f;
        drawButton(tx, 100, 100, 26,
                   active ? tabOn : tabOff,
                   active ? C2D_Color32(180,180,180,255) : C2D_Color32(20,30,100,255),
                   tabs[i], 0.42f,
                   active ? tabTxt : tabTOff);
    }

    /* Infos skin équipé */
    char skinBuf[48];
    snprintf(skinBuf, sizeof(skinBuf), "Skin: %s", SKINS[S.equipped].name);
    drawTextCenter(BOT_W/2.0f, 134.0f, 0.48f, C2D_Color32(180, 200, 255, 220), skinBuf);

    /* Niveau */
    char lvlBuf[48];
    snprintf(lvlBuf, sizeof(lvlBuf), "Niveau chance: %d / %d", S.lvl, MAXLVL);
    drawTextCenter(BOT_W/2.0f, 150.0f, 0.45f, C2D_Color32(160, 220, 160, 220), lvlBuf);

    /* Instructions */
    drawTextCenter(BOT_W/2.0f, BOT_H - 20.0f, 0.38f,
                   C2D_Color32(140, 150, 200, 200),
                   "A=Jouer  L/R=Map  D-pad=Navigation");
}

/* ============================================================
 * ÉCRAN DE JEU
 * ============================================================ */
static void drawGame(C3D_RenderTarget *top, C3D_RenderTarget *bot) {
    int mapIdx = (S.map < MAX_MAPS) ? S.map : MAX_MAPS - 1;
    const Map *m = &MAPS[mapIdx];

    /* --- ÉCRAN DU HAUT : gameplay --- */
    C2D_TargetClear(top, m->sea);
    C2D_SceneBegin(top);

    /* Sol */
    drawRect(0, TOP_H * 0.15f, TOP_W, TOP_H * 0.85f, m->ground);

    /* Bordure du sol */
    drawRect(0, TOP_H * 0.15f, TOP_W, 3, m->edge);

    /* Coffres */
    Chest *nearChest = NULL;
    float bestDist = 42.0f;
    for (int i = 0; i < MAX_CHESTS; i++) {
        float d = hypotf(chests[i].x - px, chests[i].y - py);
        if (!chests[i].open && d < bestDist) {
            bestDist = d;
            nearChest = &chests[i];
        }
    }
    for (int i = 0; i < MAX_CHESTS; i++) {
        drawChest(&chests[i], &chests[i] == nearChest);
        /* Animation ouverture */
        if (!chests[i].open && chests[i].anim > 0) {
            chests[i].anim -= 0.08f;
            if (chests[i].anim < 0) chests[i].anim = 0;
        }
    }

    /* Particules */
    drawParticles();

    /* Joueur */
    drawPlayer(px, py, pFace, pAnimT);

    /* HUD haut */
    char buf[80];
    u32 gold = C2D_Color32(255, 212, 31, 255);
    u32 white = C2D_Color32(255, 255, 255, 255);

    snprintf(buf, sizeof(buf), "%d/%d", openedCount, MAX_CHESTS);
    drawText(6, 4, 0.58f, white, buf);

    snprintf(buf, sizeof(buf), "%llu", S.coins);
    drawTextCenter(TOP_W/2.0f, 4, 0.58f, gold, buf);

    if (nearChest) {
        drawTextCenter(TOP_W/2.0f, TOP_H - 22.0f, 0.52f, gold, "Appuie sur A");
    }

    /* Toast */
    if (toastTimer > 0) {
        toastTimer--;
        toastY -= 0.3f;
        float alpha = (float)toastTimer / 90.0f;
        u32 toastCol = (u8)(alpha * 255) | 0;
        u32 col = C2D_Color32(255, 212, 31, (u8)(alpha * 255));
        drawTextCenter(TOP_W/2.0f, toastY, 0.9f, col, toastMsg);
    }

    /* Fin de map */
    if (gameDone) {
        drawRect(TOP_W/2.0f - 110, TOP_H/2.0f - 20, 220, 40,
                 C2D_Color32(0, 0, 0, 180));
        drawTextCenter(TOP_W/2.0f, TOP_H/2.0f - 12.0f, 0.7f, gold, "MAP TERMINEE !");
    }

    /* Roue bonus */
    if (wheelActive) {
        float cx2 = TOP_W/2.0f, cy2 = TOP_H/2.0f;
        /* Fond semi-transparent */
        drawRect(cx2 - 80, cy2 - 90, 160, 160, C2D_Color32(0, 0, 0, 200));
        drawTextCenter(cx2, cy2 - 82.0f, 0.65f, gold, "ROUE BONUS !");

        /* Roue simple : cercle + pointeur */
        int numSeg = 10;
        for (int i = 0; i < numSeg; i++) {
            float a1 = wheelAngle + i * (6.2832f / numSeg);
            float a2 = a1 + (6.2832f / numSeg);
            /* Dessin simplifié : tranches alternées */
            u32 col2 = (i % 2 == 0) ? C2D_Color32(255, 212, 31, 255)
                                     : C2D_Color32(50, 50, 80, 255);
            (void)a1; (void)a2;
            /* citro2d ne gère pas les arcs : on fait un polygone approché */
            float r = 60.0f;
            float mx2 = cx2 + cosf(a1 + 0.314f) * r * 0.6f;
            float my2 = cy2 + sinf(a1 + 0.314f) * r * 0.6f;
            (void)mx2; (void)my2;
            drawRect(cx2 - 60 + i * 12, cy2 - 55, 11, 90 - (i % 3) * 10, col2);
        }
        /* Pointeur */
        drawRect(cx2 - 3, cy2 - 92, 6, 20, C2D_Color32(255, 50, 50, 255));

        if (wheelDone) {
            char wbuf[32];
            snprintf(wbuf, sizeof(wbuf), "x%d !", wheelResult);
            drawTextCenter(cx2, cy2 + 20.0f, 1.2f, gold, wbuf);
            drawTextCenter(cx2, cy2 + 50.0f, 0.5f, white, "A pour continuer");
        }
    }

    /* --- ÉCRAN DU BAS : infos en cours de partie --- */
    C2D_TargetClear(bot, C2D_Color32(15, 25, 80, 255));
    C2D_SceneBegin(bot);

    drawTextCenter(BOT_W/2.0f, 5.0f, 0.65f, gold, "EN JEU");

    /* Stats */
    char s1[48], s2[48], s3[48];
    snprintf(s1, sizeof(s1), "Coffres: %d / %d", openedCount, MAX_CHESTS);
    snprintf(s2, sizeof(s2), "Pieces: %llu", S.coins);
    snprintf(s3, sizeof(s3), "Chance: x%.2f", luckVal());
    drawText(10, 35, 0.52f, white, s1);
    drawText(10, 55, 0.52f, gold, s2);
    drawText(10, 75, 0.52f, C2D_Color32(100, 220, 100, 255), s3);

    /* Barre de progression */
    float prog = (float)openedCount / MAX_CHESTS;
    drawRect(10, 100, BOT_W - 20, 14, C2D_Color32(30, 40, 100, 255));
    drawRect(10, 100, (BOT_W - 20) * prog, 14, C2D_Color32(80, 180, 255, 255));
    drawText(BOT_W/2.0f - 10, 100, 0.42f, white, "");

    /* Skin */
    char sk[48];
    snprintf(sk, sizeof(sk), "Skin: %s", SKINS[S.equipped].name);
    drawText(10, 125, 0.48f, C2D_Color32(180, 200, 255, 220), sk);

    /* Map */
    char mapName[48];
    snprintf(mapName, sizeof(mapName), "Map: %s", MAPS[(S.map < MAX_MAPS) ? S.map : 0].name);
    drawText(10, 143, 0.48f, C2D_Color32(180, 200, 255, 220), mapName);

    /* Instructions */
    drawText(10, BOT_H - 40, 0.4f, C2D_Color32(140, 150, 200, 200), "START = Pause");
    drawText(10, BOT_H - 24, 0.4f, C2D_Color32(140, 150, 200, 200), "A = Ouvrir coffre");
}

/* ============================================================
 * ÉCRAN PAUSE
 * ============================================================ */
static void drawPause(C3D_RenderTarget *top, C3D_RenderTarget *bot) {
    /* Fond jeu en arrière plan (top) */
    drawGame(top, bot);

    /* Overlay pause (top) */
    C2D_SceneBegin(top);
    drawRect(TOP_W/2.0f - 95, TOP_H/2.0f - 70, 190, 140,
             C2D_Color32(5, 10, 40, 220));
    drawTextCenter(TOP_W/2.0f, TOP_H/2.0f - 60.0f, 0.8f,
                   C2D_Color32(255, 212, 31, 255), "PAUSE");

    const char *items[] = { "Reprendre", "Lobby", "Son: ", "Quitter" };
    for (int i = 0; i < 4; i++) {
        u32 col = (i == pauseSelY) ? C2D_Color32(255, 240, 80, 255)
                                   : C2D_Color32(200, 210, 255, 255);
        char label[48];
        if (i == 2) {
            snprintf(label, sizeof(label), "Son: %s", S.mute ? "OFF" : "ON");
        } else {
            snprintf(label, sizeof(label), "%s", items[i]);
        }
        drawTextCenter(TOP_W/2.0f, TOP_H/2.0f - 30.0f + i * 24.0f, 0.6f, col, label);
    }

    /* Bas : infos */
    C2D_SceneBegin(bot);
    drawRect(0, 0, BOT_W, BOT_H, C2D_Color32(5, 10, 40, 240));
    drawTextCenter(BOT_W/2.0f, 10.0f, 0.65f,
                   C2D_Color32(255, 212, 31, 255), "MENU PAUSE");
    drawTextCenter(BOT_W/2.0f, 50.0f, 0.5f,
                   C2D_Color32(200, 210, 255, 255), "Haut/Bas : navigation");
    drawTextCenter(BOT_W/2.0f, 70.0f, 0.5f,
                   C2D_Color32(200, 210, 255, 255), "A : valider");
    drawTextCenter(BOT_W/2.0f, 90.0f, 0.5f,
                   C2D_Color32(200, 210, 255, 255), "START / B : reprendre");
}

/* ============================================================
 * ÉCRAN CASIER
 * ============================================================ */
static void drawLocker(C3D_RenderTarget *top, C3D_RenderTarget *bot) {
    C2D_TargetClear(top, C2D_Color32(8, 12, 40, 255));
    C2D_SceneBegin(top);

    u32 gold = C2D_Color32(255, 212, 31, 255);
    u32 white = C2D_Color32(255, 255, 255, 255);

    drawTextCenter(TOP_W/2.0f, 10.0f, 0.75f, gold, "CASIER");
    drawTextCenter(TOP_W/2.0f, 32.0f, 0.45f,
                   C2D_Color32(180, 200, 255, 200),
                   "Haut/Bas=naviguer  A=equiper");

    /* Liste des skins possédés */
    int y = 55;
    int visibleRows = (TOP_H - 65) / 28;
    int ownedCount = 0;
    int ownedIdx[MAX_SKINS];
    for (int i = 0; i < MAX_SKINS; i++) {
        if (S.skins_owned[i]) ownedIdx[ownedCount++] = i;
    }

    for (int r = lockerScroll; r < ownedCount && r < lockerScroll + visibleRows; r++) {
        int i = ownedIdx[r];
        int eq = (S.equipped == i);
        u32 bg = eq ? C2D_Color32(20, 100, 20, 255) : C2D_Color32(30, 40, 100, 220);
        u32 border = eq ? C2D_Color32(80, 220, 80, 255) : C2D_Color32(80, 100, 200, 180);

        drawRect(10, y - 2, TOP_W - 20, 26, C2D_Color32(0, 0, 0, 100));
        drawRect(10, y - 2, TOP_W - 20, 26, bg);
        drawRect(10, y - 2, 4, 26, border);

        /* Mini-perso coloré */
        u32 sc = SKINS[i].color;
        drawRect(18, y + 1, 10, 18, sc);
        drawCircle(23, y - 1, 7.0f, sc);

        char nameBuf[48];
        if (eq) snprintf(nameBuf, sizeof(nameBuf), "* %s [EQUIPE]", SKINS[i].name);
        else     snprintf(nameBuf, sizeof(nameBuf), "  %s", SKINS[i].name);

        drawText(36, y + 2, 0.52f, eq ? C2D_Color32(100, 255, 100, 255) : white, nameBuf);

        /* Bonus */
        char bon[48] = "";
        if (SKINS[i].spd != 1.0f)
            snprintf(bon, sizeof(bon), "Spd x%.1f", SKINS[i].spd);
        if (SKINS[i].cm != 1.0f)
            snprintf(bon + strlen(bon), sizeof(bon) - strlen(bon), " Coin x%.0f", SKINS[i].cm);
        if (SKINS[i].ch != 1.0f)
            snprintf(bon + strlen(bon), sizeof(bon) - strlen(bon), " Luck x%.1f", SKINS[i].ch);
        if (bon[0])
            drawText(190, y + 2, 0.4f, C2D_Color32(200, 240, 180, 220), bon);

        y += 28;
    }

    /* Scrollbar */
    if (ownedCount > visibleRows) {
        float ratio = (float)lockerScroll / (ownedCount - visibleRows);
        drawRect(TOP_W - 7, 55 + ratio * (TOP_H - 80), 5, 20,
                 C2D_Color32(100, 120, 220, 200));
    }

    /* Bas : infos */
    C2D_TargetClear(bot, C2D_Color32(8, 12, 40, 255));
    C2D_SceneBegin(bot);

    /* Onglets */
    const char *tabs[] = { "ACCUEIL", "CASIER", "BOUTIQUE" };
    for (int i = 0; i < 3; i++) {
        int active = (curScreen == (GameScreen)i);
        drawButton(8.0f + i * 102.0f, 4, 100, 26,
                   active ? white : C2D_Color32(50, 70, 180, 255),
                   active ? C2D_Color32(180,180,180,255) : C2D_Color32(20,30,100,255),
                   tabs[i], 0.42f,
                   active ? C2D_Color32(20, 20, 20, 255) : C2D_Color32(200, 210, 255, 255));
    }

    /* Pièces */
    char cBuf[32];
    snprintf(cBuf, sizeof(cBuf), "Pieces: %llu", S.coins);
    drawTextCenter(BOT_W/2.0f, 36.0f, 0.55f, gold, cBuf);

    /* Skin équipé détail */
    const Skin *sk = &SKINS[S.equipped];
    drawTextCenter(BOT_W/2.0f, 58.0f, 0.52f, C2D_Color32(180, 200, 255, 220),
                   "Skin equipe :");
    drawTextCenter(BOT_W/2.0f, 74.0f, 0.65f, gold, sk->name);

    char bon2[80] = "";
    if (sk->spd != 1.0f) snprintf(bon2, sizeof(bon2), "Vitesse x%.1f  ", sk->spd);
    if (sk->cm != 1.0f)  snprintf(bon2 + strlen(bon2), sizeof(bon2)-strlen(bon2), "Pieces x%.0f  ", sk->cm);
    if (sk->ch != 1.0f)  snprintf(bon2 + strlen(bon2), sizeof(bon2)-strlen(bon2), "Chance x%.1f", sk->ch);
    if (bon2[0])
        drawTextCenter(BOT_W/2.0f, 92.0f, 0.45f,
                       C2D_Color32(160, 240, 160, 220), bon2);

    drawTextCenter(BOT_W/2.0f, BOT_H - 20.0f, 0.38f,
                   C2D_Color32(140, 150, 200, 200),
                   "D-pad Haut/Bas: naviguer");
}

/* ============================================================
 * ÉCRAN BOUTIQUE
 * ============================================================ */
static void drawShop(C3D_RenderTarget *top, C3D_RenderTarget *bot) {
    C2D_TargetClear(top, C2D_Color32(8, 12, 40, 255));
    C2D_SceneBegin(top);

    u32 gold = C2D_Color32(255, 212, 31, 255);
    u32 white = C2D_Color32(255, 255, 255, 255);

    drawTextCenter(TOP_W/2.0f, 10.0f, 0.75f, gold, "BOUTIQUE");

    /* Sous-onglets : Skins / Upgrades / Rebirth */
    const char *subTabs[] = { "SKINS", "AMELIO", "REBIRTH" };
    for (int i = 0; i < 3; i++) {
        int active = (shopTab == i);
        drawButton(10.0f + i * 128.0f, 32, 125, 24,
                   active ? white : C2D_Color32(50, 70, 180, 255),
                   active ? C2D_Color32(180,180,180,255) : C2D_Color32(20,30,100,255),
                   subTabs[i], 0.45f,
                   active ? C2D_Color32(20,20,20,255) : C2D_Color32(200,210,255,255));
    }

    int y = 65;

    if (shopTab == 0) {
        /* Liste des skins */
        int visRows = (TOP_H - 70) / 30;
        for (int r = shopScroll; r < MAX_SKINS - 1 && r < shopScroll + visRows; r++) {
            /* Skins non-légendaires uniquement (index 0-19) */
            const Skin *sk = &SKINS[r];
            int owned = S.skins_owned[r];
            int price = sk->price;
            u32 bg = owned ? C2D_Color32(20, 70, 20, 220) : C2D_Color32(30, 40, 100, 220);

            drawRect(8, y - 1, TOP_W - 16, 28, bg);

            /* Couleur skin */
            drawRect(14, y + 2, 10, 20, sk->color);
            drawCircle(19, y, 7.0f, sk->color);

            /* Nom */
            drawText(32, y + 2, 0.5f, white, sk->name);

            /* Statut / prix */
            char pBuf[32];
            if (owned) {
                snprintf(pBuf, sizeof(pBuf), "POSSEDE");
                drawText(TOP_W - 85, y + 5, 0.45f, C2D_Color32(100, 220, 100, 255), pBuf);
            } else if (price == 0) {
                snprintf(pBuf, sizeof(pBuf), "GRATUIT");
                drawText(TOP_W - 85, y + 5, 0.45f, C2D_Color32(80, 200, 255, 255), pBuf);
            } else {
                snprintf(pBuf, sizeof(pBuf), "%d", price);
                drawText(TOP_W - 85, y + 5, 0.45f, gold, pBuf);
            }

            y += 30;
        }
    }
    else if (shopTab == 1) {
        /* Améliorations */
        /* Carte chance */
        int c = upCost();
        u32 cardBg = C2D_Color32(43, 140, 255, 255);
        drawRect(10, y, TOP_W - 20, 55, cardBg);
        char lBuf[64];
        snprintf(lBuf, sizeof(lBuf), "x%.2f Chance", luckVal());
        drawText(18, y + 6, 0.65f, C2D_Color32(255, 240, 80, 255), lBuf);
        char lvBuf[48];
        snprintf(lvBuf, sizeof(lvBuf), "Niv %d / %d", S.lvl, MAXLVL);
        drawText(18, y + 26, 0.5f, white, lvBuf);

        if (S.lvl >= MAXLVL) {
            drawText(TOP_W - 100, y + 18, 0.55f, C2D_Color32(100, 220, 100, 255), "MAX");
        } else {
            char cBuf2[32];
            snprintf(cBuf2, sizeof(cBuf2), "%d", c);
            drawButton(TOP_W - 108, y + 10, 95, 32,
                       C2D_Color32(10, 20, 80, 255),
                       C2D_Color32(5, 10, 40, 255),
                       cBuf2, 0.5f, gold);
        }
        y += 65;

        /* Probabilités */
        drawText(14, y, 0.5f, C2D_Color32(180, 200, 255, 220), "Probabilites des recompenses:");
        y += 18;
        for (int i = 0; i < NUM_ODDS; i++) {
            double L = luckVal();
            double p = ODDS[i].base_prob * L;
            if (p > 0.35) p = 0.35;
            char odBuf[64];
            snprintf(odBuf, sizeof(odBuf), "%s : 1/%d", ODDS[i].label,
                     (p > 0) ? (int)(1.0/p) : 999999);
            drawText(14, y, 0.42f, ODDS[i].color, odBuf);
            y += 16;
        }
    }
    else {
        /* Rebirth */
        unsigned long long rc = rbCost();
        int maxRb = (S.rb >= MAX_MAPS - 1);

        u32 rbBg = C2D_Color32(138, 43, 226, 255);
        drawRect(10, y, TOP_W - 20, 80, rbBg);

        char rb1[64];
        snprintf(rb1, sizeof(rb1), "Rebirth %d", S.rb);
        drawText(18, y + 8, 0.65f, gold, rb1);

        char rb2[80];
        snprintf(rb2, sizeof(rb2), "Multiplicateur: x%llu pieces", coinMult());
        drawText(18, y + 30, 0.5f, white, rb2);

        if (!maxRb) {
            char rb3[80];
            snprintf(rb3, sizeof(rb3), "Prochain: x%llu + map \"%s\"",
                     coinMult() * 2,
                     (S.rb + 1 < MAX_MAPS) ? MAPS[S.rb + 1].name : "?");
            drawText(18, y + 48, 0.42f, C2D_Color32(200, 180, 255, 220), rb3);

            char rcBuf[48];
            snprintf(rcBuf, sizeof(rcBuf), "%llu", rc);
            drawButton(TOP_W - 120, y + 20, 108, 36,
                       C2D_Color32(10, 20, 80, 255),
                       C2D_Color32(5, 10, 40, 255),
                       rcBuf, 0.5f, gold);
        } else {
            drawTextCenter(TOP_W/2.0f, y + 50.0f, 0.6f,
                           C2D_Color32(100, 220, 100, 255), "TOUTES LES MAPS DEBLOQUEES");
        }
        y += 90;

        /* Note rebirth */
        drawText(10, y, 0.42f, C2D_Color32(200, 180, 255, 180),
                 "Rebirth: remet a zero pieces");
        drawText(10, y + 16, 0.42f, C2D_Color32(200, 180, 255, 180),
                 "et ameliorations. Skins restes.");
    }

    /* --- ÉCRAN DU BAS --- */
    C2D_TargetClear(bot, C2D_Color32(8, 12, 40, 255));
    C2D_SceneBegin(bot);

    /* Onglets navigation */
    const char *tabs[] = { "ACCUEIL", "CASIER", "BOUTIQUE" };
    for (int i = 0; i < 3; i++) {
        int active = (curScreen == (GameScreen)i);
        drawButton(8.0f + i * 102.0f, 4, 100, 26,
                   active ? white : C2D_Color32(50, 70, 180, 255),
                   active ? C2D_Color32(180,180,180,255) : C2D_Color32(20,30,100,255),
                   tabs[i], 0.42f,
                   active ? C2D_Color32(20,20,20,255) : C2D_Color32(200,210,255,255));
    }

    /* Pièces */
    char coinsBuf[48];
    snprintf(coinsBuf, sizeof(coinsBuf), "Pieces: %llu", S.coins);
    drawTextCenter(BOT_W/2.0f, 36.0f, 0.58f, gold, coinsBuf);

    /* Infos selon onglet */
    if (shopTab == 0) {
        drawTextCenter(BOT_W/2.0f, 65.0f, 0.48f, white, "A = Acheter le skin selectionne");
        drawTextCenter(BOT_W/2.0f, 82.0f, 0.45f, C2D_Color32(180,200,255,200),
                       "Haut/Bas : naviguer");
        char selBuf[48];
        snprintf(selBuf, sizeof(selBuf), "Selectionne: %s", SKINS[shopScroll].name);
        drawTextCenter(BOT_W/2.0f, 100.0f, 0.5f, gold, selBuf);
    } else if (shopTab == 1) {
        drawTextCenter(BOT_W/2.0f, 65.0f, 0.48f, white, "A = Acheter amelioration");
        char nextBuf[48];
        if (S.lvl < MAXLVL) {
            snprintf(nextBuf, sizeof(nextBuf), "Prochain: x%.2f", luckVal() * 1.1f);
            drawTextCenter(BOT_W/2.0f, 82.0f, 0.5f, gold, nextBuf);
        }
    } else {
        drawTextCenter(BOT_W/2.0f, 65.0f, 0.48f, white, "A = Faire le Rebirth");
        drawTextCenter(BOT_W/2.0f, 82.0f, 0.42f,
                       C2D_Color32(220, 180, 255, 200),
                       "Attention: remet a zero!");
    }

    /* Sous-onglets (bas) */
    drawTextCenter(BOT_W/2.0f, BOT_H - 20.0f, 0.38f,
                   C2D_Color32(140,150,200,200),
                   "L/R=onglet  Haut/Bas=naviguer");
}

/* ============================================================
 * LOGIQUE PRINCIPALE
 * ============================================================ */
static void handleLobby(u32 kd, u32 kh) {
    (void)kh;
    /* Navigation D-pad entre onglets */
    if (kd & KEY_DLEFT) {
        if (curScreen > 0) curScreen = (GameScreen)(curScreen - 1);
        else curScreen = SCREEN_SHOP;
    }
    if (kd & KEY_DRIGHT) {
        if (curScreen < SCREEN_SHOP) curScreen = (GameScreen)(curScreen + 1);
        else curScreen = SCREEN_LOBBY;
    }
    /* Changer de map */
    if (kd & KEY_L) {
        S.map = (S.map + S.rb) % (S.rb + 1);
        saveGame();
    }
    if (kd & KEY_R) {
        S.map = (S.map + 1) % (S.rb + 1);
        saveGame();
    }
    /* Bouton A ou Start = jouer */
    if (kd & (KEY_A | KEY_START)) {
        newMap();
        curScreen = SCREEN_GAME;
    }
    /* Touch écran bas : boutons */
    if (kd & KEY_TOUCH) {
        touchPosition tp;
        hidTouchRead(&tp);
        /* Bouton < map */
        if (tp.px >= 10 && tp.px <= 46 && tp.py >= 30 && tp.py <= 60) {
            S.map = (S.map + S.rb) % (S.rb + 1);
            saveGame();
        }
        /* Bouton > map */
        if (tp.px >= BOT_W - 46 && tp.px <= BOT_W - 10 && tp.py >= 30 && tp.py <= 60) {
            S.map = (S.map + 1) % (S.rb + 1);
            saveGame();
        }
        /* Bouton JOUER */
        if (tp.px >= 50 && tp.px <= BOT_W - 50 && tp.py >= 25 && tp.py <= 65) {
            newMap();
            curScreen = SCREEN_GAME;
        }
        /* Onglets */
        for (int i = 0; i < 3; i++) {
            float tx = 8.0f + i * 102.0f;
            if (tp.px >= tx && tp.px <= tx + 100 && tp.py >= 100 && tp.py <= 126) {
                curScreen = (GameScreen)i;
            }
        }
    }
}

static void handleGame(u32 kd, u32 kh) {
    /* Roue bonus en cours */
    if (wheelActive) {
        wheelAngle += wheelSpeed;
        wheelSpeed *= 0.985f;
        if (!wheelDone && wheelSpeed < 0.015f) {
            wheelDone = 1;
            /* Résultat : 50% chance x2, sinon x1 */
            wheelResult = (rand() % 2 == 0) ? 2 : 1;
        }
        if (wheelDone && (kd & KEY_A)) {
            wheelActive = 0;
            wheelDone = 0;
            if (pendingReward >= 0) {
                applyReward(pendingReward, wheelResult);
                pendingReward = -1;
            }
            saveGame();
        }
        return;
    }

    /* Fin de map */
    if (gameDone) {
        if (kd & KEY_A) {
            curScreen = SCREEN_LOBBY;
        }
        return;
    }

    /* Pause */
    if (kd & KEY_START) {
        prevScreen = SCREEN_GAME;
        pauseSelY = 0;
        curScreen = SCREEN_PAUSE;
        return;
    }

    /* Mouvement */
    circlePosition cp;
    hidCircleRead(&cp);
    float dx = cp.dx / 156.0f;
    float dy = -cp.dy / 156.0f;
    if (kh & KEY_DLEFT)  dx = -1.0f;
    if (kh & KEY_DRIGHT) dx =  1.0f;
    if (kh & KEY_DUP)    dy = -1.0f;
    if (kh & KEY_DDOWN)  dy =  1.0f;

    float spd = 2.5f * SKINS[S.equipped].spd;
    bool moving = (fabsf(dx) > 0.1f || fabsf(dy) > 0.1f);

    if (moving) {
        px += dx * spd;
        py += dy * spd;
        pAnimT += 0.016f;
        if (fabsf(dx) > 0.1f) pFace = dx > 0 ? 1.0f : -1.0f;
    }

    /* Bords */
    if (px < 15) px = 15;
    if (px > TOP_W - 15) px = TOP_W - 15;
    if (py < 38) py = 38;
    if (py > TOP_H - 15) py = TOP_H - 15;

    /* Coffre le plus proche */
    Chest *nearChest = NULL;
    float bestDist = 42.0f;
    for (int i = 0; i < MAX_CHESTS; i++) {
        float d = hypotf(chests[i].x - px, chests[i].y - py);
        if (!chests[i].open && d < bestDist) {
            bestDist = d;
            nearChest = &chests[i];
        }
    }

    /* Ouvrir coffre */
    if ((kd & KEY_A) && nearChest) {
        nearChest->open = 1;
        nearChest->anim = 1.0f;
        openedCount++;

        /* Particules */
        u32 pCol = nearChest->rare ? C2D_Color32(180, 60, 255, 255)
                                   : C2D_Color32(255, 212, 31, 255);
        spawnParticles(nearChest->x, nearChest->y - 10, pCol, nearChest->rare ? 20 : 10);

        int reward = openChestReward(nearChest->rare);

        /* Roue bonus pour coffre rare */
        if (nearChest->rare && reward >= 2) {
            wheelActive = 1;
            wheelDone = 0;
            wheelAngle = 0;
            wheelSpeed = 0.35f;
            pendingReward = reward;
        } else {
            applyReward(reward, 1);
        }

        if (openedCount >= MAX_CHESTS) {
            gameDone = 1;
        }
        saveGame();
    }

    /* Touch = ouvrir coffre */
    if (kd & KEY_TOUCH) {
        touchPosition tp;
        hidTouchRead(&tp);
        /* Le touch sur l'écran bas pointe vers le coffre le plus proche */
        (void)tp;
        if (nearChest) {
            nearChest->open = 1;
            nearChest->anim = 1.0f;
            openedCount++;
            u32 pCol = nearChest->rare ? C2D_Color32(180, 60, 255, 255)
                                       : C2D_Color32(255, 212, 31, 255);
            spawnParticles(nearChest->x, nearChest->y - 10, pCol, nearChest->rare ? 20 : 10);
            int reward = openChestReward(nearChest->rare);
            if (nearChest->rare && reward >= 2) {
                wheelActive = 1; wheelDone = 0; wheelAngle = 0;
                wheelSpeed = 0.35f; pendingReward = reward;
            } else {
                applyReward(reward, 1);
            }
            if (openedCount >= MAX_CHESTS) gameDone = 1;
            saveGame();
        }
    }

    updateParticles();
}

static void handlePause(u32 kd) {
    if (kd & (KEY_B | KEY_START)) {
        curScreen = SCREEN_GAME;
        return;
    }
    if (kd & KEY_DUP)   if (pauseSelY > 0) pauseSelY--;
    if (kd & KEY_DDOWN) if (pauseSelY < 3) pauseSelY++;

    if (kd & KEY_A) {
        switch (pauseSelY) {
            case 0: curScreen = SCREEN_GAME; break;    /* Reprendre */
            case 1: curScreen = SCREEN_LOBBY; break;   /* Lobby */
            case 2: S.mute = !S.mute; saveGame(); break; /* Son */
            case 3:                                     /* Quitter */
                saveGame();
                curScreen = SCREEN_LOBBY;
                break;
        }
    }
}

static void handleLocker(u32 kd) {
    if (kd & KEY_DLEFT) { curScreen = SCREEN_LOBBY; return; }
    if (kd & KEY_DRIGHT){ curScreen = SCREEN_SHOP;  return; }
    if (kd & KEY_L)     { curScreen = SCREEN_LOBBY; return; }
    if (kd & KEY_R)     { curScreen = SCREEN_SHOP;  return; }

    /* Compter les skins possédés */
    int ownedCount = 0;
    int ownedIdx[MAX_SKINS];
    for (int i = 0; i < MAX_SKINS; i++)
        if (S.skins_owned[i]) ownedIdx[ownedCount++] = i;

    int visRows = (TOP_H - 65) / 28;

    if (kd & KEY_DUP)   if (lockerScroll > 0) lockerScroll--;
    if (kd & KEY_DDOWN) if (lockerScroll + visRows < ownedCount) lockerScroll++;

    if (kd & KEY_A) {
        /* Équiper le skin sélectionné */
        int row = lockerScroll;
        if (row < ownedCount) {
            S.equipped = ownedIdx[row];
            saveGame();
        }
    }

    if (kd & KEY_TOUCH) {
        touchPosition tp;
        hidTouchRead(&tp);
        /* Onglets bas */
        for (int i = 0; i < 3; i++) {
            float tx = 8.0f + i * 102.0f;
            if (tp.px >= tx && tp.px <= tx + 100 && tp.py >= 4 && tp.py <= 30) {
                curScreen = (GameScreen)i;
                return;
            }
        }
        /* Clic sur une rangée de skin */
        if (tp.py >= 4 && tp.py <= 30) return; /* onglets, déjà traité */
        /* Touch sur top-screen pas disponible pour le casier (c'est l'écran haut) */
    }
}

static void handleShop(u32 kd) {
    if (kd & KEY_DLEFT) { curScreen = SCREEN_LOCKER; return; }
    if (kd & KEY_DRIGHT){ curScreen = SCREEN_LOBBY;  return; }
    if (kd & KEY_L) {
        if (shopTab > 0) shopTab--;
        else shopTab = 2;
        shopScroll = 0;
        return;
    }
    if (kd & KEY_R) {
        if (shopTab < 2) shopTab++;
        else shopTab = 0;
        shopScroll = 0;
        return;
    }

    if (shopTab == 0) {
        int maxScroll = MAX_SKINS - 2;
        if (kd & KEY_DUP)   if (shopScroll > 0) shopScroll--;
        if (kd & KEY_DDOWN) if (shopScroll < maxScroll) shopScroll++;

        if (kd & KEY_A) {
            int i = shopScroll;
            if (i >= MAX_SKINS - 1) return; /* legend non achetable */
            int price = SKINS[i].price;
            if (!S.skins_owned[i] && S.coins >= (unsigned long long)price) {
                S.coins -= price;
                S.skins_owned[i] = 1;
                saveGame();
            }
        }
    }
    else if (shopTab == 1) {
        if (kd & KEY_A) {
            if (S.lvl < MAXLVL) {
                int c = upCost();
                if (S.coins >= (unsigned long long)c) {
                    S.coins -= c;
                    S.lvl++;
                    saveGame();
                }
            }
        }
    }
    else { /* Rebirth */
        if (kd & KEY_A) {
            if (S.rb < MAX_MAPS - 1) {
                unsigned long long rc = rbCost();
                if (S.coins >= rc) {
                    S.coins = 0;
                    S.lvl = 0;
                    S.rb++;
                    S.map = S.rb;
                    saveGame();
                }
            }
        }
    }

    if (kd & KEY_TOUCH) {
        touchPosition tp;
        hidTouchRead(&tp);
        /* Onglets navigation */
        for (int i = 0; i < 3; i++) {
            float tx2 = 8.0f + i * 102.0f;
            if (tp.px >= tx2 && tp.px <= tx2 + 100 && tp.py >= 4 && tp.py <= 30) {
                curScreen = (GameScreen)i;
                return;
            }
        }
        /* Sous-onglets (sur top screen) : via L/R uniquement ici */
        /* Bouton acheter (A géré plus haut) */
    }
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    tb = C2D_TextBufNew(TEXT_MAX);

    srand((unsigned int)time(NULL));
    loadGame();

    curScreen = SCREEN_LOBBY;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kd = hidKeysDown();
        u32 kh = hidKeysHeld();

        /* Quitter (HOME est géré par aptMainLoop) */
        if ((kd & KEY_SELECT) && (kh & KEY_START)) break;

        switch (curScreen) {
            case SCREEN_LOBBY:  handleLobby(kd, kh); break;
            case SCREEN_GAME:   handleGame(kd, kh);  break;
            case SCREEN_PAUSE:  handlePause(kd);     break;
            case SCREEN_LOCKER: handleLocker(kd);    break;
            case SCREEN_SHOP:   handleShop(kd);      break;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TextBufClear(tb);

        switch (curScreen) {
            case SCREEN_LOBBY:
                drawLobby(top, bot);
                break;
            case SCREEN_GAME:
                drawGame(top, bot);
                break;
            case SCREEN_PAUSE:
                drawPause(top, bot);
                break;
            case SCREEN_LOCKER:
                drawLocker(top, bot);
                break;
            case SCREEN_SHOP:
                drawShop(top, bot);
                break;
        }

        C3D_FrameEnd(0);
    }

    saveGame();
    C2D_TextBufDelete(tb);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
