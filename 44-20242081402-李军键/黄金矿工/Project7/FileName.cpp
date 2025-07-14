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
const int TARGET_SCORE = 100;  // Ŀ�����
const int COUNTDOWN_SECONDS = 60;  // ����ʱ����

// ��Ϸ״̬
enum GameState { START_SCREEN, PLAYING, GAME_OVER };
GameState currentState = START_SCREEN;

// ��������ͣ״̬����
bool isPaused = false;  // �����Ϸ�Ƿ���ͣ


enum GoldSize { SMALL, MEDIUM, LARGE };


struct Gold {
    int x, y;
    GoldSize size;
    int value;  // �÷�ֵ
    Gold(int _x, int _y) {
        x = _x;
        y = _y;
        // ������ɽ��Ӵ�С
        int r = rand() % 3;
        if (r == 0) {
            size = SMALL;
            value = 5;  // С����5��
        }
        else if (r == 1) {
            size = MEDIUM;
            value = 10;  // �н���10��
        }
        else {
            size = LARGE;
            value = 20;  // �����20��
        }
    }
};


int minerX = WIDTH / 2;
int minerY = HEIGHT - 500;  // ��λ��
int clawBaseY = minerY + 5;  // ������֧�����ӵ�
int clawLength = 600;
int currentClawLength = 0;
double clawAngle = 0;
bool isClawActive = false;
bool isRetracting = false;
int score = 0;
int carriedGoldIndex = -1;  // ��Я���Ľ�������
DWORD gameStartTime = 0;  // ��Ϸ��ʼʱ��
std::vector<Gold> golds;
IMAGE backgroundImg;  // ����ͼƬ



// �Ƕ�ת������
const double DEG_TO_RAD = 3.141592653589793 / 180.0;

void setup() {
    initgraph(WIDTH, HEIGHT);
    srand((unsigned int)time(0));

    // ����ͼ
    loadimage(&backgroundImg, L"background.jpg", WIDTH, HEIGHT, true);

    // ���ɳ�ʼ����
    for (int i = 0; i < 5; ++i) {
        int x = rand() % (WIDTH - 100) + 50;
        int y = rand() % (HEIGHT - 100) + 100;  // Y��100-600֮��
        golds.push_back(Gold(x, y));
    }
}

void drawStartScreen() {
    // ���Ʊ���
    setbkcolor(BLACK);
    cleardevice();

    // ���Ʊ���
    settextcolor(YELLOW);
    settextstyle(50, 0, L"����");
    outtextxy(WIDTH / 2 - 150, HEIGHT / 2 - 100, L"�ƽ��");

    // ������ʾ���֣�������ͣ��ʾ��
    settextcolor(WHITE);
    settextstyle(25, 0, L"����");
    outtextxy(WIDTH / 2 - 180, HEIGHT / 2 + 50, L"��Enter����ʼ��Ϸ");
    outtextxy(WIDTH / 2 - 200, HEIGHT / 2 + 80, L"Q/E����ת����  �ո������");
    outtextxy(WIDTH / 2 - 180, HEIGHT / 2 + 110, L"Ŀ��: 60���ڻ��100��");
    outtextxy(WIDTH / 2 - 150, HEIGHT / 2 + 140, L"P��: ��ͣ/������Ϸ");  // ����
}

void drawBackground() {
    // ���Ʊ���ͼ
    putimage(0, 0, &backgroundImg);


    setlinecolor(RGB(100, 100, 100));
    line(0, 100, WIDTH, 100);
}

void drawMiner() {
    setfillcolor(YELLOW);
    solidrectangle(minerX - 15, minerY - 20, minerX + 15, minerY - 5);  // ������
    setfillcolor(BROWN);
    solidrectangle(minerX - 5, minerY - 4, minerX + 5, minerY + 5);  // ֧��
}

void drawClaw() {
    int displayLength = isClawActive ? currentClawLength : 20;
    int clawEndX = minerX + displayLength * cos(clawAngle);
    int clawEndY = clawBaseY + displayLength * sin(clawAngle);

    setlinecolor(LIGHTBLUE);
    setlinestyle(PS_SOLID, 6);
    line(minerX, clawBaseY, clawEndX, clawEndY);

    // === ���������� ===
    const int CLAW_WIDTH = 20;
    const int CLAW_HEIGHT = 25;
    const int TOOTH_WIDTH = 4;
    const int TOOTH_LENGTH = 10;

    double sinA = sin(clawAngle);
    double cosA = cos(clawAngle);

    // ��������ĵ㣨��˳ʱ�룩
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

    // ���
    POINT leftTooth[3] = {
        clawBody[2],
        { (int)(clawBody[2].x + TOOTH_LENGTH * cosA + TOOTH_WIDTH * sinA),
          (int)(clawBody[2].y + TOOTH_LENGTH * sinA - TOOTH_WIDTH * cosA) },
        { (int)(clawBody[2].x + TOOTH_LENGTH * cosA - TOOTH_WIDTH * sinA),
          (int)(clawBody[2].y + TOOTH_LENGTH * sinA + TOOTH_WIDTH * cosA) }
    };
    setfillcolor(WHITE);
    fillpolygon(leftTooth, 3);

    //�ҳ�
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

        // ����Ч��
        setfillcolor(WHITE);
        solidcircle(gold.x - radius / 3, gold.y - radius / 3, radius / 5);
    }
}

void drawScore() {
    wchar_t scoreText[50];
    swprintf(scoreText, 50, L"����: %d / %d", score, TARGET_SCORE);
    settextcolor(WHITE);
    settextstyle(20, 0, L"����");
    outtextxy(10, 10, scoreText);
}

void drawTimer() {
    // ��������ͣʱ�����µ���ʱ
    if (isPaused) return;

    DWORD elapsedTime = (timeGetTime() - gameStartTime) / 1000;
    int remainingTime = COUNTDOWN_SECONDS - elapsedTime;

    if (remainingTime <= 0) {
        remainingTime = 0;
        currentState = GAME_OVER;
    }

    wchar_t timerText[20];
    swprintf(timerText, 20, L"ʱ��: %02d��", remainingTime);
    settextcolor(WHITE);
    settextstyle(20, 0, L"����");
    outtextxy(WIDTH - 130, 10, timerText);
}

// �޸ģ���չ��Ϸ��������Ϊͨ�����ֽ��棨֧����ͣ��
void drawGameOver() {

    setfillcolor(RGB(0, 0, 0, 180));
    solidrectangle(0, 0, WIDTH, HEIGHT);

    settextcolor(YELLOW);
    settextstyle(50, 0, L"����");

    if (currentState == GAME_OVER) {
        bool gameWon = (score >= TARGET_SCORE);
        if (gameWon) {
            outtextxy(WIDTH / 2 - 150, HEIGHT / 2 - 80, L"��ϲʤ��!");
        }
        else {
            outtextxy(WIDTH / 2 - 150, HEIGHT / 2 - 80, L"��Ϸ����");
        }
    }
    else if (isPaused) {  // ��������ͣʱ��ʾ������
        outtextxy(WIDTH / 2 - 120, HEIGHT / 2 - 80, L"��Ϸ��ͣ");
    }

    // ��ʾ����
    settextcolor(WHITE);
    settextstyle(30, 0, L"����");
    wchar_t finalScore[50];
    swprintf(finalScore, 50, L"��ǰ����: %d", score);
    outtextxy(WIDTH / 2 - 100, HEIGHT / 2 + 20, finalScore);


    settextstyle(20, 0, L"����");
    if (currentState == GAME_OVER) {
        outtextxy(WIDTH / 2 - 120, HEIGHT / 2 + 80, L"��Enter�����¿�ʼ");
    }
    else if (isPaused) {  // ��������ͣʱ����ʾ
        outtextxy(WIDTH / 2 - 120, HEIGHT / 2 + 80, L"��P��������Ϸ");
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
    isPaused = false;  // ������������ͣ״̬


    for (int i = 0; i < 5; ++i) {
        int x = rand() % (WIDTH - 100) + 50;
        int y = rand() % (HEIGHT - 100) + 100;
        golds.push_back(Gold(x, y));
    }

    gameStartTime = timeGetTime();
}

// �޸ģ��ƶ�����������ͣ�ж�
void moveMiner() {
    if (currentState != PLAYING || isPaused) return;  // ��ͣʱ����Ӧ����


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

// �޸ģ���Ϸ�߼�����������ͣ�ж�
void updateGame() {
    if (currentState != PLAYING || isPaused) return;  // ��ͣʱ�������߼�

    if (!isClawActive) return;

    // ���¼��ӳ���
    if (!isRetracting) {
        currentClawLength += 5;  // �쳤�ٶȹ̶�
        if (currentClawLength >= clawLength) {
            isRetracting = true;
        }
    }
    else {
        // ���ݽ��Ӵ�С�����ջ��ٶ�
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
            // �ӷֲ����ý���
            if (carriedGoldIndex != -1) {
                score += golds[carriedGoldIndex].value;
                // ����Ƿ���Ŀ��
                if (score >= TARGET_SCORE) {
                    currentState = GAME_OVER;
                }
                // �������ɽ���
                golds[carriedGoldIndex] = Gold(
                    rand() % (WIDTH - 100) + 50,
                    rand() % (HEIGHT - 100) + 100
                );
                carriedGoldIndex = -1;
            }
        }
    }

    // ����ĩ������
    int clawEndX = minerX + currentClawLength * cos(clawAngle);
    int clawEndY = clawBaseY + currentClawLength * sin(clawAngle);

    // ���Ӹ�������ƶ�
    if (carriedGoldIndex != -1) {
        golds[carriedGoldIndex].x = clawEndX;
        golds[carriedGoldIndex].y = clawEndY;
    }
    else {
        // ��ײ���
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

    // �߽���
    if (clawEndX < 0 || clawEndX > WIDTH || clawEndY < 0 || clawEndY > HEIGHT) {
        isRetracting = true;
    }
}

// �޸ģ���Ϸ��ѭ��������ͣ�߼�
void gameLoop() {
    DWORD lastFrameTime = timeGetTime();

    while (true) {
        DWORD currentTime = timeGetTime();
        DWORD elapsedTime = currentTime - lastFrameTime;

        if (elapsedTime >= FRAME_DELAY) {
            lastFrameTime = currentTime;

            // ������ȫ����ͣ������⣨P����
            if (currentState == PLAYING && isKeyPressed('P')) {
                isPaused = !isPaused;  // �л���ͣ״̬
                Sleep(200);  // ����������������������
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

                // �����������ͣ��������ͣ����
                if (isPaused) {
                    drawGameOver();  // �������ֽ���
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