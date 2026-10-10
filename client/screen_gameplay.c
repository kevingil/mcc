/**********************************************************************************************
*
*   Voxel Game - Open World Voxel Game
*
*   Open world voxel game with first-person movement, block interaction, and infinite terrain
*
*   Features:
*   - First-person camera with WASD movement and mouse look
*   - Voxel world with chunk-based loading and generation
*   - Block placement and destruction with left/right mouse clicks
*   - Infinite terrain generation using noise
*   - Basic block types: grass, dirt, stone, wood, leaves, water
*   - Hotbar inventory system
*   - Collision detection and physics
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "voxel_types.h"
#include "voxel_world.h"
#include "voxel_renderer.h"
#include "world_generation.h"
#include "player.h"
#include "world_catalog.h"
#include "world_save.h"
#include "net_session.h"
#include "menu_ui.h"
#include "biomes.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

// Pause menu state
static bool gamePaused = false;
static int pauseMenuSelection = 0;
static int pauseMenuItemCount = 6;
static const char *pauseNotice = NULL;

// Voxel game systems
static VoxelWorld world;
static Player player;
static bool gameInitialized = false;
static double nextWorldSave = 0.0;
static bool savedWhilePaused = false;
static int aimLock = 0;
static int aimX = 0;
static int aimY = 0;
static int aimZ = 0;
static int lookLock = 0;
static float lockYaw = 0.0f;
static float lockPitch = -0.45f;

static int ColumnStandY(VoxelWorld *ground, int x, int z, int *standY)
{
    int y = 0;

    for (y = WORLD_HEIGHT - 3; y >= 1; y--)
    {
        BlockPos below = { x, y - 1, z };
        BlockPos feet = { x, y, z };
        BlockPos head = { x, y + 1, z };

        if (!IsBlockSolid(GetBlock(ground, below))) continue;
        if (IsBlockSolid(GetBlock(ground, feet))) continue;
        if (IsBlockSolid(GetBlock(ground, head))) continue;
        *standY = y;
        return 1;
    }
    return 0;
}

static void FaceBlock(Player *body, int bx, int by, int bz)
{
    float dx = (bx + 0.5f) - body->position.x;
    float dz = (bz + 0.5f) - body->position.z;
    float dy = (by + 0.45f) - (body->position.y + 1.62f);
    float horiz = sqrtf((dx*dx) + (dz*dz));
    float yaw = atan2f(dx, dz);
    float pitch = atan2f(dy, (horiz < 0.05f) ? 0.05f : horiz);

    SetPlayerLook(body, yaw, pitch);
}

static void StandOnColumn(Player *body, VoxelWorld *ground, int x, int z, float yaw, float pitch)
{
    int standY = WATER_LEVEL + 1;
    int radius = 0;

    for (radius = 0; radius <= 4; radius++)
    {
        int dx = 0;
        int dz = 0;
        int found = 0;
        int bestX = x;
        int bestZ = z;
        int bestY = standY;
        float bestScore = 1000000.0f;

        for (dx = -radius; dx <= radius; dx++)
        {
            for (dz = -radius; dz <= radius; dz++)
            {
                int y = 0;
                float score = 0.0f;

                if ((radius > 0) && (abs(dx) != radius) && (abs(dz) != radius)) continue;
                if (!ColumnStandY(ground, x + dx, z + dz, &y)) continue;
                if (y <= WATER_LEVEL) continue;
                score = fabsf((float)dx) + fabsf((float)dz);
                if (score < bestScore)
                {
                    found = 1;
                    bestScore = score;
                    bestX = x + dx;
                    bestZ = z + dz;
                    bestY = y;
                }
            }
        }
        if (found)
        {
            body->position.x = bestX + 0.5f;
            body->position.y = (float)bestY;
            body->position.z = bestZ + 0.5f;
            body->velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
            body->onGround = true;
            SetPlayerLook(body, yaw, pitch);
            return;
        }
    }
    body->position.y = (float)standY;
    body->velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    body->onGround = true;
    SetPlayerLook(body, yaw, pitch);
}

static void StandFacingBlock(Player *body, VoxelWorld *ground, int bx, int by, int bz, float preferX, float preferZ)
{
    int radius = 0;
    int found = 0;
    int bestX = (int)floorf(preferX);
    int bestZ = (int)floorf(preferZ);
    int bestY = WATER_LEVEL + 2;
    float bestScore = 1000000.0f;

    for (radius = 0; radius <= 5; radius++)
    {
        int dx = 0;
        int dz = 0;

        for (dx = -radius; dx <= radius; dx++)
        {
            for (dz = -radius; dz <= radius; dz++)
            {
                int x = bx + dx;
                int z = bz + dz;
                int y = 0;
                float dist = 0.0f;
                float prefer = 0.0f;
                float score = 0.0f;

                if ((radius > 0) && (abs(dx) != radius) && (abs(dz) != radius)) continue;
                if (!ColumnStandY(ground, x, z, &y)) continue;
                if (y <= WATER_LEVEL) continue;
                dist = sqrtf(((x + 0.5f) - (bx + 0.5f))*((x + 0.5f) - (bx + 0.5f)) + ((z + 0.5f) - (bz + 0.5f))*((z + 0.5f) - (bz + 0.5f)));
                if ((dist < 1.6f) || (dist > 4.8f)) continue;
                prefer = hypotf((x + 0.5f) - preferX, (z + 0.5f) - preferZ);
                score = fabsf(dist - 2.6f) + (0.2f*prefer);
                if (!found || (score < bestScore))
                {
                    found = 1;
                    bestScore = score;
                    bestX = x;
                    bestZ = z;
                    bestY = y;
                }
            }
        }
    }
    if (!found)
    {
        StandOnColumn(body, ground, bestX, bestZ, 0.0f, -0.4f);
    }
    else
    {
        body->position.x = bestX + 0.5f;
        body->position.y = (float)bestY;
        body->position.z = bestZ + 0.5f;
        body->velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        body->onGround = true;
    }
    FaceBlock(body, bx, by, bz);
    aimX = bx;
    aimY = by;
    aimZ = bz;
    aimLock = 180;
}

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void LayoutPause(MenuButton *buttons);
static void DrawPauseMenu(void);

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    
    // Reset pause state
    gamePaused = false;
    pauseMenuSelection = 0;
    
    if (!gameInitialized) {
        Vector3 startPosition = { 0.0f, 0.0f, 0.0f };
        int online = NetSessionIsInWorld();

        if (BiomeTourActive())
        {
            WorldSaveBind("");
            SetWorldGenerationSeed(1);
            online = 0;
        }
        else if (online)
        {
            WorldSaveBind("");
            SetWorldGenerationSeed(NetSessionSeed());
        }
        else
        {
            WorldSaveBind(GetActiveWorldFolder());
            SetWorldGenerationSeed(WorldSaveSeed());
        }
        InitVoxelWorld(&world);

        Vector3 shore = { 0.0f, 0.0f, 0.0f };
        float shoreYaw = 0.0f;
        float shorePitch = -0.45f;
        bool onShore = false;
        int lookX = 0;
        int lookY = 0;
        int lookZ = 0;
        int lookBlock = 0;
        int haveLook = 0;

        aimLock = 0;
        startPosition = (Vector3){ 0.0f, GetSurfaceLevel(0, 0), 0.0f };
        if (!online && !WorldSaveHasPlayer()) onShore = FindShoreSpawn(&shore, &shoreYaw);
        if (online && !NetSessionHasPose()) onShore = FindShoreSpawn(&shore, &shoreYaw);
        if (online && NetSessionHasPose())
        {
            float px = 0.0f;
            float py = 0.0f;
            float pz = 0.0f;

            NetSessionWelcomePose(&px, &py, &pz, &shoreYaw, &shorePitch);
            shore.x = px;
            shore.y = py;
            shore.z = pz;
            onShore = true;
        }
        if (onShore) startPosition = shore;
        if (online && !NetSessionHasPose())
        {
            float side = cosf(shoreYaw);
            float forward = -sinf(shoreYaw);
            int slot = NetSessionSpawnSlot();

            startPosition.x += slot*2.2f*side;
            startPosition.z += slot*2.2f*forward;
        }
        if (BiomeTourActive())
        {
            int columnX = BIOME_STRIDE/2;
            int columnZ = BIOME_STRIDE/2;

            startPosition = (Vector3){ columnX + 0.5f, GetSurfaceLevel(columnX, columnZ), columnZ + 0.5f };
            onShore = false;
        }
        else if (!online && WorldSaveHasPlayer()) startPosition = WorldSavePlayerPosition();
        else if (!online) WorldSaveSetSpawn((int)startPosition.x, (int)startPosition.y, (int)startPosition.z);

        InitPlayer(&player, startPosition);
        if (online) NetSessionApplySpawnInventory(&player);
        else WorldSaveApplyPlayer(&player);
        if (onShore) SetPlayerLook(&player, shoreYaw, shorePitch);

        // Load initial chunks near spawn BEFORE player physics start
        // otherwise player will fall through the world forever
        LoadChunksAroundPlayer(&world, player.position);
        if (online) NetSessionOverlayEdits(&world);
        if (online)
        {
            haveLook = NetSessionLookBlock(&lookX, &lookY, &lookZ, &lookBlock);
            if (haveLook) StandFacingBlock(&player, &world, lookX, lookY, lookZ, player.position.x, player.position.z);
            else StandOnColumn(&player, &world, (int)floorf(player.position.x), (int)floorf(player.position.z), player.yaw, player.pitch);
            LoadChunksAroundPlayer(&world, player.position);
            NetSessionOverlayEdits(&world);
            if (haveLook) StandFacingBlock(&player, &world, lookX, lookY, lookZ, player.position.x, player.position.z);
            else
            {
                lockYaw = player.yaw;
                lockPitch = player.pitch;
                lookLock = 300;
            }
            NetSessionSyncPose(&player);
        }
        nextWorldSave = GetTime() + 20.0;
        savedWhilePaused = false;
        
        // Initialize renderer
        InitVoxelRenderer();
        
        gameInitialized = true;
    }
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    framesCounter++;
    
    // Handle ESC key for pause menu (only when inventory is not open)
    if (IsKeyPressed(KEY_ESCAPE))
    {
        // If inventory is open, close it first
        if (player.inventoryOpen) {
            InventoryClose(&player);
        } else {
            // Otherwise toggle pause menu
            gamePaused = !gamePaused;
            pauseMenuSelection = 0; // Reset selection when opening menu
            
            if (gamePaused) {
                EnableCursor(); // Show cursor in pause menu
            } else {
                DisableCursor(); // Hide cursor when resuming game
            }
            PlaySound(fxCoin);
        }
    }
    
    if (gamePaused)
    {
        if (!savedWhilePaused)
        {
            if (NetSessionIsInWorld()) NetSessionSyncInventory(&player);
            else WorldSaveFlush(&player, &world);
            savedWhilePaused = true;
        }

        {
            MenuButton buttons[6] = { 0 };
            int activated = -1;
            int i = 0;

            LayoutPause(buttons);
            for (i = 0; i < pauseMenuItemCount; i++)
            {
                UpdateMenuButton(&buttons[i]);
                if (buttons[i].clicked) activated = i;
            }
            if (IsKeyPressed(KEY_UP) && (pauseMenuSelection > 0))
            {
                pauseMenuSelection--;
                PlaySound(fxCoin);
            }
            if (IsKeyPressed(KEY_DOWN) && (pauseMenuSelection < pauseMenuItemCount - 1))
            {
                pauseMenuSelection++;
                PlaySound(fxCoin);
            }
            if (IsKeyPressed(KEY_ENTER)) activated = pauseMenuSelection;
            if (activated >= 0)
            {
                PlaySound(fxCoin);
                pauseNotice = NULL;
                if (activated == 0)
                {
                    gamePaused = false;
                    DisableCursor();
                }
                else if (activated == 5) finishScreen = 1;
                else if (activated == 3) pauseNotice = "Options are on the title screen.";
                else pauseNotice = "Coming soon.";
            }
        }
    }
    else
    {
        savedWhilePaused = false;
        if (GetTime() >= nextWorldSave)
        {
            if (NetSessionIsInWorld()) NetSessionSyncInventory(&player);
            else WorldSaveFlush(&player, &world);
            nextWorldSave = GetTime() + 20.0;
        }

        // Normal gameplay updates when not paused
        // Update world (chunk loading/unloading)
        UpdateVoxelWorld(&world, player.position);
        
        // Update player (handles input, physics, interaction)
        UpdatePlayer(&player, &world);

        if (NetSessionIsInWorld())
        {
            NetSessionPoll(&world);
            NetSessionOverlayEdits(&world);
            {
                int lx = 0;
                int ly = 0;
                int lz = 0;
                int lblock = 0;

                if (NetSessionLookBlock(&lx, &ly, &lz, &lblock))
                {
                    if ((aimX != lx) || (aimY != ly) || (aimZ != lz) || (aimLock == 0 && lookLock))
                    {
                        aimX = lx;
                        aimY = ly;
                        aimZ = lz;
                        aimLock = 180;
                        lookLock = 0;
                    }
                    if (aimLock > 0)
                    {
                        FaceBlock(&player, aimX, aimY, aimZ);
                        aimLock--;
                    }
                }
                else if (lookLock)
                {
                    SetPlayerLook(&player, lockYaw, lockPitch);
                }
            }
            NetSessionSyncInventory(&player);
            NetSessionSyncPose(&player);
        }
        
        // Exit to menu (alternative method - keeping ENTER as backup)
        if (IsKeyPressed(KEY_ENTER) && IsCursorOnScreen() && !player.inventoryOpen)
        {
            finishScreen = 1;
            PlaySound(fxCoin);
        }
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // Clear background with sky color
    ClearBackground((Color){135, 206, 235, 255}); // Sky blue
    
    // 3D rendering
    BeginMode3D(player.camera);
    {
        // Render the voxel world
        RenderVoxelWorld(&world, player.camera);
        
        // Draw block outline for targeted block
        if (player.hasTarget && !gamePaused) {
            DrawBlockOutline(player.targetBlock);
        }
    }
    EndMode3D();
    
    // 2D UI rendering
    if (!gamePaused) {
        // Draw inventory UI if inventory is open, otherwise draw normal UI
        if (player.inventoryOpen) {
            DrawInventory(&player);
        } else {
            DrawPlayerUI(&player);
        }
    }
    
    // Debug information stays off the inventory and the pause menu.
    if (!gamePaused && !player.inventoryOpen) {
        DrawFPS(10, 10);
        
        // Position info
        DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)", 
                 player.position.x, player.position.y, player.position.z), 
                 10, 30, 20, WHITE);
        
        // Chunk info
        ChunkPos playerChunk = WorldToChunk(player.position);
        DrawText(TextFormat("Chunk: (%d, %d) | Loaded Chunks: %d", 
                 playerChunk.x, playerChunk.z, world.chunkCount), 
                 10, 50, 20, WHITE);
        
        // Debug: Check if current chunk is loaded
        Chunk* currentChunk = GetChunk(&world, playerChunk);
        Color chunkStatusColor = currentChunk ? GREEN : RED;
        DrawText(TextFormat("Current Chunk: %s", currentChunk ? "LOADED" : "NOT LOADED"), 
                 10, 70, 20, chunkStatusColor);
        
        // Debug: Check ground block
        BlockPos groundPos = {(int)player.position.x, (int)(player.position.y - 1), (int)player.position.z};
        BlockType groundBlock = GetBlock(&world, groundPos);
        DrawText(TextFormat("Ground Block: %d (%s)", groundBlock, 
                 groundBlock == BLOCK_AIR ? "AIR" : "SOLID"), 
                 10, 90, 20, groundBlock == BLOCK_AIR ? RED : GREEN);

        // Always render block debug info, handle null/air targetBlock
        BlockType targetBlock = GetBlock(&world, player.targetBlock);
        const char* blockName = GetBlockName(targetBlock);
        const char* textureName = GetBlockTextureName(targetBlock, FACE_TOP);

        if (player.hasTarget && targetBlock != BLOCK_AIR) {
            DrawText(TextFormat("Target Block: %s", blockName), 10, 110, 20, YELLOW);
            DrawText(TextFormat("Texture: %s.png", textureName), 10, 130, 20, LIGHTGRAY);
            DrawText(TextFormat("Block Pos: (%d, %d, %d)", 
                     player.targetBlock.x, player.targetBlock.y, player.targetBlock.z), 
                     10, 150, 20, GRAY);
        } else {
            DrawText("Target Block: (none)", 10, 110, 20, DARKGRAY);
            DrawText("Texture: (none)", 10, 130, 20, DARKGRAY);
            DrawText("Block Pos: (-, -, -)", 10, 150, 20, DARKGRAY);
        }
    }
    
    // Controls help (when cursor is visible and game not paused)
    if (!IsCursorHidden() && !gamePaused && !player.inventoryOpen) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        
        DrawText("VOXEL WORLD GAME", screenWidth/2 - 150, 100, 30, WHITE);
        DrawText("CONTROLS:", 50, 150, 20, YELLOW);
        DrawText("WASD - Move", 50, 180, 18, WHITE);
        DrawText("Mouse - Look around", 50, 200, 18, WHITE);
        DrawText("SPACE - Jump", 50, 220, 18, WHITE);
        DrawText("LEFT SHIFT - Run", 50, 240, 18, WHITE);
        DrawText("LEFT CLICK - Break block", 50, 260, 18, WHITE);
        DrawText("RIGHT CLICK - Place block", 50, 280, 18, WHITE);
        DrawText("BUCKET - Pick up and place water", 50, 300, 18, WHITE);
        DrawText("1-9 - Select block type", 50, 320, 18, WHITE);
        DrawText("E - Open inventory", 50, 340, 18, WHITE);
        DrawText("ESC - Open pause menu", 50, 360, 18, WHITE);
        DrawText("ENTER - Return to menu", 50, 380, 18, WHITE);
        
        DrawText("Click to start playing!", screenWidth/2 - 120, screenHeight - 50, 20, YELLOW);
    }
    
    if (BiomeTourActive())
    {
        const Biome *biome = BiomeAt((int)player.position.x, (int)player.position.z);
        int scale = MenuScale();
        const char *title = TextFormat("%s    %d / %d", biome->name, BiomeTourIndex() + 1, BiomeCount());
        int fontSize = 12*scale;
        int y = GetScreenHeight() - 22*scale - fontSize - 10*scale;

        DrawRectangle(0, y - 4*scale, GetScreenWidth(), fontSize + 8*scale, Fade(BLACK, 0.55f));
        DrawMenuTextCentered(GetScreenWidth()/2, y, fontSize, title, WHITE);
    }

    // Draw pause menu
    if (gamePaused) {
        DrawPauseMenu();
    }

    if (NetSessionIsInWorld())
    {
        const char *held = GetBlockName(player.selectedBlock);
        const char *edit = NetSessionEditLine();

        DrawText(TextFormat("Online  %s", NetSessionName()), 12, 190, 22, YELLOW);
        DrawText(TextFormat("Held  %s   slot %d", held, player.hotbarSlot + 1), 12, 214, 22, WHITE);
        DrawText("F places the held block", 12, 238, 20, (Color){ 220, 220, 220, 255 });
        if ((edit != NULL) && (edit[0] != '\0'))
        {
            DrawText(edit, 12, 172, 22, (Color){ 80, 255, 120, 255 });
        }
    }

    if (GetActiveWorldName()[0] != '\0')
    {
        const char *worldName = GetActiveWorldName();
        int nameWidth = MeasureText(worldName, 20);
        char seedLabel[32] = { 0 };
        int seedWidth = 0;

        DrawText(worldName, GetScreenWidth()/2 - nameWidth/2 + 1, 9, 20, BLACK);
        DrawText(worldName, GetScreenWidth()/2 - nameWidth/2, 8, 20, WHITE);
        snprintf(seedLabel, sizeof(seedLabel), "Seed %u", WorldSaveSeed());
        seedWidth = MeasureText(seedLabel, 16);
        DrawText(seedLabel, GetScreenWidth()/2 - seedWidth/2 + 1, 31, 16, BLACK);
        DrawText(seedLabel, GetScreenWidth()/2 - seedWidth/2, 30, 16, WHITE);
    }
}

static void LayoutPause(MenuButton *buttons)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fullW = 200*scale;
    int halfW = 98*scale;
    int buttonH = 20*scale;
    int gap = 4*scale;
    int y = GetScreenHeight()/2 - (buttonH*4 + gap*3)/2;

    buttons[0].bounds = (Rectangle){ (float)(cx - fullW/2), (float)y, (float)fullW, (float)buttonH };
    buttons[0].label = "Back to Game";
    buttons[0].enabled = true;

    buttons[1].bounds = (Rectangle){ (float)(cx - fullW/2), (float)(y + (buttonH + gap)), (float)halfW, (float)buttonH };
    buttons[1].label = "Advancements";
    buttons[1].enabled = true;

    buttons[2].bounds = (Rectangle){ (float)(cx - fullW/2 + halfW + gap), (float)(y + (buttonH + gap)), (float)halfW, (float)buttonH };
    buttons[2].label = "Statistics";
    buttons[2].enabled = true;

    buttons[3].bounds = (Rectangle){ (float)(cx - fullW/2), (float)(y + 2*(buttonH + gap)), (float)halfW, (float)buttonH };
    buttons[3].label = "Options...";
    buttons[3].enabled = true;

    buttons[4].bounds = (Rectangle){ (float)(cx - fullW/2 + halfW + gap), (float)(y + 2*(buttonH + gap)), (float)halfW, (float)buttonH };
    buttons[4].label = "Open to LAN";
    buttons[4].enabled = true;

    buttons[5].bounds = (Rectangle){ (float)(cx - fullW/2), (float)(y + 3*(buttonH + gap)), (float)fullW, (float)buttonH };
    buttons[5].label = "Save and Quit to Title";
    buttons[5].enabled = true;
}

static void DrawPauseMenu(void)
{
    MenuButton buttons[6] = { 0 };
    int scale = MenuScale();
    int i = 0;

    LayoutPause(buttons);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));
    DrawMenuTextCentered(GetScreenWidth()/2, (int)buttons[0].bounds.y - 18*scale, 8*scale, "Game Menu", WHITE);
    for (i = 0; i < pauseMenuItemCount; i++)
    {
        buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
        DrawStoneButton(&buttons[i], i == pauseMenuSelection);
    }
    if (pauseNotice != NULL)
    {
        DrawMenuTextCentered(GetScreenWidth()/2, (int)buttons[5].bounds.y + 28*scale, 8*scale, pauseNotice, YELLOW);
    }
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    EnableCursor();

    if (gameInitialized) {
        if (NetSessionIsInWorld()) NetSessionLeave(&player);
        else WorldSaveFlush(&player, &world);
        UnloadVoxelWorld(&world);
        UnloadVoxelRenderer();
        gameInitialized = false;
    }
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}
