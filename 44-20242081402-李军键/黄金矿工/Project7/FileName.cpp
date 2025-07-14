#include <graphics.h>
#include <windows.h>
#include <mmsystem.h>
#include <ctime>
#include <cmath>
#include <cwchar>
#include <vector>

#pragma comment(lib, "winmm.lib")

const int WIDTH = 800;
const int HEIGHT = 600;
const int FPS = 60;
const int FRAME_DELAY = 1000 / FPS;
const int TARGET_SCORE = 100;  // 目标分数
const int COUNTDOWN_SECONDS = 60;  // 倒计时秒数

// 游戏状态
enum GameState { START_SCREEN, PLAYING, GAME_OVER };
GameState currentState = START_SCREEN;

// 新增：暂停状态变量
bool isPaused = false;  // 标记游戏是否暂停


enum GoldSize { SMALL, MEDIUM, LARGE };


struct Gold {
    int x, y;
    GoldSize size;
    int value;  // 得分值
    Gold(int _x, int _y) {
        x = _x;
        y = _y;
        // 随机生成金子大小
        int r = rand() % 3;
        if (r == 0) {
            size = SMALL;
            value = 5;  // 小金子5分
        }
        else if (r == 1) {
            size = MEDIUM;
            value = 10;  // 中金子10分
        }
        else {
            size = LARGE;
            value = 20;  // 大金子20分
        }
    }
};


int minerX = WIDTH / 2;
int minerY = HEIGHT - 500;  // 矿工位置
int clawBaseY = minerY + 5;  // 夹子与支架连接点
int clawLength = 600;
int currentClawLength = 0;
double clawAngle = 0;
bool isClawActive = false;
bool isRetracting = false;
int score = 0;
int carriedGoldIndex = -1;  // 被携带的金子索引
DWORD gameStartTime = 0;  // 游戏开始时间
std::vector<Gold> golds;
IMAGE backgroundImg;  // 背景图片



// 角度转换常量
const double DEG_TO_RAD = 3.141592653589793 / 180.0;

void setup() {
    initgraph(WIDTH, HEIGHT);
    srand((unsigned int)time(0));

    // 背景图
    loadimage(&backgroundImg, L"background.jpg", WIDTH, HEIGHT, true);

    // 生成初始金子
    for (int i = 0; i < 5; ++i) {
        int x = rand() % (WIDTH - 100) + 50;
        int y = rand() % (HEIGHT - 100) + 100;  // Y在100-600之间
        golds.push_back(Gold(x, y));
    }
}

void drawStartScreen() {
    // 绘制背景
    setbkcolor(BLACK);
    cleardevice();

    // 绘制标题
    settextcolor(YELLOW);
    settextstyle(50, 0, L"黑体");
    outtextxy(WIDTH / 2 - 150, HEIGHT / 2 - 100, L"黄金矿工");

    // 绘制提示文字（新增暂停提示）
    settextcolor(WHITE);
    settextstyle(25, 0, L"宋体");
    outtextxy(WIDTH / 2 - 180, HEIGHT / 2 + 50, L"按Enter键开始游戏");
    outtextxy(WIDTH / 2 - 200, HEIGHT / 2 + 80, L"Q/E键旋转夹子  空格键发射");
    outtextxy(WIDTH / 2 - 180, HEIGHT / 2 + 110, L"目标: 60秒内获得100分");
    outtextxy(WIDTH / 2 - 150, HEIGHT / 2 + 140, L"P键: 暂停/继续游戏");  // 新增
}

void drawBackground() {
    // 绘制背景图
    putimage(0, 0, &backgroundImg);


    setlinecolor(RGB(100, 100, 100));
    line(0, 100, WIDTH, 100);
}

void drawMiner() {
    setfillcolor(YELLOW);
    solidrectangle(minerX - 15, minerY - 20, minerX + 15, minerY - 5);  // 矿工主体
    setfillcolor(BROWN);
    solidrectangle(minerX - 5, minerY - 4, minerX + 5, minerY + 5);  // 支架
}

void drawClaw() {
    int displayLength = isClawActive ? currentClawLength : 20;
    int clawEndX = minerX + displayLength * cos(clawAngle);
    int clawEndY = clawBaseY + displayLength * sin(clawAngle);

    setlinecolor(LIGHTBLUE);
    setlinestyle(PS_SOLID, 6);
    line(minerX, clawBaseY, clawEndX, clawEndY);

    // === 主夹子主体 ===
    const int CLAW_WIDTH = 20;
    const int CLAW_HEIGHT = 25;
    const int TOOTH_WIDTH = 4;
    const int TOOTH_LENGTH = 10;

    double sinA = sin(clawAngle);
    double cosA = cos(clawAngle);

    // 主体矩形四点（按顺时针）
    POINT clawBody[4] = {
        { (int)(clawEndX - CLAW_WIDTH / 2 * sinA - CLAW_HEIGHT * cosA),
          (int)(clawEndY + CLAW_WIDTH / 2 * cosA - CLAW_HEIGHT * sinA) },

        { (int)(clawEndX + CLAW_WIDTH / 2 * sinA - CLAW_HEIGHT * cosA),
          (int)(clawEndY - CLAW_WIDTH / 2 * cosA - CLAW_HEIGHT * sinA) },

        { (int)(clawEndX + CLAW_WIDTH / 2 * sinA),
          (int)(clawEndY - CLAW_WIDTH / 2 * cosA) },

        { (int)(clawEndX - CLAW_WIDTH / 2 * sinA),
          (int)(clawEndY + CLAW_WIDTH / 2 * cosA) }
    };

    setfillcolor(BLUE);
    fillpolygon(clawBody, 4);

    // 左齿
    POINT leftTooth[3] = {
        clawBody[2],
        { (int)(clawBody[2].x + TOOTH_LENGTH * cosA + TOOTH_WIDTH * sinA),
          (int)(clawBody[2].y + TOOTH_LENGTH * sinA - TOOTH_WIDTH * cosA) },
        { (int)(clawBody[2].x + TOOTH_LENGTH * cosA - TOOTH_WIDTH * sinA),
          (int)(clawBody[2].y + TOOTH_LENGTH * sinA + TOOTH_WIDTH * cosA) }
    };
    setfillcolor(WHITE);
    fillpolygon(leftTooth, 3);

    //右齿
    POINT rightTooth[3] = {
        clawBody[3],
        { (int)(clawBody[3].x + TOOTH_LENGTH * cosA + TOOTH_WIDTH * sinA),
          (int)(clawBody[3].y + TOOTH_LENGTH * sinA - TOOTH_WIDTH * cosA) },
        { (int)(clawBody[3].x + TOOTH_LENGTH * cosA - TOOTH_WIDTH * sinA),
          (int)(clawBody[3].y + TOOTH_LENGTH * sinA + TOOTH_WIDTH * cosA) }
    };
    setfillcolor(WHITE);
    fillpolygon(rightTooth, 3);
}

void drawGolden() {
    for (const auto& gold : golds) {

        int radius;
        switch (gold.size) {
        case SMALL: radius = 8; break;
        case MEDIUM: radius = 12; break;
        case LARGE: radius = 16; break;
        }

        setfillcolor(YELLOW);
        solidcircle(gold.x, gold.y, radius);

        // 光泽效果
        setfillcolor(WHITE);
        solidcircle(gold.x - radius / 3, gold.y - radius / 3, radius / 5);
    }
}

void drawScore() {
    wchar_t scoreText[50];
    swprintf(scoreText, 50, L"分数: %d / %d", score, TARGET_SCORE);
    settextcolor(WHITE);
    settextstyle(20, 0, L"宋体");
    outtextxy(10, 10, scoreText);
}

void drawTimer() {
    // 新增：暂停时不更新倒计时
    if (isPaused) return;

    DWORD elapsedTime = (timeGetTime() - gameStartTime) / 1000;
    int remainingTime = COUNTDOWN_SECONDS - elapsedTime;

    if (remainingTime <= 0) {
        remainingTime = 0;
        currentState = GAME_OVER;
    }

    wchar_t timerText[20];
    swprintf(timerText, 20, L"时间: %02d秒", remainingTime);
    settextcolor(WHITE);
    settextstyle(20, 0, L"宋体");
    outtextxy(WIDTH - 130, 10, timerText);
}

// 修改：扩展游戏结束界面为通用遮罩界面（支持暂停）
void drawGameOver() {

    setfillcolor(RGB(0, 0, 0, 180));
    solidrectangle(0, 0, WIDTH, HEIGHT);

    settextcolor(YELLOW);
    settextstyle(50, 0, L"黑体");

    if (currentState == GAME_OVER) {
        bool gameWon = (score >= TARGET_SCORE);
        if (gameWon) {
            outtextxy(WIDTH / 2 - 150, HEIGHT / 2 - 80, L"恭喜胜利!");
        }
        else {
            outtextxy(WIDTH / 2 - 150, HEIGHT / 2 - 80, L"游戏结束");
        }
    }
    else if (isPaused) {  // 新增：暂停时显示的文字
        outtextxy(WIDTH / 2 - 120, HEIGHT / 2 - 80, L"游戏暂停");
    }

    // 显示分数
    settextcolor(WHITE);
    settextstyle(30, 0, L"宋体");
    wchar_t finalScore[50];
    swprintf(finalScore, 50, L"当前分数: %d", score);
    outtextxy(WIDTH / 2 - 100, HEIGHT / 2 + 20, finalScore);


    settextstyle(20, 0, L"宋体");
    if (currentState == GAME_OVER) {
        outtextxy(WIDTH / 2 - 120, HEIGHT / 2 + 80, L"按Enter键重新开始");
    }
    else if (isPaused) {  // 新增：暂停时的提示
        outtextxy(WIDTH / 2 - 120, HEIGHT / 2 + 80, L"按P键继续游戏");
    }
}

bool isKeyPressed(int key) {
    return GetAsyncKeyState(key) & 0x8000;
}

void resetGame() {
    score = 0;
    isClawActive = false;
    isRetracting = false;
    currentClawLength = 0;
    clawAngle = 0;
    carriedGoldIndex = -1;
    golds.clear();
    isPaused = false;  // 新增：重置暂停状态


    for (int i = 0; i < 5; ++i) {
        int x = rand() % (WIDTH - 100) + 50;
        int y = rand() % (HEIGHT - 100) + 100;
        golds.push_back(Gold(x, y));
    }

    gameStartTime = timeGetTime();
}

// 修改：移动控制增加暂停判断
void moveMiner() {
    if (currentState != PLAYING || isPaused) return;  // 暂停时不响应控制


    if (!isClawActive) {
        if (isKeyPressed('Q')) {
            clawAngle += 0.8 * DEG_TO_RAD;
        }
        if (isKeyPressed('E')) {
            clawAngle -= 0.8 * DEG_TO_RAD;
        }
    }

    if (isKeyPressed(VK_SPACE) && !isClawActive) {
        isClawActive = true;
        isRetracting = false;
        currentClawLength = 0;
        carriedGoldIndex = -1;
    }
}

// 修改：游戏逻辑更新增加暂停判断
void updateGame() {
    if (currentState != PLAYING || isPaused) return;  // 暂停时不更新逻辑

    if (!isClawActive) return;

    // 更新夹子长度
    if (!isRetracting) {
        currentClawLength += 5;  // 伸长速度固定
        if (currentClawLength >= clawLength) {
            isRetracting = true;
        }
    }
    else {
        // 根据金子大小调整收回速度
        int retractSpeed = 5;
        if (carriedGoldIndex != -1) {
            switch (golds[carriedGoldIndex].size) {
            case SMALL: retractSpeed = 6; break;
            case MEDIUM: retractSpeed = 4; break;
            case LARGE: retractSpeed = 2; break;
            }
        }

        currentClawLength -= retractSpeed;
        if (currentClawLength <= 0) {
            isClawActive = false;
            // 加分并重置金子
            if (carriedGoldIndex != -1) {
                score += golds[carriedGoldIndex].value;
                // 检查是否达成目标
                if (score >= TARGET_SCORE) {
                    currentState = GAME_OVER;
                }
                // 重新生成金子
                golds[carriedGoldIndex] = Gold(
                    rand() % (WIDTH - 100) + 50,
                    rand() % (HEIGHT - 100) + 100
                );
                carriedGoldIndex = -1;
            }
        }
    }

    // 夹子末端坐标
    int clawEndX = minerX + currentClawLength * cos(clawAngle);
    int clawEndY = clawBaseY + currentClawLength * sin(clawAngle);

    // 金子跟随夹子移动
    if (carriedGoldIndex != -1) {
        golds[carriedGoldIndex].x = clawEndX;
        golds[carriedGoldIndex].y = clawEndY;
    }
    else {
        // 碰撞检测
        for (int i = 0; i < golds.size(); ++i) {
            int collisionRange;
            switch (golds[i].size) {
            case SMALL: collisionRange = 15; break;
            case MEDIUM: collisionRange = 20; break;
            case LARGE: collisionRange = 25; break;
            }

            double distance = sqrt(pow(clawEndX - golds[i].x, 2) + pow(clawEndY - golds[i].y, 2));
            if (distance < collisionRange) {
                carriedGoldIndex = i;
                isRetracting = true;
                break;
            }
        }
    }

    // 边界检测
    if (clawEndX < 0 || clawEndX > WIDTH || clawEndY < 0 || clawEndY > HEIGHT) {
        isRetracting = true;
    }
}

// 修改：游戏主循环增加暂停逻辑
void gameLoop() {
    DWORD lastFrameTime = timeGetTime();

    while (true) {
        DWORD currentTime = timeGetTime();
        DWORD elapsedTime = currentTime - lastFrameTime;

        if (elapsedTime >= FRAME_DELAY) {
            lastFrameTime = currentTime;

            // 新增：全局暂停按键检测（P键）
            if (currentState == PLAYING && isKeyPressed('P')) {
                isPaused = !isPaused;  // 切换暂停状态
                Sleep(200);  // 防抖处理（避免连续触发）
            }

            BeginBatchDraw();

            if (currentState == START_SCREEN) {
                drawStartScreen();
                if (isKeyPressed(VK_RETURN)) {
                    currentState = PLAYING;
                    resetGame();
                }
            }
            else if (currentState == PLAYING) {
                drawBackground();
                moveMiner();
                updateGame();
                drawMiner();
                drawClaw();
                drawGolden();
                drawScore();
                drawTimer();

                // 新增：如果暂停，绘制暂停界面
                if (isPaused) {
                    drawGameOver();  // 复用遮罩界面
                }
            }
            else if (currentState == GAME_OVER) {
                drawGameOver();
                if (isKeyPressed(VK_RETURN)) {
                    currentState = START_SCREEN;
                }
            }

            FlushBatchDraw();
        }
        else {
            Sleep(FRAME_DELAY - elapsedTime);
        }
    }
}


int main() {
    setup();
    gameLoop();
    closegraph();
    return 0;
}