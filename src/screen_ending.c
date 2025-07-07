/**********************************************************************************************
*
*   Ending Screen Functions Definitions (Init, Update, Draw, Unload)
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

static int lettersCount = 3;  // Start with full text

static int topSideRecWidth = 256;    // Start with full bars
static int leftSideRecHeight = 256;
static int bottomSideRecWidth = 256;
static int rightSideRecHeight = 256;

static int state = 0;              // Logo animation states (reverse)
static float alpha = 1.0f;         // Start fully visible

//----------------------------------------------------------------------------------
// Ending Screen Functions Definition
//----------------------------------------------------------------------------------

// Ending Screen Initialization logic
void InitEndingScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    
    // Start with full logo visible
    lettersCount = 3;
    
    logoPositionX = GetScreenWidth()/2 - 128;
    logoPositionY = GetScreenHeight()/2 - 128;

    topSideRecWidth = 256;
    leftSideRecHeight = 256;
    bottomSideRecWidth = 256;
    rightSideRecHeight = 256;

    state = 0;
    alpha = 1.0f;
}

// Ending Screen Update logic
void UpdateEndingScreen(void)
{
    if (state == 0)                 // State 0: Show full logo, then start fade
    {
        framesCounter++;

        if (framesCounter > 5)    // Wait a bit before starting reverse animation
        {
            state = 1;
            framesCounter = 0;
        }
    }
    else if (state == 1)            // State 1: Fade out text and start shrinking
    {
        framesCounter++;
        
        // Fade out text first
        if (framesCounter > 60) {
            alpha -= 0.02f;
            
            if (alpha <= 0.0f) {
                alpha = 0.0f;
                state = 2;
                framesCounter = 0;
            }
        }
    }
    else if (state == 2)            // State 2: Shrink bottom and right bars (reverse of logo state 2)
    {
        bottomSideRecWidth -= 8;
        rightSideRecHeight -= 8;

        if (bottomSideRecWidth <= 16) state = 3;
    }
    else if (state == 3)            // State 3: Shrink top and left bars (reverse of logo state 1)
    {
        topSideRecWidth -= 8;
        leftSideRecHeight -= 8;

        if (topSideRecWidth <= 16) {
            state = 4;
            framesCounter = 0;
        }
    }
    else if (state == 4)            // State 4: Blinking square then exit (reverse of logo state 0)
    {
        framesCounter++;

        if (framesCounter >= 80)    // After blinking animation
        {
            // Exit the game instead of going to next screen
            finishScreen = 2;  // Special value to indicate game exit
        }
    }
}

// Ending Screen Draw logic
void DrawEndingScreen(void)
{
    // Draw black background
    ClearBackground(BLACK);
    
    if (state == 0)         // Show full logo
    {
        // Draw full logo
        DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, GREEN);           // Top = grass
        DrawRectangle(logoPositionX, logoPositionY + 16, 16, leftSideRecHeight - 32, BROWN); // Left side = dirt

        DrawRectangle(logoPositionX + 240, logoPositionY + 16, 16, rightSideRecHeight - 32, BROWN); // Right side = dirt
        DrawRectangle(logoPositionX, logoPositionY + 240, bottomSideRecWidth, 16, BROWN);           // Bottom = dirt

        DrawRectangle(GetScreenWidth()/2 - 112, GetScreenHeight()/2 - 112, 224, 224, BLACK);

        DrawText("mcc", GetScreenWidth()/2 - 32, GetScreenHeight()/2 + 48, 50, GREEN);
        DrawText("Thanks for playing!", logoPositionX, logoPositionY - 27, 20, BROWN);
    }
    else if (state == 1)    // Fade out text
    {
        // Draw full logo with fading text
        DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, GREEN);           // Top = grass
        DrawRectangle(logoPositionX, logoPositionY + 16, 16, leftSideRecHeight - 32, BROWN); // Left side = dirt

        DrawRectangle(logoPositionX + 240, logoPositionY + 16, 16, rightSideRecHeight - 32, BROWN); // Right side = dirt
        DrawRectangle(logoPositionX, logoPositionY + 240, bottomSideRecWidth, 16, BROWN);           // Bottom = dirt

        DrawRectangle(GetScreenWidth()/2 - 112, GetScreenHeight()/2 - 112, 224, 224, BLACK);

        DrawText("mcc", GetScreenWidth()/2 - 32, GetScreenHeight()/2 + 48, 50, Fade(GREEN, alpha));
        DrawText("Thanks for playing!", logoPositionX, logoPositionY - 27, 20, Fade(BROWN, alpha));
    }
    else if (state == 2)    // Shrink bottom and right bars
    {
        DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, GREEN);       // Top = grass
        DrawRectangle(logoPositionX, logoPositionY, 16, leftSideRecHeight, BROWN);    // Left side = dirt

        DrawRectangle(logoPositionX + 240, logoPositionY, 16, rightSideRecHeight, BROWN);  // Right side = dirt
        DrawRectangle(logoPositionX, logoPositionY + 240, bottomSideRecWidth, 16, BROWN);  // Bottom = dirt
    }
    else if (state == 3)    // Shrink top and left bars
    {
        DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, GREEN);        // Top = grass
        DrawRectangle(logoPositionX, logoPositionY, 16, leftSideRecHeight, BROWN);     // Left side = dirt
    }
    else if (state == 4)    // Blinking square
    {
        if ((framesCounter/10)%2) DrawRectangle(logoPositionX, logoPositionY, 16, 16, GREEN);
    }
}

// Ending Screen Unload logic
void UnloadEndingScreen(void)
{
    // Unload ENDING screen variables here!
}

// Ending Screen should finish?
int FinishEndingScreen(void)
{
    return finishScreen;
}
