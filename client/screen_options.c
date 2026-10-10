/**********************************************************************************************
*
*   Options Screen
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "menu_ui.h"

//----------------------------------------------------------------------------------
// Module Variables
//----------------------------------------------------------------------------------
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Options Screen
//----------------------------------------------------------------------------------
void InitOptionsScreen(void)
{
    finishScreen = 0;
    EnableCursor();
}

void UpdateOptionsScreen(void)
{
    int scale = MenuScale();
    MenuButton done = { 0 };

    done.bounds = (Rectangle){
        (float)(GetScreenWidth()/2 - 100*scale),
        (float)(GetScreenHeight() - 28*scale),
        (float)(200*scale),
        (float)(20*scale)
    };
    done.label = "Done";
    done.enabled = true;
    UpdateMenuButton(&done);

    if (done.clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
    {
        PlaySound(fxCoin);
        finishScreen = 1;
    }
}

void DrawOptionsScreen(void)
{
    int scale = MenuScale();
    int fontSize = 8*scale;
    MenuButton done = { 0 };

    done.bounds = (Rectangle){
        (float)(GetScreenWidth()/2 - 100*scale),
        (float)(GetScreenHeight() - 28*scale),
        (float)(200*scale),
        (float)(20*scale)
    };
    done.label = "Done";
    done.enabled = true;
    done.hovered = CheckCollisionPointRec(GetMousePosition(), done.bounds);

    DrawDirtBackground();
    DrawMenuTextCentered(GetScreenWidth()/2, 8*scale, fontSize, "Options", WHITE);
    DrawMenuTextCentered(GetScreenWidth()/2, GetScreenHeight()/2 - fontSize, fontSize, "More options are coming soon.", (Color){ 224, 224, 224, 255 });
    DrawStoneButton(&done, false);
}

void UnloadOptionsScreen(void)
{
}

int FinishOptionsScreen(void)
{
    return finishScreen;
}
