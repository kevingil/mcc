/**********************************************************************************************
*
*   Logo Screen Functions Definitions (Init, Update, Draw, Unload)
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

static int logoPositionX = 0;
static int logoPositionY = 0;

static int lettersCount = 0;

static int topSideRecWidth = 0;
static int leftSideRecHeight = 0;

static int bottomSideRecWidth = 0;
static int rightSideRecHeight = 0;

static int state = 0;              // Logo animation states
static float alpha = 1.0f;         // Useful for fading

//----------------------------------------------------------------------------------
// Logo Screen Functions Definition
//----------------------------------------------------------------------------------

// Logo Screen Initialization logic
void InitLogoScreen(void)
{
    finishScreen = 0;
    framesCounter = 0;
    lettersCount = 0;

    logoPositionX = GetScreenWidth()/2 - 128;
    logoPositionY = GetScreenHeight()/2 - 128;

    topSideRecWidth = 16;
    leftSideRecHeight = 16;
    bottomSideRecWidth = 16;
    rightSideRecHeight = 16;

    state = 0;
    alpha = 1.0f;
}

// Logo Screen Update logic
void UpdateLogoScreen(void)
{
    if (state == 0)                 // State 0: Top-left square corner blink logic
    {
        framesCounter++;

        if (framesCounter == 80)
        {
            state = 1;
            framesCounter = 0;      // Reset counter... will be used later...
        }
    }
    else if (state == 1)            // State 1: Bars animation logic: top and left
    {
        topSideRecWidth += 8;
        leftSideRecHeight += 8;

        if (topSideRecWidth == 256) state = 2;
    }
    else if (state == 2)            // State 2: Bars animation logic: bottom and right
    {
        bottomSideRecWidth += 8;
        rightSideRecHeight += 8;

        if (bottomSideRecWidth == 256) state = 3;
    }
    else if (state == 3)            // State 3: "mcc" text-write animation logic
    {
        framesCounter++;

        if (lettersCount < 3)
        {
            if (framesCounter/12)   // Every 12 frames, one more letter!
            {
                lettersCount++;
                framesCounter = 0;
            }
        }
        else    // When all letters have appeared, just fade out everything
        {
            if (framesCounter > 200)
            {
                alpha -= 0.02f;

                if (alpha <= 0.0f)
                {
                    alpha = 0.0f;
                    finishScreen = 1;   // Jump to next screen
                }
            }
        }
    }
}

// Logo Screen Draw logic
void DrawLogoScreen(void)
{
    // Draw black background
    ClearBackground(BLACK);
    
    if (state == 0)         // Draw blinking top-left square corner
    {
        if ((framesCounter/10)%2) DrawRectangle(logoPositionX, logoPositionY, 16, 16, GREEN);
    }
    else if (state == 1)    // Draw bars animation: top and left
    {
        DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, GREEN);        // Top = grass
        DrawRectangle(logoPositionX, logoPositionY, 16, leftSideRecHeight, BROWN);     // Left side = dirt
    }
    else if (state == 2)    // Draw bars animation: bottom and right
    {
        DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, GREEN);       // Top = grass
        DrawRectangle(logoPositionX, logoPositionY, 16, leftSideRecHeight, BROWN);    // Left side = dirt

        DrawRectangle(logoPositionX + 240, logoPositionY, 16, rightSideRecHeight, BROWN);  // Right side = dirt
        DrawRectangle(logoPositionX, logoPositionY + 240, bottomSideRecWidth, 16, BROWN);  // Bottom = dirt
    }
    else if (state == 3)    // Draw "mcc" text-write animation + "voxel world"
    {
        DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, Fade(GREEN, alpha));           // Top = grass
        DrawRectangle(logoPositionX, logoPositionY + 16, 16, leftSideRecHeight - 32, Fade(BROWN, alpha)); // Left side = dirt

        DrawRectangle(logoPositionX + 240, logoPositionY + 16, 16, rightSideRecHeight - 32, Fade(BROWN, alpha)); // Right side = dirt
        DrawRectangle(logoPositionX, logoPositionY + 240, bottomSideRecWidth, 16, Fade(BROWN, alpha));           // Bottom = dirt

        DrawRectangle(GetScreenWidth()/2 - 112, GetScreenHeight()/2 - 112, 224, 224, Fade(BLACK, alpha));

        DrawText(TextSubtext("mcc", 0, lettersCount), GetScreenWidth()/2 - 32, GetScreenHeight()/2 + 48, 50, Fade(GREEN, alpha));

        if (framesCounter > 20) DrawText("reeeee", logoPositionX, logoPositionY - 27, 20, Fade(BROWN, alpha));
    }
}

// Logo Screen Unload logic
void UnloadLogoScreen(void)
{
    // Unload LOGO screen variables here!
}

// Logo Screen should finish?
int FinishLogoScreen(void)
{
    return finishScreen;
}
