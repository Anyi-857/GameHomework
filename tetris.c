/* 俄罗斯方块 Tetris - 初学者版
 * 运行环境：Windows（使用了 conio.h 与 windows.h）
 * 编译方式：
 *   - 安装了 MinGW/GCC：gcc tetris.c -o tetris.exe
 *   - 或用 Dev-C++ / Code::Blocks 直接打开本文件编译运行
 *
 * 操作说明：
 *   ← →   左右移动
 *   ↑      旋转
 *   ↓      加速下落一格
 *   空格    直接落到底
 *   Q       退出
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>    // _kbhit(): 是否有按键；_getch(): 读取按键(不回显)
#include <windows.h>  // Sleep(): 让程序暂停若干毫秒

#define WIDTH 10    // 游戏区宽度（列数）
#define HEIGHT 20   // 游戏区高度（行数）
#define BLOCK 4     // 每个方块用 4x4 的小矩阵来表示

/* 游戏区(棋盘)：0 表示空格，1~7 表示已经固定下来的方块 */
int board[HEIGHT][WIDTH];

/* 7 种俄罗斯方块的基础形状，每种用 4x4 矩阵描述（1=有格子，0=空）
 * 顺序：I, O, T, S, Z, J, L */
int shapes[7][BLOCK][BLOCK] = {
    /* I */ {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    /* O */ {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    /* T */ {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    /* S */ {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    /* Z */ {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    /* J */ {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    /* L */ {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
};

/* 当前正在下落的方块 */
int piece[BLOCK][BLOCK];   // 方块的 4x4 形状（可能已经旋转过）
int pieceType;              // 当前方块类型 0~6
int pieceRow;               // 方块左上角在 board 中的行号
int pieceCol;               // 方块左上角在 board 中的列号

int score = 0;              // 得分
int lines = 0;              // 已消除的行数
int gameOver = 0;           // 游戏结束标志

/* 把 4x4 矩阵顺时针旋转 90 度（用于旋转方块） */
void rotate(int src[BLOCK][BLOCK], int dst[BLOCK][BLOCK]) {
    for (int i = 0; i < BLOCK; i++)
        for (int j = 0; j < BLOCK; j++)
            dst[j][BLOCK - 1 - i] = src[i][j];
}

/* 碰撞检测：判断方块放在 (row,col) 是否会出界或撞到已有的方块。
 * 返回 1 表示“不能放”，返回 0 表示“可以放”。 */
int collide(int p[BLOCK][BLOCK], int row, int col) {
    for (int i = 0; i < BLOCK; i++) {
        for (int j = 0; j < BLOCK; j++) {
            if (!p[i][j]) continue;            // 空格子不用管
            int r = row + i;
            int c = col + j;
            if (r < 0 || r >= HEIGHT || c < 0 || c >= WIDTH) return 1; // 越界
            if (board[r][c] != 0) return 1;    // 撞到已固定的方块
        }
    }
    return 0;
}

/* 生成新方块：随机选一种，放到顶部中间 */
void spawn() {
    pieceType = rand() % 7;
    for (int i = 0; i < BLOCK; i++)
        for (int j = 0; j < BLOCK; j++)
            piece[i][j] = shapes[pieceType][i][j];
    pieceRow = 0;
    pieceCol = (WIDTH - BLOCK) / 2;            // 居中
    if (collide(piece, pieceRow, pieceCol))    // 一出生就放不下 = 游戏结束
        gameOver = 1;
}

/* 把当前方块“焊死”到游戏区里 */
void lockPiece() {
    for (int i = 0; i < BLOCK; i++)
        for (int j = 0; j < BLOCK; j++)
            if (piece[i][j]) {
                int r = pieceRow + i;
                int c = pieceCol + j;
                if (r >= 0 && r < HEIGHT && c >= 0 && c < WIDTH)
                    board[r][c] = pieceType + 1;
            }
}

/* 消行判定：从下往上扫，遇到填满的行就删掉，并把上面的行整体下移 */
void clearLines() {
    int cleared = 0;
    for (int r = HEIGHT - 1; r >= 0; ) {
        int full = 1;
        for (int c = 0; c < WIDTH; c++)
            if (board[r][c] == 0) { full = 0; break; }
        if (full) {
            cleared++;
            /* 上面所有行整体下移一行 */
            for (int k = r; k > 0; k--)
                for (int c = 0; c < WIDTH; c++)
                    board[k][c] = board[k - 1][c];
            for (int c = 0; c < WIDTH; c++) board[0][c] = 0; // 顶行清空
            /* 不移动 r，因为下移后这一行还要再检查一次 */
        } else {
            r--;
        }
    }
    lines += cleared;
    /* 得分：一次消的行越多，得分越高 */
    if (cleared == 1) score += 100;
    else if (cleared == 2) score += 300;
    else if (cleared == 3) score += 500;
    else if (cleared == 4) score += 800;
}

/* 绘制整个游戏画面 */
void draw() {
    system("cls");
    /* 把已固定的方块复制到一个临时缓冲区 */
    int buf[HEIGHT][WIDTH];
    for (int r = 0; r < HEIGHT; r++)
        for (int c = 0; c < WIDTH; c++)
            buf[r][c] = board[r][c];
    /* 把正在下落的方块盖到缓冲区上 */
    for (int i = 0; i < BLOCK; i++)
        for (int j = 0; j < BLOCK; j++)
            if (piece[i][j]) {
                int r = pieceRow + i, c = pieceCol + j;
                if (r >= 0 && r < HEIGHT && c >= 0 && c < WIDTH)
                    buf[r][c] = pieceType + 1;
            }
    /* 上边框 */
    printf("  ");
    for (int c = 0; c < WIDTH; c++) printf("-");
    printf("\n");
    /* 每一行 */
    for (int r = 0; r < HEIGHT; r++) {
        printf("|");
        for (int c = 0; c < WIDTH; c++)
            printf(buf[r][c] ? "#" : " ");
        printf("|\n");
    }
    /* 下边框 */
    printf("  ");
    for (int c = 0; c < WIDTH; c++) printf("-");
    printf("\n");
    printf("得分: %d    消除行数: %d\n", score, lines);
    printf("方向键: 左/右 移动, 上 旋转, 下 加速, 空格 直接落下, Q 退出\n");
}

int main() {
    SetConsoleOutputCP(CP_UTF8);   /* 控制台切换为 UTF-8，中文不再乱码 */
    srand((unsigned)time(NULL));   // 用当前时间做随机种子
    /* 清空游戏区 */
    for (int r = 0; r < HEIGHT; r++)
        for (int c = 0; c < WIDTH; c++)
            board[r][c] = 0;

    spawn();
    draw();

    int dropInterval = 500;  // 每 500 毫秒自动下落一格
    int elapsed = 0;         // 已经累计的时间

    while (!gameOver) {
        /* 1) 处理按键（非阻塞：有按键才处理，不卡住下落） */
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 224 || ch == 0) {        // 方向键是双字节，首字节为 224 或 0
                int key = _getch();
                if (key == 75) {               // ←
                    if (!collide(piece, pieceRow, pieceCol - 1)) pieceCol--;
                } else if (key == 77) {        // →
                    if (!collide(piece, pieceRow, pieceCol + 1)) pieceCol++;
                } else if (key == 72) {        // ↑ 旋转
                    int rotated[BLOCK][BLOCK];
                    rotate(piece, rotated);
                    if (!collide(rotated, pieceRow, pieceCol))
                        for (int i = 0; i < BLOCK; i++)
                            for (int j = 0; j < BLOCK; j++)
                                piece[i][j] = rotated[i][j];
                } else if (key == 80) {        // ↓ 加速下落
                    if (!collide(piece, pieceRow + 1, pieceCol)) { pieceRow++; elapsed = 0; }
                }
            } else if (ch == ' ') {            // 空格：直接落到底
                while (!collide(piece, pieceRow + 1, pieceCol)) pieceRow++;
                lockPiece(); clearLines(); spawn();
            } else if (ch == 'q' || ch == 'Q') {
                break;
            }
            draw();
        }

        /* 2) 计时自动下落 */
        Sleep(50);
        elapsed += 50;
        if (elapsed >= dropInterval) {
            elapsed = 0;
            if (!collide(piece, pieceRow + 1, pieceCol)) {
                pieceRow++;                    // 还能往下，就移动一格
            } else {
                lockPiece();                   // 不能往下了，固定下来
                clearLines();                  // 看看能不能消行
                spawn();                       // 生成下一个方块
            }
            draw();
        }
    }

    draw();
    printf("游戏结束！最终得分: %d，消除行数: %d\n", score, lines);
    system("pause");
    return 0;
}
