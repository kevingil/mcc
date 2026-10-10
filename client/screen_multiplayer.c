/**********************************************************************************************
*
*   OpenCraft Multiplayer / Realms
*   Realms stays a coming-soon screen. Multiplayer creates an account or logs in,
*   then joins the dedicated server. Accounts are not created on a website.
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "menu_ui.h"
#include "net_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Module Variables
//----------------------------------------------------------------------------------
static int finishScreen = 0;
static int frames = 0;
static int focus = 2;
static char host[64] = "127.0.0.1";
static char portText[8] = "25570";
static char username[OC_NAME_MAX + 1] = "";
static char password[OC_PASS_MAX + 1] = "";
static char email[OC_EMAIL_MAX + 1] = "";
static char status[160] = "";
static int statusError = 0;

//----------------------------------------------------------------------------------
// Local Functions
//----------------------------------------------------------------------------------
static void LayoutRealms(MenuButton *buttons)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int buttonH = 20*scale;
    int y1 = GetScreenHeight() - 52*scale;

    buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y1, (float)(150*scale), (float)buttonH };
    buttons[0].label = "Join Realm";
    buttons[0].enabled = false;

    buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y1, (float)(150*scale), (float)buttonH };
    buttons[1].label = "Cancel";
    buttons[1].enabled = true;

    buttons[2].enabled = false;
    buttons[2].label = NULL;
    buttons[2].bounds = (Rectangle){ 0, 0, 0, 0 };
    buttons[3] = buttons[2];
}

static void LayoutOnline(MenuButton *buttons)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int buttonW = 150*scale;
    int buttonH = 20*scale;
    int y1 = GetScreenHeight() - 52*scale;
    int y2 = GetScreenHeight() - 28*scale;

    buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y1, (float)buttonW, (float)buttonH };
    buttons[0].label = "Create Account";
    buttons[0].enabled = true;

    buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y1, (float)buttonW, (float)buttonH };
    buttons[1].label = "Log In";
    buttons[1].enabled = true;

    buttons[2].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y2, (float)buttonW, (float)buttonH };
    buttons[2].label = "Join World";
    buttons[2].enabled = NetSessionIsOnline() && !NetSessionIsInWorld();

    buttons[3].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y2, (float)buttonW, (float)buttonH };
    buttons[3].label = "Cancel";
    buttons[3].enabled = true;
}

static Rectangle FieldRect(int index)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fieldW = 308*scale;
    int fieldH = 18*scale;
    int y = (36 + index*28)*scale;

    return (Rectangle){ (float)(cx - fieldW/2), (float)y, (float)fieldW, (float)fieldH };
}

static void TypeInto(char *buf, int cap, int digitsOnly)
{
    int len = (int)strlen(buf);
    int key = GetCharPressed();

    while (key > 0)
    {
        int ok = (key >= 32) && (key < 127);
        if (digitsOnly) ok = (key >= '0') && (key <= '9');
        if (ok && (len < cap - 1))
        {
            buf[len] = (char)key;
            len++;
            buf[len] = '\0';
        }
        key = GetCharPressed();
    }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && (len > 0))
    {
        buf[len - 1] = '\0';
    }
}

static void EditFocus(void)
{
    if (focus == 0) TypeInto(host, (int)sizeof(host), 0);
    else if (focus == 1) TypeInto(portText, (int)sizeof(portText), 1);
    else if (focus == 2) TypeInto(username, (int)sizeof(username), 0);
    else if (focus == 3) TypeInto(password, (int)sizeof(password), 0);
    else TypeInto(email, (int)sizeof(email), 0);
}

static int PortValue(void)
{
    int port = atoi(portText);

    if ((port <= 0) || (port > 65535)) return 0;
    return port;
}

static void ShowSessionStatus(int error)
{
    snprintf(status, sizeof(status), "%s", NetSessionStatus());
    statusError = error;
}

static void CreateAccount(void)
{
    int port = PortValue();

    if (host[0] == '\0') host[0] = '\0';
    if (port == 0)
    {
        snprintf(status, sizeof(status), "Port must be 1-65535");
        statusError = 1;
        return;
    }
    if (NetSessionCreateAccount(host, port, username, password, email))
    {
        ShowSessionStatus(0);
    }
    else ShowSessionStatus(1);
}

static void Login(void)
{
    int port = PortValue();

    if (port == 0)
    {
        snprintf(status, sizeof(status), "Port must be 1-65535");
        statusError = 1;
        return;
    }
    if (NetSessionLogin(host, port, username, password)) ShowSessionStatus(0);
    else ShowSessionStatus(1);
}

static void JoinWorld(void)
{
    if (!NetSessionIsOnline())
    {
        snprintf(status, sizeof(status), "Log in first");
        statusError = 1;
        return;
    }
    if (NetSessionJoinWorld())
    {
        ShowSessionStatus(0);
        finishScreen = 2;
    }
    else ShowSessionStatus(1);
}

static void DrawField(int index, const char *label, const char *text)
{
    Rectangle box = FieldRect(index);
    int scale = MenuScale();
    int fontSize = 8*scale;

    DrawMenuText((int)box.x, (int)box.y - fontSize - scale, fontSize, label, (Color){ 224, 224, 224, 255 });
    DrawMenuTextField(box, text, focus == index, frames);
}

//----------------------------------------------------------------------------------
// Multiplayer Screen
//----------------------------------------------------------------------------------
void InitMultiplayerScreen(void)
{
    finishScreen = 0;
    frames = 0;
    focus = 2;
    status[0] = '\0';
    statusError = 0;
    EnableCursor();
}

void UpdateMultiplayerScreen(void)
{
    MenuButton buttons[4] = { 0 };
    int realms = GetUnavailableMenu() == UNAVAILABLE_REALMS;
    int i = 0;

    frames++;
    if (realms)
    {
        LayoutRealms(buttons);
        for (i = 0; i < 4; i++)
        {
            if (buttons[i].label == NULL) continue;
            UpdateMenuButton(&buttons[i]);
            if (buttons[i].clicked)
            {
                PlaySound(fxCoin);
                finishScreen = 1;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
        {
            PlaySound(fxCoin);
            finishScreen = 1;
        }
        return;
    }

    LayoutOnline(buttons);
    EditFocus();
    if (IsKeyPressed(KEY_TAB))
    {
        focus = (focus + (IsKeyDown(KEY_LEFT_SHIFT) ? 4 : 1)) % 5;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 mouse = GetMousePosition();

        for (i = 0; i < 5; i++)
        {
            if (CheckCollisionPointRec(mouse, FieldRect(i))) focus = i;
        }
    }
    for (i = 0; i < 4; i++)
    {
        UpdateMenuButton(&buttons[i]);
        if (!buttons[i].clicked) continue;
        PlaySound(fxCoin);
        if (i == 0) CreateAccount();
        else if (i == 1) Login();
        else if (i == 2) JoinWorld();
        else
        {
            if (!NetSessionIsInWorld()) NetSessionDisconnect();
            finishScreen = 1;
        }
    }
    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (!NetSessionIsInWorld()) NetSessionDisconnect();
        PlaySound(fxCoin);
        finishScreen = 1;
    }
}

void DrawMultiplayerScreen(void)
{
    MenuButton buttons[4] = { 0 };
    int realms = GetUnavailableMenu() == UNAVAILABLE_REALMS;
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fontSize = 8*scale;
    int i = 0;
    char masked[OC_PASS_MAX + 1];
    int passLen = (int)strlen(password);

    if (realms)
    {
        int listTop = 48*scale;
        int listBottom = GetScreenHeight() - 64*scale;
        int mid = (listTop + listBottom)/2;

        LayoutRealms(buttons);
        for (i = 0; i < 4; i++)
        {
            if (buttons[i].label == NULL) continue;
            buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
        }
        DrawDirtBackground();
        DrawMenuTextCentered(cx, 4*scale, fontSize, "Realms", WHITE);
        DrawMenuTextCentered(cx, mid - fontSize, fontSize*2, "Coming soon", WHITE);
        DrawMenuTextCentered(cx, mid + fontSize, fontSize, "Realms are unavailable for now.", (Color){ 180, 180, 180, 255 });
        for (i = 0; i < 4; i++)
        {
            if (buttons[i].label == NULL) continue;
            DrawStoneButton(&buttons[i], false);
        }
        return;
    }

    if (passLen > OC_PASS_MAX) passLen = OC_PASS_MAX;
    for (i = 0; i < passLen; i++) masked[i] = '*';
    masked[passLen] = '\0';

    LayoutOnline(buttons);
    for (i = 0; i < 4; i++) buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
    DrawDirtBackground();
    DrawMenuTextCentered(cx, 4*scale, fontSize, "Play Multiplayer", WHITE);
    DrawField(0, "Server", host);
    DrawField(1, "Port", portText);
    DrawField(2, "Username", username);
    DrawField(3, "Password", masked);
    DrawField(4, "Email (optional)", email);

    if (status[0] != '\0')
    {
        Color color = statusError ? (Color){ 255, 120, 120, 255 } : (Color){ 120, 255, 140, 255 };
        DrawMenuTextCentered(cx, GetScreenHeight() - 72*scale, fontSize, status, color);
    }
    else if (NetSessionIsOnline())
    {
        DrawMenuTextCentered(cx, GetScreenHeight() - 72*scale, fontSize, TextFormat("Logged in as %s", NetSessionName()), (Color){ 120, 255, 140, 255 });
    }

    for (i = 0; i < 4; i++) DrawStoneButton(&buttons[i], false);
}

void UnloadMultiplayerScreen(void)
{
    if (!NetSessionIsInWorld()) NetSessionDisconnect();
}

int FinishMultiplayerScreen(void)
{
    return finishScreen;
}
