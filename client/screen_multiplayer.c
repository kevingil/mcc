/**********************************************************************************************
*
*   OpenCraft Multiplayer
*   Sign in once. After that the screen is a saved server list, not the account form.
*   Accounts are created from this client. There is no signup site.
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "menu_ui.h"
#include "net_session.h"
#include "server_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODE_LOGIN 0
#define MODE_LIST 1
#define MODE_EDIT 2
#define MODE_DIRECT 3

static int finishScreen = 0;
static int frames = 0;
static int mode = MODE_LOGIN;
static int signedIn = 0;
static int focus = 0;
static int selected = 0;
static int scroll = 0;
static int editing = -1;
static int showServer = 0;
static int statusError = 0;
static char sessionUser[OC_NAME_MAX + 1] = "";
static char sessionPass[OC_PASS_MAX + 1] = "";
static char sessionEmail[OC_EMAIL_MAX + 1] = "";
static char username[OC_NAME_MAX + 1] = "";
static char password[OC_PASS_MAX + 1] = "";
static char email[OC_EMAIL_MAX + 1] = "";
static char host[SERVER_HOST_MAX] = "127.0.0.1";
static char portText[8] = "25570";
static char nameField[SERVER_NAME_MAX] = "";
static char status[160] = "";

static void SetStatus(const char *text, int error)
{
    snprintf(status, sizeof(status), "%s", (text != NULL) ? text : "");
    statusError = error;
}

static void RememberSession(void)
{
    snprintf(sessionUser, sizeof(sessionUser), "%s", username);
    snprintf(sessionPass, sizeof(sessionPass), "%s", password);
    snprintf(sessionEmail, sizeof(sessionEmail), "%s", email);
    signedIn = 1;
}

static int PortValue(const char *text)
{
    int port = atoi((text != NULL) ? text : "");
    if ((port <= 0) || (port > 65535)) return 0;
    return port;
}

static ServerEntry *AccountServer(void)
{
    ServerListLoad();
    if (ServerListCount() <= 0) return NULL;
    return ServerListAt(0);
}

static void TypeInto(char *buf, int cap)
{
    int len = (int)strlen(buf);
    int key = GetCharPressed();

    while (key > 0)
    {
        if ((key >= 32) && (key < 127) && (len < cap - 1))
        {
            buf[len] = (char)key;
            len++;
            buf[len] = '\0';
        }
        key = GetCharPressed();
    }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && (len > 0)) buf[len - 1] = '\0';
}

static Rectangle FieldRect(int index, int count)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fieldW = 308*scale;
    int fieldH = 18*scale;
    int y = (48 + index*28)*scale;

    (void)count;
    return (Rectangle){ (float)(cx - fieldW/2), (float)y, (float)fieldW, (float)fieldH };
}

static void DrawField(int index, const char *label, const char *text)
{
    Rectangle box = FieldRect(index, 0);
    int scale = MenuScale();
    int fontSize = 8*scale;

    DrawMenuText((int)box.x, (int)box.y - fontSize - scale, fontSize, label, (Color){ 224, 224, 224, 255 });
    DrawMenuTextField(box, text, focus == index, frames);
}

static void Mask(const char *src, char *dst, int cap)
{
    int n = (int)strlen(src);
    int i = 0;

    if (n >= cap) n = cap - 1;
    for (i = 0; i < n; i++) dst[i] = '*';
    dst[n] = '\0';
}

static int Authenticate(const char *toHost, int port, int create)
{
    int ok = 0;

    if (create) ok = NetSessionCreateAccount(toHost, port, username, password, email);
    else ok = NetSessionLogin(toHost, port, username, password);
    if (!ok)
    {
        SetStatus(NetSessionStatus(), 1);
        return 0;
    }
    RememberSession();
    SetStatus(NetSessionStatus(), 0);
    NetSessionDisconnect();
    mode = MODE_LIST;
    return 1;
}

static int JoinEntry(const ServerEntry *entry)
{
    int ok = 0;

    if (entry == NULL) return 0;
    ok = NetSessionLogin(entry->host, entry->port, sessionUser, sessionPass);
    if (!ok && (NetSessionLastReject() == OC_REJECT_UNKNOWN))
    {
        ok = NetSessionCreateAccount(entry->host, entry->port, sessionUser, sessionPass, sessionEmail);
    }
    if (!ok)
    {
        SetStatus(NetSessionStatus(), 1);
        return 0;
    }
    if (!NetSessionJoinWorld())
    {
        SetStatus(NetSessionStatus(), 1);
        return 0;
    }
    SetStatus(NetSessionStatus(), 0);
    finishScreen = 2;
    return 1;
}

static void LayoutLogin(MenuButton *buttons)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int buttonW = 150*scale;
    int buttonH = 20*scale;
    int y1 = GetScreenHeight() - 76*scale;
    int y2 = GetScreenHeight() - 52*scale;
    int y3 = GetScreenHeight() - 28*scale;

    buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y1, (float)buttonW, (float)buttonH };
    buttons[0].label = "Create Account";
    buttons[0].enabled = true;
    buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y1, (float)buttonW, (float)buttonH };
    buttons[1].label = "Log In";
    buttons[1].enabled = true;
    buttons[2].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y2, (float)(308*scale), (float)buttonH };
    buttons[2].label = showServer ? "Hide Server" : "Different Server";
    buttons[2].enabled = true;
    buttons[3].bounds = (Rectangle){ (float)(cx - 100*scale), (float)y3, (float)(200*scale), (float)buttonH };
    buttons[3].label = "Cancel";
    buttons[3].enabled = true;
}

static void LayoutList(MenuButton *buttons)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int buttonW = 150*scale;
    int buttonH = 20*scale;
    int y1 = GetScreenHeight() - 100*scale;
    int y2 = GetScreenHeight() - 76*scale;
    int y3 = GetScreenHeight() - 52*scale;
    int y4 = GetScreenHeight() - 28*scale;
    int have = (selected >= 0) && (selected < ServerListCount());

    buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y1, (float)buttonW, (float)buttonH };
    buttons[0].label = "Join Server";
    buttons[0].enabled = have;
    buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y1, (float)buttonW, (float)buttonH };
    buttons[1].label = "Direct Connection";
    buttons[1].enabled = true;
    buttons[2].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y2, (float)buttonW, (float)buttonH };
    buttons[2].label = "Add Server";
    buttons[2].enabled = ServerListCount() < SERVER_LIST_MAX;
    buttons[3].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y2, (float)buttonW, (float)buttonH };
    buttons[3].label = "Edit";
    buttons[3].enabled = have;
    buttons[4].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y3, (float)buttonW, (float)buttonH };
    buttons[4].label = "Remove";
    buttons[4].enabled = have && (ServerListCount() > 1);
    buttons[5].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y3, (float)buttonW, (float)buttonH };
    buttons[5].label = "Refresh";
    buttons[5].enabled = true;
    buttons[6].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y4, (float)buttonW, (float)buttonH };
    buttons[6].label = "Log Out";
    buttons[6].enabled = true;
    buttons[7].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y4, (float)buttonW, (float)buttonH };
    buttons[7].label = "Cancel";
    buttons[7].enabled = true;
}

static void LayoutEdit(MenuButton *buttons, int direct)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int buttonW = 150*scale;
    int buttonH = 20*scale;
    int y = GetScreenHeight() - 40*scale;

    buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y, (float)buttonW, (float)buttonH };
    buttons[0].label = direct ? "Join Server" : "Done";
    buttons[0].enabled = true;
    buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y, (float)buttonW, (float)buttonH };
    buttons[1].label = "Cancel";
    buttons[1].enabled = true;
}

static void BeginEdit(int index)
{
    ServerEntry *entry = ServerListAt(index);

    editing = index;
    focus = 0;
    mode = MODE_EDIT;
    if (entry == NULL)
    {
        nameField[0] = '\0';
        snprintf(host, sizeof(host), "127.0.0.1");
        snprintf(portText, sizeof(portText), "25570");
        return;
    }
    snprintf(nameField, sizeof(nameField), "%s", entry->name);
    snprintf(host, sizeof(host), "%s", entry->host);
    snprintf(portText, sizeof(portText), "%d", entry->port);
}

static void BeginDirect(void)
{
    mode = MODE_DIRECT;
    focus = 0;
    snprintf(host, sizeof(host), "127.0.0.1");
    snprintf(portText, sizeof(portText), "25570");
}

static void FinishEdit(void)
{
    int port = PortValue(portText);

    if (port == 0)
    {
        SetStatus("Port must be 1-65535", 1);
        return;
    }
    if (host[0] == '\0')
    {
        SetStatus("Server address is empty", 1);
        return;
    }
    if (editing < 0)
    {
        if (nameField[0] == '\0') snprintf(nameField, sizeof(nameField), "%s", host);
        if (!ServerListAdd(nameField, host, port)) SetStatus("Could not add that server", 1);
        else
        {
            selected = ServerListCount() - 1;
            SetStatus("Server saved", 0);
        }
    }
    else
    {
        if (nameField[0] == '\0') snprintf(nameField, sizeof(nameField), "%s", host);
        ServerListUpdate(editing, nameField, host, port);
        SetStatus("Server saved", 0);
    }
    mode = MODE_LIST;
}

static void LoginTarget(char *outHost, int outCap, int *outPort)
{
    ServerEntry *entry = AccountServer();

    if (showServer)
    {
        snprintf(outHost, (size_t)outCap, "%s", host);
        *outPort = PortValue(portText);
        return;
    }
    if (entry != NULL)
    {
        snprintf(outHost, (size_t)outCap, "%s", entry->host);
        *outPort = entry->port;
        return;
    }
    snprintf(outHost, (size_t)outCap, "127.0.0.1");
    *outPort = 25570;
}

static void DrawStatusLine(int y)
{
    int scale = MenuScale();
    Color color = statusError ? (Color){ 255, 120, 120, 255 } : (Color){ 120, 255, 140, 255 };

    if (status[0] == '\0') return;
    DrawMenuTextCentered(GetScreenWidth()/2, y, 8*scale, status, color);
}

static void DrawList(void)
{
    int scale = MenuScale();
    int count = ServerListCount();
    int top = 36*scale;
    int bottom = GetScreenHeight() - 108*scale;
    int rowH = 28*scale;
    int visible = 0;
    int i = 0;
    Rectangle list = { (float)(GetScreenWidth()/2 - 154*scale), (float)top, (float)(308*scale), (float)(bottom - top) };

    if (rowH < 1) rowH = 1;
    visible = (int)list.height/rowH;
    if (visible < 1) visible = 1;
    if (selected < 0) selected = 0;
    if (selected >= count) selected = count - 1;
    if (selected < scroll) scroll = selected;
    if (selected >= scroll + visible) scroll = selected - visible + 1;
    if (scroll < 0) scroll = 0;

    DrawRectangleRec(list, (Color){ 0, 0, 0, 90 });
    DrawRectangleLinesEx(list, (float)scale, (Color){ 0, 0, 0, 180 });
    for (i = scroll; (i < count) && (i < scroll + visible); i++)
    {
        ServerEntry *entry = ServerListAt(i);
        Rectangle row = { list.x + 2, list.y + (float)((i - scroll)*rowH), list.width - 4, (float)rowH - 2 };
        Color nameColor = (i == selected) ? YELLOW : WHITE;
        char line[96];

        if (entry == NULL) continue;
        if (CheckCollisionPointRec(GetMousePosition(), row) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (selected == i) JoinEntry(entry);
            else selected = i;
        }
        if (i == selected) DrawRectangleRec(row, (Color){ 255, 255, 255, 40 });
        DrawMenuText((int)row.x + 4*scale, (int)row.y + 2*scale, 8*scale, entry->name, nameColor);
        snprintf(line, sizeof(line), "%s:%d", entry->host, entry->port);
        DrawMenuText((int)row.x + 4*scale, (int)row.y + 12*scale, 8*scale, line, (Color){ 180, 180, 180, 255 });
    }
}

void InitMultiplayerScreen(void)
{
    finishScreen = 0;
    frames = 0;
    focus = 0;
    status[0] = '\0';
    statusError = 0;
    showServer = 0;
    ServerListLoad();
    mode = signedIn ? MODE_LIST : MODE_LOGIN;
    if (!signedIn)
    {
        snprintf(username, sizeof(username), "%s", sessionUser);
        password[0] = '\0';
        email[0] = '\0';
    }
    EnableCursor();
}

void UpdateMultiplayerScreen(void)
{
    int realms = GetUnavailableMenu() == UNAVAILABLE_REALMS;
    int i = 0;

    frames++;
    if (realms)
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            PlaySound(fxCoin);
            finishScreen = 1;
        }
        return;
    }

    if (mode == MODE_LOGIN)
    {
        MenuButton buttons[4] = { 0 };
        int fields = showServer ? 5 : 3;
        char targetHost[SERVER_HOST_MAX];
        int targetPort = 0;

        LayoutLogin(buttons);
        if (focus == 0) TypeInto(username, (int)sizeof(username));
        else if (focus == 1) TypeInto(password, (int)sizeof(password));
        else if (focus == 2) TypeInto(email, (int)sizeof(email));
        else if (showServer && (focus == 3)) TypeInto(host, (int)sizeof(host));
        else if (showServer && (focus == 4)) TypeInto(portText, (int)sizeof(portText));
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN)) focus = (focus + 1)%fields;
        if (IsKeyPressed(KEY_UP)) focus = (focus + fields - 1)%fields;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mouse = GetMousePosition();
            for (i = 0; i < fields; i++)
            {
                if (CheckCollisionPointRec(mouse, FieldRect(i, fields))) focus = i;
            }
        }
        for (i = 0; i < 4; i++)
        {
            UpdateMenuButton(&buttons[i]);
            if (!buttons[i].clicked) continue;
            PlaySound(fxCoin);
            if (i == 2) showServer = !showServer;
            else if (i == 3)
            {
                NetSessionDisconnect();
                finishScreen = 1;
            }
            else
            {
                LoginTarget(targetHost, (int)sizeof(targetHost), &targetPort);
                if (targetPort == 0) SetStatus("Port must be 1-65535", 1);
                else Authenticate(targetHost, targetPort, i == 0);
            }
        }
        if (IsKeyPressed(KEY_ESCAPE))
        {
            PlaySound(fxCoin);
            finishScreen = 1;
        }
        return;
    }

    if (mode == MODE_LIST)
    {
        MenuButton buttons[8] = { 0 };
        int count = ServerListCount();
        float wheel = GetMouseWheelMove();

        LayoutList(buttons);
        if (wheel > 0.0f) scroll--;
        if (wheel < 0.0f) scroll++;
        if (IsKeyPressed(KEY_UP) && (selected > 0)) selected--;
        if (IsKeyPressed(KEY_DOWN) && (selected + 1 < count)) selected++;
        if (IsKeyPressed(KEY_ENTER) && (count > 0)) JoinEntry(ServerListAt(selected));
        for (i = 0; i < 8; i++)
        {
            UpdateMenuButton(&buttons[i]);
            if (!buttons[i].clicked) continue;
            PlaySound(fxCoin);
            if (i == 0) JoinEntry(ServerListAt(selected));
            else if (i == 1) BeginDirect();
            else if (i == 2) BeginEdit(-1);
            else if (i == 3) BeginEdit(selected);
            else if (i == 4)
            {
                ServerListRemove(selected);
                if (selected >= ServerListCount()) selected = ServerListCount() - 1;
                SetStatus("Server removed", 0);
            }
            else if (i == 5) SetStatus("Server list reloaded", 0);
            else if (i == 6)
            {
                signedIn = 0;
                password[0] = '\0';
                sessionPass[0] = '\0';
                mode = MODE_LOGIN;
                SetStatus("Signed out", 0);
            }
            else finishScreen = 1;
        }
        if (IsKeyPressed(KEY_ESCAPE))
        {
            PlaySound(fxCoin);
            finishScreen = 1;
        }
        return;
    }

    {
        MenuButton buttons[2] = { 0 };
        int direct = mode == MODE_DIRECT;
        int fields = direct ? 2 : 3;

        LayoutEdit(buttons, direct);
        if (!direct && (focus == 0)) TypeInto(nameField, (int)sizeof(nameField));
        else if ((!direct && (focus == 1)) || (direct && (focus == 0))) TypeInto(host, (int)sizeof(host));
        else TypeInto(portText, (int)sizeof(portText));
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN)) focus = (focus + 1)%fields;
        if (IsKeyPressed(KEY_UP)) focus = (focus + fields - 1)%fields;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mouse = GetMousePosition();
            for (i = 0; i < fields; i++)
            {
                if (CheckCollisionPointRec(mouse, FieldRect(i, fields))) focus = i;
            }
        }
        for (i = 0; i < 2; i++)
        {
            UpdateMenuButton(&buttons[i]);
            if (!buttons[i].clicked) continue;
            PlaySound(fxCoin);
            if (i == 1) mode = MODE_LIST;
            else if (direct)
            {
                ServerEntry temp = { 0 };
                int port = PortValue(portText);
                if (port == 0) SetStatus("Port must be 1-65535", 1);
                else
                {
                    snprintf(temp.name, sizeof(temp.name), "%s", host);
                    snprintf(temp.host, sizeof(temp.host), "%s", host);
                    temp.port = port;
                    JoinEntry(&temp);
                }
            }
            else FinishEdit();
        }
        if (IsKeyPressed(KEY_ESCAPE))
        {
            PlaySound(fxCoin);
            mode = MODE_LIST;
        }
    }
}

void DrawMultiplayerScreen(void)
{
    int realms = GetUnavailableMenu() == UNAVAILABLE_REALMS;
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fontSize = 8*scale;
    int i = 0;

    DrawDirtBackground();
    if (realms)
    {
        DrawMenuTextCentered(cx, GetScreenHeight()/2 - fontSize, fontSize*2, "Coming soon", WHITE);
        DrawMenuTextCentered(cx, GetScreenHeight()/2 + fontSize, fontSize, "Realms are unavailable for now.", (Color){ 180, 180, 180, 255 });
        return;
    }

    if (mode == MODE_LOGIN)
    {
        MenuButton buttons[4] = { 0 };
        char masked[OC_PASS_MAX + 1];
        char targetHost[SERVER_HOST_MAX];
        int targetPort = 0;
        ServerEntry *entry = AccountServer();

        Mask(password, masked, (int)sizeof(masked));
        LayoutLogin(buttons);
        for (i = 0; i < 4; i++) buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
        DrawMenuTextCentered(cx, 8*scale, fontSize, "Sign In", WHITE);
        DrawMenuTextCentered(cx, 20*scale, fontSize, "Multiplayer needs an account. You only do this once.", (Color){ 180, 180, 180, 255 });
        DrawField(0, "Username", username);
        DrawField(1, "Password", masked);
        DrawField(2, "Email (optional)", email);
        if (showServer)
        {
            DrawField(3, "Server", host);
            DrawField(4, "Port", portText);
        }
        else if (entry != NULL)
        {
            LoginTarget(targetHost, (int)sizeof(targetHost), &targetPort);
            DrawMenuTextCentered(cx, 132*scale, fontSize, TextFormat("Account server: %s (%s:%d)", entry->name, targetHost, targetPort), (Color){ 180, 180, 180, 255 });
        }
        DrawStatusLine(GetScreenHeight() - 92*scale);
        for (i = 0; i < 4; i++) DrawStoneButton(&buttons[i], false);
        return;
    }

    if (mode == MODE_LIST)
    {
        MenuButton buttons[8] = { 0 };

        LayoutList(buttons);
        for (i = 0; i < 8; i++) buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
        DrawMenuTextCentered(cx, 6*scale, fontSize, "Play Multiplayer", WHITE);
        DrawMenuTextCentered(cx, 18*scale, fontSize, TextFormat("Logged in as %s", sessionUser), (Color){ 120, 255, 140, 255 });
        DrawList();
        DrawStatusLine(GetScreenHeight() - 116*scale);
        for (i = 0; i < 8; i++) DrawStoneButton(&buttons[i], false);
        return;
    }

    {
        MenuButton buttons[2] = { 0 };
        int direct = mode == MODE_DIRECT;

        LayoutEdit(buttons, direct);
        for (i = 0; i < 2; i++) buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
        DrawMenuTextCentered(cx, 8*scale, fontSize, direct ? "Direct Connection" : ((editing < 0) ? "Add Server" : "Edit Server"), WHITE);
        if (!direct) DrawField(0, "Server Name", nameField);
        DrawField(direct ? 0 : 1, "Server Address", host);
        DrawField(direct ? 1 : 2, "Port", portText);
        DrawStatusLine(GetScreenHeight() - 64*scale);
        for (i = 0; i < 2; i++) DrawStoneButton(&buttons[i], false);
    }
}

void UnloadMultiplayerScreen(void)
{
    if (!NetSessionIsInWorld()) NetSessionDisconnect();
}

int FinishMultiplayerScreen(void)
{
    return finishScreen;
}
