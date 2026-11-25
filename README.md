# 俄罗斯方块（C 语言控制台版）

## 编译运行

```bash
# MinGW / Dev-C++
gcc tetris.c -o tetris.exe && tetris.exe

# MSVC（Visual Studio 开发者命令行，需加 /utf-8 防止中文乱码）
cl /utf-8 tetris.c && tetris.exe
```

## 玩法

| 按键 | 功能 |
|------|------|
| ← → | 左右移动 |
| ↑ | 旋转方块 |
| ↓ | 加速下落一格 |
| 空格 | 直接落到底 |
| Q | 退出 |

7 种方块形状（I、O、T、S、Z、J、L），整行填满自动消除并计分，方块堆积到顶部则游戏结束。
