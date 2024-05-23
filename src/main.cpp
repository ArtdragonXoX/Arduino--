#include <Arduino.h>

#include "LiquidCrystal_I2C.h"
#include "LedControl.h"

#include "List.h"

#define VERT_PIN_A A0
#define HORZ_PIN_A A1
#define SEL_PIN_A 9
#define VERT_PIN_B A3
#define HORZ_PIN_B A2
#define SEL_PIN_B 8

#define INITIAL 0
#define START_MENU 1
#define GAMING 2
#define END_GAME 3
#define END_MENU 4

#define UP byte(1)
#define DOWN byte(2)
#define LEFT byte(3)
#define RIGHT byte(4)
#define SEL byte(5)

#define BLOCK byte(0)
#define LEFTARROWS byte(1)
#define RIGHTARROWS byte(2)
#define HOOK byte(3)

#define SNAKE 1
#define TETRIS 2
#define DIE 3

#define PLAYER_A 65
#define PLAYER_B 66

typedef uint8_t RockerSignal;

typedef uint16_t Time;

struct Location
{
    uint8_t x, y;
    void SetData(uint8_t x, uint8_t y)
    {
        this->x = x;
        this->y = y;
    }
    // 重载==运算符用于判断是否坐标相等
    bool operator==(const Location &a)
    {
        if (x == a.x && y == a.y)
            return true;
        return false;
    }
    bool operator==(const ListNode &a)
    {
        if (x == a.x && y == a.y)
            return true;
        return false;
    }
};

// 摇杆PIN口
uint8_t RockerPin_A[] = {VERT_PIN_A, HORZ_PIN_A, SEL_PIN_A};
uint8_t RockerPin_B[] = {VERT_PIN_B, HORZ_PIN_B, SEL_PIN_B};

// 系统状态
uint8_t state = INITIAL;

// 玩家标签
bool playFlagA = false;
bool playFlagB = false;

// 玩家坐标
Location playerA, playerB;

// 玩家数据链表
List *listA, *listB;

// 主时钟
Time masterClock = 0;

// 强制移动的方向
uint8_t playerDirA;
uint8_t playerDirB;
// 伤害
uint8_t damageA;
uint8_t damageB;
// 攻击时间
Time ackClockA;
Time ackClockB;
uint8_t tick;             // 一秒的游戏刻
uint8_t tickTime;         // 一游戏刻的时间
uint8_t lengthA, lengthB; // 玩家长度

// 强制移动的时间
Time moveClockA;
Time moveClockB;

// 玩家状态
uint8_t playerStateA;
uint8_t playerStateB;

// 食物
Location foodA, foodB;

byte block_char[8] =
    {
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111};

byte leftArrows_char[8] =
    {
        B00010,
        B00100,
        B01000,
        B11111,
        B01000,
        B00100,
        B00010,
        B00000};

byte rightArrows_char[8] =
    {
        B01000,
        B00100,
        B00010,
        B11111,
        B00010,
        B00100,
        B01000,
        B00000};

byte hook_char[8] =
    {
        B00000,
        B00000,
        B00000,
        B00001,
        B00010,
        B10100,
        B01000,
        B00000};

// 初始化点阵屏
// 编号0为下半部分，右下为原点
LedControl lcA = LedControl(5, 7, 6, 2);
LedControl lcB = LedControl(2, 4, 3, 2);
// 声明lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 地图
uint8_t map_A[16] = {0};
uint8_t map_B[16] = {0};

// 位逆序
uint8_t ReverseBit(uint8_t x)
{
    x = (((x & 0xaa) >> 1) | ((x & 0x55) << 1));
    x = (((x & 0xcc) >> 2) | ((x & 0x33) << 2));

    return ((x >> 4) | (x << 4));
}

// 更新地图
void UpdateMap(char player, uint8_t x, uint8_t y, bool arg)
{
    uint8_t *map;
    if (player == PLAYER_A)
        map = map_A;
    if (player == PLAYER_B)
        map = map_B;
    uint8_t updateData = 0x80 >> x;
    if (arg)
        map[y] = map[y] | updateData;
    else
        map[y] = map[y] & ~updateData;
}

void UpdateMap(char player, const Location &location, bool arg)
{
    UpdateMap(player, location.x, location.y, arg);
}

// 查询地图
bool QueryMap(char player, uint8_t x, uint8_t y)
{
    uint8_t *map;
    if (player == PLAYER_A)
        map = map_A;
    if (player == PLAYER_B)
        map = map_B;
    uint8_t mask = 0x80 >> x;
    uint8_t data = mask & map[y];
    if (data > 0)
        return true;
    return false;
}

// 更新显示屏
void UpdateDisplay(char player, uint8_t y)
{
    LedControl *lc;
    uint8_t *map;
    if (player == PLAYER_A)
        map = map_A;
    if (player == PLAYER_B)
        map = map_B;
    if (player == PLAYER_A)
        lc = &lcA;
    if (player == PLAYER_B)
        lc = &lcB;
    lc->setColumn(y < 8 ? 0 : 1, y % 8, ReverseBit(map[y]));
}

void UpdateDisplay(char player)
{
    for (int i = 0; i < 16; i++)
        UpdateDisplay(player, i);
}

void ClearMap(char player)
{
    if (player == PLAYER_A)
        memset(map_A, 0, sizeof(map_A));
    if (player == PLAYER_B)
        memset(map_B, 0, sizeof(map_B));
}

void ClearMap()
{
    ClearMap(PLAYER_A);
    ClearMap(PLAYER_B);
}

// 读取摇杆数据
RockerSignal GetRockerSignal(char player)
{
    uint8_t *pin = nullptr;
    if (player == PLAYER_A)
        pin = RockerPin_A;
    if (player == PLAYER_B)
        pin = RockerPin_B;
    if (digitalRead(pin[2]) == LOW)
        return SEL;
    if (analogRead(pin[0]) < 300)
        return DOWN;
    if (analogRead(pin[0]) > 700)
        return UP;
    if (analogRead(pin[1]) < 300)
        return RIGHT;
    if (analogRead(pin[1]) > 700)
        return LEFT;
    return 0;
}

// 清除游戏屏幕
void ClearLED(char player)
{
    if (player == PLAYER_A)
    {
        lcA.clearDisplay(0);
        lcA.clearDisplay(1);
    }
    if (player == PLAYER_B)
    {
        lcB.clearDisplay(0);
        lcB.clearDisplay(1);
    }
}

void ClearLED()
{
    ClearLED(PLAYER_A);
    ClearLED(PLAYER_B);
}

// 上移地图
bool ShiftUpMap(char player, uint8_t data)
{
    uint8_t *map;
    if (player == PLAYER_A)
        map = map_A;
    if (player == PLAYER_B)
        map = map_B;
    if (map[15] > 0)
        return false;
    for (uint8_t i = 15; i > 1; i--)
        map[i] = map[i - 1];
    map[0] = data;
    return true;
}

// 去除地图其中一行
void RemoveRow(char player, uint8_t row)
{
    uint8_t *map;
    if (player == PLAYER_A)
        map = map_A;
    if (player == PLAYER_B)
        map = map_B;
    for (uint8_t i = row; i < 15; i++)
        map[i] = map[i + 1];
}

// 检查地图是有可消除行
uint8_t CheckMap(char player)
{
    uint8_t res = 0;
    uint8_t *map;
    if (player == PLAYER_A)
        map = map_A;
    if (player == PLAYER_B)
        map = map_B;
    for (uint8_t i = 0; i < 15; i++)
    {
        if (map[i - res] == 0xff)
        {
            RemoveRow(player, i - res);
            res++;
        }
    }
    return res;
}

// 俄罗斯方块实体化
bool TetrisTrMap(char player)
{
    List *playerList;
    if (player == PLAYER_A)
        playerList = listA;
    if (player == PLAYER_B)
        playerList = listB;
    playerList->InitIter();
    while (playerList->Iter())
    {
        auto iter = playerList->Iter();
        if (iter->y > 15)
            return false;
        UpdateMap(player, iter->x, iter->y, true);
    }
    return true;
}

// 玩家坐标移动
bool MovePlayer(char player, uint8_t direction)
{
    if (direction == 0)
        return true;
    uint8_t *map;
    Location *playerL;
    if (player == PLAYER_A)
    {
        playerL = &playerA;
        map = map_A;
    }
    if (player == PLAYER_B)
    {
        playerL = &playerB;
        map = map_B;
    }
    Location _playerL = *playerL;
    if (direction == LEFT)
        playerL->x--;
    else if (direction == RIGHT)
        playerL->x++;
    else if (direction == UP)
        playerL->y++;
    else if (direction == DOWN)
        playerL->y--;
    uint8_t mapData = (map[playerL->y] << playerL->x) & 0x80;
    if (playerL->x > 7 || playerL->x < 0 || playerL->y < 0 || playerL->y > 15 || mapData == 0x80)
    {
        *playerL = _playerL;
        return false;
    }
    return true;
}

// 蛇移动
bool MoveSnake(char player, uint8_t direction, uint8_t length)
{
    List *playerList;
    Location *playerL;
    if (player == PLAYER_A)
    {
        playerList = listA;
        playerL = &playerA;
    }
    if (player == PLAYER_B)
    {
        playerList = listB;
        playerL = &playerB;
    }
    playerList->InitIter();
    uint8_t nowLength = playerList->Count();
    if (!MovePlayer(player, direction))
        return false;
    if (nowLength < length)
    {
        playerList->CopyBack();
        auto back = playerList->Back();
        if (back->x == playerL->x && back->y == playerL->y)
            return false;
    }
    else
        playerList->RemoveBack();
    while (playerList->Iter())
    {
        auto iter = playerList->Iter();
        if (iter->x == playerL->x && iter->y == playerL->y)
            return false;
        playerList->NextIter();
    }
    playerList->PushFront(playerL->x, playerL->y);
    return true;
}

// 俄罗斯方块移动
bool MoveTetris(char player, uint8_t direction)
{
    List *playerList;
    if (player == PLAYER_A)
        playerList = listA;
    if (player == PLAYER_B)
        playerList = listB;
    playerList->InitIter();
    while (playerList->Iter())
    {
        auto iter = playerList->Iter();
        if (direction == SEL)
        {
            while (MoveTetris(player, DOWN))
                return false;
        }
        else if (direction == DOWN)
        {
            if (QueryMap(player, iter->x, iter->y - 1))
                return false;
        }
        else if (direction == LEFT)
        {
            if (iter->x == 0 || QueryMap(player, iter->x - 1, iter->y))
            {
                return true;
            }
        }
        else if (direction == RIGHT)
        {
            if (iter->x == 7 || QueryMap(player, iter->x + 1, iter->y))
            {
                return true;
            }
        }
    }
    if (direction == DOWN)
    {
        playerList->SubY();
    }
    else if (direction == LEFT)
    {
        playerList->SubX();
    }
    else if (direction == RIGHT)
    {
        playerList->AddX();
    }
    return true;
}

// 初始化系统
void InitSystem()
{
    playFlagA = false;
    playFlagB = false;
    playerA.SetData(0, 0);
    playerB.SetData(0, 0);
    listA->RemoveAll();
    listB->RemoveAll();
    masterClock = 0;
    playerDirA = DOWN;
    playerDirB = DOWN;
    damageA = 0;
    damageB = 0;
    ackClockA = 0;
    ackClockB = 0;
    lengthA = 0;
    lengthB = 0;
    tick = 20;
    tickTime = 1000 / tick;
    moveClockA = tick;
    moveClockB = tick;
    playerStateA = INITIAL;
    playerStateB = INITIAL;
    ClearMap();
    ClearLED();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 8; j++)
        {
            lcA.setLed(i < 8 ? 0 : 1, i % 2 == 1 ? j : 7 - j, i % 8, true);
            lcB.setLed(i < 8 ? 0 : 1, i % 2 == 1 ? j : 7 - j, i % 8, true);
            delay(0);
        }

    ClearLED();
    lcd.clear();
    state = START_MENU;
    delay(100);
}

// 开始菜单
void StartMenu()
{
    lcd.setCursor(2, 0);
    lcd.print("WAIT PLAYERS");
    // 玩家准备标签
    bool readyFlagA = false;
    bool readyFlagB = false;
    // 等待玩家
    while (true)
    {
        if (playFlagA)
        {
            uint8_t opA = GetRockerSignal(PLAYER_A);
            MovePlayer(PLAYER_A, opA);
            if (opA == SEL)
                readyFlagA = true;
            UpdateMap(PLAYER_A, playerA, true);
            UpdateDisplay(PLAYER_A);
        }
        if (!playFlagA)
            if (GetRockerSignal(PLAYER_A))
            {
                playFlagA = true;
                lcd.setCursor(0, 1);
                lcd.write(LEFTARROWS);
                lcd.print("JOIN A");
            }
        if (playFlagB)
        {
            uint8_t opB = GetRockerSignal(PLAYER_B);
            MovePlayer(PLAYER_B, opB);
            if (opB == SEL)
                readyFlagB = true;
            UpdateMap(PLAYER_B, playerB, true);
            UpdateDisplay(PLAYER_B);
        }
        if (!playFlagB)
            if (GetRockerSignal(PLAYER_B))
            {
                playFlagB = true;
                lcd.setCursor(9, 1);
                lcd.print("B JOIN");
                lcd.write(RIGHTARROWS);
            }
        if (readyFlagA)
        {
            lcd.setCursor(0, 1);
            lcd.write(HOOK);
            lcd.print("READY ");
        }
        if (readyFlagB)
        {
            lcd.setCursor(9, 1);
            lcd.print(" READY");
            lcd.write(HOOK);
        }
        if (playFlagA && readyFlagA && !playFlagB || playFlagB && readyFlagB && !playFlagA || readyFlagA && readyFlagB)
            break;
        delay(20);
    }
    delay(100);
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("START GAME");
    lcd.setCursor(5, 1);
    for (int i = 3; i > 0; i--)
    {
        lcd.print(i);
        lcd.print(' ');
        delay(1000);
    }
    state = GAMING;
    lcd.clear();
    ClearMap();
    ClearLED();
}

void PlayerEvent(char player, uint8_t &length, Location &food, Time &moveClock, uint8_t &playerDir, Location &playerL, uint8_t &playerState, List *list)
{
    RockerSignal op = GetRockerSignal(player);
    if (playerState == SNAKE)
    {
        if (op != 0 && op != SEL)
        {
            playerDir = op;
            if (!MoveSnake(player, op, length)) // 判断移动是否成功
            {
                playerState = DIE;
            }
            else
            {
                if (playerL == food) // 如果吃到食物
                {
                    playerState = TETRIS;
                }
                moveClock = masterClock + tick;
            }
        }
        else
        {
            if (moveClock == masterClock)
            {
                if (!MoveSnake(player, playerDir, length)) // 判断移动是否成功
                {
                    playerState = DIE;
                }
                else
                {
                    if (playerL == food) // 如果吃到食物
                    {
                        playerState = TETRIS;
                    }
                    moveClock = masterClock + tick;
                }
            }
        }
    }
    else if (playerState == TETRIS)
    {
    }
    else if (playerState == INITIAL)
    {
        playerL.SetData(random(0, 8), 15);
        list->RemoveAll();
        list->PushFront(playerL.x, 15);
        playerDir = DOWN;
        playerState = SNAKE;
        food.SetData(random(1, 7), random(10, 13));
        length = random(3, 7);
        moveClock = masterClock + tick;
    }
}

// 游戏进程
void Gaming()
{
    while (true)
    {
        if (playFlagA) // 处理玩家A事件
        {
            PlayerEvent(PLAYER_A, lengthA, foodA, moveClockA, playerDirA, playerA, playerStateA, listA);
            UpdateDisplay(PLAYER_A);
            listA->InitIter();

            while (listA->Iter())
            {
                auto iter = listA->Iter();
                lcA.setLed(iter->y < 8 ? 0 : 1, 7 - iter->x, iter->y % 8, true);
                lcd.setCursor(0, 0);
                lcd.clear();
                lcd.print(iter->y);
                lcd.print(iter->x);
            }
        }
        delay(tickTime);
        masterClock++;
    }
}

void setup()
{
    lcd.init();
    lcd.backlight();
    // 初始化摇杆
    pinMode(VERT_PIN_A, INPUT);
    pinMode(HORZ_PIN_A, INPUT);
    pinMode(SEL_PIN_A, INPUT_PULLUP);
    pinMode(VERT_PIN_B, INPUT);
    pinMode(HORZ_PIN_B, INPUT);
    pinMode(SEL_PIN_B, INPUT_PULLUP);
    // 初始化lcd屏
    lcd.begin(16, 2);
    lcd.createChar(BLOCK, block_char);
    lcd.createChar(LEFTARROWS, leftArrows_char);
    lcd.createChar(RIGHTARROWS, rightArrows_char);
    lcd.createChar(HOOK, hook_char);
    listA = InitList();
    listB = InitList();

    randomSeed(analogRead(A4));
    InitSystem();
    StartMenu();
    Gaming();
}

void loop()
{
}
