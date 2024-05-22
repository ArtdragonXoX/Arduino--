#include <Arduino.h>

#include "LiquidCrystal.h"
#include "LedControl.h"

#include "List.h"

#define VERT_PIN_A A0
#define HORZ_PIN_A A1
#define SEL_PIN_A A2
#define VERT_PIN_B A3
#define HORZ_PIN_B A4
#define SEL_PIN_B A5

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

#define DIE 0
#define SNAKE 1
#define TETRIS 2

#define PLAYER_A 65
#define PLAYER_B 66

typedef uint8_t RockerSignal;

uint8_t RockerPin_A[] = {VERT_PIN_A, HORZ_PIN_A, SEL_PIN_A};
uint8_t RockerPin_B[] = {VERT_PIN_B, HORZ_PIN_B, SEL_PIN_B};

uint8_t state = INITIAL;

uint8_t playerStateA = 0;
uint8_t playerStateB = 0;

bool playFlagA = false;
bool playFlagB = false;

uint8_t playAX, playAY, playBX, playBY;

List *listA, *listB;

int masterClock = 0;
int playerClockA = 0;
int playerClockB = 0;

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
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

uint8_t map_A[16] = {0};
uint8_t map_B[16] = {0};

// 位逆序
uint8_t ReverseBit(uint8_t x)
{
    x = (((x & 0xaa) >> 1) | ((x & 0x55) << 1));
    x = (((x & 0xcc) >> 2) | ((x & 0x33) << 2));

    return ((x >> 4) | (x << 4));
}

bool MovePlayer(char player, uint8_t direction)
{
    if (direction == 0)
        return true;
    if (player == PLAYER_A)
    {
        uint8_t _playAX = playAX, _playAY = playAY;
        if (direction == LEFT)
            playAX--;
        else if (direction == RIGHT)
            playAX++;
        else if (direction == UP)
            playAY++;
        else if (direction == DOWN)
            playAY--;

        uint8_t mapData = (map_A[playAY] << playAX) & 0x80;

        if (playAX > 7 || playAX < 0 || playAY < 0 || playAY > 15 || mapData == 0x80)
        {
            playAX = _playAX;
            playAY = _playAY;
            return false;
        }
    }
    else if (player == PLAYER_B)
    {
        uint8_t _playBX = playBX, _playBY = playBY;
        if (direction == LEFT)
            playBX--;
        else if (direction == RIGHT)
            playBX++;
        else if (direction == UP)
            playBY++;
        else if (direction == DOWN)
            playBY--;

        uint8_t mapData = (map_B[playBY] << playBX) & 0x80;

        if (playBX > 7 || playBX < 0 || playBY < 0 || playBY > 15 || mapData == 0x80)
        {
            playBX = _playBX;
            playBY = _playBY;
            return false;
        }
    }
    return true;
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

// 初始化系统
void InitSystem()
{
    playFlagA = false;
    playFlagB = false;
    playAX = playAY = playBX = playBY = 0;
    listA->RemoveAll();
    listB->RemoveAll();
    masterClock = 0;
    playerClockA = 0;
    playerClockB = 0;
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
    bool readyFlagA = false;
    bool readyFlagB = false;
    while (true)
    {
        if (playFlagA)
        {
            uint8_t opA = GetRockerSignal(PLAYER_A);
            MovePlayer(PLAYER_A, opA);
            if (opA == SEL)
                readyFlagA = true;
            UpdateMap(PLAYER_A, playAX, playAY, true);
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
            UpdateMap(PLAYER_B, playBX, playBY, true);
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
}

// 游戏进程
void Gaming()
{
    uint8_t playerDirA = DOWN;
    uint8_t playerDirB = DOWN;
    uint8_t ackA = 0;
    uint8_t ackB = 0;
    uint8_t ackClock = 3;
}

void setup()
{
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

    randomSeed(analogRead(0));
    InitSystem();
    StartMenu();
}

void loop()
{
}
