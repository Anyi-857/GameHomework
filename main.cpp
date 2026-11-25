// ============================================================
// 大鱼吃小鱼 - EasyX 小游戏（C 风格写法，需按 C++ 编译）
// ------------------------------------------------------------
// 布局（按你的原型图）：
//   - 竖屏窗口 600 x 800（宽:高 ≈ 3:4）。
//   - 顶部 HUD：左上「得分」、中间「存活时间」、右上略低「当前生命」。
//
// 本次新增功能：
//   1) 开始界面（标题 + 按任意键/点击开始）
//   2) 游戏结束界面有可点击的「重新开始」按钮（也可按任意键重开）
//   3) 水中持续上浮的气泡粒子特效（bubble.png 或图形绘制）
//   4) 多种敌人鱼（enemy / enemy2 / enemy3，不同颜色/种类，随机出现）
//
// 怎么运行：
//   1. 安装 Visual Studio（社区版免费）+ EasyX（https://easyx.cn）。
//   2. 新建“空项目 / 控制台应用”，本文件存为 main.cpp 加进工程。
//   3. 编译运行即可。没有图片也能跑（自动用图形绘制兜底）。
//   4. 把 bg.jpg / player.png / enemy.png / enemy2.png / enemy3.png
//      （气泡暂用代码画，无需图片）放到 exe 同目录，重开就自动换成贴图。
//
// 玩法：
//   - 方向键 ↑↓←→ 控制小鱼上下左右移动（敌人只能左右移动）。
//   - 你的体型比敌人大时碰到它 = 吃掉它，体型 +3，得分 +10。
//   - 敌人比你大时碰到你 = 扣 1 条生命（共 3 条），随后短暂无敌闪烁；
//     生命扣光 = 游戏结束。
//   - 随“存活时间”和“你的体型”增大，敌人的体型也会变大（越来越难）。
// ============================================================

#include <graphics.h>   // EasyX 图形库
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>

#pragma comment(lib, "Msimg32.lib")  // AlphaBlend 透明贴图需要

// ---------- 窗口与游戏参数 ----------
const int WIN_W = 600;        // 窗口宽（竖屏，窄）
const int WIN_H = 800;        // 窗口高（竖屏，高）
const int HUD_H = 70;         // 顶部信息栏高度
const int MAX_ENEMY = 60;     // 屏幕最多敌人数量
const int START_LIVES = 3;    // 初始生命数
const int MAX_PLAYER_SIZE = 110; // 玩家体型上限（防止鱼撑满屏幕 / 边界判断反转卡死）
const int SPAWN_INTERVAL = 40;// 约每 0.6 秒生成一个敌人
const int BUBBLE_N = 40;      // 气泡数量

// 重新开始按钮的位置与大小（结束界面用）
const int BTN_W = 200, BTN_H = 60;
const int BTN_X = (WIN_W - BTN_W) / 2;
const int BTN_Y = WIN_H / 2 + 120;

// 游戏状态机
enum GameState { STATE_START, STATE_PLAY, STATE_OVER };
GameState gameState = STATE_START;

// ---------- 工具：文件是否存在 ----------
bool fileExists(LPCTSTR path) {
    DWORD attr = GetFileAttributes(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

// 注意：EasyX 里 GetImageHDC() 与 GetImageBuffer() 不能对同一个 IMAGE 混用，
// 否则会触发 0xc0000005 崩溃。所以这里统一只用 HDC + GDI。

// ---------- 工具：画背景（不透明，GDI 缩放拷贝） ----------
void drawBg(IMAGE* img, int x, int y, int w, int h) {
    HDC hdc = GetImageHDC(NULL);
    HDC src = GetImageHDC(img);
    StretchBlt(hdc, x, y, w, h, src, 0, 0, img->getwidth(), img->getheight(), SRCCOPY);
}

// ---------- 工具：画精灵（PNG 带 alpha 透明通道，用 AlphaBlend 贴图） ----------
void drawSprite(IMAGE* img, int x, int y, int w, int h) {
    HDC hdc = GetImageHDC(NULL);
    HDC src = GetImageHDC(img);
    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;   // 使用图片自身的透明通道
    AlphaBlend(hdc, x, y, w, h, src, 0, 0, img->getwidth(), img->getheight(), bf);
}

// ---------- 工具：水平翻转图片（朝左游时用） ----------
IMAGE* flipImageH(IMAGE* src) {
    int w = src->getwidth(), h = src->getheight();
    IMAGE* dst = new IMAGE(w, h);
    DWORD* s = GetImageBuffer(src);
    DWORD* d = GetImageBuffer(dst);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            d[y * w + (w - 1 - x)] = s[y * w + x];
    return dst;
}

// ---------- 玩家 ----------
struct Player {
    int x, y, size, speed, dir;
};

// ---------- 敌人（含种类 type） ----------
struct Enemy {
    int x, y, size, speed, dir, type;  // type: 0/1/2 三种敌人
    bool active;
};

// ---------- 气泡 ----------
struct Bubble {
    int x, y, r, spd;
};

// ---------- 图片资源 ----------
IMAGE imgBg, imgPlayer, imgEnemy, imgEnemy2, imgEnemy3, imgBubble;
IMAGE* imgPlayerL = NULL;
IMAGE* imgEnemyL[3] = { NULL, NULL, NULL };  // 三种敌人的朝左翻转图
bool useBg = false, usePlayer = false, useBubble = false;
bool useEnemyType[3] = { false, false, false };

Player player;
Enemy enemies[MAX_ENEMY];
Bubble bubbles[BUBBLE_N];
int score = 0;
int lives = START_LIVES;
int invuln = 0;             // 受伤后的无敌帧数
int spawnTimer = 0;
double startTime = 0;

// ============================================================
// 加载图片资源（程序启动只调用一次）
// ============================================================
void loadAssets() {
    // 背景：优先 bg.jpg，其次 bg.png（你的原图是 jpg）
    if (fileExists(_T("bg.jpg")))      { loadimage(&imgBg, _T("bg.jpg"));  useBg = true; }
    else if (fileExists(_T("bg.png"))) { loadimage(&imgBg, _T("bg.png")); useBg = true; }
    if (fileExists(_T("player.png")))  { loadimage(&imgPlayer, _T("player.png"));  usePlayer = true; imgPlayerL = flipImageH(&imgPlayer); }
    if (fileExists(_T("enemy.png")))   { loadimage(&imgEnemy, _T("enemy.png"));    useEnemyType[0] = true; imgEnemyL[0] = flipImageH(&imgEnemy); }
    if (fileExists(_T("enemy2.png")))  { loadimage(&imgEnemy2, _T("enemy2.png"));  useEnemyType[1] = true; imgEnemyL[1] = flipImageH(&imgEnemy2); }
    if (fileExists(_T("enemy3.png")))  { loadimage(&imgEnemy3, _T("enemy3.png"));  useEnemyType[2] = true; imgEnemyL[2] = flipImageH(&imgEnemy3); }
    // 注意：bubble.png 是“白气泡+浅灰白底”，颜色键会把气泡一起抠掉，
    // 所以气泡统一用代码画的（drawBubbles 的形状兜底），这里不加载它。
}

// ============================================================
// 初始化 / 重置一局游戏（只重置玩法状态，不重复加载图片）
// ============================================================
void initGame() {
    player.x = WIN_W / 2; player.y = WIN_H / 2;
    player.size = 30; player.speed = 5; player.dir = 1;
    score = 0; lives = START_LIVES; invuln = 0; spawnTimer = 0;
    startTime = (double)clock() / CLOCKS_PER_SEC;
    for (int i = 0; i < MAX_ENEMY; i++) enemies[i].active = false;

    // 初始化气泡（从底部到顶部随机分布）
    for (int i = 0; i < BUBBLE_N; i++) {
        bubbles[i].x = rand() % WIN_W;
        bubbles[i].y = HUD_H + rand() % (WIN_H - HUD_H);
        bubbles[i].r = 2 + rand() % 5;
        bubbles[i].spd = 1 + rand() % 2;
    }
}

// ============================================================
// 生成一个敌人（从左边或右边进入，只能左右移动，随机种类）
// ============================================================
void spawnEnemy() {
    int idx = -1;
    for (int i = 0; i < MAX_ENEMY; i++)
        if (!enemies[i].active) { idx = i; break; }
    if (idx == -1) return;

    double t = (double)clock() / CLOCKS_PER_SEC - startTime;

    // 难度：敌人基础体型随“存活时间”和“玩家体型”增大
    // 系数需足够大，否则敌人永远比玩家小、游戏无法结束。
    int baseSize = (int)(16 + t * 1.1 + (player.size - 30) * 0.7);
    if (baseSize < 12) baseSize = 12;

    Enemy* e = &enemies[idx];
    e->active = true;
    e->dir = (rand() % 2 == 0) ? 1 : -1;
    e->type = rand() % 3;                                  // 随机三种敌人
    e->size = baseSize + rand() % (baseSize / 2 + 6) - baseSize / 4;
    if (e->size < 8) e->size = 8;
    e->speed = 1 + rand() % 3 + (int)(t / 30);
    e->y = HUD_H + 30 + rand() % (WIN_H - HUD_H - 60);     // 纵向固定（只左右移动）
    if (e->dir == 1) e->x = -e->size;
    else             e->x = WIN_W + e->size;
}

// ============================================================
// 更新敌人位置（只左右移动，y 不变）
// ============================================================
void updateEnemies() {
    for (int i = 0; i < MAX_ENEMY; i++) {
        if (!enemies[i].active) continue;
        Enemy* e = &enemies[i];
        e->x += e->speed * e->dir;
        if ((e->dir == 1  && e->x - e->size > WIN_W) ||
            (e->dir == -1 && e->x + e->size < 0)) {
            e->active = false;
        }
    }
    if (invuln > 0) invuln--;
}

// ============================================================
// 气泡：向上漂浮，飘出顶部后在底部重生
// ============================================================
void updateBubbles() {
    for (int i = 0; i < BUBBLE_N; i++) {
        bubbles[i].y -= bubbles[i].spd;
        if (bubbles[i].y + bubbles[i].r < HUD_H) {
            bubbles[i].y = WIN_H + bubbles[i].r;
            bubbles[i].x = rand() % WIN_W;
        }
    }
}

// ============================================================
// 碰撞检测：大吃小规则 + 得分 + 生命
// ============================================================
void checkCollision() {
    for (int i = 0; i < MAX_ENEMY; i++) {
        if (!enemies[i].active) continue;
        Enemy* e = &enemies[i];
        int dx = player.x - e->x;
        int dy = player.y - e->y;
        int dist = (int)sqrt((double)(dx * dx + dy * dy));
        if (dist < player.size + e->size) {
            if (player.size > e->size) {
                e->active = false;
                if (player.size < MAX_PLAYER_SIZE) player.size += 3;  // 体型增长（有上限）
                score += 10;          // 每吃一个 +10 分
            } else if (e->size > player.size) {
                e->active = false;    // 碰到大鱼就移除，避免无敌结束后被同一条鱼连扣
                if (invuln <= 0) {    // 无敌期间不扣命
                    lives--;
                    invuln = 90;      // 约 1.5 秒无敌闪烁
                }
            }
            // 体型相近则互不伤害
        }
    }
}

// ============================================================
// 键盘操控：玩家可以上下左右移动
// ============================================================
void handleInput() {
    if (GetAsyncKeyState(VK_UP)    & 0x8000) player.y -= player.speed;
    if (GetAsyncKeyState(VK_DOWN)  & 0x8000) player.y += player.speed;
    if (GetAsyncKeyState(VK_LEFT)  & 0x8000) { player.x -= player.speed; player.dir = -1; }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { player.x += player.speed; player.dir = 1; }

    if (player.x < player.size)              player.x = player.size;
    if (player.x > WIN_W - player.size)      player.x = WIN_W - player.size;
    if (player.y < HUD_H + player.size)      player.y = HUD_H + player.size;
    if (player.y > WIN_H - player.size)      player.y = WIN_H - player.size;
}

// ============================================================
// 图形兜底：椭圆身 + 三角尾 + 眼睛
// ============================================================
void drawFishShape(int x, int y, int size, int dir, int bodyColor, int tailColor) {
    int r = size;
    setfillcolor(bodyColor);
    solidellipse(x - r, y - (int)(r * 0.6), x + r, y + (int)(r * 0.6));
    setfillcolor(tailColor);
    POINT pts[3];
    if (dir == 1) {
        pts[0] = { x - r, y };
        pts[1] = { x - r - (int)(r * 0.6), y - (int)(r * 0.5) };
        pts[2] = { x - r - (int)(r * 0.6), y + (int)(r * 0.5) };
    } else {
        pts[0] = { x + r, y };
        pts[1] = { x + r + (int)(r * 0.6), y - (int)(r * 0.5) };
        pts[2] = { x + r + (int)(r * 0.6), y + (int)(r * 0.5) };
    }
    solidpolygon(pts, 3);
    setfillcolor(WHITE);
    int ex = (dir == 1) ? x + (int)(r * 0.5) : x - (int)(r * 0.5);
    solidcircle(ex, y - (int)(r * 0.15), (int)(r * 0.18));
    setfillcolor(BLACK);
    solidcircle(ex, y - (int)(r * 0.15), (int)(r * 0.08));
}

void drawPlayer() {
    if (invuln > 0 && ((int)(clock() / 80) % 2 == 0)) return;  // 受伤闪烁
    int d = player.size * 2;
    if (usePlayer) {
        if (player.dir == 1) drawSprite(&imgPlayer, player.x - player.size, player.y - player.size, d, d);
        else                 drawSprite(imgPlayerL, player.x - player.size, player.y - player.size, d, d);
    } else {
        drawFishShape(player.x, player.y, player.size, player.dir, RGB(255, 165, 0), RGB(255, 120, 0));
    }
}

// 三种敌人对应的图像指针与兜底颜色
IMAGE* enemyImg[3] = { &imgEnemy, &imgEnemy2, &imgEnemy3 };
void drawEnemy(Enemy* e) {
    int d = e->size * 2, t = e->type;
    if (useEnemyType[t]) {
        if (e->dir == 1) drawSprite(enemyImg[t], e->x - e->size, e->y - e->size, d, d);
        else             drawSprite(imgEnemyL[t], e->x - e->size, e->y - e->size, d, d);
    } else {
        int body = (t == 0) ? RGB(200, 60, 60) : (t == 1) ? RGB(60, 160, 80) : RGB(140, 80, 200);
        int tail = (t == 0) ? RGB(150, 30, 30) : (t == 1) ? RGB(40, 120, 60) : RGB(100, 50, 160);
        drawFishShape(e->x, e->y, e->size, e->dir, body, tail);
    }
}

void drawBubbles() {
    // 气泡统一用代码画的（你生成的 bubble.png 是白气泡+浅灰白底，白底抠图会把它一起抠掉）
    for (int i = 0; i < BUBBLE_N; i++) {
        setfillcolor(RGB(220, 240, 255));
        solidcircle(bubbles[i].x, bubbles[i].y, bubbles[i].r);
        setfillcolor(RGB(255, 255, 255));
        solidcircle(bubbles[i].x - bubbles[i].r / 3, bubbles[i].y - bubbles[i].r / 3, bubbles[i].r / 3);
    }
}

// ============================================================
// 顶部信息栏（得分 / 存活时间 / 当前生命）
// ============================================================
void drawHUD() {
    setfillcolor(RGB(20, 40, 70));
    solidrectangle(0, 0, WIN_W, HUD_H);
    settextcolor(WHITE);
    settextstyle(22, 0, _T("微软雅黑"));

    wchar_t s1[64];
    swprintf(s1, 64, _T("得分: %d"), score);
    outtextxy(12, 14, s1);

    double t = (double)clock() / CLOCKS_PER_SEC - startTime;
    wchar_t s2[64];
    swprintf(s2, 64, _T("存活时间: %d秒"), (int)t);
    int w2 = textwidth(s2);
    outtextxy((WIN_W - w2) / 2, 14, s2);

    wchar_t s3[64];
    swprintf(s3, 64, _T("当前生命: %d"), lives);
    int w3 = textwidth(s3);
    outtextxy(WIN_W - 12 - w3, 40, s3);
}

// ============================================================
// 开始界面
// ============================================================
void drawStartScreen() {
    settextcolor(WHITE);
    settextstyle(56, 0, _T("微软雅黑"));
    const wchar_t* title = _T("大鱼吃小鱼");
    int wt = textwidth(title);
    outtextxy((WIN_W - wt) / 2, 200, title);

    drawFishShape(WIN_W / 2, 360, 40, 1, RGB(255, 165, 0), RGB(255, 120, 0));

    settextstyle(24, 0, _T("微软雅黑"));
    const wchar_t* hint = _T("按任意键 或 点击屏幕 开始");
    int wh = textwidth(hint);
    outtextxy((WIN_W - wh) / 2, 470, hint);

    settextstyle(20, 0, _T("微软雅黑"));
    const wchar_t* ctrl = _T("方向键 ↑↓←→ 控制小鱼移动");
    int wc = textwidth(ctrl);
    outtextxy((WIN_W - wc) / 2, 530, ctrl);
    const wchar_t* rule = _T("体型比敌人大才能吃掉它，否则扣生命");
    int wr = textwidth(rule);
    outtextxy((WIN_W - wr) / 2, 565, rule);
}

// ============================================================
// 游戏结束界面（含可点击的“重新开始”按钮）
// ============================================================
void drawOverScreen() {
    setfillcolor(RGB(0, 0, 0));
    solidrectangle(0, 0, WIN_W, WIN_H);   // 变暗

    settextcolor(RGB(255, 80, 80));
    settextstyle(48, 0, _T("微软雅黑"));
    const wchar_t* go = _T("游戏结束");
    int wgo = textwidth(go);
    outtextxy((WIN_W - wgo) / 2, WIN_H / 2 - 130, go);

    settextcolor(WHITE);
    settextstyle(26, 0, _T("微软雅黑"));
    wchar_t buf[64];
    swprintf(buf, 64, _T("最终得分: %d"), score);
    int wb = textwidth(buf); outtextxy((WIN_W - wb) / 2, WIN_H / 2 - 50, buf);
    swprintf(buf, 64, _T("存活时间: %d秒"), (int)((double)clock() / CLOCKS_PER_SEC - startTime));
    wb = textwidth(buf); outtextxy((WIN_W - wb) / 2, WIN_H / 2 - 10, buf);

    // 重新开始按钮
    setfillcolor(RGB(40, 120, 200));
    solidrectangle(BTN_X, BTN_Y, BTN_X + BTN_W, BTN_Y + BTN_H);
    settextcolor(WHITE);
    settextstyle(26, 0, _T("微软雅黑"));
    const wchar_t* bt = _T("重新开始");
    int wbt = textwidth(bt);
    outtextxy(BTN_X + (BTN_W - wbt) / 2, BTN_Y + (BTN_H - 26) / 2, bt);

    settextstyle(18, 0, _T("微软雅黑"));
    const wchar_t* h2 = _T("（也可按任意键重开）");
    int wh2 = textwidth(h2);
    outtextxy((WIN_W - wh2) / 2, BTN_Y + BTN_H + 24, h2);
}

// 判断点是否落在矩形内
bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

// ============================================================
// 主循环
// ============================================================
int main() {
    initgraph(WIN_W, WIN_H);
    srand((unsigned)time(NULL));
    loadAssets();
    gameState = STATE_START;

    BeginBatchDraw();   // 双缓冲：整帧画完再一次性刷新，消除闪烁
    while (true) {
        // ---------- 输入处理（按状态分流） ----------
        if (gameState == STATE_START) {
            if (_kbhit()) {
                _getch(); initGame(); gameState = STATE_PLAY;
            } else if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN) { initGame(); gameState = STATE_PLAY; }
            }
        } else if (gameState == STATE_PLAY) {
            handleInput();
            updateEnemies();
            updateBubbles();
            checkCollision();
            if (lives <= 0) {
                gameState = STATE_OVER;
            } else {
                spawnTimer++;
                if (spawnTimer >= SPAWN_INTERVAL) { spawnTimer = 0; spawnEnemy(); }
            }
        } else if (gameState == STATE_OVER) {
            bool restart = false;
            if (_kbhit()) {
                _getch(); restart = true;
            } else if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN && inRect(m.x, m.y, BTN_X, BTN_Y, BTN_W, BTN_H))
                    restart = true;
            }
            if (restart) { initGame(); gameState = STATE_PLAY; }
        }

        // ---------- 绘制 ----------
        if (useBg) drawBg(&imgBg, 0, 0, WIN_W, WIN_H);
        else { setbkcolor(RGB(30, 120, 200)); cleardevice(); }

        if (gameState == STATE_START) {
            drawStartScreen();
        } else {
            drawBubbles();
            for (int i = 0; i < MAX_ENEMY; i++)
                if (enemies[i].active) drawEnemy(&enemies[i]);
            drawPlayer();
            drawHUD();
            if (gameState == STATE_OVER) drawOverScreen();
        }

        FlushBatchDraw();
        Sleep(16);
    }
    closegraph();
    return 0;
}
