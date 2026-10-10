/*******************************************************************************************
*
*   MC.C
*   An Open World Voxel Game
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
********************************************************************************************/

#include "raylib.h"
#include "screens.h"    // NOTE: Declares global (extern) variables and screens functions
#include "menu_ui.h"
#include "client_log.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
// NOTE: Those variables are shared between modules through screens.h
//----------------------------------------------------------------------------------
GameScreen currentScreen = LOGO;
Font font = { 0 };
Music music = { 0 };
Sound fxCoin = { 0 };

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static int screenWidth = 1280;
static int screenHeight = 720;

// Required variables to manage screen transitions (fade-in, fade-out)
static float transAlpha = 0.0f;
static bool onTransition = false;
static bool transFadeOut = false;
static int transFromScreen = -1;
static GameScreen transToScreen = UNKNOWN;
static bool shouldExitGame = false;

void RequestGameExit(void)
{
    shouldExitGame = true;
}

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void ChangeToScreen(int screen);     // Change to screen, no transition effect

static void TransitionToScreen(int screen); // Request transition to next screen
static void UpdateTransition(void);         // Update transition effect
static void DrawTransition(void);           // Draw transition effect (full-screen rectangle)

static void UpdateDrawFrame(void);          // Update and draw one frame

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    ClientLogInit("logs");
    ClientLog("OpenCraft started");

    if (getenv("MCC_WIDTH") != NULL) screenWidth = atoi(getenv("MCC_WIDTH"));
    if (getenv("MCC_HEIGHT") != NULL) screenHeight = atoi(getenv("MCC_HEIGHT"));
    if (screenWidth < 640) screenWidth = 640;
    if (screenHeight < 480) screenHeight = 480;

    InitWindow(screenWidth, screenHeight, "OpenCraft");
    if (getenv("MCC_WINDOW_X") != NULL)
    {
        int windowY = 40;
        if (getenv("MCC_WINDOW_Y") != NULL) windowY = atoi(getenv("MCC_WINDOW_Y"));
        SetWindowPosition(atoi(getenv("MCC_WINDOW_X")), windowY);
    }

    InitAudioDevice();      // Initialize audio device

    // Load global data (assets that must be available in all screens, i.e. font)
    font = LoadFont("resources/mecha.png");
    //music = LoadMusicStream("resources/ambient.ogg"); // TODO: Load music
    fxCoin = LoadSound("resources/coin.wav");
    InitMenuUi();

    SetMusicVolume(music, 1.0f);
    PlayMusicStream(music);

    // Setup and init first screen. MCC_SKIP_LOGO is for the multiplayer demo.
    if (getenv("MCC_SKIP_LOGO") != NULL)
    {
        currentScreen = TITLE;
        InitTitleScreen();
    }
    else
    {
        currentScreen = LOGO;
        InitLogoScreen();
    }

    // Disable ESC key for closing window (we handle it in pause menu)
    SetExitKey(KEY_NULL);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose() && !shouldExitGame)    // Detect window close button only (ESC disabled) or ending screen exit
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload current screen data before closing
    switch (currentScreen)
    {
        case LOGO: UnloadLogoScreen(); break;
        case TITLE: UnloadTitleScreen(); break;
        case OPTIONS: UnloadOptionsScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case ENDING: UnloadEndingScreen(); break;
        case SELECT_WORLD: UnloadSelectWorldScreen(); break;
        case MULTIPLAYER: UnloadMultiplayerScreen(); break;
        default: break;
    }

    // Unload global data loaded
    UnloadMenuUi();
    UnloadFont(font);
    UnloadMusicStream(music);
    UnloadSound(fxCoin);

    CloseAudioDevice();     // Close audio context

    CloseWindow();          // Close window and OpenGL context
    ClientLogClose();
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------
// Change to next screen, no transition
static void ChangeToScreen(int screen)
{
    // Unload current screen
    switch (currentScreen)
    {
        case LOGO: UnloadLogoScreen(); break;
        case TITLE: UnloadTitleScreen(); break;
        case OPTIONS: UnloadOptionsScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case ENDING: UnloadEndingScreen(); break;
        case SELECT_WORLD: UnloadSelectWorldScreen(); break;
        case MULTIPLAYER: UnloadMultiplayerScreen(); break;
        default: break;
    }

    // Init next screen
    switch (screen)
    {
        case LOGO: InitLogoScreen(); break;
        case TITLE: InitTitleScreen(); break;
        case OPTIONS: InitOptionsScreen(); break;
        case GAMEPLAY: InitGameplayScreen(); break;
        case ENDING: InitEndingScreen(); break;
        case SELECT_WORLD: InitSelectWorldScreen(); break;
        case MULTIPLAYER: InitMultiplayerScreen(); break;
        default: break;
    }

    currentScreen = screen;
}

// Request transition to next screen
static void TransitionToScreen(int screen)
{
    onTransition = true;
    transFadeOut = false;
    transFromScreen = currentScreen;
    transToScreen = screen;
    transAlpha = 0.0f;
}

// Update transition effect (fade-in, fade-out)
static void UpdateTransition(void)
{
    if (!transFadeOut)
    {
        transAlpha += 0.05f;

        // NOTE: Due to float internal representation, condition jumps on 1.0f instead of 1.05f
        // For that reason we compare against 1.01f, to avoid last frame loading stop
        if (transAlpha > 1.01f)
        {
            transAlpha = 1.0f;

            // Unload current screen
            switch (transFromScreen)
            {
                case LOGO: UnloadLogoScreen(); break;
                case TITLE: UnloadTitleScreen(); break;
                case OPTIONS: UnloadOptionsScreen(); break;
                case GAMEPLAY: UnloadGameplayScreen(); break;
                case ENDING: UnloadEndingScreen(); break;
                case SELECT_WORLD: UnloadSelectWorldScreen(); break;
                case MULTIPLAYER: UnloadMultiplayerScreen(); break;
                default: break;
            }

            // Load next screen
            switch (transToScreen)
            {
                case LOGO: InitLogoScreen(); break;
                case TITLE: InitTitleScreen(); break;
                case OPTIONS: InitOptionsScreen(); break;
                case GAMEPLAY: InitGameplayScreen(); break;
                case ENDING: InitEndingScreen(); break;
                case SELECT_WORLD: InitSelectWorldScreen(); break;
                case MULTIPLAYER: InitMultiplayerScreen(); break;
                default: break;
            }

            currentScreen = transToScreen;

            // Activate fade out effect to next loaded screen
            transFadeOut = true;
        }
    }
    else  // Transition fade out logic
    {
        transAlpha -= 0.02f;

        if (transAlpha < -0.01f)
        {
            transAlpha = 0.0f;
            transFadeOut = false;
            onTransition = false;
            transFromScreen = -1;
            transToScreen = UNKNOWN;
        }
    }
}

// Draw transition effect (full-screen rectangle)
static void DrawTransition(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transAlpha));
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    //UpdateMusicStream(music);       // NOTE: Music keeps playing between screens

    if (!onTransition)
    {
        switch(currentScreen)
        {
            case LOGO:
            {
                UpdateLogoScreen();

                if (FinishLogoScreen()) TransitionToScreen(TITLE);

            } break;
            case TITLE:
            {
                UpdateTitleScreen();

                if (FinishTitleScreen() == 1) TransitionToScreen(OPTIONS);
                else if (FinishTitleScreen() == 3) TransitionToScreen(SELECT_WORLD);
                else if (FinishTitleScreen() == 4) TransitionToScreen(MULTIPLAYER);
                else if (FinishTitleScreen() == 5) RequestGameExit();

            } break;
            case OPTIONS:
            {
                UpdateOptionsScreen();

                if (FinishOptionsScreen()) TransitionToScreen(TITLE);

            } break;
            case SELECT_WORLD:
            {
                UpdateSelectWorldScreen();

                if (FinishSelectWorldScreen() == 1) TransitionToScreen(TITLE);
                else if (FinishSelectWorldScreen() == 2) TransitionToScreen(GAMEPLAY);

            } break;
            case MULTIPLAYER:
            {
                UpdateMultiplayerScreen();

                if (FinishMultiplayerScreen() == 1) TransitionToScreen(TITLE);
                else if (FinishMultiplayerScreen() == 2) TransitionToScreen(GAMEPLAY);

            } break;
            case GAMEPLAY:
            {
                UpdateGameplayScreen();

                if (FinishGameplayScreen() == 1) TransitionToScreen(TITLE);

            } break;
            case ENDING:
            {
                UpdateEndingScreen();

                if (FinishEndingScreen() == 1) TransitionToScreen(TITLE);
                else if (FinishEndingScreen() == 2) {
                    // Signal to exit the game after ending animation
                    shouldExitGame = true;
                }

            } break;
            default: break;
        }
    }
    else UpdateTransition();    // Update transition (fade-in, fade-out)
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        switch(currentScreen)
        {
            case LOGO: DrawLogoScreen(); break;
            case TITLE: DrawTitleScreen(); break;
            case OPTIONS: DrawOptionsScreen(); break;
            case GAMEPLAY: DrawGameplayScreen(); break;
            case ENDING: DrawEndingScreen(); break;
            case SELECT_WORLD: DrawSelectWorldScreen(); break;
            case MULTIPLAYER: DrawMultiplayerScreen(); break;
            default: break;
        }

        // Draw full screen rectangle in front of everything
        if (onTransition) DrawTransition();

        //DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
