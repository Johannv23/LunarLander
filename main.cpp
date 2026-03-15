/**
* Author: Johann Varghese
* Assignment: Lunar Lander
* Date due: [date]
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/cs3113.h"
#include <cmath>
#include <string>

// Constants

constexpr int   SCREEN_WIDTH  = 800;
constexpr int   SCREEN_HEIGHT = 600;
constexpr int   FPS           = 60;

constexpr float FIXED_TIMESTEP        = 1.0f / 60.0f;
constexpr float GRAVITY               = 100.0f;   // small space gravity
constexpr float THRUST_FORCE          = 250.0f;   // upward thrust
constexpr float SIDE_FORCE            = 150.0f;   // left/right acceleration
constexpr float DRAG                  = 0.96f;    // velocity damping per frame
constexpr float MAX_FUEL              = 300.0f;   // total fuel units
constexpr float FUEL_BURN_RATE        = 30.0f;    // fuel per second when thrusting

// Moving platform
constexpr float PLATFORM_SPEED        = 120.0f;
constexpr float PLATFORM_TRAVEL       = 200.0f;   // how far left/right it moves

// Asset paths
constexpr char BACKGROUND_FP[]        = "assets/background.jpg";
constexpr char SHIP_FP[]              = "assets/ship-removebg-preview.png";
constexpr char FLAME_FP[]             = "assets/sheetsprite.png";
constexpr char GROUND_FP[]            = "assets/ground-removebg-preview.png";
constexpr char PLATFORM_FP[]          = "assets/platform-removebg-preview.png";
constexpr char MOVING_PLATFORM_FP[]   = "assets/moving_platform-removebg-preview.png";
constexpr char FUEL_BAR_FP[]          = "assets/fuel-removebg-preview.png";
constexpr char WIN_FP[]               = "assets/win-removebg-preview.png";
constexpr char LOSE_FP[]              = "assets/lose-removebg-preview.png";

// Enums

enum GameResult  { NONE, WIN, LOSE };


class Entity
{
public:
    // --- Transform ---
    Vector2 mPosition;
    Vector2 mVelocity;
    Vector2 mAcceleration;
    Vector2 mScale;
    Vector2 mColliderDimensions;

    // --- Texture ---
    Texture2D mTexture;
    bool      mIsActive;

    // --- Platform type flag ---
    bool mIsSafePlatform;
    bool mIsMovingPlatform;
    bool mIsForbidden;   // touching this = lose

    // --- Collision flags ---
    bool mIsCollidingBottom;
    bool mIsCollidingTop;
    bool mIsCollidingLeft;
    bool mIsCollidingRight;

    Entity() :
        mPosition{0,0}, mVelocity{0,0}, mAcceleration{0,0},
        mScale{50,50}, mColliderDimensions{50,50},
        mIsActive(true),
        mIsSafePlatform(false), mIsMovingPlatform(false), mIsForbidden(false),
        mIsCollidingBottom(false), mIsCollidingTop(false),
        mIsCollidingLeft(false), mIsCollidingRight(false)
    { mTexture = {0}; }

    void setTexture(const char *fp)
    {
        if (mTexture.id != 0) UnloadTexture(mTexture);
        mTexture = LoadTexture(fp);
    }

    void render() const
    {
        if (!mIsActive) return;
        Rectangle src  = {0, 0, (float)mTexture.width, (float)mTexture.height};
        Rectangle dest = {
            mPosition.x - mColliderDimensions.x / 2.0f,
            mPosition.y - mColliderDimensions.y / 2.0f,
            mScale.x,
            mScale.y
        };
        DrawTexturePro(mTexture, src, dest, {0,0}, 0.0f, WHITE);
    }

    void resetCollisionFlags()
    {
        mIsCollidingBottom = false;
        mIsCollidingTop    = false;
        mIsCollidingLeft   = false;
        mIsCollidingRight  = false;
    }

    bool isColliding(const Entity &other) const
    {
        if (!other.mIsActive) return false;
        float xDist = fabs(mPosition.x - other.mPosition.x)
                      - ((mColliderDimensions.x + other.mColliderDimensions.x) / 2.0f);
        float yDist = fabs(mPosition.y - other.mPosition.y)
                      - ((mColliderDimensions.y + other.mColliderDimensions.y) / 2.0f);
        return (xDist < 0.0f && yDist < 0.0f);
    }

    // Resolve vertical overlap with another entity
    void resolveCollisionY(const Entity &other)
    {
        float yDist    = fabs(mPosition.y - other.mPosition.y);
        float yOverlap = fabs(yDist
                              - (mColliderDimensions.y / 2.0f)
                              - (other.mColliderDimensions.y / 2.0f));
        if (mVelocity.y > 0.0f)
        {
            mPosition.y        -= yOverlap;
            mVelocity.y         = 0.0f;
            mIsCollidingBottom  = true;
        }
        else if (mVelocity.y < 0.0f)
        {
            mPosition.y      += yOverlap;
            mVelocity.y       = 0.0f;
            mIsCollidingTop   = true;
        }
    }

    // Resolve horizontal overlap with another entity
    void resolveCollisionX(const Entity &other)
    {
        float yDist    = fabs(mPosition.y - other.mPosition.y);
        float yOverlap = fabs(yDist
                              - (mColliderDimensions.y / 2.0f)
                              - (other.mColliderDimensions.y / 2.0f));

        if (yOverlap < 0.5f) return;

        float xDist    = fabs(mPosition.x - other.mPosition.x);
        float xOverlap = fabs(xDist
                              - (mColliderDimensions.x / 2.0f)
                              - (other.mColliderDimensions.x / 2.0f));
        if (mVelocity.x > 0.0f)
        {
            mPosition.x       -= xOverlap;
            mVelocity.x        = 0.0f;
            mIsCollidingRight  = true;
        }
        else if (mVelocity.x < 0.0f)
        {
            mPosition.x      += xOverlap;
            mVelocity.x       = 0.0f;
            mIsCollidingLeft  = true;
        }
    }
};

// GameState struct

struct GameState
{
    Entity *ship;
    Entity *flame;         // shown when thrusting
    Entity *safePlatform;
    Entity *movingPlatform;
    Entity *ground;        // the rocky terrain
};

// Globals

AppStatus  gAppStatus = RUNNING;
GameResult gGameResult = NONE;
GameState  gState;

bool gLastCollidedSafe   = false;
bool gLastCollidedGround = false;

Texture2D gBackgroundTexture;
Texture2D gFuelBarTexture;
Texture2D gWinTexture;
Texture2D gLoseTexture;

int   gFlameFrame     = 0;
float gFlameTimer     = 0.0f;
constexpr float FLAME_FRAME_SPEED = 0.08f; // seconds per frame
constexpr int   FLAME_FRAMES      = 4;
constexpr float FLAME_FRAME_W     = 1.0f / FLAME_FRAMES; // UV fraction per frame

float gFuel            = MAX_FUEL;
bool  gIsThrustingUp   = false;
bool  gIsThrustingLeft = false;
bool  gIsThrustingRight= false;

float gPreviousTicks   = 0.0f;
float gTimeAccumulator = 0.0f;

// Moving platform state
float gMovingPlatformDir    = 1.0f;   // +1 = right, -1 = left
float gMovingPlatformOriginX = 0.0f;  // set in initialise

// Helper

void drawFuelBar()
{
    float fuelRatio = gFuel / MAX_FUEL;

    // Background (empty bar)
    DrawRectangle(20, 20, 200, 20, DARKGRAY);

    // Filled portion — color shifts red as fuel runs low
    Color barColor = fuelRatio > 0.5f ? GREEN
                   : fuelRatio > 0.25f ? YELLOW
                   : RED;
    DrawRectangle(20, 20, (int)(200 * fuelRatio), 20, barColor);

    // Fuel bar texture overlay (decorative)
    Rectangle src  = {0, 0, (float)gFuelBarTexture.width, (float)gFuelBarTexture.height};
    Rectangle dest = {20, 20, 200, 20};
    DrawTexturePro(gFuelBarTexture, src, dest, {0,0}, 0.0f, WHITE);

    DrawText("FUEL", 228, 20, 18, WHITE);
}

//initialise

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lunar Lander");
    SetTargetFPS(FPS);

    gBackgroundTexture = LoadTexture(BACKGROUND_FP);
    gFuelBarTexture    = LoadTexture(FUEL_BAR_FP);
    gWinTexture        = LoadTexture(WIN_FP);
    gLoseTexture       = LoadTexture(LOSE_FP);

    //SHIP
    gState.ship = new Entity();
    gState.ship->setTexture(SHIP_FP);
    gState.ship->mPosition           = {(float)SCREEN_WIDTH / 2.0f, 80.0f};
    gState.ship->mScale              = {70.0f, 70.0f};
	gState.ship->mColliderDimensions = {30.0f, 35.0f};
    gState.ship->mAcceleration       = {0.0f, GRAVITY};

	gState.flame = new Entity();
	gState.flame->setTexture(FLAME_FP);
	gState.flame->mScale              = {40.0f, 50.0f};
	gState.flame->mColliderDimensions = {40.0f, 50.0f};
	gState.flame->mIsActive           = false;

    gState.safePlatform = new Entity();
    gState.safePlatform->setTexture(PLATFORM_FP);
    gState.safePlatform->mPosition           = {200.0f, (float)SCREEN_HEIGHT - 160.0f};
    gState.safePlatform->mScale              = {180.0f, 40.0f};
	gState.safePlatform->mColliderDimensions   = {120.0f, 8.0f};
    gState.safePlatform->mIsSafePlatform     = true;

    gState.movingPlatform = new Entity();
    gState.movingPlatform->setTexture(MOVING_PLATFORM_FP);
    gState.movingPlatform->mPosition           = {550.0f, (float)SCREEN_HEIGHT - 250.0f};
    gState.movingPlatform->mScale              = {180.0f, 35.0f};
	gState.movingPlatform->mColliderDimensions = {120.0f, 8.0f};
    gState.movingPlatform->mIsMovingPlatform   = true;
    gState.movingPlatform->mIsSafePlatform     = true;  // landing = win
    gMovingPlatformOriginX = 550.0f;

    gState.ground = new Entity();
    gState.ground->setTexture(GROUND_FP);
    gState.ground->mPosition           = {(float)SCREEN_WIDTH / 2.0f,
                                          (float)SCREEN_HEIGHT - 40.0f};
    gState.ground->mScale              = {(float)SCREEN_WIDTH, 80.0f};
	gState.ground->mColliderDimensions = {(float)SCREEN_WIDTH, 20.0f};
	gState.ground->mIsForbidden        = true;
}


void processInput()
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;
    if (gGameResult != NONE) return;  // freeze input when game is over

    gIsThrustingUp    = false;
    gIsThrustingLeft  = false;
    gIsThrustingRight = false;

    // Only allow thrust if there's fuel
    if (gFuel > 0.0f)
    {
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
        {
            gState.ship->mAcceleration.y = GRAVITY - THRUST_FORCE;
            gIsThrustingUp = true;
        }
        else
        {
            gState.ship->mAcceleration.y = GRAVITY;
        }

        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        {
            gState.ship->mAcceleration.x = -SIDE_FORCE;
            gIsThrustingLeft = true;
        }
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        {
            gState.ship->mAcceleration.x = SIDE_FORCE;
            gIsThrustingRight = true;
        }
        else
        {
            gState.ship->mAcceleration.x = 0.0f;  // only reset x, not y
            gState.ship->mVelocity.x    *= DRAG;
        }
    }
    else
    {
        // Out of fuel — only gravity applies
        gState.ship->mAcceleration = {0.0f, GRAVITY};
    }
}

// update  (uses fixed timestep)

void update()
{
    if (gGameResult != NONE) return;

    float ticks     = (float)GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    deltaTime += gTimeAccumulator;

    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP)
    {
        // Burn fuel
		    gLastCollidedSafe   = false;
    		gLastCollidedGround = false;  //




        bool anyThrust = gIsThrustingUp || gIsThrustingLeft || gIsThrustingRight;
        if (anyThrust && gFuel > 0.0f)
            gFuel -= FUEL_BURN_RATE * FIXED_TIMESTEP;
        if (gFuel < 0.0f) gFuel = 0.0f;

        //Move moving platform left/right
        gState.movingPlatform->mPosition.x +=
            gMovingPlatformDir * PLATFORM_SPEED * FIXED_TIMESTEP;

        if (gState.movingPlatform->mPosition.x >
                gMovingPlatformOriginX + PLATFORM_TRAVEL)
            gMovingPlatformDir = -1.0f;

        if (gState.movingPlatform->mPosition.x <
                gMovingPlatformOriginX - PLATFORM_TRAVEL)
            gMovingPlatformDir = 1.0f;

        //Integrate ship velocity
        gState.ship->resetCollisionFlags();

        gState.ship->mVelocity.x += gState.ship->mAcceleration.x * FIXED_TIMESTEP;
        gState.ship->mVelocity.y += gState.ship->mAcceleration.y * FIXED_TIMESTEP;

        // Apply drag to horizontal velocity (drift / deceleration)
        gState.ship->mVelocity.x *= DRAG;

        // Move Y and check vertical collisions
        gState.ship->mPosition.y += gState.ship->mVelocity.y * FIXED_TIMESTEP;

        // Check against safe platform
if (gState.ship->isColliding(*gState.safePlatform))
        {
            gState.ship->resolveCollisionY(*gState.safePlatform);
            gLastCollidedSafe = gState.ship->mIsCollidingBottom;
        }

        if (gState.ship->isColliding(*gState.movingPlatform))
        {
            gState.ship->resolveCollisionY(*gState.movingPlatform);
            gLastCollidedSafe = gState.ship->mIsCollidingBottom;
        }

        if (gState.ship->isColliding(*gState.ground))
        {
            gState.ship->resolveCollisionY(*gState.ground);
            gLastCollidedGround = gState.ship->mIsCollidingBottom;
        }
        // Move X and check horizontal collisions
        gState.ship->mPosition.x += gState.ship->mVelocity.x * FIXED_TIMESTEP;

        if (gState.ship->isColliding(*gState.safePlatform))
            gState.ship->resolveCollisionX(*gState.safePlatform);

        if (gState.ship->isColliding(*gState.movingPlatform))
            gState.ship->resolveCollisionX(*gState.movingPlatform);

        if (gState.ship->isColliding(*gState.ground))
            gState.ship->resolveCollisionX(*gState.ground);

        //Screen boundary (left/right walls = lose)
        if (gState.ship->mPosition.x < 0 ||
            gState.ship->mPosition.x > SCREEN_WIDTH)
        {
            gGameResult = LOSE;
        }

        // Win/Lose detection
        // Landing on safe platform
// Win/Lose detection
		if (gState.ship->mIsCollidingBottom)
			{
    			if (gLastCollidedSafe)
        			gGameResult = WIN;
    			else if (gLastCollidedGround)
        			gGameResult = LOSE;
		}

        // Hitting ceiling = lose
        if (gState.ship->mPosition.y < 0)
            gGameResult = LOSE;

        // Position flame below ship
        gState.flame->mIsActive   = anyThrust && gFuel > 0.0f;
		gState.flame->mPosition = {
            gState.ship->mPosition.x + gState.ship->mScale.x / 3.0f,
            gState.ship->mPosition.y +
                gState.ship->mColliderDimensions.y / 2.0f +
                gState.flame->mScale.y / 2.0f
        };

        deltaTime -= FIXED_TIMESTEP;
    }

    gTimeAccumulator = deltaTime;
}

// render

void render()
{
    BeginDrawing();
    ClearBackground(BLACK);



    // Background
    Rectangle bgSrc  = {0, 0,
        (float)gBackgroundTexture.width,
        (float)gBackgroundTexture.height};
    Rectangle bgDest = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    DrawTexturePro(gBackgroundTexture, bgSrc, bgDest, {0,0}, 0.0f, WHITE);

    if (gGameResult == NONE)
    {
        // Draw game objects
        gState.ground->render();
        gState.safePlatform->render();
        gState.movingPlatform->render();
// Flame gets its OWN custom draw — NOT gState.flame->render()
        DrawText(gState.flame->mIsActive ? "FLAME ON" : "FLAME OFF", 400, 20, 18, WHITE); // debug


        if (gState.flame->mIsActive)
        {
            gFlameTimer += GetFrameTime();
            if (gFlameTimer >= FLAME_FRAME_SPEED)
            {
                gFlameFrame = (gFlameFrame + 1) % FLAME_FRAMES;
                gFlameTimer = 0.0f;
            }

            float frameW = (float)gState.flame->mTexture.width / FLAME_FRAMES;

            Rectangle flameSrc  = {
                gFlameFrame * frameW,
                0,
                frameW,
                (float)gState.flame->mTexture.height
            };
            Rectangle flameDest = {
                gState.flame->mPosition.x - gState.flame->mScale.x / 2.0f,
                gState.flame->mPosition.y - gState.flame->mScale.y / 2.0f,
                gState.flame->mScale.x,
                gState.flame->mScale.y
            };
            DrawTexturePro(gState.flame->mTexture, flameSrc, flameDest,
                           {0,0}, 0.0f, WHITE);
        }
        gState.ship->render();

        // Fuel bar
        drawFuelBar();

        // Out of fuel warning
        if (gFuel <= 0.0f)
            DrawText("NO FUEL!", SCREEN_WIDTH/2 - 60, 60, 28, RED);

        // Controls hint
        DrawText("ARROWS/WASD: Thrust    Land on platform to WIN",
                 10, SCREEN_HEIGHT - 25, 14, LIGHTGRAY);
    }
    else
    {
        // End screen
        Texture2D endTex = (gGameResult == WIN) ? gWinTexture : gLoseTexture;

        float scale       = 0.6f;
        float scaledW     = endTex.width  * scale;
        float scaledH     = endTex.height * scale;

        Rectangle endSrc  = {0, 0, (float)endTex.width, (float)endTex.height};
        Rectangle endDest = {
            (SCREEN_WIDTH  - scaledW) / 2.0f,
            (SCREEN_HEIGHT - scaledH) / 2.0f,
            scaledW,
            scaledH
        };
        DrawTexturePro(endTex, endSrc, endDest, {0,0}, 0.0f, WHITE);
    }

    EndDrawing();
}

// shutdown
void shutdown()
{
    delete gState.ship;
    delete gState.flame;
    delete gState.safePlatform;
    delete gState.movingPlatform;
    delete gState.ground;

    UnloadTexture(gBackgroundTexture);
    UnloadTexture(gFuelBarTexture);
    UnloadTexture(gWinTexture);
    UnloadTexture(gLoseTexture);

    CloseWindow();
}

// main

int main()
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();
    return 0;
}
