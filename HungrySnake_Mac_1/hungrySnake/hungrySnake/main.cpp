/*
 *  终极贪吃蛇 v4.4 - 彻底修复所有编译错误版
 *  菜单只用 ↑↓ + Enter，WASD 只在游戏内生效，零串台
 *  编译：gcc snake.c -lncursesw -o snake
 */

#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define GRID_WIDTH  20
#define GRID_HEIGHT 20
#define INIT_LENGTH 3
#define MAX_LENGTH  200

//游戏的状态机，分为（主菜单，设置，帮助界面，游戏中，游戏结束画面
//enum是自动命名
enum GameState { MAIN_MENU, SETTINGS, HELP_SCREEN, PLAYING, GAME_OVER_SCREEN };

//菜单的选择栏目
enum MenuOption { START_GAME, SETTINGS_MENU, HELP, EXIT_GAME, MENU_COUNT };


//定义了一个Game类型，包含了在游戏进行中会出现的实体
struct Game {
    // dir: 1左 2右 3上 4下
    int score, gameOver, dir;
    //分别用两个数组储存蛇每一节的x,y坐标
    int snakeX[MAX_LENGTH], snakeY[MAX_LENGTH];
    //标定了水果的出现位置
    int length, fruitX, fruitY;
};

//创建了一个Game类型的game变量
struct Game game;


//创建了一个GameState类型的gameState变量，用于标定当前的状态机
//这里初始化状态机为主菜单
enum GameState gameState = MAIN_MENU;
int selectedOption = START_GAME;
int difficulty = 2;             // 1=难 2=中 3=易
int isPaused = 0;

//消除闪烁（局部刷新，提高整体）
//用两个数组记录移动之前的老蛇的身体（用来判断老蛇还在不在新蛇上，若不在，则删除）
int prevSnakeX[MAX_LENGTH], prevSnakeY[MAX_LENGTH];
//记录上一帧蛇多长，防止在变长时擦错位置
int prevLength = 0;
int prevFruitX = -1, prevFruitY = -1;
//上一帧显示的分数
int prevScore = -1;

void initGame() {
    //把终端从“普通 shell”变成“游戏全屏模式”
    //作用：（整个屏幕被清空，光标跳到左上角，所有输出都由程序完全接管）
    //注意：要在程序结束的时候用endwin();退出游戏模式
    initscr();
    
    //按键不回显
    //你敲键盘，屏幕不显示你敲的字符
    noecho();
    
    //隐藏光标
    curs_set(0);
    
    //开启特殊键（↑↓←→、Enter、F1-F12等）
    keypad(stdscr, TRUE);
    
    //键盘变成“非阻塞”（敲了才反应，不敲不等）
    nodelay(stdscr, TRUE);
    //启动颜色功能
    start_color();
    //初始化颜色对
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED,   COLOR_BLACK);
}

void drawChar(int x, int y, char ch, int color_pair = 0) {
    if (color_pair) attron(COLOR_PAIR(color_pair));
    mvaddch(y + 1, x + 1, ch);
    if (color_pair) attroff(COLOR_PAIR(color_pair));
}

// 正方形边框（横排42个#，竖排20个# + 上下边框）
void drawBorderAndUI() {
    clear();
    attron(COLOR_PAIR(2));

    // 上边框：20格 × 2 + 左右两个# = 42个#
    mvhline(0, 0, '#', GRID_WIDTH * 2 + 2);

    // 下边框：同上
    mvhline(GRID_HEIGHT + 1, 0, '#', GRID_WIDTH * 2 + 2);

    // 左右边框：20 行（不包括上下边框）
    for (int i = 0; i < GRID_HEIGHT; i++) {
        mvaddch(i + 1, 0, '#');                    // 左#
        mvhline(i + 1, 1, ' ', GRID_WIDTH * 2);     // 中间 40 个空格（20格×2）
        mvaddch(i + 1, GRID_WIDTH * 2 + 1, '#');    // 右#
    }
    attroff(COLOR_PAIR(2));

    // 信息栏
    mvprintw(GRID_HEIGHT + 3, 0, "得分: 0");
    mvprintw(GRID_HEIGHT + 4, 0, "控制: WASD移动  空格暂停  X返回菜单");
    refresh();
}
void drawGame() {
    static int first = 1;
    if (first) { drawBorderAndUI(); first = 0; }

    for (int i = 0; i < prevLength; i++) {
        int exists = 0;
        for (int j = 0; j < game.length; j++) {
            if (prevSnakeX[i] == game.snakeX[j] && prevSnakeY[i] == game.snakeY[j]) {
                exists = 1; break;
            }
        }
        if (!exists) drawChar(prevSnakeX[i], prevSnakeY[i], ' ', 0);
    }

    for (int i = 0; i < game.length; i++) {
        char ch = (i == 0) ? 'O' : 'o';
        drawChar(game.snakeX[i], game.snakeY[i], ch, i == 0 ? 2 : 0);
        prevSnakeX[i] = game.snakeX[i];
        prevSnakeY[i] = game.snakeY[i];
    }
    prevLength = game.length;

    if (prevFruitX != game.fruitX || prevFruitY != game.fruitY) {
        if (prevFruitX != -1) drawChar(prevFruitX, prevFruitY, ' ', 0);
        drawChar(game.fruitX, game.fruitY, 'F', 3);
        prevFruitX = game.fruitX;
        prevFruitY = game.fruitY;
    }

    if (prevScore != game.score) {
        mvprintw(GRID_HEIGHT + 3, 6, "%-4d", game.score);
        prevScore = game.score;
    }

    if (isPaused) {
        attron(A_BOLD | COLOR_PAIR(1));
        mvprintw(GRID_HEIGHT/2, GRID_WIDTH/2 - 4, " 暂停中 ");
        attroff(A_BOLD | COLOR_PAIR(1));
    }
    refresh();
}

void setupGame() {
    clear();                        // 清屏
    prevLength = 0;                 // 重置上一帧缓存
    prevFruitX = prevFruitY = -1;    // 重置食物缓存
    prevScore = -1;                 // 重置分数缓存
    refresh();                      // 强制刷新
    drawBorderAndUI();              // 重新画边框和UI
    game.gameOver = 0;
    game.score = 0;
    game.dir = 2;
    game.length = INIT_LENGTH;
    isPaused = 0;

    int midX = GRID_WIDTH / 2;
    int midY = GRID_HEIGHT / 2;
    for (int i = 0; i < game.length; i++) {
        game.snakeX[i] = midX - i;
        game.snakeY[i] = midY;
    }

    int ok;
    do {
        ok = 1;
        game.fruitX = rand() % (GRID_WIDTH - 2) + 1;
        game.fruitY = rand() % (GRID_HEIGHT - 2) + 1;
        for (int i = 0; i < game.length; i++) {
            if (game.fruitX == game.snakeX[i] && game.fruitY == game.snakeY[i]) { ok = 0; break; }
        }
    } while (!ok);

    prevLength = 0; prevFruitX = prevFruitY = -1; prevScore = -1;
}

void gameLogic() {
    if (game.gameOver || isPaused) return;

    int tailX = game.snakeX[game.length - 1];
    int tailY = game.snakeY[game.length - 1];

    for (int i = game.length - 1; i > 0; i--) {
        game.snakeX[i] = game.snakeX[i-1];
        game.snakeY[i] = game.snakeY[i-1];
    }

    switch (game.dir) {
        case 1: game.snakeX[0]--; break;
        case 2: game.snakeX[0]++; break;
        case 3: game.snakeY[0]--; break;
        case 4: game.snakeY[0]++; break;
    }

    // 撞墙
    if (game.snakeX[0] <= 0 || game.snakeX[0] >= (GRID_WIDTH)*2||
        game.snakeY[0] <= 0 || game.snakeY[0] >= GRID_HEIGHT) {
        game.gameOver = 1;
        return;
    }

    // 撞自己
    for (int i = 1; i < game.length; i++) {
        if (game.snakeX[0] == game.snakeX[i] && game.snakeY[0] == game.snakeY[i]) {
            game.gameOver = 1;
            return;
        }
    }

    // 吃食物
    if (game.snakeX[0] == game.fruitX && game.snakeY[0] == game.fruitY) {
        game.score += 10;
        game.length++;
        game.snakeX[game.length-1] = tailX;
        game.snakeY[game.length-1] = tailY;

        int ok;
        do {
            ok = 1;
            game.fruitX = rand() % (GRID_WIDTH - 2) + 1;
            game.fruitY = rand() % (GRID_HEIGHT - 2) + 1;
            for (int i = 0; i < game.length; i++) {
                if (game.fruitX == game.snakeX[i] && game.fruitY == game.snakeY[i]) { ok = 0; break; break; }
            }
        } while (!ok);
    }
}

void drawMainMenu() {
    clear();
    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(4, 8, "╔══════════════════════════╗");
    mvprintw(5, 8, "║       贪吃蛇游戏         ║");
    mvprintw(6, 8, "╚══════════════════════════╝");
    attroff(A_BOLD | COLOR_PAIR(2));

    const char* items[] = {"开始游戏", "游戏设置", "游戏说明", "退出游戏"};
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == selectedOption) attron(A_REVERSE | COLOR_PAIR(1));
        mvprintw(9 + i, 12, i == selectedOption ? " %s" : "  %s", items[i]);
        if (i == selectedOption) attroff(A_REVERSE | COLOR_PAIR(1));
    }
    mvprintw(16, 8, "↑↓ 选择项目    Enter 确认");
    refresh();
}

void drawSettings() {
    clear();
    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(4, 10, "╔══════════════════╗");
    mvprintw(5, 10, "║     游戏设置     ║");
    mvprintw(6, 10, "╚══════════════════╝");
    attroff(A_BOLD | COLOR_PAIR(2));

    const char* diffs[] = {"困难 (60ms)", "中等 (120ms)", "简单 (200ms)"};
    for (int i = 0; i < 3; i++) {
        if (i == difficulty-1) attron(A_REVERSE | COLOR_PAIR(1));
        mvprintw(9 + i, 12, i == difficulty-1 ? " %s" : "  %s", diffs[i]);
        if (i == difficulty-1) attroff(A_REVERSE | COLOR_PAIR(1));
    }
    mvprintw(15, 8, "当前速度：%s", diffs[difficulty-1]);
    mvprintw(17, 8, "↑↓ 选择    Enter 确认    ESC 返回");
    refresh();
}

void drawHelp() {
    clear();
    mvprintw(3, 8, "╔══════════════════════╗");
    mvprintw(4, 8, "║       游戏说明       ║");
    mvprintw(5, 8, "╚══════════════════════╝");
    mvprintw(7,  6, "控制：W↑ S↓ A← D→");
    mvprintw(8,  6, "空格键 — 暂停/继续");
    mvprintw(9,  6, "X 键   — 返回主菜单");
    mvprintw(11, 6, "规则：");
    mvprintw(12, 8, "• 吃到 F 得10分并变长");
    mvprintw(13, 8, "• 撞墙或撞自己结束");
    mvprintw(16, 6, "按任意键返回…");
    refresh();
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
    gameState = MAIN_MENU;
}

void drawGameOver() {
    clear();
    attron(A_BOLD | COLOR_PAIR(3));
    mvprintw(8, 8, "╔══════════════════╗");
    mvprintw(9, 8, "║    游戏结束！    ║");
    mvprintw(10,8, "╚══════════════════╝");
    attroff(A_BOLD | COLOR_PAIR(3));
    mvprintw(12, 10, "最终得分：%d", game.score);
    mvprintw(15, 8, "按 R 重新开始");
    mvprintw(16, 8, "按 X 退出游戏");
    refresh();
}

int main() {
    // ============ 解决中文乱码的终极三板斧 ============
    setlocale(LC_ALL, "");     // 让 C 运行库支持 UTF-8（必须）
    // =================================================
    
    srand(time(NULL));
    initGame();

    while (1) {
        int key = getch();

        switch (gameState) {
            case MAIN_MENU: {
                if (key != ERR) {
                    if (key == KEY_UP)   selectedOption = (selectedOption - 1 + MENU_COUNT) % MENU_COUNT;
                    if (key == KEY_DOWN) selectedOption = (selectedOption + 1) % MENU_COUNT;
                    if (key == '\n' || key == KEY_ENTER) {
                        if (selectedOption == START_GAME) { gameState = PLAYING; setupGame(); }
                        else if (selectedOption == SETTINGS_MENU) gameState = SETTINGS;
                        else if (selectedOption == HELP) drawHelp();
                        else if (selectedOption == EXIT_GAME) { endwin();
                                                                return 0; }
                    }
                }
                drawMainMenu();
                break;
            }

            case SETTINGS: {
                if (key != ERR) {
                    if (key == KEY_UP)   difficulty = difficulty==1 ? 3 : difficulty-1;
                    if (key == KEY_DOWN) difficulty = difficulty==3 ? 1 : difficulty+1;
                    if (key == '\n' || key == KEY_ENTER || key == 27) gameState = MAIN_MENU;
                }
                drawSettings();
                break;
            }

            case PLAYING: {
                if (key != ERR) {
                    if (key == 'a' || key == 'A') if (game.dir != 2) game.dir = 1;
                    if (key == 'd' || key == 'D') if (game.dir != 1) game.dir = 2;
                    if (key == 'w' || key == 'W') if (game.dir != 4) game.dir = 3;
                    if (key == 's' || key == 'S') if (game.dir != 3) game.dir = 4;
                    if (key == ' ') isPaused = !isPaused;
                    if (key == 'x' || key == 'X') gameState = MAIN_MENU;
                }

                drawGame();
                gameLogic();

                int delay = difficulty==1 ? 60000 : difficulty==2 ? 120000 : 200000;
                usleep(delay);

                if (game.gameOver) gameState = GAME_OVER_SCREEN;
                break;
            }

            case GAME_OVER_SCREEN: {
                drawGameOver();
                if (key == 'r' || key == 'R') { gameState = PLAYING; setupGame(); }
                if (key == 'x' || key == 'X') { endwin(); return 0; }
                break;
            }
        }
        usleep(10000);
    }

    endwin();
    return 0;
}
