#include <nds.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define COL_BLACK   RGB15(0,0,0)
#define COL_WHITE   RGB15(31,31,31)
#define COL_SKIN    RGB15(30,22,18)
#define COL_HAIR    RGB15(5,4,3)
#define COL_SUIT    RGB15(2,7,25)
#define COL_RED     RGB15(30,3,3)
#define COL_GREEN   RGB15(4,20,4)
#define COL_GOLD    RGB15(31,26,0)
#define COL_GRAY    RGB15(16,16,16)
#define COL_DARK    RGB15(6,6,8)

const char* FACE_SANCHEZ[16] = {
    "0000222222220000",
    "0002222222222000",
    "0022222222222200",
    "0021111111111200",
    "0011111111111100",
    "0333333113333330",
    "0322223333222230",
    "0322223113222230",
    "0033331111333300",
    "0011111111111100",
    "0011116666111100",
    "0001111111111000",
    "0000111111110000",
    "0033664444663300",
    "0333366446633330",
    "3333336446333333"
};

const char* FACE_ABASCAL[16] = {
    "0000222222220000",
    "0002222222222000",
    "0022111111112200",
    "0021111111111200",
    "0011222112221100",
    "0011262112621100",
    "0011111111111100",
    "0021111111111200",
    "0022111111112200",
    "0022222222222200",
    "0022221111222200",
    "0022222222222200",
    "0002222222222000",
    "0055522222255500",
    "0555556666555550",
    "5555555665555555"
};

const char* ICON_IG[16] = {
    "0044444444444400",
    "0447777777777440",
    "4470000000000744",
    "4700000000077074",
    "4700006666077074",
    "4700066006600074",
    "4700660000660074",
    "4700660000660074",
    "4700660000660074",
    "4700066006600074",
    "4700006666000074",
    "4700000000000074",
    "4700000000000074",
    "4470000000000744",
    "0447777777777440",
    "0044444444444400"
};

typedef struct {
    float x, y, vx, vy;
    int w, h;
    bool onGround;
    int speedTimer;
    int invulnTimer;
} Player;

typedef struct {
    int x, y, w, h, type;
    bool used;
} Platform;

typedef struct {
    float x, y, vx;
    int w, h, type;
    bool active;
} Item;

const char* CITY_NAMES[6] = {
    "1. GALICIA (Santiago)",
    "2. CATALUNA (Barcelona)",
    "3. ANDALUCIA (Sevilla)",
    "4. VALENCIA (Artes)",
    "5. PAIS VASCO (Bilbao)",
    "6. MADRID (Moncloa - FINAL)"
};
const int MAP_X[6] = {62, 182, 105, 162, 128, 118};
const int MAP_Y[6] = {42, 58, 142, 105, 38, 86};

Player p;
Platform plats[20];
int numPlats = 0;
Item items[25];
int numItems = 0;
float bossX = 860, bossY = 128, bossVx = -1.4f;
int bossHp = 5;
float fireX = -100, fireY = 140, fireVx = -2.5f;
int fireTimer = 0;
float camX = 0;
char bannerMsg[48] = "";
int msgTimer = 0;

void playBeep(int freq) {
    soundPlayPSG(DutyCycle_50, freq, 90, 64);
}

u16 getPalColor(char c) {
    switch(c) {
        case '1': return COL_SKIN;
        case '2': return COL_HAIR;
        case '3': return COL_SUIT;
        case '4': return COL_RED;
        case '5': return COL_GREEN;
        case '6': return COL_WHITE;
        case '7': return COL_GOLD;
        default: return 0;
    }
}

void drawRect(u16* fb, int x, int y, int w, int h, u16 color) {
    for (int j = y; j < y + h; j++) {
        if (j < 0 || j >= 192) continue;
        for (int i = x; i < x + w; i++) {
            if (i < 0 || i >= 256) continue;
            fb[j * 256 + i] = color;
        }
    }
}

void drawSprite16Scaled(u16* fb, const char* spr[16], int x, int y, int scale) {
    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c++) {
            u16 col = getPalColor(spr[r][c]);
            if (col != 0) {
                drawRect(fb, x + c * scale, y + r * scale, scale, scale, col);
            }
        }
    }
}

bool hitBox(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void loadLevel(int city) {
    p = (Player){20, 130, 0, 0, 16, 24, false, 0, 0};
    camX = 0;
    numPlats = 0;
    numItems = 0;
    bannerMsg[0] = '\0';
    msgTimer = 0;

    plats[numPlats++] = (Platform){0, 162, 260, 30, 0, false};
    plats[numPlats++] = (Platform){295 + (city * 5), 162, 250, 30, 0, false};
    plats[numPlats++] = (Platform){580, 162, 380, 30, 0, false};

    plats[numPlats++] = (Platform){90, 118, 55, 10, 1, false};
    plats[numPlats++] = (Platform){165, 112, 16, 16, 2, false};
    plats[numPlats++] = (Platform){220, 85, 60, 10, 1, false};
    plats[numPlats++] = (Platform){340, 115, 16, 16, 2, false};
    plats[numPlats++] = (Platform){400, 90, 75, 10, 1, false};
    plats[numPlats++] = (Platform){500, 115, 16, 16, 2, false};
    plats[numPlats++] = (Platform){650, 110, 80, 10, 1, false};

    int coinCoords[8][2] = {{110,98},{235,65},{255,65},{420,70},{445,70},{610,142},{680,90},{750,142}};
    for (int i = 0; i < 8; i++) {
        items[numItems++] = (Item){(float)coinCoords[i][0], (float)coinCoords[i][1], 0, 8, 8, 0, true};
    }

    items[numItems++] = (Item){200, 146, -0.8f, 14, 16, 4, true};
    items[numItems++] = (Item){430, 146, -1.0f, 14, 16, 4, true};
    if (city >= 2) items[numItems++] = (Item){690, 146, -1.2f, 14, 16, 4, true};

    if (city % 2 == 1) {
        items[numItems++] = (Item){315, 142, 0, 12, 14, 1, true};
    }
    items[numItems++] = (Item){240, 68, 0, 10, 14, 2, true};
    items[numItems++] = (Item){685, 92, 0, 14, 10, 3, true};

    if (city == 5) {
        bossX = 850; bossY = 128; bossVx = -1.5f; bossHp = 5;
        fireX = -100;
    }
}

int main(void) {
    soundEnable();
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    u16* fb = VRAM_A;
    consoleDemoInit();

    int screenState = 0;
    int menuChoice = 0;
    int selectedCity = 0;
    int unlockedCity = 0;
    int lives = 6;
    int coins = 0;
    int goalX = 920;

    while (1) {
        scanKeys();
        int down = keysDown();
        int held = keysHeld();
        touchPosition touch;
        touchRead(&touch);

        if (screenState == 0) {
            if (down & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) {
                menuChoice = 1 - menuChoice;
                playBeep(600);
            }
            if (down & KEY_TOUCH) {
                if (touch.py > 80 && touch.py < 130) { menuChoice = 0; down |= KEY_A; }
                if (touch.py >= 130 && touch.py < 180) { menuChoice = 1; down |= KEY_A; }
            }
            if (down & (KEY_A | KEY_START)) {
                playBeep(900);
                if (menuChoice == 0) {
                    lives = 6; coins = 0; unlockedCity = 0; selectedCity = 0;
                    screenState = 2;
                } else {
                    screenState = 1;
                }
            }

            drawRect(fb, 0, 0, 256, 192, RGB15(3, 6, 16));
            drawRect(fb, 8, 8, 240, 52, RGB15(26, 4, 4));
            drawRect(fb, 12, 12, 232, 44, COL_GOLD);
            drawRect(fb, 15, 15, 226, 38, RGB15(22, 3, 3));

            drawSprite16Scaled(fb, FACE_SANCHEZ, 26, 76, 3);
            drawSprite16Scaled(fb, FACE_ABASCAL, 182, 76, 3);

            drawRect(fb, 34, 142, 84, 28, menuChoice == 0 ? COL_GOLD : COL_GRAY);
            drawRect(fb, 37, 145, 78, 22, menuChoice == 0 ? COL_RED : COL_DARK);
            drawRect(fb, 138, 142, 84, 28, menuChoice == 1 ? COL_GOLD : COL_GRAY);
            drawRect(fb, 141, 145, 78, 22, menuChoice == 1 ? COL_SUIT : COL_DARK);

            consoleClear();
            printf("\x1b[2;4HPERRO SANCHEZ:");
            printf("\x1b[3;4HEL PODER DE ABASCAL");
            printf("\x1b[8;5H[ JUGAR ]      [ CREDITOS ]");
            printf("\x1b[12;3HToca aqui o usa Cruceta + A");
            printf("\x1b[15;4H> %s <", menuChoice == 0 ? "EMPEZAR PARTIDA" : "VER CREDITOS");

        } else if (screenState == 1) {
            if (down & (KEY_B | KEY_A | KEY_START | KEY_TOUCH)) {
                playBeep(500);
                screenState = 0;
            }
            drawRect(fb, 0, 0, 256, 192, RGB15(8, 3, 14));
            drawSprite16Scaled(fb, ICON_IG, 96, 45, 4);

            consoleClear();
            printf("\x1b[3;10H=== CREDITOS ===");
            printf("\x1b[7;7HCreado por:");
            printf("\x1b[9;7HInstagram: @juaan.pd");
            printf("\x1b[14;5HPulsa B o toca para volver");

        } else if (screenState == 2) {
            if (down & (KEY_RIGHT | KEY_DOWN)) {
                selectedCity = (selectedCity + 1) % 6;
                if (selectedCity > unlockedCity) selectedCity = 0;
                playBeep(650);
            }
            if (down & (KEY_LEFT | KEY_UP)) {
                selectedCity--;
                if (selectedCity < 0) selectedCity = unlockedCity;
                playBeep(650);
            }
            if (down & (KEY_A | KEY_START | KEY_TOUCH)) {
                playBeep(880);
                loadLevel(selectedCity);
                screenState = 3;
            }

            drawRect(fb, 0, 0, 256, 192, RGB15(4, 14, 26));
            drawRect(fb, 54, 32, 138, 22, RGB15(8, 22, 8));
            drawRect(fb, 62, 54, 134, 34, RGB15(18, 24, 8));
            drawRect(fb, 66, 88, 112, 34, RGB15(24, 20, 8));
            drawRect(fb, 72, 122, 86, 32, RGB15(26, 22, 10));
            drawRect(fb, 94, 154, 34, 12, RGB15(26, 22, 10));
            drawRect(fb, 202, 95, 16, 10, RGB15(22, 22, 10));
            drawRect(fb, 222, 92, 8, 6, RGB15(22, 22, 10));
            drawRect(fb, 16, 155, 14, 8, RGB15(14, 20, 10));
            drawRect(fb, 34, 158, 12, 8, RGB15(14, 20, 10));

            for (int i = 0; i < 6; i++) {
                u16 dotCol = (i == selectedCity) ? COL_GOLD : (i <= unlockedCity ? COL_RED : COL_GRAY);
                drawRect(fb, MAP_X[i] - 4, MAP_Y[i] - 4, 8, 8, COL_WHITE);
                drawRect(fb, MAP_X[i] - 3, MAP_Y[i] - 3, 6, 6, dotCol);
            }
            drawSprite16Scaled(fb, FACE_SANCHEZ, MAP_X[selectedCity] - 8, MAP_Y[selectedCity] - 20, 1);

            consoleClear();
            printf("\x1b[2;3HMAPA DE ESPANA - ELIGE CIUDAD");
            printf("\x1b[5;2HDestino: %s", CITY_NAMES[selectedCity]);


