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
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
static int debugHud = 0;
static float frameSamples[240];
static int frameCursor = 0;
static int frameFilled = 0;

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
static void NoteFrameTime(void);

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
    NoteFrameTime();
    if (IsKeyPressed(KEY_F3) && !player.searchFocused) debugHud = !debugHud;
    
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

static void NoteFrameTime(void)
{
    frameSamples[frameCursor] = GetFrameTime()*1000.0f;
    frameCursor = (frameCursor + 1)%240;
    if (frameFilled < 240) frameFilled++;
}

static void DebugText(int x, int y, const char *text, int alignRight)
{
    int size = 10;
    int width = MeasureText(text, size);
    int drawX = alignRight ? (GetScreenWidth() - 4 - width) : x;

    DrawText(text, drawX + 1, y + 1, size, BLACK);
    DrawText(text, drawX, y, size, WHITE);
}

static void ReadMachine(char *cpu, int cpuCap, int *cores, long *rssKb, long *virtKb)
{
    FILE *info = NULL;
    FILE *mem = NULL;
    char line[256];
    long pages = 0;
    long resident = 0;
    long page = sysconf(_SC_PAGESIZE);

    if ((cpu[0] == '\0') && ((info = fopen("/proc/cpuinfo", "r")) != NULL))
    {
        *cores = 0;
        while (fgets(line, (int)sizeof(line), info) != NULL)
        {
            if (strncmp(line, "processor", 9) == 0) (*cores)++;
            if ((cpu[0] == '\0') && (strncmp(line, "model name", 10) == 0))
            {
                char *colon = strchr(line, ':');
                if (colon != NULL)
                {
                    colon++;
                    while (*colon == ' ') colon++;
                    snprintf(cpu, (size_t)cpuCap, "%s", colon);
                    cpu[strcspn(cpu, "\n")] = '\0';
                }
            }
        }
        fclose(info);
    }
    mem = fopen("/proc/self/statm", "r");
    if (mem != NULL)
    {
        if (fscanf(mem, "%ld %ld", &pages, &resident) == 2)
        {
            *virtKb = pages*page/1024;
            *rssKb = resident*page/1024;
        }
        fclose(mem);
    }
}

static int SkyLightAt(VoxelWorld *ground, int x, int y, int z)
{
    int above = y + 1;

    for (; above < WORLD_HEIGHT; above++)
    {
        BlockPos pos = { x, above, z };
        BlockType block = GetBlock(ground, pos);
        if ((block != BLOCK_AIR) && !IsWaterBlock(block)) return 0;
    }
    return 15;
}

static void DrawFrameGraph(int x, int y, int width, int height, Color color)
{
    int i = 0;
    float minMs = 1000.0f;
    float maxMs = 0.0f;
    float sum = 0.0f;

    DrawRectangle(x, y - height, width, height, (Color){ 0, 0, 0, 140 });
    if (frameFilled <= 0) return;
    for (i = 0; i < frameFilled; i++)
    {
        float sample = frameSamples[i];
        int bar = 0;
        int sx = 0;

        if (sample < minMs) minMs = sample;
        if (sample > maxMs) maxMs = sample;
        sum += sample;
        bar = (int)(sample*2.0f);
        if (bar < 1) bar = 1;
        if (bar > height) bar = height;
        sx = x + (i*width)/240;
        DrawRectangle(sx, y - bar, 1, bar, color);
    }
    DebugText(x, y - height - 12, TextFormat("%.0f ms min   %.0f ms avg   %.0f ms max", minMs, sum/(float)frameFilled, maxMs), 0);
}

static void DrawDebugHud(void)
{
    static char cpu[160] = "";
    static int cores = 0;
    static long lastRss = 0;
    static double lastMemTime = 0.0;
    static double allocRate = 0.0;
    long rssKb = 0;
    long virtKb = 0;
    int blockX = (int)floorf(player.position.x);
    int blockY = (int)floorf(player.position.y);
    int blockZ = (int)floorf(player.position.z);
    int localX = blockX%CHUNK_SIZE;
    int localY = blockY;
    int localZ = blockZ%CHUNK_SIZE;
    ChunkPos chunk = WorldToChunk(player.position);
    const Biome *biome = BiomeAt(blockX, blockZ);
    BlockPos feet = { blockX, blockY, blockZ };
    BlockType feetBlock = GetBlock(&world, feet);
    int meshed = 0;
    int loaded = 0;
    int i = 0;
    int y = 2;
    int line = 11;
    float yawDeg = player.yaw*(180.0f/PI);
    float pitchDeg = player.pitch*(180.0f/PI);
    float lookX = sinf(player.yaw);
    float lookZ = cosf(player.yaw);
    const char *facing = "south";
    const char *axis = "positive Z";
    const char *gpu = (const char *)glGetString(GL_RENDERER);
    const char *glVersion = (const char *)glGetString(GL_VERSION);
    unsigned int seed = NetSessionIsInWorld() ? NetSessionSeed() : WorldSaveSeed();
    float terrain = SimplexNoise2D((float)blockX*0.01f, (float)blockZ*0.01f);
    float continent = SimplexNoise2D((float)blockX*0.0025f, (float)blockZ*0.0025f);
    double now = GetTime();

    if (localX < 0) localX += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;
    if (fabsf(lookX) > fabsf(lookZ))
    {
        if (lookX > 0.0f) { facing = "east"; axis = "positive X"; }
        else { facing = "west"; axis = "negative X"; }
    }
    else if (lookZ < 0.0f) { facing = "north"; axis = "negative Z"; }
    while (yawDeg < 0.0f) yawDeg += 360.0f;
    while (yawDeg >= 360.0f) yawDeg -= 360.0f;

    ReadMachine(cpu, (int)sizeof(cpu), &cores, &rssKb, &virtKb);
    if ((lastMemTime == 0.0) || (now - lastMemTime >= 1.0))
    {
        allocRate = (double)(rssKb - lastRss);
        lastRss = rssKb;
        lastMemTime = now;
    }
    for (i = 0; i < MAX_CHUNKS; i++)
    {
        if (!world.chunks[i].isLoaded) continue;
        loaded++;
        if (world.chunks[i].hasMesh) meshed++;
    }

    DebugText(2, y, TextFormat("OpenCraft %s", OPENCRAFT_VERSION), 0); y += line;
    DebugText(2, y, TextFormat("%d fps  T: %.1f ms  vsync: %s", GetFPS(), GetFrameTime()*1000.0f,
        IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off"), 0); y += line;
    DebugText(2, y, NetSessionIsInWorld() ? "Server: dedicated opencraft-server" : "Integrated server: local world", 0); y += line;
    DebugText(2, y, TextFormat("C: %d/%d  D: %d  E: 1/1", meshed, loaded, GameRenderDistance()), 0); y += line;
    DebugText(2, y, TextFormat("Chunks: %d loaded, height %d", loaded, WORLD_HEIGHT), 0); y += line;
    DebugText(2, y, TextFormat("minecraft:overworld  seed %u", seed), 0); y += line;
    DebugText(2, y, TextFormat("XYZ: %.3f / %.5f / %.3f", player.position.x, player.position.y, player.position.z), 0); y += line;
    DebugText(2, y, TextFormat("Block: %d %d %d [%s]", blockX, blockY, blockZ, GetBlockName(feetBlock)), 0); y += line;
    DebugText(2, y, TextFormat("Chunk: %d %d %d in r.%d.%d.mca", localX, localY, localZ, chunk.x >> 5, chunk.z >> 5), 0); y += line;
    DebugText(2, y, TextFormat("Facing: %s (Towards %s) (%.1f / %.1f)", facing, axis, yawDeg, pitchDeg), 0); y += line;
    DebugText(2, y, TextFormat("Client Light: %d sky, 0 block", SkyLightAt(&world, blockX, blockY, blockZ)), 0); y += line;
    DebugText(2, y, TextFormat("Biome: %s", (biome != NULL) ? biome->id : "unknown"), 0); y += line;
    DebugText(2, y, "Local Difficulty: 0.00 / 0.00 (no day cycle)", 0); y += line;
    DebugText(2, y, TextFormat("Noise  T: %.3f  C: %.3f", terrain, continent), 0); y += line;
    DebugText(2, y, TextFormat("Biome builder: %s / %s",
        (biome != NULL) ? biome->name : "unknown",
        (biome != NULL) ? GetBlockName((BlockType)biome->surface) : "air"), 0); y += line;
    if (player.hasTarget)
    {
        DebugText(2, y, TextFormat("Targeted: %s %d %d %d", GetBlockName((BlockType)GetBlock(&world, player.targetBlock)),
            player.targetBlock.x, player.targetBlock.y, player.targetBlock.z), 0);
        y += line;
    }
    DebugText(2, y, "SC: 0  Sounds: 0/0 (Mood 0%)", 0); y += line;
    DebugText(2, y, "For help: press F3", 0);

    y = 2;
    DebugText(0, y, TextFormat("Mem: %ldMB / %ldMB", rssKb/1024, virtKb/1024), 1); y += line;
    DebugText(0, y, TextFormat("Allocation rate: %.0fKB/s", allocRate), 1); y += line;
    DebugText(0, y, TextFormat("Allocated: %ldMB", rssKb/1024), 1); y += line;
    DebugText(0, y, TextFormat("CPU: %dx %s", cores, (cpu[0] != '\0') ? cpu : "unknown"), 1); y += line;
    DebugText(0, y, TextFormat("Display: %dx%d", GetScreenWidth(), GetScreenHeight()), 1); y += line;
    DebugText(0, y, (gpu != NULL) ? gpu : "GPU unknown", 1); y += line;
    DebugText(0, y, (glVersion != NULL) ? glVersion : "", 1);

    DrawFrameGraph(4, GetScreenHeight() - 8, 240, 36, (Color){ 80, 255, 80, 255 });
    DebugText(4, GetScreenHeight() - 62, TextFormat("%d FPS", GetFPS()), 0);
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
    
    if (debugHud && !gamePaused && !player.inventoryOpen) DrawDebugHud();

    if (!IsCursorHidden() && !gamePaused && !player.inventoryOpen && !debugHud) {
        DrawText("Click to play", GetScreenWidth()/2 - 70, GetScreenHeight() - 40, 20, YELLOW);
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

    if (NetSessionIsInWorld() && !debugHud)
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
