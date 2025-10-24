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
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")  // link Windows multimedia library


int WIN_W = 900, WIN_H = 600;

enum GameState { MENU, RUNNING, PAUSED, GAMEOVER, HELP };
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

// World coordinate helpers: we use an orthographic -1..1 space
float screenToWorldX(int sx)
{
    return (sx / (float)WIN_W)*2.0f - 1.0f;
}

// Drawing primitives
void drawCircle(float cx, float cy, float r, int steps=28)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for(int i=0; i<=steps; i++)
    {
        float a = (i/(float)steps) * 2.0f * M_PI;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

void drawEllipse(float cx, float cy, float rx, float ry, int steps=28)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for(int i=0; i<=steps; i++)
    {
        float a = (i/(float)steps) * 2.0f * M_PI;
        glVertex2f(cx + cosf(a)*rx, cy + sinf(a)*ry);
    }
    glEnd();
}

// Draw background (sky gradient + grass)
void drawBackground()
{
    // Sky gradient (two rects)
    glBegin(GL_QUADS);
    glColor3f(0.69f,0.9f,1.0f);
    glVertex2f(-1,-0.05f);
    glColor3f(0.33f,0.75f,0.98f);
    glVertex2f(1,-0.05f);
    glColor3f(0.2f,0.6f,0.95f);
    glVertex2f(1,1);
    glColor3f(0.6f,0.9f,1.0f);
    glVertex2f(-1,1);
    glEnd();

    // Grass strip
    glColor3f(0.43f,0.87f,0.48f);
    glBegin(GL_QUADS);
    glVertex2f(-1,-1);
    glVertex2f(1,-1);
    glVertex2f(1,-0.05f);
    glVertex2f(-1,-0.05f);
    glEnd();

    // Simple clouds - white ellipses
    glColor3f(1,1,1);
    drawEllipse(-0.7f, 0.7f, 0.16f, 0.08f);
    drawEllipse(-0.55f, 0.73f, 0.12f, 0.06f);
    drawEllipse(-0.6f, 0.68f, 0.12f, 0.07f);

    drawEllipse(0.5f, 0.6f, 0.18f, 0.09f);
    drawEllipse(0.65f, 0.63f, 0.12f, 0.06f);
    drawEllipse(0.55f, 0.58f, 0.12f, 0.07f);
}

// Draw bamboo stick in the center
// Draw natural horizontal bamboo branch with gently swaying leaves
void drawBamboo()
{
    float branchY = chickenY - 0.07f; // just below the chicken

    // Get elapsed time (for leaf animation)
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // main branch – slightly curved
    glColor3f(0.22f, 0.55f, 0.16f);
    glBegin(GL_QUAD_STRIP);
    for (float x = -0.9f; x <= 0.9f; x += 0.05f)
    {
        float curve = 0.03f * sinf(x * 3.0f);
        glVertex2f(x, branchY + curve - 0.012f);
        glVertex2f(x, branchY + curve + 0.012f);
    }
    glEnd();

    // bamboo joints
    glColor3f(0.15f, 0.4f, 0.12f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    for (float x = -0.9f; x <= 0.9f; x += 0.18f)
    {
        float curve = 0.03f * sinf(x * 3.0f);
        glVertex2f(x, branchY + curve - 0.012f);
        glVertex2f(x, branchY + curve + 0.012f);
    }
    glEnd();

    // animated leaves
    glColor3f(0.2f, 0.75f, 0.25f);
    for (float x = -0.8f; x <= 0.8f; x += 0.25f)
    {
        float curve = 0.03f * sinf(x * 3.0f);
        float leafY = branchY + curve + 0.02f;

        // sway offset using time and position
        float sway = 0.015f * sinf(t * 2.0f + x * 3.5f);

        // right leaf
        glBegin(GL_TRIANGLES);
        glVertex2f(x, leafY);
        glVertex2f(x + sway + 0.04f, leafY + 0.05f);
        glVertex2f(x + sway + 0.02f, leafY);
        glEnd();

        // left leaf
        glBegin(GL_TRIANGLES);
        glVertex2f(x, leafY);
        glVertex2f(x + sway - 0.04f, leafY + 0.05f);
        glVertex2f(x + sway - 0.02f, leafY);
        glEnd();
    }
}

// Draw a stylized chicken (simple shapes)
void drawChicken(float cx, float cy)
{
    // body
    glColor3f(1.0f,0.92f,0.6f);
    drawEllipse(cx-0.02f, cy-0.01f, 0.08f, 0.05f);
    // head
    glColor3f(1.0f,0.92f,0.6f);
    drawCircle(cx+0.06f, cy+0.02f, 0.032f);
    // eye
    glColor3f(0,0,0);
    drawCircle(cx+0.069f, cy+0.028f, 0.006f);
    // beak (triangle)
    glColor3f(1.0f,0.6f,0.12f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx+0.095f, cy+0.02f);
    glVertex2f(cx+0.115f, cy+0.01f);
    glVertex2f(cx+0.095f, cy-0.00f);
    glEnd();
    // wing
    glColor3f(1.0f,0.85f,0.5f);
    drawEllipse(cx-0.055f, cy-0.01f, 0.03f, 0.02f);
    // feet (tiny lines)
    glColor3f(1.0f,0.6f,0.12f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glVertex2f(cx-0.02f, cy-0.05f);
    glVertex2f(cx-0.03f, cy-0.08f);
    glVertex2f(cx+0.02f, cy-0.05f);
    glVertex2f(cx+0.03f, cy-0.08f);
    glEnd();
}

// Draw basket (rounded rectangle)
void drawBasket(float bx, float by, float halfw)
{
    // rim
    glColor3f(0.45f,0.28f,0.12f);
    glBegin(GL_QUADS);
    glVertex2f(bx - halfw, by);
    glVertex2f(bx + halfw, by);
    glVertex2f(bx + halfw, by - 0.02f);
    glVertex2f(bx - halfw, by - 0.02f);
    glEnd();

// body
    glColor3f(0.7f,0.48f,0.2f);
    glBegin(GL_QUADS);
    glVertex2f(bx - halfw * 0.95f, by - 0.02f);
    glVertex2f(bx + halfw * 0.95f, by - 0.02f);
    glVertex2f(bx + halfw * 0.6f, by - basketHeight);
    glVertex2f(bx - halfw * 0.6f, by - basketHeight);
    glEnd();

// weave lines
    glColor3f(0.56f,0.36f,0.17f);
    glLineWidth(2);
    glBegin(GL_LINES);
    for(float x = bx - halfw * 0.6f; x <= bx + halfw * 0.6f; x += (halfw * 0.9f)/4.0f)
    {
        glVertex2f(x, by - 0.02f);
        glVertex2f(x, by - basketHeight);
    }
    glEnd();

}

// Draw an item (egg/poop/perk)
void drawItem(const Item &it)
{
    switch(it.t)
    {
    case NORMAL:
        glColor3f(1.0f,1.0f,0.95f);
        drawEllipse(it.x, it.y, 0.016f, 0.022f);
        break;
    case BLUE:
        glColor3f(0.18f,0.35f,0.92f);
        drawEllipse(it.x, it.y, 0.018f, 0.024f);
        break;
    case GOLDEN:
        glColor3f(1.0f,0.85f,0.12f);
        drawEllipse(it.x, it.y, 0.02f, 0.028f);
        break;
    case POOP:
        glColor3f(0.25f,0.12f,0.03f);
        // small stack of circles to look like poop
        drawCircle(it.x, it.y, 0.018f);
        drawCircle(it.x-0.009f, it.y+0.007f, 0.012f);
        drawCircle(it.x+0.009f, it.y+0.007f, 0.012f);
        break;
    case PERK_SLOW:
        glColor3f(0.7f,0.9f,1.0f);
        glBegin(GL_QUADS);
        glVertex2f(it.x-0.02f, it.y-0.02f);
        glVertex2f(it.x+0.02f, it.y-0.02f);
        glVertex2f(it.x+0.02f, it.y+0.02f);
        glVertex2f(it.x-0.02f, it.y+0.02f);
        glEnd();
        glColor3f(0,0,0);
        drawText(it.x-0.012f, it.y-0.006f, "S");
        break;
    case PERK_LARGE:
        glColor3f(0.88f,0.7f,1.0f);
        glBegin(GL_QUADS);
        glVertex2f(it.x-0.02f, it.y-0.02f);
        glVertex2f(it.x+0.02f, it.y-0.02f);
        glVertex2f(it.x+0.02f, it.y+0.02f);
        glVertex2f(it.x-0.02f, it.y+0.02f);
        glEnd();
        glColor3f(0,0,0);
        drawText(it.x-0.012f, it.y-0.006f, "L");
        break;
    case PERK_TIME:
        glColor3f(0.7f,1.0f,0.7f);
        glBegin(GL_QUADS);
        glVertex2f(it.x-0.02f, it.y-0.02f);
        glVertex2f(it.x+0.02f, it.y-0.02f);
        glVertex2f(it.x+0.02f, it.y+0.02f);
        glVertex2f(it.x-0.02f, it.y+0.02f);
        glEnd();
        glColor3f(0,0,0);
        drawText(it.x-0.012f, it.y-0.006f, "+");
        break;
    }
}


void playBackgroundMusic(const char* file)
{
    // Windows example
    PlaySound(TEXT(file), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
}

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
    std::stringstream ss;   // create a string stream (used to join text + numbers easily)
    ss << "Score: " << score;          // put "Score: <number>" inside ss
    drawText(-0.95f, 0.92f, ss.str()); // show it on screen
    ss.str("");   // empty old text from ss (clear string data)
    ss.clear();   // reset flags so we can use ss again safely
    ss << "High: " << std::max(score, highscore); // put new text
    drawText(-0.3f, 0.92f, ss.str());             // show on screen
    ss.str("");   // clear text again
    ss.clear();   // reset again for next use
    ss << "Time: " << timeLeft;       // put new text
    drawText(0.55f, 0.92f, ss.str()); // show it

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
    else if(state == HELP)
    {
        glColor3f(0,0,0);
        drawText(-0.20f, 0.6f, "HELP MENU");
        drawText(-0.45f, 0.4f, "Catch eggs: Normal=1, Blue=5, Golden=10");
        drawText(-0.45f, 0.3f, "Avoid poop: -10 points");
        drawText(-0.45f, 0.2f, "Arrow keys / Mouse(Click and Hold to play): Move basket");
        drawText(-0.45f, 0.1f, "P: Pause/Resume game");
        drawText(-0.45f, 0.0f, "H: Help menu");
        drawText(-0.45f, -0.1f, "SPACE: Start game from menu");
        drawText(-0.45f, -0.2f, "Press Esc to quit the game");
        drawText(-0.20f, -0.4f, "Press 'p' to resume this game" );
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
// Game logic update
void updateSim(float dt)
{
    if(state != RUNNING) return;

    // chicken moves horizontally along stick
    chickenX += chickenDir * chickenSpeed * dt;
    if(chickenX > 0.88f) { chickenX = 0.88f; chickenDir = -1; }
    if(chickenX < -0.88f) { chickenX = -0.88f; chickenDir = 1; }

    // laying interval (slowed by perk)
    sinceLastLay += dt;
    float currentLayInterval = layInterval;
    double now = glutGet(GLUT_ELAPSED_TIME)/1000.0;
    if(now < perkSlow_until) currentLayInterval *= 1.6f;
    if(sinceLastLay >= currentLayInterval){
        sinceLastLay = 0.0f;
        spawnItem(chickenX + randf(-0.07f, 0.07f));
    }

    // move items downward, rotate them slightly
    for(Item &it: items){
        float speedFactor = 1.0f;
        if(now < perkSlow_until) speedFactor = 0.55f;
        it.y -= it.vy * speedFactor * dt;
        it.rot += 60.0f * dt;
    }

    // collisions & out-of-bounds
    std::vector<Item> remaining;
    for(const Item &it: items){
        if(it.y < -1.4f) continue; // fell away
        if(catchCheck(it, basketX, basketY, basketHalfWidth)){
                        // caught 
            switch(it.t)
            {
            case NORMAL:
                score += 1;
                PlaySound(TEXT("white.wav"), NULL, SND_FILENAME | SND_ASYNC); // white egg
                break;
            case BLUE:
                score += 5;
                PlaySound(TEXT("blue.wav"), NULL, SND_FILENAME | SND_ASYNC); // blue egg
                break;
            case GOLDEN:
                score += 10;
                PlaySound(TEXT("yellow.wav"), NULL, SND_FILENAME | SND_ASYNC); // golden/yellow egg
                break;
            case POOP:
                score -= 10;
                if(score < 0) score = 0;
                PlaySound(TEXT("poop.wav"), NULL, SND_FILENAME | SND_ASYNC); // poop
                break;
            case PERK_SLOW:
                perkSlow_until = now + 6.0;
                PlaySound(TEXT("bonus.wav"), NULL, SND_FILENAME | SND_ASYNC); // perk bonus
                break;
            case PERK_LARGE:
                perkLarge_until = now + 8.0;
                basketHalfWidth = 0.22f;
                PlaySound(TEXT("bonus.wav"), NULL, SND_FILENAME | SND_ASYNC); // perk bonus
                break;
            case PERK_TIME:
                timeLeft += 6;
                PlaySound(TEXT("bonus.wav"), NULL, SND_FILENAME | SND_ASYNC); // perk bonus
                break;
            }

        } else {
            remaining.push_back(it);
        }
    }
    items.swap(remaining);

    // revert large perk when expired
    if(now > perkLarge_until) basketHalfWidth = 0.13f;

    // timer countdown
    static float accum = 0.0f;
    accum += dt;
    if(accum >= 1.0f){
        timeLeft -= (int)accum;
        accum = 0.0f;
    }
    if(timeLeft <= 0){
        state = GAMEOVER;
        saveHighscore();
    }
}

// Timer callback ~60fps
void timerFunc(int v){
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTick)/1000.0f;
    if(lastTick == 0) dt = 1.0f/60.0f;
    lastTick = now;

    if(state == RUNNING) updateSim(dt);

    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);
}

// Input handlers
void keyboard(unsigned char key, int x, int y){
    if(key == 27){ // Esc
        if(state == RUNNING || state == PAUSED || state == GAMEOVER){
            state = MENU;
            saveHighscore();
        } else exit(0);
    }
    if(state == MENU){
        if(key == ' '){
            startGame();
        }
    } else if(state == RUNNING){
        if(key == 'p' || key == 'P'){
            state = PAUSED;
        }
    } else if(state == PAUSED){
        if(key == 'p' || key == 'P'){
            state = RUNNING;
            lastTick = glutGet(GLUT_ELAPSED_TIME);
        }
    } else if(state == GAMEOVER){
        if(key == 'r' || key == 'R'){
            startGame();
        }
        if(key == 'h' || key == 'H')    // open help from gameover
        {
            state = HELP;
        }
    }
    else if(state == HELP)   // currently in help menu
    {
        if(key == 'p' || key == 'P' || key == 13)  // Enter or R to go back
        {
            state = RUNNING;  // or RUNNING if you want resume
        }
        if(key == 27)   // Esc also goes back
        {
            state = MENU;
        }
    }
}


void specialKey(int k, int x, int y){
    if(state != RUNNING) return;
    float move = 0.06f;
    if(k == GLUT_KEY_LEFT) basketX -= move;
    if(k == GLUT_KEY_RIGHT) basketX += move;
    if(basketX < -0.95f) basketX = -0.95f;
    if(basketX > 0.95f) basketX = 0.95f;
}

void passiveMouse(int mx, int my){
    if(state != RUNNING) return;
    basketX = screenToWorldX(mx);
    if(basketX < -0.95f) basketX = -0.95f;
    if(basketX > 0.95f) basketX = 0.95f;
}

// Window reshape
void reshape(int w, int h){
    WIN_W = w; WIN_H = h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

int main(int argc, char** argv)
{
    srand((unsigned)time(0));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Catch the Eggs");

    loadHighscore();

    glClearColor(1,1,1,1);

    PlaySound(TEXT("background.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

    glutDisplayFunc(display);
    glutIdleFunc([](){            // lambda to call updateSim
        static int lastTime = glutGet(GLUT_ELAPSED_TIME);
        int current = glutGet(GLUT_ELAPSED_TIME);
        float dt = (current - lastTime) / 1000.0f;
        lastTime = current;
        updateSim(dt);
        glutPostRedisplay();
    });

    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(passiveMouse);
    glutSpecialFunc(specialKey);
    
    glutMotionFunc([](int x, int y){   // mouse movement
        basketX = screenToWorldX(x);
    });

    glutMainLoop();
    return 0;
}
