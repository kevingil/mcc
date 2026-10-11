#include "hud.h"
#include "gui.h"
#include "player.h"

#include "rlgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TICK_SECONDS 0.05f
#define CHAT_HISTORY 100
#define CHAT_VISIBLE 10
#define CHAT_LIFETIME 200
#define CHAT_WIDTH 320
#define MAX_AIR 300

typedef struct {
    char text[160];
    Color color;
    int addedTick;
} ChatLine;

static ChatLine chatLines[CHAT_HISTORY];
static int chatCount = 0;
static int hudTicks = 0;
static float tickClock = 0.0f;
static BlockType lastHeld = BLOCK_AIR;
static BlockType lastSwingItem = BLOCK_AIR;
static int highlightTimer = 0;
static char actionText[160] = "";
static int actionTimer = 0;
static bool eyesInWater = false;

static BlockType HeldItem(const Player *player)
{
    int slot = player->hotbarSlot;

    if ((slot < 0) || (slot >= HOTBAR_SIZE) || (player->hotbarCount[slot] <= 0)) return BLOCK_AIR;
    return player->hotbar[slot];
}

static bool EyesInWater(const Player *player, VoxelWorld *world)
{
    Vector3 eye = player->camera.position;
    BlockPos pos = WorldToBlock(eye);
    BlockType block = GetBlock(world, pos);

    if (!IsWaterBlock(block)) return false;
    return eye.y < (float)pos.y + WaterVisualHeight(block);
}

void HudReset(void)
{
    chatCount = 0;
    hudTicks = 0;
    tickClock = 0.0f;
    lastHeld = BLOCK_AIR;
    lastSwingItem = BLOCK_AIR;
    highlightTimer = 0;
    actionText[0] = '\0';
    actionTimer = 0;
    eyesInWater = false;
}

static void HudTick(Player *player, VoxelWorld *world)
{
    BlockType held = HeldItem(player);

    hudTicks++;

    if (held == BLOCK_AIR) highlightTimer = 0;
    else if (held != lastHeld) highlightTimer = 40;
    else if (highlightTimer > 0) highlightTimer--;
    lastHeld = held;

    if (held != lastSwingItem)
    {
        player->attackTicks = 0;
        lastSwingItem = held;
    }
    if (player->attackTicks < 10000) player->attackTicks++;

    if (actionTimer > 0) actionTimer--;

    eyesInWater = EyesInWater(player, world);
    if (eyesInWater)
    {
        if (player->gameMode == GAME_MODE_SURVIVAL)
        {
            player->airSupply--;
            if (player->airSupply <= -20) player->airSupply = 0;
        }
    }
    else if (player->airSupply < MAX_AIR)
    {
        player->airSupply += 4;
        if (player->airSupply > MAX_AIR) player->airSupply = MAX_AIR;
    }
}

void HudUpdate(Player *player, VoxelWorld *world)
{
    tickClock += GetFrameTime();
    if (tickClock > 1.0f) tickClock = 1.0f;
    while (tickClock >= TICK_SECONDS)
    {
        tickClock -= TICK_SECONDS;
        HudTick(player, world);
    }
}

void HudChat(const char *text, Color color)
{
    if ((text == NULL) || (text[0] == '\0')) return;
    if (chatCount < CHAT_HISTORY) chatCount++;
    memmove(&chatLines[1], &chatLines[0], sizeof(ChatLine)*(size_t)(chatCount - 1));
    snprintf(chatLines[0].text, sizeof(chatLines[0].text), "%s", text);
    chatLines[0].color = color;
    chatLines[0].addedTick = hudTicks;
}

void HudClearChat(void)
{
    chatCount = 0;
}

void HudActionBar(const char *text)
{
    snprintf(actionText, sizeof(actionText), "%s", (text != NULL) ? text : "");
    actionTimer = 60;
}

//----------------------------------------------------------------------------------
// Drawing
//----------------------------------------------------------------------------------
static void DrawHotbar(const Player *player, int sw, int sh)
{
    int cx = sw/2;
    bool offhand = (player->offhand != BLOCK_AIR) && (player->offhandCount > 0);

    GuiDrawSprite(GUI_SPRITE_HOTBAR, cx - 91, sh - 22);
    GuiDrawSprite(GUI_SPRITE_HOTBAR_SELECTION, cx - 92 + player->hotbarSlot*20, sh - 23);
    if (offhand) GuiDrawSprite(GUI_SPRITE_HOTBAR_OFFHAND, cx - 120, sh - 23);

    for (int i = 0; i < HOTBAR_SIZE; i++)
    {
        GuiDrawItemStack(player->hotbar[i], player->hotbarCount[i], cx - 88 + i*20, sh - 19);
    }
    if (offhand) GuiDrawItemStack(player->offhand, player->offhandCount, cx - 117, sh - 19);
}

// The crosshair and the attack indicator under it invert what is behind them.
static void DrawCrosshair(const Player *player, int sw, int sh)
{
    float strength = (float)player->attackTicks/5.0f;

    rlDrawRenderBatchActive();
    rlSetBlendFactors(RL_ONE_MINUS_DST_COLOR, RL_ONE_MINUS_SRC_COLOR, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    GuiDrawSprite(GUI_SPRITE_CROSSHAIR, (sw - 15)/2, (sh - 15)/2);
    if (strength < 1.0f)
    {
        int x = sw/2 - 8;
        int y = sh/2 - 7 + 16;

        GuiDrawSprite(GUI_SPRITE_ATTACK_BACKGROUND, x, y);
        GuiDrawSpriteRegion(GUI_SPRITE_ATTACK_PROGRESS, 0, 0, (int)(strength*17.0f), 4, x, y);
    }
    EndBlendMode();
}

// F3 swaps the crosshair for the world axes: X red, Y green, Z blue, each on
// a black line twice as wide. Lengths are 10 GUI pixels.
static void DrawAxisGizmo(const Player *player, int sw, int sh)
{
    static const Vector3 axes[3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    static const Color colors[3] = { { 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 127, 127, 255, 255 } };
    int scale = GuiScale();
    Vector2 center = { (float)((sw/2)*scale), (float)((sh/2)*scale) };
    Vector3 forward = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, (Vector3){ 0.0f, 1.0f, 0.0f }));
    Vector3 up = Vector3CrossProduct(right, forward);
    Vector2 ends[3] = { 0 };
    float length = 10.0f*(float)scale;

    for (int i = 0; i < 3; i++)
    {
        ends[i].x = center.x + Vector3DotProduct(axes[i], right)*length;
        ends[i].y = center.y - Vector3DotProduct(axes[i], up)*length;
    }
    for (int i = 0; i < 3; i++) DrawLineEx(center, ends[i], 4.0f, BLACK);
    for (int i = 0; i < 3; i++) DrawLineEx(center, ends[i], 2.0f, colors[i]);
}

static void DrawStatusBars(const Player *player, int sw, int sh)
{
    int left = sw/2 - 91;
    int right = sw/2 + 91;
    int y = sh - 39;

    for (int k = 9; k >= 0; k--)
    {
        GuiDrawSprite(GUI_SPRITE_HEART_CONTAINER, left + k*8, y);
        GuiDrawSprite(GUI_SPRITE_HEART_FULL, left + k*8, y);
    }
    for (int k = 0; k < 10; k++)
    {
        GuiDrawSprite(GUI_SPRITE_FOOD_EMPTY, right - k*8 - 9, y);
        GuiDrawSprite(GUI_SPRITE_FOOD_FULL, right - k*8 - 9, y);
    }

    {
        int air = (player->airSupply < MAX_AIR) ? player->airSupply : MAX_AIR;

        if (eyesInWater || (air < MAX_AIR))
        {
            int full = (int)ceil((double)(air - 2)*10.0/(double)MAX_AIR);
            int bursting = (int)ceil((double)air*10.0/(double)MAX_AIR) - full;

            for (int k = 0; k < full + bursting; k++)
            {
                GuiDrawSprite((k < full) ? GUI_SPRITE_AIR : GUI_SPRITE_AIR_BURSTING, right - k*8 - 9, sh - 49);
            }
        }
    }
}

static void DrawHeldName(const Player *player, int sw, int sh)
{
    const char *name = NULL;
    int alpha = highlightTimer*256/10;
    int y = sh - 59;

    if ((highlightTimer <= 0) || (lastHeld == BLOCK_AIR)) return;
    if (alpha > 255) alpha = 255;
    if (player->gameMode == GAME_MODE_CREATIVE) y += 14;
    name = GetBlockName(lastHeld);
    GuiDrawText(name, (sw - GuiTextWidth(name))/2, y, (Color){ 255, 255, 255, (unsigned char)alpha }, true);
}

void HudDraw(const Player *player, bool debug)
{
    int sw = GuiWidth();
    int sh = GuiHeight();

    GuiBegin();
    DrawHotbar(player, sw, sh);
    if (!debug) DrawCrosshair(player, sw, sh);
    if (player->gameMode == GAME_MODE_SURVIVAL)
    {
        DrawStatusBars(player, sw, sh);
        GuiDrawSprite(GUI_SPRITE_XP_BACKGROUND, sw/2 - 91, sh - 29);
    }
    DrawHeldName(player, sw, sh);
    GuiEnd();

    if (debug) DrawAxisGizmo(player, sw, sh);
}

static double ChatFade(int age)
{
    double fade = (1.0 - (double)age/(double)CHAT_LIFETIME)*10.0;

    if (fade < 0.0) fade = 0.0;
    if (fade > 1.0) fade = 1.0;
    return fade*fade;
}

void HudDrawMessages(void)
{
    int sw = GuiWidth();
    int sh = GuiHeight();
    float partial = tickClock/TICK_SECONDS;

    GuiBegin();
    if ((actionTimer > 0) && (actionText[0] != '\0'))
    {
        int alpha = (int)(((float)actionTimer - partial)*255.0f/20.0f);

        if (alpha > 255) alpha = 255;
        if (alpha > 8)
        {
            int width = GuiTextWidth(actionText);

            GuiDrawText(actionText, sw/2 + (-width/2), sh - 68 - 4, (Color){ 255, 255, 255, (unsigned char)alpha }, true);
        }
    }

    for (int i = 0; (i < chatCount) && (i < CHAT_VISIBLE); i++)
    {
        const ChatLine *line = &chatLines[i];
        int age = hudTicks - line->addedTick;
        double fade = 0.0;
        int textAlpha = 0;
        int y = sh - 40 - i*9;
        Color color = line->color;

        if (age >= CHAT_LIFETIME) continue;
        fade = ChatFade(age);
        textAlpha = (int)(255.0*fade);
        if (textAlpha <= 3) continue;
        GuiFill(0, y - 9, CHAT_WIDTH + 12, y, (Color){ 0, 0, 0, (unsigned char)(int)(127.5*fade) });
        color.a = (unsigned char)textAlpha;
        GuiDrawText(line->text, 4, y - 8, color, true);
    }
    GuiEnd();
}
