#include <GL/glut.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <sstream>

int WIN_W = 900, WIN_H = 600;

enum GameState { MENU, RUNNING, PAUSED, GAMEOVER };
GameState state = MENU;

// Timing
int TIME_LIMIT = 60;      // seconds
int timeLeft = TIME_LIMIT;
int lastTick = 0;         // ms

// Score/highscore
int score = 0;
int highscore = 0;

// Chicken
float chickenX = 0.0f;    // -1..1 world coords
const float chickenY = 0.80f;
float chickenSpeed = 0.7f; // units/sec
int chickenDir = 1;       // 1 right -1 left
float layInterval = 0.8f;
float sinceLastLay = 0.0f;

// Basket
float basketX = 0.0f;
const float basketY = -0.78f;
float basketHalfWidth = 0.13f; // default
const float basketHeight = 0.08f;

// Perks
double perkSlow_until = 0.0;
double perkLarge_until = 0.0;

// Items
enum ItemType { NORMAL, BLUE, GOLDEN, POOP, PERK_SLOW, PERK_LARGE, PERK_TIME };
struct Item
{
    ItemType t;
    float x, y;
    float vy;
    float rot; // rotation degrees
};
std::vector<Item> items;

// Random helper
float randf(float a, float b)
{
    return a + (b-a)*(rand()/(float)RAND_MAX);
}

// Spawn distribution
ItemType randomItemType()
{
    float r = randf(0,1);
    if(r < 0.60f) return NORMAL;
    if(r < 0.80f) return BLUE;
    if(r < 0.88f) return GOLDEN;
    if(r < 0.98f) return POOP;
    float p = randf(0,1);
    if(p < 0.33f) return PERK_SLOW;
    if(p < 0.66f) return PERK_LARGE;
    return PERK_TIME;
}

// Highscore
void loadHighscore()
{
    std::ifstream f("highscore.txt");
    if(f) f >> highscore;
}
