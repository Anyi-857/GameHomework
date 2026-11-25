# 游戏作业合集

本仓库包含三个课程作业游戏：

| 游戏 | 技术栈 | 目录 | 运行方式 |
|------|--------|------|----------|
| 飞机大战 | HTML5 + Canvas + JavaScript | `plane-war/` | 浏览器直接打开 `index.html` |
| 大鱼吃小鱼 | C++ + EasyX 图形库 | `fish-game/` | Visual Studio 打开 `.sln`，F5 编译运行 |
| 俄罗斯方块 | C 语言 + 控制台 | `tetris/` | `gcc tetris.c -o tetris.exe` 后运行 |

---

## 1. 飞机大战（plane-war）

纯前端网页游戏，所有图片资源已用 Base64 内嵌在 HTML 中，**双击 `index.html` 即可在浏览器里玩**，无需任何环境配置。

- 方向键 / WASD 控制飞机移动，空格发射子弹
- 敌机随机生成，击中爆炸，带得分和生命系统

## 2. 大鱼吃小鱼（fish-game）

用 C++ 和 EasyX 图形库实现的窗口游戏，包含开始界面、三种敌人鱼、气泡粒子特效、生命与得分系统。

- 用 Visual Studio 打开 `FishGame.sln`，选择 x64 + Debug，按 F5 运行
- EasyX 的头文件和 lib 已包含在 `easyx/` 目录中，无需单独安装
- 方向键控制移动，大鱼吃小鱼，被大鱼碰到扣生命

## 3. 俄罗斯方块（tetris）

C 语言控制台版俄罗斯方块，7 种方块形状、旋转、消行、计分、难度递增。

- 编译：`gcc tetris.c -o tetris.exe`（MinGW/Dev-C++）或 `cl /utf-8 tetris.c`（MSVC）
- 操作：← → 移动，↑ 旋转，↓ 加速，空格 直接落底，Q 退出

---

## 项目结构

```
.
├── README.md              # 本文件
├── .gitignore
├── plane-war/
│   └── index.html         # 飞机大战（单文件，资源已内嵌）
├── fish-game/
│   ├── main.cpp           # 大鱼吃小鱼全部逻辑
│   ├── FishGame.sln       # VS 解决方案
│   ├── FishGame.vcxproj   # VS 工程配置
│   ├── *.png / *.jpg      # 游戏图片资源
│   └── easyx/             # EasyX 头文件与 lib（编译依赖）
└── tetris/
    └── tetris.c           # 俄罗斯方块最终版源码
```
