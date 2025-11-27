//
//  main.cpp
//  hungrySnake
//
//  Created by 姚晨 & 崔源on 2025/11/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>  // 替换 conio.h 和 windows.h
#include <time.h>
#include <unistd.h>   // 用于 usleep (代替 Sleep)

#define WIDTH 20
#define HEIGHT 20
#define INIT_SNAKE_LENGTH 3

enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };

struct Game {
    int gameOver;
    int score;
    enum Direction dir;
    int snakeX[100], snakeY[100];
    int snakeLength;
    int fruitX, fruitY;
};

// 上一帧记录（使用 ncurses 时可简化，因为 ncurses 有自己的窗口管理）
int prevSnakeX[100], prevSnakeY[100];  // 记录上一帧蛇位置
int prevSnakeLength = 0;
int prevFruitX = -1, prevFruitY = -1;
int prevScore = -1;
int firstDraw = 1;

void initConsole() {
    initscr();             // 初始化 ncurses
    noecho();              // 禁用回显
    keypad(stdscr, TRUE);  // 启用特殊键
    curs_set(0);           // 隐藏光标
    nodelay(stdscr, TRUE); // 非阻塞输入
    timeout(0);            // getch() 非阻塞
}

void drawChar(short x, short y, char ch) {
    mvaddch(y, x, ch);  // 在 (y, x) 位置绘制字符（注意 ncurses 是行优先）
}

void drawStaticBorder() {
    clear();  // 清屏（替换 system("cls")）

    // 上边界
    for (int i = 0; i < WIDTH + 2; i++)
        addch('#');
    addch('\n');

    // 中间区域
    for (int i = 0; i < HEIGHT; i++) {
        addch('#');
        for (int j = 0; j < WIDTH; j++)
            addch(' ');
        addch('#');
        addch('\n');
    }

    // 下边界
    for (int i = 0; i < WIDTH + 2; i++)
        addch('#');
    addch('\n');

    // 分数和控制信息（使用 mvprintw 定位打印）
    mvprintw(HEIGHT + 1, 0, "得分: 0");
    mvprintw(HEIGHT + 2, 0, "控制: W-上 S-下 A-左 D-右 X-退出");

    refresh();  // 刷新屏幕
}

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
            if (prevSnakeX[i] == game->snakeX[j] + 1 &&
                prevSnakeY[i] == game->snakeY[j] + 1) {
                stillExists = 1;
                break;
            }
        }
        if (!stillExists) {
            drawChar(prevSnakeX[i], prevSnakeY[i], ' ');
        }
    }

    // 2. 绘制新蛇
    for (int i = 0; i < game->snakeLength; i++) {
        char ch = (i == 0) ? 'O' : 'o';
        drawChar(game->snakeX[i] + 1, game->snakeY[i] + 1, ch);
        // 记录当前位置供下一帧使用
        prevSnakeX[i] = game->snakeX[i] + 1;
        prevSnakeY[i] = game->snakeY[i] + 1;
    }
    prevSnakeLength = game->snakeLength;

    // 3. 更新食物
    if (prevFruitX != game->fruitX + 1 || prevFruitY != game->fruitY + 1) {
        // 清除旧食物（如果存在）
        if (prevFruitX != -1) {
            drawChar(prevFruitX, prevFruitY, ' ');
        }
        // 绘制新食物
        drawChar(game->fruitX + 1, game->fruitY + 1, 'F');
        prevFruitX = game->fruitX + 1;
        prevFruitY = game->fruitY + 1;
    }

    // 4. 更新分数
    if (prevScore != game->score) {
        mvprintw(HEIGHT + 1, 6, "%d    ", game->score);  // 用空格覆盖旧数字
        prevScore = game->score;
    }

    refresh();  // 刷新屏幕，使变化可见
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
    prevFruitX = -1;
    prevFruitY = -1;
    prevScore = -1;
}

void input(struct Game* game) {
    int key = getch();  // ncurses 的 getch()，非阻塞模式下返回 ERR 如果无输入

    if (key != ERR) {   // 有输入时处理（替换 _kbhit() 和 _getch()）
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
        case 'x': case 'X':
            game->gameOver = 1;
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
    case STOP:        // 加上这行
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
    struct Game game;

    printf("=== 流畅版贪吃蛇游戏 ===\n");
    printf("控制键: W-上, S-下, A-左, D-右, X-退出\n");
    printf("按任意键开始游戏...");
    getchar();  // 等待按键（标准输入）

    // 初始化控制台
    initConsole();

    setup(&game);

    while (!game.gameOver) {
        drawGame(&game);  // 使用局部更新绘制
        input(&game);
        logic(&game);
        usleep(200000);  // 控制游戏速度（200ms），替换 Sleep(200)
    }

    // 游戏结束显示
    mvprintw(HEIGHT + 3, 0, "游戏结束！最终得分: %d", game.score);
    mvprintw(HEIGHT + 4, 0, "按任意键退出...");
    refresh();

    // 等待按键退出（切换到阻塞模式）
    nodelay(stdscr, FALSE);
    getch();
    endwin();  // 结束 ncurses 模式

    return 0;
}
