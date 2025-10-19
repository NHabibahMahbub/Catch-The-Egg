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

void saveHighscore()
{
    if(score > highscore)
    {
        std::ofstream f("highscore.txt");
        if(f) f << score;
    }
}

// Utility to draw text
void drawText(float x, float y, const std::string &s)
{
    glRasterPos2f(x,y);
    for(char c: s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}


//Jannatullllllllllllllllllllllllllllll
// Spawn an item at x (chicken position)
void spawnItem(float x)
{
    Item it;
    it.t = randomItemType();
    it.x = x;
    it.y = chickenY - 0.06f;
    it.vy = 0.36f + randf(0.0f, 0.45f);
    it.rot = randf(-40.0f, 40.0f);
    items.push_back(it);
}

// Collision check with basket
bool catchCheck(const Item &it, float bx, float by, float halfw)
{
    float left = bx-halfw, right = bx+halfw;
    float top = by;
    float bottom = by - basketHeight;
    return (it.x > left && it.x < right && it.y < top && it.y > bottom);
}

// Start / reset game
void startGame()
{
    state = RUNNING;
    score = 0;
    timeLeft = TIME_LIMIT;
    lastTick = glutGet(GLUT_ELAPSED_TIME);
    chickenX = 0.0f;
    items.clear();
    sinceLastLay = 0.0f;
    perkSlow_until = perkLarge_until = 0.0;
    basketHalfWidth = 0.13f;
}

// Rendering
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // background
    drawBackground();

    // bamboo stick & chicken
    drawBamboo();
    drawChicken(chickenX, chickenY);

    // items
    for(const Item &it: items) drawItem(it);

    // basket
    drawBasket(basketX, basketY, basketHalfWidth);

    // HUD
    glColor3f(0,0,0);
    std::stringstream ss;
    ss << "Score: " << score;
    drawText(-0.95f, 0.92f, ss.str());
    ss.str("");
    ss.clear();
    ss << "High: " << std::max(score, highscore);
    drawText(-0.3f, 0.92f, ss.str());
    ss.str("");
    ss.clear();
    ss << "Time: " << timeLeft;
    drawText(0.55f, 0.92f, ss.str());

    double now = glutGet(GLUT_ELAPSED_TIME)/1000.0;
    if(now < perkSlow_until) drawText(-0.95f, 0.86f, "PERK: Slow");
    if(now < perkLarge_until) drawText(-0.95f, 0.82f, "PERK: Large");

    // Menu / paused / gameover overlays
    if(state == MENU)
    {
        glColor3f(0,0,0);
        drawText(-0.18f, 0.2f, "CATCH THE EGGS");
        drawText(-0.32f, 0.05f, "Press SPACE to Start | Esc to Quit");
        drawText(-0.32f, -0.05f, "Move basket: Arrow keys or Mouse");
        drawText(-0.32f, -0.15f, "Catch eggs (golden 10, blue 5, normal 1). Avoid poop (-10).");
    }
    else if(state == PAUSED)
    {
        drawText(-0.08f, 0.0f, "PAUSED - press 'p' to resume");
    }
    else if(state == GAMEOVER)
    {
        drawText(-0.2f, 0.05f, "GAME OVER");
        ss.str("");
        ss.clear();
        ss << "Final Score: " << score;
        drawText(-0.28f, -0.05f, ss.str());
        drawText(-0.28f, -0.15f, "Press 'r' to Restart or Esc for Menu");
    }

    glutSwapBuffers();
}
