#include <nds.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    float x, y;
    float vx, vy;
    int w, h;
    bool onGround;
    int hp;
} Entity;

typedef struct {
    int x, y, w, h;
} Platform;

void drawRect(u16* fb, int x, int y, int w, int h, u16 color) {
    for (int j = y; j < y + h; j++) {
        if (j < 0 || j >= 192) continue;
        for (int i = x; i < x + w; i++) {
            if (i < 0 || i >= 256) continue;
            fb[j * 256 + i] = color;
        }
    }
}

bool checkCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

int main(void) {
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    u16* fb = VRAM_A;

    consoleDemoInit();

    Entity player = {20, 140, 0, 0, 14, 20, false, 3};
    Entity boss = {880, 132, -1.2f, 0, 22, 28, true, 3};
    
    Platform plats[] = {
        {0, 160, 320, 32},
        {350, 160, 280, 32},
        {660, 160, 300, 32},
        {110, 120, 60, 10},
        {210, 90, 60, 10},
        {410, 115, 70, 10},
        {520, 85, 60, 10},
        {740, 115, 80, 10}
    };
    int numPlats = 8;

    Platform coins[] = {
        {135, 100, 8, 8}, {235, 70, 8, 8}, {440, 95, 8, 8},
        {545, 65, 8, 8}, {775, 95, 8, 8}
    };
    bool coinTaken[5] = {false, false, false, false, false};

    float camX = 0;
    int score = 0;
    int state = 0;

    while (1) {
        scanKeys();
        int held = keysHeld();
        int down = keysDown();

        if (state == 0) {
            player.vx = 0;
            if (held & KEY_LEFT) player.vx = -2.5f;
            if (held & KEY_RIGHT) player.vx = 2.5f;
            if ((down & KEY_A) && player.onGround) {
                player.vy = -6.2f;
                player.onGround = false;
            }

            player.vy += 0.32f;
            player.x += player.vx;
            player.y += player.vy;
            if (player.x < 0) player.x = 0;

            player.onGround = false;
            for (int i = 0; i < numPlats; i++) {
                if (player.vy >= 0 &&
                    player.x + player.w > plats[i].x && player.x < plats[i].x + plats[i].w &&
                    player.y + player.h >= plats[i].y && player.y + player.h - player.vy <= plats[i].y + 6) {
                    player.y = plats[i].y - player.h;
                    player.vy = 0;
                    player.onGround = true;
                }
            }

            if (player.y > 192) state = 2;

            for (int i = 0; i < 5; i++) {
                if (!coinTaken[i] && checkCollision(player.x, player.y, player.w, player.h, coins[i].x, coins[i].y, coins[i].w, coins[i].h)) {
                    coinTaken[i] = true;
                    score += 100;
                }
            }

            if (boss.hp > 0) {
                boss.x += boss.vx;
                if (boss.x < 700 || boss.x > 920) boss.vx = -boss.vx;

                if (checkCollision(player.x, player.y, player.w, player.h, boss.x, boss.y, boss.w, boss.h)) {
                    if (player.vy > 0 && player.y + player.h - player.vy <= boss.y + 8) {
                        boss.hp--;
                        player.vy = -5.5f;
                        score += 250;
                        if (boss.hp <= 0) state = 1;
                    } else {
                        player.hp--;
                        player.x -= 30;
                        if (player.hp <= 0) state = 2;
                    }
                }
            }

            camX = player.x - 100;
            if (camX < 0) camX = 0;
            if (camX > 704) camX = 704;
        } else {
            if (down & KEY_START) {
                player = (Entity){20, 140, 0, 0, 14, 20, false, 3};
                boss = (Entity){880, 132, -1.2f, 0, 22, 28, true, 3};
                for (int i = 0; i < 5; i++) coinTaken[i] = false;
                score = 0;
                state = 0;
            }
        }

        u16 skyColor = (camX > 550) ? RGB15(6, 2, 2) : RGB15(12, 22, 31);
        drawRect(fb, 0, 0, 256, 192, skyColor);

        for (int i = 0; i < numPlats; i++) {
            u16 pColor = (i >= 2 && camX > 550) ? RGB15(15, 15, 15) : RGB15(8, 22, 6);
            drawRect(fb, plats[i].x - (int)camX, plats[i].y, plats[i].w, plats[i].h, pColor);
        }

        for (int i = 0; i < 5; i++) {
            if (!coinTaken[i]) {
                drawRect(fb, coins[i].x - (int)camX, coins[i].y, coins[i].w, coins[i].h, RGB15(31, 28, 0));
            }
        }

        drawRect(fb, (int)(player.x - camX), (int)player.y + 6, player.w, player.h - 6, RGB15(2, 8, 28));
        drawRect(fb, (int)(player.x - camX) + 2, (int)player.y, player.w - 4, 7, RGB15(30, 22, 18));

        if (boss.hp > 0) {
            drawRect(fb, (int)(boss.x - camX), (int)boss.y + 8, boss.w, boss.h - 8, RGB15(4, 18, 4));
            drawRect(fb, (int)(boss.x - camX) + 3, (int)boss.y, boss.w - 6, 9, RGB15(30, 22, 18));
            drawRect(fb, (int)(boss.x - camX) + 4, (int)boss.y + 5, boss.w - 8, 4, RGB15(6, 4, 2));
        }

        consoleClear();
        printf("\x1b[2;2HPERRO SANCHEZ:");
        printf("\x1b[3;2HEL PODER DE ABASCAL");
        printf("\x1b[6;2HVidas: %d   Puntos: %d", player.hp, score);
        printf("\x1b[8;2HAbascal HP: %d", boss.hp);
        printf("\x1b[12;2HControles:");
        printf("\x1b[13;2H- Cruceta: Moverse");
        printf("\x1b[14;2H- Boton A: Saltar");
        printf("\x1b[16;2HSalta sobre Abascal 3 veces!");

        if (state == 1) {
            printf("\x1b[19;2H*** VICTORIA! PULSA START ***");
        } else if (state == 2) {
            printf("\x1b[19;2H*** DERROTA! PULSA START ***");
        }

        swiWaitForVBlank();
    }
    return 0;
}
