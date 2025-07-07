/**********************************************************************************************
*
*   Title Screen Functions Definitions (Init, Update, Draw, Unload)
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Title Screen Functions Definition
//----------------------------------------------------------------------------------

// Title Screen Initialization logic
void InitTitleScreen(void)
{
    // TODO: Initialize TITLE screen variables here!
    framesCounter = 0;
    finishScreen = 0;
}

// Title Screen Update logic
void UpdateTitleScreen(void)
{
    // TODO: Update TITLE screen variables here!

    // Press enter or tap to change to GAMEPLAY screen
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        //finishScreen = 1;   // OPTIONS
        finishScreen = 2;   // GAMEPLAY
        PlaySound(fxCoin);
    }
}

// Title Screen Draw logic
void DrawTitleScreen(void)
{
    // Draw gradient background
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), 
                          (Color){135, 206, 235, 255}, (Color){34, 139, 34, 255}); // Sky to grass
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Main title
    Vector2 titlePos = { screenWidth/2 - 200, 100 };
    DrawTextEx(font, "VOXEL WORLD", titlePos, font.baseSize*4.0f, 4, WHITE);
    
    // Subtitle
    DrawText("An Open World Voxel Game", screenWidth/2 - 160, 180, 24, WHITE);
    
    // Game features
    DrawText("Features:", 100, 250, 20, YELLOW);
    DrawText("• Infinite procedural world generation", 120, 280, 18, WHITE);
    DrawText("• First-person exploration and building", 120, 300, 18, WHITE);
    DrawText("• Multiple block types and physics", 120, 320, 18, WHITE);
    DrawText("• Optimized chunk-based rendering", 120, 340, 18, WHITE);
    
    // Start instruction
    DrawText("PRESS ENTER TO START", screenWidth/2 - 120, screenHeight - 100, 24, YELLOW);
    DrawText("Press ENTER or TAP to begin your adventure!", screenWidth/2 - 180, screenHeight - 60, 18, WHITE);
}

// Title Screen Unload logic
void UnloadTitleScreen(void)
{
    // TODO: Unload TITLE screen variables here!
}

// Title Screen should finish?
int FinishTitleScreen(void)
{
    return finishScreen;
}
