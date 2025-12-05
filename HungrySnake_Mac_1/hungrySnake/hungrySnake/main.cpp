/*
 *  终极跨平台贪吃蛇 v4.0 - Linux/macOS 完美版
 *  功能完全对齐甚至超越 Windows 3.0 版
 *  作者：你 + Grok 联合出品
 *  编译：gcc snake.c -lncursesw -o snake
 */

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define GRID_WIDTH  20      // 实际游戏网格宽度（20格）
#define GRID_HEIGHT 20      // 实际游戏网格高度（20格）
#define INIT_LENGTH 3
#define MAX_LENGTH  200

// 游戏状态
enum GameState { MAIN_MENU, SETTINGS, HELP_SCREEN, PLAYING, GAME_OVER_SCREEN };
// 主菜单选项
enum MenuOption { START_GAME, SETTINGS_MENU, HELP, EXIT_GAME, MENU_COUNT };

struct Game {
    int score;
    int gameOver;
    int dir;                    // 0=停 1=左 2=右 3=上 4=下
    int snakeX[MAX_LENGTH], snakeY[MAX_LENGTH];
    int length;
    int fruitX, fruitY;
};

// 全局变量
struct Game game;
enum GameState gameState = MAIN_MENU;
int selectedOption = START_GAME;
int difficulty = 2;             // 1=难 2=中 3=易（默认中等）
int isPaused = 0;
int lastDir = 2;                // 记录暂停前的方向（2=右）

// 局部刷新所需缓存
int prevSnakeX[MAX_LENGTH], prevSnakeY[MAX_LENGTH];
int prevLength = 0;
int prevFruitX = -1, prevFruitY = -1;
int prevScore = -1;

// ==================== 初始化 ====================
void initGame() {
    initscr();
    noecho();
    curs_set(0);                // 隐藏光标
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);      // 非阻塞输入
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);   // 高亮反白
    init_pair(2, COLOR_GREEN, COLOR_BLACK);    // 蛇身绿色
    init_pair(3, COLOR_RED,   COLOR_BLACK);    // 食物红色
}

// ==================== 绘制工具函数 ====================
void drawChar(int x, int y, char ch, int color_pair = 0) {
    if (color_pair > 0) attron(COLOR_PAIR(color_pair));
    mvaddch(y + 1, x + 1, ch);   // +1 是因为有边框
    if (color_pair > 0) attroff(COLOR_PAIR(color_pair));
}

// ==================== 游戏界面 ====================
void drawBorderAndUI() {
    clear();
    attron(COLOR_PAIR(2));
    mvhline(0, 0, '#', GRID_WIDTH + 2);                    // 上边框
    mvhline(GRID_HEIGHT + 1, 0, '#', GRID_WIDTH + 2);      // 下边框
    for (int i = 0; i < GRID_HEIGHT; i++) {
        mvaddch(i + 1, 0, '#');
        mvaddch(i + 1, GRID_WIDTH + 1, '#');
    }
    attroff(COLOR_PAIR(2));

    mvprintw(GRID_HEIGHT + 3, 0, "得分: 0");
    mvprintw(GRID_HEIGHT + 4, 0, "控制: W↑ S↓ A← D→   空格=暂停   X=返回菜单");
    refresh();
}

// 核心渲染：局部刷新，极致防闪烁
void drawGame() {
    static int first = 1;
    if (first) { drawBorderAndUI(); first = 0; }

    // 擦除上一帧消失的蛇节
    for (int i = 0; i < prevLength; i++) {
        int exists = 0;
        for (int j = 0; j < game.length; j++) {
            if (prevSnakeX[i] == game.snakeX[j] && prevSnakeY[i] == game.snakeY[j]) {
                exists = 1; break;
            }
        }
        if (!exists) drawChar(prevSnakeX[i], prevSnakeY[i], ' ', 0);
    }

    // 画蛇
    for (int i = 0; i < game.length; i++) {
        char ch = (i == 0) ? 'O' : 'o';
        int color = (i == 0) ? 2 : 0;
        drawChar(game.snakeX[i], game.snakeY[i], ch, color);
        prevSnakeX[i] = game.snakeX[i];
        prevSnakeY[i] = game.snakeY[i];
    }
    prevLength = game.length;

    // 画食物
    if (prevFruitX != game.fruitX || prevFruitY != game.fruitY) {
        if (prevFruitX != -1) drawChar(prevFruitX, prevFruitY, ' ', 0);
        drawChar(game.fruitX, game.fruitY, 'F', 3);
        prevFruitX = game.fruitX;
        prevFruitY = game.fruitY;
    }

    // 更新分数
    if (prevScore != game.score) {
        mvprintw(GRID_HEIGHT + 3, 6, "%-4d", game.score);
        prevScore = game.score;
    }

    // 暂停提示
    if (isPaused) {
        attron(A_BOLD | COLOR_PAIR(1));
        mvprintw(GRID_HEIGHT/2, GRID_WIDTH/2 - 3, " 暂停中 ");
        attroff(A_BOLD | COLOR_PAIR(1));
    }
    refresh();
}

// ==================== 游戏逻辑 ====================
void setupGame() {
    game.gameOver = 0;
    game.score = 0;
    game.dir = 2;  // 右
    game.length = INIT_LENGTH;
    lastDir = 2;
    isPaused = 0;

    int midX = GRID_WIDTH / 2;
    int midY = GRID_HEIGHT / 2;
    for (int i = 0; i < game.length; i++) {
        game.snakeX[i] = midX - i;
        game.snakeY[i] = midY;
    }

    // 生成第一个食物
    int ok;
    do {
        ok = 1;
        game.fruitX = rand() % (GRID_WIDTH - 2) + 1;
        game.fruitY = rand() % (GRID_HEIGHT - 2) + 1;
        for (int i = 0; i < game.length; i++) {
            if (game.fruitX == game.snakeX[i] && game.fruitY == game.snakeY[i]) { ok = 0; break; }
        }
    } while (!ok);

    prevLength = 0;
    prevFruitX = prevFruitY = -1;
    prevScore = -1;
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
        case 1: game.snakeX[0]--; break;  // 左
        case 2: game.snakeX[0]++; break;  // 右
        case 3: game.snakeY[0]--; break;  // 上
        case 4: game.snakeY[0]++; break;  // 下
    }

    // 撞墙
    if (game.snakeX[0] <= 0 || game.snakeX[0] >= GRID_WIDTH-1 ||
        game.snakeY[0] <= 0 || game.snakeY[0] >= GRID_HEIGHT-1) {
        game.gameOver = 1; return;
    }

    // 撞自己
    for (int i = 1; i < game.length; i++) {
        if (game.snakeX[0] == game.snakeX[i] && game.snakeY[0] == game.snakeY[i]) {
            game.gameOver = 1; return;
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
                if (game.fruitX == game.snakeX[i] && game.fruitY == game.snakeY[i]) { ok = 0; break; }
            }
        } while (!ok);
    }
}

// ==================== 菜单界面 ====================
void drawMainMenu() {
    clear();
    clear();
    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(4, 8, "╔══════════════════════════╗");
    mvprintw(5, 8, "║       贪吃蛇游戏         ║");
    mvprintw(6, 8, "╚══════════════════════════╝");
    attroff(A_BOLD | COLOR_PAIR(2));

    const char* items[] = {"开始游戏", "游戏设置", "游戏说明", "退出游戏"};
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == selectedOption) attron(A_REVERSE | COLOR_PAIR(1));
        mvprintw(9 + i, 12, i == selectedOption ? "▶ %s" : "  %s", items[i]);
        if (i == selectedOption) attroff(A_REVERSE | COLOR_PAIR(1));
    }
    mvprintw(16, 8, "使用 ↑↓ / W S 选择，Enter 确认");
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
        mvprintw(9 + i, 12, i == difficulty-1 ? "▶ %s" : "  %s", diffs[i]);
        if (i == difficulty-1) attroff(A_REVERSE | COLOR_PAIR(1));
    }
    mvprintw(15, 8, "当前速度：%s", diffs[difficulty-1]);
    mvprintw(17, 8, "↑↓ 选择   Enter确认   ESC返回");
    refresh();
}

void drawHelp() {
    clear();
    mvprintw(3, 8, "╔══════════════════════╗");
    mvprintw(4, 8, "║       游戏说明       ║");
    mvprintw(5, 8, "╚══════════════════════╝");
    mvprintw(7,  6, "控制：W↑ S↓ A← D→");
    mvprintw(8,  6, "空格键 —— 暂停/继续");
    mvprintw(9,  6, "X 键   —— 返回主菜单");
    mvprintw(11, 6, "规则：");
    mvprintw(12, 8, "• 吃到 F 得10分并变长");
    mvprintw(13, 8, "• 撞墙或撞自己游戏结束");
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

// ==================== 主函数 ====================
int main() {
    srand(time(NULL));
    initGame();

    while (1) {
        int key = getch();

        switch (gameState) {
            case MAIN_MENU:
                if (key != ERR) {
                    switch (key) {
                        case 'w': case 'W': case KEY_UP:   selectedOption = (selectedOption - 1 + MENU_COUNT) % MENU_COUNT; break;
                        case 's': case 'S': case KEY_DOWN: selectedOption = (selectedOption + 1) % MENU_COUNT; break;
                        case '\n': case KEY_ENTER:
                            if (selectedOption == START_GAME) { gameState = PLAYING; setupGame(); }
                            else if (selectedOption == SETTINGS_MENU) gameState = SETTINGS;
                            else if (selectedOption == HELP) { drawHelp(); }
                            else if (selectedOption == EXIT_GAME) { endwin(); return 0; }
                            break;
                    }
                }
                drawMainMenu();
                break;

            case SETTINGS:
                if (key != ERR) {
                    if (key == 'w' || key == 'W' || key == KEY_UP)   difficulty = difficulty==1 ? 3 : difficulty-1;
                    if (key == 's' || key == 'S' || key == KEY_DOWN) difficulty = difficulty==3 ? 1 : difficulty+1;
                    if (key == '\n' || key == KEY_ENTER || key == 27) gameState = MAIN_MENU;
                }
                drawSettings();
                break;

            case PLAYING:
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

            case GAME_OVER_SCREEN:
                drawGameOver();
                if (key == 'r' || key == 'R') { gameState = PLAYING; setupGame(); }
                if (key == 'x' || key == 'X') { endwin(); return 0; }
                break;
        }
        usleep(10000);
    }
    endwin();
    return 0;
}
