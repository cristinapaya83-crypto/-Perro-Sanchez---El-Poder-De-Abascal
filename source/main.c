
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* =====================================================
   PERRO SANCHEZ: EL PODER DE ABASCAL
   Nintendo DSi - libnds
   Version 0.2
   ===================================================== */

#define SW 256
#define SH 192
#define WORLD_W 900
#define GROUND 165
#define PLAYER_W 14
#define PLAYER_H 24
#define MAX_COINS 12
#define MAX_ITEMS 3
#define MAX_ENEMIES 5
#define LEVELS 15

#define RGB(r,g,b) (RGB15(r,g,b) | BIT(15))

typedef enum {
    MENU,
    CREDITS,
    MAP,
    PLAY,
    PAUSE,
    GAMEOVER,
    WIN
} State;

typedef enum {
    NO_ITEM,
    HACIENDA,
    COCA,
    BLACK_CARD
} ItemType;

typedef struct {
    float x, y, vx, vy;
    int grounded;
    int facing;
    int invincible;
} Player;

typedef struct {
    int x, y, active;
} Coin;

typedef struct {
    int x, y, active;
    ItemType type;
} Item;

typedef struct {
    int x, y, active;
    int direction;
    int animation;
    int skin;
} Enemy;

typedef struct {
    int x, y, w, h;
} Platform;

typedef struct {
    int x, y;
    int health;
    int active;
    int direction;
    int timer;
} Boss;

/* =====================================================
   GLOBAL VARIABLES
   ===================================================== */

static volatile u16 *fb = (volatile u16 *)0x06800000;

static State state = MENU;

static Player player;
static Coin coins[MAX_COINS];
static Item items[MAX_ITEMS];
static Enemy enemies[MAX_ENEMIES];
static Platform platforms[6];
static Boss boss;

static int lives = 6;
static int money = 0;

static int currentLevel = 0;
static int unlockedLevel = 0;
static int selectedLevel = 0;

static int cameraX = 0;
static int frame = 0;

static int cocaTimer = 0;
static int messageTimer = 0;
static int jumpCooldown = 0;
static int damageCooldown = 0;

static char message[100] = "";

static bool bossDefeated[LEVELS];
static bool coinCollected[LEVELS][MAX_COINS];
static bool itemCollected[LEVELS][MAX_ITEMS];

static bool levelCompleted[LEVELS];

static int bossHealth = 3;
static int bossMaxHealth = 3;

static bool finalBattle = false;

/* =====================================================
   LEVEL INFORMATION
   ===================================================== */

static const char *levelNames[LEVELS] = {
    "GALICIA",
    "ASTURIAS",
    "CANTABRIA",
    "PAIS VASCO",
    "NAVARRA",
    "ARAGON",
    "CATALUNA",
    "CASTILLA Y LEON",
    "LA RIOJA",
    "MADRID",
    "EXTREMADURA",
    "CASTILLA LA MANCHA",
    "VALENCIA",
    "MURCIA",
    "ANDALUCIA"
};

static const char *levelCities[LEVELS] = {
    "Santiago",
    "Oviedo",
    "Santander",
    "Bilbao",
    "Pamplona",
    "Zaragoza",
    "Barcelona",
    "Valladolid",
    "Logrono",
    "Madrid",
    "Merida",
    "Toledo",
    "Valencia",
    "Murcia",
    "La Moncloa"
};

static const u16 skyColors[LEVELS] = {
    RGB(8,15,22),
    RGB(10,18,25),
    RGB(12,18,25),
    RGB(8,12,22),
    RGB(15,18,25),
    RGB(20,17,10),
    RGB(12,18,28),
    RGB(19,15,10),
    RGB(12,17,20),
    RGB(15,15,23),
    RGB(23,15,9),
    RGB(22,17,9),
    RGB(22,18,12),
    RGB(23,17,9),
    RGB(12,5,12)
};

/* =====================================================
   GRAPHICS
   ===================================================== */

static void clearScreen(u16 color) {
    int i;

    for (i = 0; i < SW * SH; i++)
        fb[i] = color;
}

static void pixel(int x, int y, u16 color) {
    if (x >= 0 && x < SW && y >= 0 && y < SH)
        fb[y * SW + x] = color;
}

static void rect(int x, int y, int w, int h, u16 color) {
    int i, j;

    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            pixel(x + i, y + j, color);
}

static void borderRect(int x, int y, int w, int h, u16 color) {
    rect(x, y, w, 2, color);
    rect(x, y + h - 2, w, 2, color);
    rect(x, y, 2, h, color);
    rect(x + w - 2, y, 2, h, color);
}

/* =====================================================
   PIXEL ART CHARACTERS
   ===================================================== */

static void drawPedro(int x, int y) {
    u16 skin = RGB(24,17,12);
    u16 hair = RGB(8,6,4);
    u16 suit = RGB(5,8,18);
    u16 white = RGB(31,31,31);

    rect(x+3,y,9,8,skin);
    rect(x+2,y,11,3,hair);

    pixel(x+5,y+4,white);
    pixel(x+9,y+4,white);

    rect(x+6,y+7,4,1,hair);

    rect(x+2,y+9,12,10,suit);
    rect(x+6,y+9,3,8,white);

    rect(x+3,y+19,4,5,suit);
    rect(x+10,y+19,4,5,suit);
}

static void drawAbascal(int x, int y) {
    u16 skin = RGB(23,16,11);
    u16 hair = RGB(7,5,4);
    u16 suit = RGB(9,8,12);

    rect(x+3,y,10,9,skin);
    rect(x+2,y,12,3,hair);

    pixel(x+5,y+4,RGB(31,31,31));
    pixel(x+10,y+4,RGB(31,31,31));

    rect(x+4,y+6,9,3,RGB(8,6,5));
    rect(x+1,y+10,15,11,suit);

    rect(x+7,y+10,3,9,RGB(31,31,31));

    rect(x+3,y+21,4,5,suit);
    rect(x+10,y+21,4,5,suit);
}

/* Enemigos urbanos PSG ficticios */

static void drawPSGEnemy(int x, int y, int animation, int skinType) {
    u16 skin;
    u16 blue = RGB(3,8,27);
    u16 black = RGB(2,2,3);

    if (skinType == 0)
        skin = RGB(10,7,5);
    else if (skinType == 1)
        skin = RGB(19,13,9);
    else
        skin = RGB(25,18,13);

    /* Gorra */
    rect(x+2,y,12,4,black);
    rect(x+5,y-2,8,3,RGB(20,16,8));

    /* Cabeza */
    rect(x+4,y+4,9,8,skin);

    /* Ojos */
    pixel(x+6,y+7,RGB(31,31,31));
    pixel(x+10,y+7,RGB(31,31,31));

    /* Chandal */
    rect(x+2,y+12,14,9,blue);
    rect(x+8,y+12,2,8,RGB(31,3,5));

    /* Piernas */
    if (animation % 2 == 0) {
        rect(x+3,y+21,4,6,blue);
        rect(x+11,y+21,4,4,blue);
    } else {
        rect(x+3,y+21,4,4,blue);
        rect(x+11,y+21,4,6,blue);
    }
}

/* Mini jefe regional */

static void drawRegionalBoss(int x, int y) {
    rect(x+4,y,18,17,RGB(24,17,12));
    rect(x+3,y,20,5,RGB(7,6,5));

    pixel(x+9,y+8,RGB(31,31,31));
    pixel(x+17,y+8,RGB(31,31,31));

    rect(x+2,y+17,22,22,RGB(10,10,18));
    rect(x+9,y+18,7,16,RGB(31,31,31));

    rect(x+4,y+39,6,7,RGB(5,5,8));
    rect(x+16,y+39,6,7,RGB(5,5,8));
}

/* =====================================================
   BACKGROUNDS
   ===================================================== */

static void drawBackground(void) {
    int i;

    clearScreen(skyColors[currentLevel]);

    /* Sol */
    rect(210,20,16,16,RGB(31,26,13));

    /* Montanas */
    for (i=0; i<5; i++) {
        int x = (i*80 - cameraX/4) % 320;

        if (x < -50) x += 320;

        rect(x,110,48,30,RGB(8,10,14));
        rect(x+12,98,25,12,RGB(12,13,17));
    }

    /* Elementos de ciudad */
    if (currentLevel == 6) {
        /* Barcelona */
        rect(170,65,25,80,RGB(20,17,12));
        rect(177,48,12,17,RGB(24,18,12));
        rect(174,45,18,3,RGB(31,22,9));
    }

    if (currentLevel == 9) {
        /* Madrid */
        rect(175,65,45,80,RGB(18,13,15));
        rect(185,45,25,20,RGB(23,17,15));
    }

    if (currentLevel == 12) {
        /* Valencia */
        rect(30,130,60,12,RGB(18,14,8));
    }

    /* Suelo */
    rect(0,GROUND,SW,27,RGB(6,18,8));
    rect(0,GROUND,SW,3,RGB(15,25,10));

    for (i=0; i<8; i++) {
        int x = (i*45-cameraX/2)%280;
        if (x < 0) x += 280;
        rect(x,145,4,8,RGB(5,20,7));
    }
}

/* =====================================================
   MAP
   ===================================================== */

static void drawMap(void) {
    int i;

    clearScreen(RGB(5,10,18));

    /* Silueta aproximada */
    rect(48,40,150,95,RGB(12,20,12));
    rect(65,30,110,15,RGB(12,20,12));
    rect(38,55,20,65,RGB(12,20,12));
    rect(180,125,35,18,RGB(12,20,12));

    /* Marcadores */
    for (i=0; i<LEVELS; i++) {
        int x = 52+(i%5)*37;
        int y = 35+(i/5)*39;

        u16 c = RGB(10,18,10);

        if (i <= unlockedLevel)
            c = RGB(12,27,12);

        if (i == selectedLevel)
            c = RGB(31,25,5);

        if (i == 14)
            c = RGB(25,5,5);

        rect(x,y,12,12,c);
        rect(x+3,y+3,6,6,RGB(31,31,31));
    }

    {
        int x = 52+(selectedLevel%5)*37;
        int y = 35+(selectedLevel/5)*39;
        borderRect(x-3,y-3,18,18,RGB(31,20,2));
    }
}

/* =====================================================
   INITIALIZE LEVEL
   ===================================================== */

static void initLevel(void) {
    int i;

    player.x = 15;
    player.y = 130;
    player.vx = 0;
    player.vy = 0;
    player.grounded = 0;
    player.facing = 1;
    player.invincible = 0;

    cameraX = 0;

    cocaTimer = 0;
    jumpCooldown = 0;
    damageCooldown = 0;

    bossMaxHealth = (currentLevel == 14) ? 8 : 3;
    bossHealth = bossDefeated[currentLevel] ? 0 : bossMaxHealth;

    boss.x = 720;
    boss.y = GROUND-42;
    boss.health = bossHealth;
    boss.active = !bossDefeated[currentLevel];
    boss.direction = -1;
    boss.timer = 0;

    finalBattle = (currentLevel == 14);

    for (i=0; i<6; i++) {
        platforms[i].x = 125+i*125;
        platforms[i].y = 125-(i%2)*25;
        platforms[i].w = 45;
        platforms[i].h = 8;
    }

    for (i=0; i<MAX_COINS; i++) {
        coins[i].x = 100+i*65;
        coins[i].y = 105-(i%3)*12;
        coins[i].active = !coinCollected[currentLevel][i];
    }

    for (i=0; i<MAX_ITEMS; i++) {
        items[i].x = 220+i*240;
        items[i].y = 110;
        items[i].active = !itemCollected[currentLevel][i];

        if (i==0)
            items[i].type = HACIENDA;
        else if (i==1)
            items[i].type = COCA;
        else
            items[i].type = BLACK_CARD;
    }

    /* Hacienda muy poco frecuente */
    if (currentLevel != 3 &&
        currentLevel != 8 &&
        currentLevel != 13) {
        items[0].active = 0;
    }

    for (i=0; i<MAX_ENEMIES; i++) {
        enemies[i].x = 180+i*115;
        enemies[i].y = GROUND-27;
        enemies[i].active = 1;
        enemies[i].direction = -1;
        enemies[i].animation = 0;
        enemies[i].skin = i%3;
    }

    if (bossDefeated[currentLevel])
        boss.active = 0;
}

/* =====================================================
   MESSAGE
   ===================================================== */

static void showMessage(const char *text) {
    strncpy(message,text,sizeof(message)-1);
    message[sizeof(message)-1]='\0';
    messageTimer=150;
}

/* =====================================================
   LIFE SYSTEM
   ===================================================== */

static void loseLife(void) {
    lives--;

    if (lives <= 0) {
        lives=6;
        money=0;

        memset(coinCollected,0,sizeof(coinCollected));
        memset(itemCollected,0,sizeof(itemCollected));
        memset(bossDefeated,0,sizeof(bossDefeated));
        memset(levelCompleted,0,sizeof(levelCompleted));

        currentLevel=0;
        unlockedLevel=0;
        selectedLevel=0;

        state=GAMEOVER;
    } else {
        initLevel();
        showMessage("Has perdido una vida!");
    }
}

/* =====================================================
   ITEMS
   ===================================================== */

static void collectItem(int i) {
    if (!items[i].active)
        return;

    items[i].active=0;
    itemCollected[currentLevel][i]=true;

    if (items[i].type == HACIENDA) {
        money /= 2;
        showMessage("HACIENDA! Pierdes el 50% de monedas.");
    }

    if (items[i].type == COCA) {
        cocaTimer=360;
        showMessage("COCA! Velocidad aumentada.");
    }

    if (items[i].type == BLACK_CARD) {
        money+=100;
        showMessage("TARJETA BLACK! +100 MONEDAS.");
    }
}

/* =====================================================
   COLLISION
   ===================================================== */

static bool collision(int x1,int y1,int w1,int h1,
                      int x2,int y2,int w2,int h2) {
    return x1 < x2+w2 &&
           x1+w1 > x2 &&
           y1 < y2+h2 &&
           y1+h1 > y2;
}

/* =====================================================
   PLAYER UPDATE
   ===================================================== */

static void updatePlayer(u32 held,u32 pressed) {
    float speed = cocaTimer>0 ? 3.5f : 2.0f;

    player.vx=0;

    /* A = avanzar */
    if (held & KEY_A) {
        player.vx=speed;
        player.facing=1;
    }

    /* Y = retroceder */
    if (held & KEY_Y) {
        player.vx=-speed;
        player.facing=-1;
    }

    /* B = saltar */
    if ((pressed & KEY_B) && player.grounded) {
        player.vy=-6.0f;
        player.grounded=0;
    }

    player.x+=player.vx;

    if (player.x<0)
        player.x=0;

    if (player.x>WORLD_W-PLAYER_W)
        player.x=WORLD_W-PLAYER_W;

    /* Gravedad */
    player.vy+=0.25f;

    if (player.vy>6.0f)
        player.vy=6.0f;

    player.y+=player.vy;

    player.grounded=0;

    /* Suelo */
    if (player.y+PLAYER_H>=GROUND) {
        player.y=GROUND-PLAYER_H;
        player.vy=0;
        player.grounded=1;
    }

    /* Plataformas */
    {
        int i;

        for (i=0;i<6;i++) {
            Platform p=platforms[i];

            if (player.vy>=0 &&
                player.x+PLAYER_W>p.x &&
                player.x<p.x+p.w &&
                player.y+PLAYER_H>=p.y &&
                player.y+PLAYER_H<=p.y+12) {

                player.y=p.y-PLAYER_H;
                player.vy=0;
                player.grounded=1;
            }
        }
    }

    /* Caida */
    if (player.y>SH+30)
        loseLife();

    if (player.invincible>0)
        player.invincible--;

    if (damageCooldown>0)
        damageCooldown--;

    if (jumpCooldown>0)
        jumpCooldown--;

    if (cocaTimer>0)
        cocaTimer--;
}

/* =====================================================
   COLLECT COINS
   ===================================================== */

static void updateCoins(void) {
    int i;

    for (i=0;i<MAX_COINS;i++) {
        if (!coins[i].active)
            continue;

        if (collision((int)player.x,(int)player.y,
                      PLAYER_W,PLAYER_H,
                      coins[i].x,coins[i].y,10,10)) {

            coins[i].active=0;
            coinCollected[currentLevel][i]=true;
            money++;
        }
    }
}

/* =====================================================
   COLLECT SPECIAL ITEMS
   ===================================================== */

static void updateItems(void) {
    int i;

    for (i=0;i<MAX_ITEMS;i++) {
        if (!items[i].active)
            continue;

        if (collision((int)player.x,(int)player.y,
                      PLAYER_W,PLAYER_H,
                      items[i].x,items[i].y,16,16)) {
            collectItem(i);
        }
    }
}

/* =====================================================
   ENEMIES
   ===================================================== */

static void updateEnemies(void) {
    int i;

    for (i=0;i<MAX_ENEMIES;i++) {
        if (!enemies[i].active)
            continue;

        enemies[i].x+=enemies[i].direction;
        enemies[i].animation++;

        if (enemies[i].x<30)
            enemies[i].direction=1;

        if (enemies[i].x>WORLD_W-40)
            enemies[i].direction=-1;

        if (collision((int)player.x,(int)player.y,
                      PLAYER_W,PLAYER_H,
                      enemies[i].x,enemies[i].y,18,27)) {

            if (player.vy>0 &&
                player.y+PLAYER_H<enemies[i].y+15) {

                enemies[i].active=0;
                player.vy=-4.0f;
                money+=5;

            } else if (damageCooldown==0 &&
                       player.invincible==0) {

                damageCooldown=60;
                player.invincible=60;
                loseLife();
                return;
            }
        }
    }
}

/* =====================================================
   MINI BOSSES AND FINAL BOSS
   ===================================================== */

static void updateBoss(void) {
    if (!boss.active)
        return;

    boss.timer++;

    if (boss.timer%60==0)
        boss.direction=-boss.direction;

    boss.x+=boss.direction;

    if (boss.x<620)
        boss.direction=1;

    if (boss.x>760)
        boss.direction=-1;

    if (collision((int)player.x,(int)player.y,
                  PLAYER_W,PLAYER_H,
                  boss.x,boss.y,28,46)) {

        if (player.vy>1.0f &&
            player.y+PLAYER_H<boss.y+15) {

            boss.health--;
            bossHealth=boss.health;
            player.vy=-5.0f;

            if (boss.health<=0) {
                boss.active=0;
                bossDefeated[currentLevel]=true;
                money+=50;
                showMessage("MINI JEFE DERROTADO! +50");
            }

        } else if (damageCooldown==0 &&
                   player.invincible==0) {

            damageCooldown=60;
            player.invincible=60;
            loseLife();
        }
    }
}

/* =====================================================
   LEVEL PROGRESSION
   ===================================================== */

static void updateProgress(void) {
    if (!boss.active && player.x>WORLD_W-70) {

        levelCompleted[currentLevel]=true;

        if (currentLevel<14) {
            if (currentLevel>=unlockedLevel)
                unlockedLevel=currentLevel+1;

            selectedLevel=currentLevel+1;
            state=MAP;
        } else {
            state=WIN;
        }
    }
}

/* =====================================================
   CAMERA
   ===================================================== */

static void updateCamera(void) {
    cameraX=(int)player.x-100;

    if (cameraX<0)
        cameraX=0;

    if (cameraX>WORLD_W-SW)
        cameraX=WORLD_W-SW;
}

/* =====================================================
   GAME RENDERING
   ===================================================== */

static void drawGame(void) {
    int i;
    int sx;

    drawBackground();

    /* Platforms */
    for (i=0;i<6;i++) {
        sx=platforms[i].x-cameraX;
        rect(sx,platforms[i].y,platforms[i].w,
             platforms[i].h,RGB(10,20,10));
        rect(sx,platforms[i].y,platforms[i].w,
             2,RGB(20,28,12));
    }

    /* Coins */
    for (i=0;i<MAX_COINS;i++) {
        if (coins[i].active) {
            sx=coins[i].x-cameraX;
            rect(sx+2,coins[i].y,6,10,RGB(31,23,2));
            rect(sx,coins[i].y+2,10,6,RGB(31,27,5));
        }
    }

    /* Items */
    for (i=0;i<MAX_ITEMS;i++) {
        if (!items[i].active)
            continue;

        sx=items[i].x-cameraX;

        if (items[i].type==HACIENDA) {
            rect(sx,items[i].y,16,16,RGB(25,25,25));
            rect(sx+3,items[i].y+3,10,10,RGB(5,5,5));
            pixel(sx+7,items[i].y+6,RGB(31,5,5));
        }

        if (items[i].type==COCA) {
            rect(sx+3,items[i].y,10,16,RGB(5,25,5));
            rect(sx+5,items[i].y+3,6,10,RGB(31,31,31));
        }

        if (items[i].type==BLACK_CARD) {
            rect(sx,items[i].y+3,18,11,RGB(2,2,3));
            rect(sx+3,items[i].y+6,5,4,RGB(31,22,5));
        }
    }

    /* Enemies */
    for (i=0;i<MAX_ENEMIES;i++) {
        if (enemies[i].active) {
            sx=enemies[i].x-cameraX;
            drawPSGEnemy(sx,enemies[i].y,
                         enemies[i].animation,
                         enemies[i].skin);
        }
    }

    /* Mini boss */
    if (boss.active) {
        sx=boss.x-cameraX;

        if (currentLevel==14)
            drawAbascal(sx,boss.y);
        else
            drawRegionalBoss(sx,boss.y);
    }

    /* Player */
    if (player.invincible==0 || frame%8<4)
        drawPedro((int)player.x-cameraX,(int)player.y);

    /* Exit */
    if (!boss.active)
        rect(WORLD_W-45-cameraX,GROUND-35,
             25,35,RGB(20,15,5));

    /* Boss health bar */
    if (boss.active) {
        rect(75,8,110,8,RGB(10,3,3));
        rect(77,10,
             (boss.health*106)/bossMaxHealth,4,
             RGB(28,3,3));
    }

    /* Message */
    if (messageTimer>0) {
        borderRect(15,42,226,30,RGB(31,25,5));
        rect(17,44,222,26,RGB(3,5,12));
    }
}

/* =====================================================
   MENU RENDERING
   ===================================================== */

static void drawMenu(void) {
    clearScreen(RGB(4,5,16));

    drawPedro(40,80);
    drawAbascal(200,80);

    rect(50,35,156,3,RGB(31,20,5));
    rect(50,38,156,3,RGB(25,5,5));

    rect(75,120,105,22,RGB(10,25,10));
    borderRect(75,120,105,22,RGB(31,25,5));

    rect(75,150,105,22,RGB(10,10,20));
    borderRect(75,150,105,22,RGB(31,31,31));
}

/* =====================================================
   CREDITS
   ===================================================== */

static void drawCredits(void) {
    int i;

    clearScreen(RGB(10,5,18));

    /* Instagram pixel art */
    borderRect(100,35,55,55,RGB(31,20,20));
    borderRect(107,42,41,41,RGB(31,31,31));
    borderRect(116,51,23,23,RGB(31,31,31));

    rect(139,45,5,5,RGB(31,20,20));

    /* Decoracion */
    for (i=0;i<5;i++)
        rect(25+i*45,120,10,10,RGB(15,5,20));
}

/* =====================================================
   GAME OVER
   ===================================================== */

static void drawGameOver(void) {
    clearScreen(RGB(15,2,3));
    rect(45,65,166,60,RGB(5,2,2));
    borderRect(45,65,166,60,RGB(31,5,5));
}

/* =====================================================
   VICTORY
   ===================================================== */

static void drawVictory(void) {
    clearScreen(RGB(5,18,8));
    drawPedro(115,75);
    rect(70,120,116,4,RGB(31,25,3));
    rect(85,135,86,4,RGB(31,31,31));
}

/* =====================================================
   PAUSE
   ===================================================== */

static void drawPause(void) {
    drawGame();
    rect(65,65,126,60,RGB(3,4,12));
    borderRect(65,65,126,60,RGB(31,25,5));
}

/* =====================================================
   BOTTOM SCREEN CONSOLE
   ===================================================== */

static void updateConsole(void) {
    consoleClear();

    iprintf("\n  PERRO SANCHEZ\n");
    iprintf("  EL PODER DE ABASCAL\n");
    iprintf("--------------------------\n\n");

    if (state==MENU) {
        iprintf("  A - JUGAR\n");
        iprintf("  Y - CREDITOS\n");
        iprintf("\n  Pulsa START para comenzar");
    }

    else if (state==CREDITS) {
        iprintf("  INSTAGRAM\n\n");
        iprintf("  juaan.pd\n\n");
        iprintf("  B - VOLVER");
    }

    else if (state==MAP) {
        iprintf("  MAPA DE ESPANA\n\n");
        iprintf("  NIVEL: %s\n",levelNames[selectedLevel]);
        iprintf("  CIUDAD: %s\n",levelCities[selectedLevel]);
        iprintf("\n  IZQ/DCHA - ELEGIR\n");
        iprintf("  A - ENTRAR\n");
        iprintf("  B - VOLVER");
    }

    else if (state==PLAY || state==PAUSE) {
        iprintf("  NIVEL %d / 15\n",currentLevel+1);
        iprintf("  %s\n",levelNames[currentLevel]);
        iprintf("  CIUDAD: %s\n",levelCities[currentLevel]);

        iprintf("\n  VIDAS: %d\n",lives);
        iprintf("  MONEDAS: %d\n",money);

        if (cocaTimer>0)
            iprintf("  COCA ACTIVA!\n");

        if (boss.active)
            iprintf("  JEFE: %d / %d\n",
                    boss.health,bossMaxHealth);

        if (messageTimer>0)
            iprintf("\n  %s\n",message);

        if (state==PAUSE)
            iprintf("\n  PAUSA - START CONTINUAR");
    }

    else if (state==GAMEOVER) {
        iprintf("  GAME OVER\n");
        iprintf("\n  HAS PERDIDO TODAS LAS VIDAS\n");
        iprintf("\n  A - MENU");
    }

    else if (state==WIN) {
        iprintf("  ENHORABUENA!\n");
        iprintf("\n  HAS DERROTADO A ABASCAL\n");
        iprintf("\n  A - VOLVER AL MENU");
    }
}

/* =====================================================
   MAIN
   ===================================================== */

int main(void) {
    u32 pressed;
    u32 held;

    /* Pantalla superior framebuffer */
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);

    /* Pantalla inferior consola */
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(NULL,0,
                BgType_Text4bpp,
                BgSize_T_256x256,
                31,0,false,true);

    consoleClear();

    memset(bossDefeated,0,sizeof(bossDefeated));
    memset(coinCollected,0,sizeof(coinCollected));
    memset(itemCollected,0,sizeof(itemCollected));
    memset(levelCompleted,0,sizeof(levelCompleted));

    lives=6;
    money=0;
    currentLevel=0;
    unlockedLevel=0;
    selectedLevel=0;

    initLevel();

    while (1) {
        scanKeys();

        pressed=keysDown();
        held=keysHeld();

        frame++;

        if (messageTimer>0)
            messageTimer--;

        switch (state) {

            case MENU:

                drawMenu();

                if (pressed & KEY_A) {
                    state=MAP;
                }

                if (pressed & KEY_Y) {
                    state=CREDITS;
                }

                if (pressed & KEY_START) {
                    state=MAP;
                }

                break;

            case CREDITS:

                drawCredits();

                if (pressed & KEY_B)
                    state=MENU;

                break;

            case MAP:

                drawMap();

                if (pressed & KEY_LEFT) {
                    selectedLevel--;
                    if (selectedLevel<0)
                        selectedLevel=0;
                }

                if (pressed & KEY_RIGHT) {
                    selectedLevel++;
                    if (selectedLevel>=LEVELS)
                        selectedLevel=LEVELS-1;
                }

                if (pressed & KEY_UP) {
                    selectedLevel-=5;
                    if (selectedLevel<0)
                        selectedLevel=0;
                }

                if (pressed & KEY_DOWN) {
                    selectedLevel+=5;
                    if (selectedLevel>=LEVELS)
                        selectedLevel=LEVELS-1;
                }

                if (pressed & KEY_A) {
                    if (selectedLevel<=unlockedLevel) {
                        currentLevel=selectedLevel;
                        initLevel();
                        state=PLAY;
                    }
                }

                if (pressed & KEY_B)
                    state=MENU;

                break;

            case PLAY:

                if (pressed & KEY_START) {
                    state=PAUSE;
                    break;
                }

                updatePlayer(held,pressed);
                updateCoins();
                updateItems();
                updateEnemies();
                updateBoss();
                updateProgress();
                updateCamera();

                drawGame();

                break;

            case PAUSE:

                drawPause();

                if (pressed & KEY_START)
                    state=PLAY;

                if (pressed & KEY_B)
                    state=MAP;

                break;

            case GAMEOVER:

                drawGameOver();

                if (pressed & KEY_A)
                    state=MENU;

                break;

            case WIN:

                drawVictory();

                if (pressed & KEY_A)
                    state=MENU;

                break;
        }

        /* Actualizar informacion inferior */
        if (frame%10==0)
            updateConsole();

        swiWaitForVBlank();
    }

    return 0;
}


