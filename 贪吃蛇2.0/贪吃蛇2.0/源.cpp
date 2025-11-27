#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

#define WIDTH 20
#define HEIGHT 20
#define INIT_SNAKE_LENGTH 3

enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN,StopDirection };
enum GameState {MENU,PLAYING,GANE_OVER,EXIT};
enum Difficulty {EASY =1,NORMAL,HARD};


struct Game {
    int gameOver;
    int score;
    enum Direction dir;
    int snakeX[100], snakeY[100];
    int snakeLength;
    int fruitX, fruitY;
    int highScore;
    int speed;
    enum Difficulty difficulty;
};

// 控制台变量
HANDLE hStdOut;
COORD prevSnake[100] = { 0 };
int prevSnakeLength = 0;
COORD prevFruit = { -1, -1 };
int prevScore = -1;
int firstDraw = 1;
enum GameState gameState = MENU;


void initConsole() {
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // 隐藏光标
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hStdOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hStdOut, &cursorInfo);
}

void setCursorPos(short x, short y) {
    COORD coord = { x, y };
    SetConsoleCursorPosition(hStdOut, coord);
}

void drawChar(short x, short y, char ch) {
    setCursorPos(x, y);
    putchar(ch);
}

//开始绘制菜单部分
void setup(struct Game*game);
void drawMenu(struct Game* game) {
    system("cls");

    printf("========================================\n");
    printf("           贪吃蛇游戏\n");
    printf("========================================\n");
    printf("\n");
    printf("   当前难度: ");
    switch (game->difficulty) {
    case EASY: printf("简单"); break;
    case NORMAL: printf("普通"); break;
    case HARD: printf("困难"); break;
    }
    printf("\n");
    printf("   最高分: %d\n", game->highScore);
    printf("\n");
    printf("========================================\n");
    printf("   1. 开始游戏\n");
    printf("   2. 选择难度\n");
    printf("   3. 退出游戏\n");
    printf("========================================\n");
    printf("\n");
    printf("请选择 (1-3): ");
}

// 绘制难度选择菜单
void drawDifficultyMenu(struct Game* game) {
    system("cls");

    printf("========================================\n");
    printf("           选择难度\n");
    printf("========================================\n");
    printf("\n");
    printf("   1. 简单 - 速度慢，适合新手\n");
    printf("   2. 普通 - 标准速度\n");
    printf("   3. 困难 - 速度快，适合高手\n");
    printf("   0. 返回主菜单\n");
    printf("========================================\n");
    printf("\n");
    printf("当前难度: ");
    switch (game->difficulty) {
    case EASY: printf("简单"); break;
    case NORMAL: printf("普通"); break;
    case HARD: printf("困难"); break;
    }
    printf("\n\n");
    printf("请选择 (0-3): ");
}

// 处理菜单输入
void handleMenuInput(struct Game* game) {
    if (_kbhit()) {
        char key = _getch();
        switch (key) {
        case '1':
            gameState = PLAYING;
            setup(game);
            break;
        case '2':
            gameState = MENU;  // 临时状态，会进入难度选择
            break;
        case '3':
            gameState = EXIT;
            break;
        }
    }
}

// 处理难度选择输入
void handleDifficultyInput(struct Game* game) {
    if (_kbhit()) {
        char key = _getch();
        switch (key) {
        case '1':
            game->difficulty = EASY;
            game->speed = 200;
            gameState = MENU;
            break;
        case '2':
            game->difficulty = NORMAL;
            game->speed = 120;
            gameState = MENU;
            break;
        case '3':
            game->difficulty = HARD;
            game->speed = 80;
            gameState = MENU;
            break;
        case '0':
            gameState = MENU;
            break;
        }
    }
}

// 绘制游戏结束画面
void drawGameOver(struct Game* game) {
    setCursorPos(0, HEIGHT + 5);
    printf("========================================\n");
    printf("            游戏结束！\n");
    printf("========================================\n");
    printf("   本次得分: %d\n", game->score);
    printf("   最高分: %d\n", game->highScore);
    printf("\n");
    printf("   按任意键返回主菜单...\n");
    printf("========================================\n");
}

// 处理游戏结束输入
void handleGameOverInput() {
    if (_kbhit()) {
        _getch();  // 清空输入缓冲区
        gameState = MENU;
    }
}


// 初始绘制静态边界
void drawStaticBorder() {
    system("cls");

    // 上边界
    for (int i = 0; i < WIDTH*2+2; i++)
        printf("#");
    printf("\n");

    // 中间区域
    for (int i = 0; i < HEIGHT; i++) {
        printf("#");
        for (int j = 0; j < WIDTH*2; j++)
            printf(" ");
        printf("#\n");
    }

    // 下边界
    for (int i = 0; i < WIDTH*2+2; i++)
        printf("#");
    printf("\n");

    // 分数和控制信息
    printf("得分: 0\n");
    printf("控制: W-上 S-下 A-左 D-右 X-退出  (空格)-暂停\n");
}

// 只更新变化的部分
void drawGame(struct Game* game) {
    // 首次绘制时绘制静态边界
    if (firstDraw) {
        drawStaticBorder();
        firstDraw = 0;
    }

    // 1. 清除上一帧的蛇（只清除不再需要的部分）
    for (int i = 0; i < prevSnakeLength; i++) {
        int stillExists = 0;
        for (int j = 0; j < game->snakeLength; j++) {
            if (prevSnake[i].X == game->snakeX[j] + 1 &&
                prevSnake[i].Y == game->snakeY[j] + 1) {
                stillExists = 1;
                break;
            }
        }
        if (!stillExists) {
            drawChar(prevSnake[i].X, prevSnake[i].Y, ' ');
        }
    }

    // 2. 绘制新蛇
    for (int i = 0; i < game->snakeLength; i++) {
        char ch = (i == 0) ? 'O' : 'o';
        drawChar(game->snakeX[i] + 1, game->snakeY[i] + 1, ch);

        // 记录当前位置供下一帧使用
        prevSnake[i].X = game->snakeX[i] + 1;
        prevSnake[i].Y = game->snakeY[i] + 1;
    }
    prevSnakeLength = game->snakeLength;

    // 3. 更新食物
    if (prevFruit.X != game->fruitX + 1 || prevFruit.Y != game->fruitY + 1) {
        // 清除旧食物（如果存在）
        if (prevFruit.X != -1) {
            drawChar(prevFruit.X, prevFruit.Y, ' ');
        }
        // 绘制新食物
        drawChar(game->fruitX + 1, game->fruitY + 1, 'F');
        prevFruit.X = game->fruitX + 1;
        prevFruit.Y = game->fruitY + 1;
    }

    // 4. 更新分数
    if (prevScore != game->score) {
        setCursorPos(6, HEIGHT + 3);  // "得分: "后面开始
        printf("%d    ", game->score);  // 用空格覆盖旧数字
        prevScore = game->score;
    }
}

void setup(struct Game* game) {
    game->gameOver = 0;
    game->score = 0;
    game->dir = RIGHT;

    game->snakeLength = INIT_SNAKE_LENGTH;
    int startX = WIDTH / 2;
    int startY = HEIGHT / 2;

    for (int i = 0; i < game->snakeLength; i++) {
        game->snakeX[i] = startX - i;
        game->snakeY[i] = startY;
    }

    srand(time(NULL));

    // 生成食物，确保不在蛇身上
    int validPosition;
    do {
        validPosition = 1;
        game->fruitX = rand() % (WIDTH - 2) + 1;
        game->fruitY = rand() % (HEIGHT - 2) + 1;

        for (int i = 0; i < game->snakeLength; i++) {
            if (game->fruitX == game->snakeX[i] && game->fruitY == game->snakeY[i]) {
                validPosition = 0;
                break;
            }
        }
    } while (!validPosition);

    // 初始化上一帧记录
    prevSnakeLength = 0;
    prevFruit.X = -1;
    prevFruit.Y = -1;
    prevScore = -1;
}



void input(struct Game* game) {
    static Direction StopDirection;

    if (_kbhit()) {
        char key = _getch();
        switch (key) {
        case 'a': case 'A':
            if (game->dir != RIGHT) game->dir = LEFT;
            break;
        case 'd': case 'D':
            if (game->dir != LEFT) game->dir = RIGHT;
            break;
        case 'w': case 'W':
            if (game->dir != DOWN) game->dir = UP;
            break;
        case 's': case 'S':
            if (game->dir != UP) game->dir = DOWN;
            break;
        case ' ':  // 空格键
            if (game->dir != STOP) {
                StopDirection = game->dir;
                game->dir = STOP;
            }
            else {
                game->dir = StopDirection;
            }
            break;
        case 'x': case 'X':
            game->gameOver = 1;
            gameState = MENU; 
            break;
        }
       
    }
}

void logic(struct Game* game) {
    if (game->gameOver || game->dir == STOP) return;

    // 保存蛇尾位置
    int prevX = game->snakeX[game->snakeLength - 1];
    int prevY = game->snakeY[game->snakeLength - 1];

    // 移动蛇身（从尾部开始更新）
    for (int i = game->snakeLength - 1; i > 0; i--) {
        game->snakeX[i] = game->snakeX[i - 1];
        game->snakeY[i] = game->snakeY[i - 1];
    }

    // 移动蛇头
    switch (game->dir) {
    case LEFT:
        game->snakeX[0]--;
        break;
    case RIGHT:
        game->snakeX[0]++;
        break;
    case UP:
        game->snakeY[0]--;
        break;
    case DOWN:
        game->snakeY[0]++;
        break;
    }

    // 检测撞墙
    if (game->snakeX[0] < 0 || game->snakeX[0] >= WIDTH ||
        game->snakeY[0] < 0 || game->snakeY[0] >= HEIGHT) {
        game->gameOver = 1;
        return;
    }

    // 检测撞到自己
    for (int i = 1; i < game->snakeLength; i++) {
        if (game->snakeX[0] == game->snakeX[i] && game->snakeY[0] == game->snakeY[i]) {
            game->gameOver = 1;
            return;
        }
    }

    // 检测吃到食物
    if (game->snakeX[0] == game->fruitX && game->snakeY[0] == game->fruitY) {
        game->score += 10;

        // 蛇长度增加
        game->snakeLength++;
        game->snakeX[game->snakeLength - 1] = prevX;
        game->snakeY[game->snakeLength - 1] = prevY;

        // 生成新食物
        int validPosition;
        do {
            validPosition = 1;
            game->fruitX = rand() % (WIDTH - 2) + 1;
            game->fruitY = rand() % (HEIGHT - 2) + 1;

            // 确保食物不会生成在蛇身上
            for (int i = 0; i < game->snakeLength; i++) {
                if (game->fruitX == game->snakeX[i] && game->fruitY == game->snakeY[i]) {
                    validPosition = 0;
                    break;
                }
            }
        } while (!validPosition);
    }
}

int main() {
    int challenge;
    struct Game game;

    printf("=== 流畅版贪吃蛇游戏 ===\n");
    printf("控制键: W-上, S-下, A-左, D-右, X-退出, (空格)-暂停\n");
    printf("请选择要挑战的难度(1-难, 2-中, 3-易)：");

    // 正确读取输入
    if (scanf_s("%d", &challenge) != 1) {
        printf("输入错误！使用默认难度。\n");
        challenge = 2; // 默认难度
    }

    // 清除输入缓冲区
    while (getchar() != '\n');

    printf("按任意键开始游戏...");
    _getch();

    // 验证输入范围
    if (challenge < 1 || challenge > 3) {
        printf("难度范围错误，使用默认难度2\n");
        challenge = 2;
    }
    // 初始化控制台
    initConsole();
    setup(&game);

    while (!game.gameOver) {
        drawGame(&game);  // 使用局部更新绘制
        input(&game);
        logic(&game);
        switch (challenge) {
        case 1:
            Sleep(50);
            break;
        case 2:
            Sleep(100);
            break;
        case 3:
            Sleep(200);
            break;

        }
       
    }

    // 游戏结束显示
    setCursorPos(0, HEIGHT + 5);
    printf("\n游戏结束！最终得分: %d\n", game.score);
    printf("按任意键退出...");
    _getch();
    while (1) {
        // 显示主菜单
        drawMenu(&game);

        // 处理菜单输入
        int menuChoice = 0;
        if (scanf_s("%d", &menuChoice) == 1) {
            // 清除输入缓冲区
            while (getchar() != '\n');

            switch (menuChoice) {
            case 1: // 开始游戏
                setup(&game);
                while (!game.gameOver) {
                    drawGame(&game);  // 使用局部更新绘制
                    input(&game);
                    logic(&game);
                    switch (challenge) {
                    case 1:
                        Sleep(50);
                        break;
                    case 2:
                        Sleep(100);
                        break;
                    case 3:
                        Sleep(200);
                        break;

                    }
                }
                // 游戏结束后更新最高分
                if (game.score > game.highScore) {
                    game.highScore = game.score;
                }
                // 显示游戏结束画面
                drawGameOver(&game);
                _getch();
                break;


            case 2: // 选择难度
                while (1) {
                    drawDifficultyMenu(&game);
                    int diffChoice = 0;
                    if (scanf_s("%d", &diffChoice) == 1) {
                        while (getchar() != '\n'); // 清除输入缓冲区

                        switch (diffChoice) {
                        case 1:
                            game.difficulty = EASY;
                            game.speed = 200;
                            printf("难度已设置为：简单\n");
                            _getch();
                            break;
                        case 2:
                            game.difficulty = NORMAL;
                            game.speed = 120;
                            printf("难度已设置为：普通\n");
                            _getch();
                            break;
                        case 3:
                            game.difficulty = HARD;
                            game.speed = 80;
                            printf("难度已设置为：困难\n");
                            _getch();
                            break;
                        case 0: // 返回主菜单
                            break;
                        default:
                            printf("无效选择！请重新选择。\n");
                            _getch();
                            continue; // 继续显示难度菜单
                        }
                        break; // 退出难度选择循环
                    }
                    else {
                        // 清除无效输入
                        while (getchar() != '\n');
                        printf("请输入有效数字！\n");
                        _getch();
                    }
                }
                break;

            case 3: // 退出游戏
                system("cls");
                printf("感谢游玩贪吃蛇游戏！\n");
                printf("按任意键退出...");
                _getch();
                return 0;

               
            default:
                printf("无效选择！\n");
                _getch();
                break;
            }
        }
        else {
            // 清除无效输入
            while (getchar() != '\n');
            printf("请输入有效数字！\n");
            _getch();
        }
    }

    return 0;
}
