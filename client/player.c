#include "player.h"
#include "net_session.h"
#include "voxel_renderer.h"
#include "biomes.h"
#include "world_generation.h"
#include "raymath.h"
#include <math.h>

//----------------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------------
#define GRAVITY 20.0f
#define JUMP_VELOCITY 8.0f
#define PLAYER_HEIGHT 1.8f
#define PLAYER_WIDTH 0.6f
#define REACH_DISTANCE 5.0f
#define MOVEMENT_DAMPING 0.1f

//----------------------------------------------------------------------------------
// Player Functions
//----------------------------------------------------------------------------------
void InitPlayer(Player* player, Vector3 startPosition) {
    player->position = startPosition;
    player->velocity = (Vector3){0, 0, 0};
    player->onGround = false;
    player->inWater = false;
    
    // Initialize camera rotation
    player->yaw = 0.0f;     // Facing negative Z (forward)
    player->pitch = 0.0f;   // Looking straight ahead
    
    // Movement settings
    player->walkSpeed = 5.0f;
    player->runSpeed = 8.0f;
    player->jumpHeight = JUMP_VELOCITY;
    player->mouseSensitivity = 0.003f;
    
    // Initialize camera
    player->camera = (Camera3D){0};
    player->camera.position = Vector3Add(startPosition, (Vector3){0, PLAYER_HEIGHT * 0.9f, 0});
    player->camera.target = Vector3Add(player->camera.position, (Vector3){0, 0, -1}); // Looking forward (negative Z)
    player->camera.up = (Vector3){0, 1, 0};
    player->camera.fovy = 70.0f;
    player->camera.projection = CAMERA_PERSPECTIVE;
    
    // Block interaction
    player->hasTarget = false;
    player->selectedBlock = BLOCK_GRASS;
    player->hotbarSlot = 0;
    
    // Initialize hotbar with basic blocks
    player->hotbar[0] = BLOCK_GRASS;
    player->hotbar[1] = BLOCK_DIRT;
    player->hotbar[2] = BLOCK_STONE;
    player->hotbar[3] = BLOCK_OAK_LOG;
    player->hotbar[4] = BLOCK_OAK_LEAVES;
    player->hotbar[5] = BLOCK_BUCKET;
    player->hotbar[6] = BLOCK_COBBLESTONE;
    player->hotbar[7] = BLOCK_SAND;
    player->hotbar[8] = BLOCK_BRICKS;
    
    // Initialize inventory system
    player->inventoryOpen = false;
    player->inventorySelectedSlot = 0;
    player->inventoryScrollOffset = 0;
    player->craftSize = 2;
    player->cursorBlock = BLOCK_AIR;
    player->cursorCount = 0;
    player->recipeBookOpen = 0;
    player->recipeIndex = -1;
    for (int craftIndex = 0; craftIndex < 9; craftIndex++) {
        player->craft[craftIndex] = BLOCK_AIR;
        player->craftCount[craftIndex] = 0;
    }
    
    // Fill inventory with all available blocks
    int slotIndex = 0;
    for (int i = 1; i < BLOCK_COUNT && slotIndex < INVENTORY_SIZE; i++) {
        BlockType filled = (BlockType)i;
        if (IsWaterBlock(filled) && (filled != BLOCK_WATER)) continue;
        if ((filled == BLOCK_BUCKET) || (filled == BLOCK_WATER_BUCKET)) continue;
        player->inventory.blocks[slotIndex] = filled;
        player->inventory.quantities[slotIndex] = 64; // Full stack
        slotIndex++;
    }
    
    // Fill remaining slots with air
    for (int i = slotIndex; i < INVENTORY_SIZE; i++) {
        player->inventory.blocks[i] = BLOCK_AIR;
        player->inventory.quantities[i] = 0;
    }
    
    DisableCursor(); // Lock cursor for first-person view
}

void SetPlayerLook(Player* player, float yaw, float pitch) {
    Vector3 forward = {0};
    float limit = PI/2.0f - 0.1f;

    if (pitch > limit) pitch = limit;
    if (pitch < -limit) pitch = -limit;

    player->yaw = yaw;
    player->pitch = pitch;
    player->camera.position = Vector3Add(player->position, (Vector3){0, PLAYER_HEIGHT * 0.9f, 0});
    forward.x = cosf(pitch)*sinf(yaw);
    forward.y = sinf(pitch);
    forward.z = cosf(pitch)*cosf(yaw);
    player->camera.target = Vector3Add(player->camera.position, forward);
}

void UpdatePlayer(Player* player, VoxelWorld* world) {
    if (BiomeTourActive()) {
        int columnX = BiomeTourIndex()*BIOME_STRIDE + BIOME_STRIDE/2;
        int columnZ = BIOME_STRIDE/2;

        player->position = (Vector3){ columnX + 0.5f, GetSurfaceLevel(columnX, columnZ), columnZ + 0.5f };
        player->velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        player->onGround = true;
        SetPlayerLook(player, 0.15f, -0.62f);
        BiomeTourTick(GetFrameTime());
        return;
    }

    {
        int wasOpen = player->inventoryOpen;

        HandlePlayerInput(player);
        if (!wasOpen && player->inventoryOpen) {
            int i = 0;

            player->craftSize = 2;
            if (player->hasTarget && (GetBlock(world, player->targetBlock) == BLOCK_CRAFTING_TABLE)) {
                player->craftSize = 3;
            }
            for (i = 0; i < 9; i++) {
                player->craft[i] = BLOCK_AIR;
                player->craftCount[i] = 0;
            }
            player->recipeBookOpen = 0;
            player->recipeIndex = -1;
        }
    }
    UpdatePlayerPhysics(player, world);
    UpdatePlayerInteraction(player, world);
    
    // Update camera position
    player->camera.position = Vector3Add(player->position, (Vector3){0, PLAYER_HEIGHT * 0.9f, 0});
}

void HandlePlayerInput(Player* player) {
    HandlePlayerMouseLook(player);
    HandlePlayerMovement(player);
    
    if (IsKeyPressed(KEY_E)) {
        if (player->inventoryOpen) InventoryClose(player);
        else {
            player->inventoryOpen = true;
            EnableCursor();
        }
    }
    if (player->inventoryOpen) InventoryHandleInput(player);
    
    // Hotbar selection (only when inventory is closed)
    if (!player->inventoryOpen) {
        for (int i = 0; i < 9; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                player->hotbarSlot = i;
                player->selectedBlock = player->hotbar[i];
            }
        }
    }
    
    // Note: ESC key handling moved to screen_gameplay.c for pause menu
}

void HandlePlayerMovement(Player* player) {
    if (IsCursorHidden()) {
        Vector3 movement = {0, 0, 0};
        
        // Calculate horizontal forward and right vectors directly from yaw angle
        // This is completely independent of pitch and eliminates any drift
        Vector3 horizontalForward = {
            sinf(player->yaw),   // X
            0,                   // Y (always 0 for horizontal movement)
            cosf(player->yaw)    // Z
        };
        
        Vector3 horizontalRight = {
            -cosf(player->yaw),  // X (90 degrees rotated from forward)
            0,                   // Y (always 0 for horizontal movement)
            sinf(player->yaw)    // Z (90 degrees rotated from forward)
        };
        
        // Calculate movement input using purely horizontal vectors
        if (IsKeyDown(KEY_W)) movement = Vector3Add(movement, horizontalForward);
        if (IsKeyDown(KEY_S)) movement = Vector3Subtract(movement, horizontalForward);
        if (IsKeyDown(KEY_A)) movement = Vector3Subtract(movement, horizontalRight);
        if (IsKeyDown(KEY_D)) movement = Vector3Add(movement, horizontalRight);
        
        // Normalize diagonal movement
        if (Vector3Length(movement) > 0) {
            movement = Vector3Normalize(movement);
        }
        
        // Apply speed
        float speed = IsKeyDown(KEY_LEFT_SHIFT) ? player->runSpeed : player->walkSpeed;
        movement = Vector3Scale(movement, speed);
        
        // Apply movement to velocity (horizontal only)
        player->velocity.x = movement.x;
        player->velocity.z = movement.z;
        
        // Jumping
        if (IsKeyPressed(KEY_SPACE) && player->onGround) {
            player->velocity.y = player->jumpHeight;
            player->onGround = false;
        }
    }
}

void HandlePlayerMouseLook(Player* player) {
    if (IsCursorHidden()) {
        Vector2 mouseDelta = GetMouseDelta();
        
        // Update yaw (horizontal rotation)
        player->yaw -= mouseDelta.x * player->mouseSensitivity;
        
        // Update pitch (vertical rotation) with limits
        player->pitch -= mouseDelta.y * player->mouseSensitivity;
        
        // Limit pitch to prevent over-rotation
        const float maxPitch = PI/2 - 0.1f;  // Just under 90 degrees
        if (player->pitch > maxPitch) player->pitch = maxPitch;
        if (player->pitch < -maxPitch) player->pitch = -maxPitch;
        
        // Calculate forward vector from yaw and pitch
        Vector3 forward = {
            cosf(player->pitch) * sinf(player->yaw),  // X
            sinf(player->pitch),                      // Y  
            cosf(player->pitch) * cosf(player->yaw)   // Z
        };
        
        // Update camera target
        player->camera.target = Vector3Add(player->camera.position, forward);
    }
}

static bool PlayerInWater(Player* player, VoxelWorld* world) {
    BlockPos feet = WorldToBlock(player->position);
    BlockPos waist = feet;
    waist.y += 1;
    return IsWaterBlock(GetBlock(world, feet)) || IsWaterBlock(GetBlock(world, waist));
}

void UpdatePlayerPhysics(Player* player, VoxelWorld* world) {
    float deltaTime = GetFrameTime();

    player->inWater = PlayerInWater(player, world);
    
    // Apply gravity
    ApplyGravity(player);

    if (player->inWater) {
        player->velocity.x *= 0.55f;
        player->velocity.z *= 0.55f;
        if (IsKeyDown(KEY_SPACE)) player->velocity.y = 4.5f;
    }
    
    // Check collision and move player
    Vector3 newPosition = Vector3Add(player->position, Vector3Scale(player->velocity, deltaTime));
    
    // Check Y collision (vertical)
    Vector3 verticalPos = player->position;
    verticalPos.y = newPosition.y;
    if (!CheckCollision(player, world, verticalPos)) {
        player->position.y = verticalPos.y;
        player->onGround = false;
    } else {
        if (player->velocity.y < 0) {
            player->onGround = true;
        }
        player->velocity.y = 0;
    }
    
    // Check X collision (horizontal)
    Vector3 horizontalPosX = player->position;
    horizontalPosX.x = newPosition.x;
    if (!CheckCollision(player, world, horizontalPosX)) {
        player->position.x = horizontalPosX.x;
    } else {
        player->velocity.x = 0;
    }
    
    // Check Z collision (horizontal)
    Vector3 horizontalPosZ = player->position;
    horizontalPosZ.z = newPosition.z;
    if (!CheckCollision(player, world, horizontalPosZ)) {
        player->position.z = horizontalPosZ.z;
    } else {
        player->velocity.z = 0;
    }
    
    // Apply damping
    player->velocity.x *= (1.0f - MOVEMENT_DAMPING);
    player->velocity.z *= (1.0f - MOVEMENT_DAMPING);
}

void ApplyGravity(Player* player) {
    float deltaTime = GetFrameTime();
    float gravity = player->inWater ? GRAVITY*0.12f : GRAVITY;
    player->velocity.y -= gravity * deltaTime;

    if (player->inWater && (player->velocity.y < -2.5f)) {
        player->velocity.y = -2.5f;
    }
    
    // Terminal velocity
    if (player->velocity.y < -50.0f) {
        player->velocity.y = -50.0f;
    }
}

bool CheckCollision(Player* player, VoxelWorld* world, Vector3 newPosition) {
    // Check collision box around player
    float halfWidth = PLAYER_WIDTH * 0.5f;
    
    // Check multiple points around the player
    Vector3 checkPoints[8] = {
        {newPosition.x - halfWidth, newPosition.y, newPosition.z - halfWidth},
        {newPosition.x + halfWidth, newPosition.y, newPosition.z - halfWidth},
        {newPosition.x - halfWidth, newPosition.y, newPosition.z + halfWidth},
        {newPosition.x + halfWidth, newPosition.y, newPosition.z + halfWidth},
        {newPosition.x - halfWidth, newPosition.y + PLAYER_HEIGHT, newPosition.z - halfWidth},
        {newPosition.x + halfWidth, newPosition.y + PLAYER_HEIGHT, newPosition.z - halfWidth},
        {newPosition.x - halfWidth, newPosition.y + PLAYER_HEIGHT, newPosition.z + halfWidth},
        {newPosition.x + halfWidth, newPosition.y + PLAYER_HEIGHT, newPosition.z + halfWidth}
    };
    
    for (int i = 0; i < 8; i++) {
        BlockPos blockPos = WorldToBlock(checkPoints[i]);
        BlockType block = GetBlock(world, blockPos);
        if (IsBlockSolid(block)) {
            return true; // Collision detected
        }
    }
    
    return false; // No collision
}

static void CommitBlock(VoxelWorld* world, BlockPos pos, BlockType block);

void UpdatePlayerInteraction(Player* player, VoxelWorld* world) {
    UpdateBlockTarget(player, world);
    
    if (IsCursorHidden()) {
        // Block breaking (left click)
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            HandleBlockBreaking(player, world);
        }
        
        // Block placement (right click). F also drops the held block
        // one step ahead, so a key can place when the cursor is grabbed.
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            HandleBlockPlacement(player, world);
        }
        if (IsKeyPressed(KEY_F) && (player->selectedBlock != BLOCK_AIR)) {
            BlockPos pos = player->targetBlock;

            if (!player->hasTarget) {
                pos.x = (int)floorf(player->position.x + sinf(player->yaw)*1.5f);
                pos.y = (int)floorf(player->position.y);
                pos.z = (int)floorf(player->position.z + cosf(player->yaw)*1.5f);
                if (pos.y < 1) pos.y = 1;
            }
            CommitBlock(world, pos, player->selectedBlock);
        }
    }
}

void UpdateBlockTarget(Player* player, VoxelWorld* world) {
    Vector3 rayOrigin = player->camera.position;
    Vector3 rayDirection = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
    
    Vector3 hitNormal;
    player->hasTarget = RaycastToBlock(rayOrigin, rayDirection, world, &player->targetBlock, &hitNormal);
}

static void CommitBlock(VoxelWorld* world, BlockPos pos, BlockType block)
{
    SetBlock(world, pos, block);
    NetSessionLocalBlock(pos.x, pos.y, pos.z, (int)block);
}

static void UseBucket(Player* player, VoxelWorld* world) {
    BlockType target = BLOCK_AIR;

    if (!player->hasTarget) return;
    target = GetBlock(world, player->targetBlock);
    if (target != BLOCK_WATER) return;

    CommitBlock(world, player->targetBlock, BLOCK_AIR);
    player->hotbar[player->hotbarSlot] = BLOCK_WATER_BUCKET;
    player->selectedBlock = BLOCK_WATER_BUCKET;
}

static void PlaceWaterBucket(Player* player, VoxelWorld* world) {
    Vector3 rayOrigin = player->camera.position;
    Vector3 rayDirection = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
    BlockPos hitBlock = {0};
    Vector3 hitNormal = {0};
    BlockPos placePos = {0};
    BlockType there = BLOCK_AIR;

    if (!RaycastToBlock(rayOrigin, rayDirection, world, &hitBlock, &hitNormal)) return;

    placePos.x = hitBlock.x + (int)hitNormal.x;
    placePos.y = hitBlock.y + (int)hitNormal.y;
    placePos.z = hitBlock.z + (int)hitNormal.z;
    there = GetBlock(world, placePos);
    if ((there != BLOCK_AIR) && !IsWaterBlock(there)) return;
    if (there == BLOCK_WATER) return;

    CommitBlock(world, placePos, BLOCK_WATER);
    player->hotbar[player->hotbarSlot] = BLOCK_BUCKET;
    player->selectedBlock = BLOCK_BUCKET;
}

void HandleBlockPlacement(Player* player, VoxelWorld* world) {
    if (player->selectedBlock == BLOCK_BUCKET) {
        UseBucket(player, world);
        return;
    }
    if (player->selectedBlock == BLOCK_WATER_BUCKET) {
        PlaceWaterBucket(player, world);
        return;
    }
    if (!player->hasTarget || player->selectedBlock == BLOCK_AIR) return;
    
    Vector3 rayOrigin = player->camera.position;
    Vector3 rayDirection = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
    
    BlockPos hitBlock;
    Vector3 hitNormal;
    
    if (RaycastToBlock(rayOrigin, rayDirection, world, &hitBlock, &hitNormal)) {
        // Calculate placement position (adjacent to hit block)
        BlockPos placePos = {
            hitBlock.x + (int)hitNormal.x,
            hitBlock.y + (int)hitNormal.y,
            hitBlock.z + (int)hitNormal.z
        };
        
        // Check if placement position is valid and not inside player
        Vector3 placeWorldPos = {placePos.x + 0.5f, placePos.y + 0.5f, placePos.z + 0.5f};
        Vector3 playerFeet = player->position;
        Vector3 playerHead = Vector3Add(player->position, (Vector3){0, PLAYER_HEIGHT, 0});
        
        // Don't place block if it would intersect with player
        bool wouldIntersectPlayer = (
            placeWorldPos.x >= playerFeet.x - PLAYER_WIDTH/2 && placeWorldPos.x <= playerFeet.x + PLAYER_WIDTH/2 &&
            placeWorldPos.z >= playerFeet.z - PLAYER_WIDTH/2 && placeWorldPos.z <= playerFeet.z + PLAYER_WIDTH/2 &&
            placeWorldPos.y >= playerFeet.y && placeWorldPos.y <= playerHead.y
        );
        
        if (!wouldIntersectPlayer && GetBlock(world, placePos) == BLOCK_AIR) {
            CommitBlock(world, placePos, player->selectedBlock);
        }
    }
}

void HandleBlockBreaking(Player* player, VoxelWorld* world) {
    if (!player->hasTarget) return;
    
    BlockType currentBlock = GetBlock(world, player->targetBlock);
    if (IsWaterBlock(currentBlock)) return;
    if (currentBlock != BLOCK_AIR) {
        CommitBlock(world, player->targetBlock, BLOCK_AIR);
    }
}

bool RaycastToBlock(Vector3 origin, Vector3 direction, VoxelWorld* world, BlockPos* hitBlock, Vector3* hitNormal) {
    Vector3 rayPos = origin;
    Vector3 rayStep = Vector3Scale(Vector3Normalize(direction), 0.1f);
    
    for (float distance = 0; distance < REACH_DISTANCE; distance += 0.1f) {
        BlockPos currentBlock = WorldToBlock(rayPos);
        BlockType block = GetBlock(world, currentBlock);
        
        if (IsBlockSolid(block) || IsWaterBlock(block)) {
            *hitBlock = currentBlock;
            
            // Calculate hit normal (simplified)
            Vector3 blockCenter = {currentBlock.x + 0.5f, currentBlock.y + 0.5f, currentBlock.z + 0.5f};
            Vector3 hitPoint = rayPos;
            Vector3 diff = Vector3Subtract(hitPoint, blockCenter);
            
            // Find the largest component to determine which face was hit
            if (fabsf(diff.x) > fabsf(diff.y) && fabsf(diff.x) > fabsf(diff.z)) {
                *hitNormal = (Vector3){diff.x > 0 ? 1 : -1, 0, 0};
            } else if (fabsf(diff.y) > fabsf(diff.z)) {
                *hitNormal = (Vector3){0, diff.y > 0 ? 1 : -1, 0};
            } else {
                *hitNormal = (Vector3){0, 0, diff.z > 0 ? 1 : -1};
            }
            
            return true;
        }
        
        rayPos = Vector3Add(rayPos, rayStep);
    }
    
    return false;
}

//----------------------------------------------------------------------------------
// UI Functions
//----------------------------------------------------------------------------------
void DrawBlockDebugInfo(Player* player, VoxelWorld* world) {
    if (!player->hasTarget) return;
    
    // Get block information
    BlockType targetBlock = GetBlock(world, player->targetBlock);
    if (targetBlock == BLOCK_AIR) return;
    
    const char* blockName = GetBlockName(targetBlock);
    
    // Get texture name for the top face (most representative)
    const char* textureName = GetBlockTextureName(targetBlock, FACE_TOP);
    
    // Draw semi-transparent background
    int panelWidth = 300;
    int panelHeight = 80;
    int screenWidth = GetScreenWidth();
    int x = screenWidth - panelWidth - 20;
    int y = 20;
    
    DrawRectangle(x, y, panelWidth, panelHeight, (Color){0, 0, 0, 150});
    DrawRectangleLines(x, y, panelWidth, panelHeight, WHITE);
    
    // Draw debug text
    DrawText("Block Debug Info", x + 10, y + 10, 18, YELLOW);
    DrawText(TextFormat("Name: %s", blockName), x + 10, y + 30, 16, WHITE);
    DrawText(TextFormat("Texture: %s.png", textureName), x + 10, y + 50, 16, LIGHTGRAY);
    
    // Draw block position
    DrawText(TextFormat("Pos: (%d, %d, %d)", 
             player->targetBlock.x, player->targetBlock.y, player->targetBlock.z), 
             x + 10, y + 70, 14, GRAY);
}

void DrawPlayerUI(Player* player) {
    if (player->inventoryOpen) {
        DrawInventory(player);
        return;
    }
    DrawCrosshair();
    DrawHotbar(player);

    if (player->hasTarget) {
        DrawBlockOutline(player->targetBlock);
    }
}

void DrawCrosshair(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    int size = 10;
    
    DrawLine(centerX - size, centerY, centerX + size, centerY, WHITE);
    DrawLine(centerX, centerY - size, centerX, centerY + size, WHITE);
}

void DrawBlockOutline(BlockPos position) {
    Vector3 blockPos = {position.x, position.y, position.z};
    Vector3 size = {1.0f, 1.0f, 1.0f};
    DrawCubeWires(Vector3Add(blockPos, Vector3Scale(size, 0.5f)), size.x, size.y, size.z, RED);
}

//----------------------------------------------------------------------------------
// Inventory UI Functions
//----------------------------------------------------------------------------------
const char* GetBlockName(BlockType block) {
    if (IsWaterBlock(block)) return "Water";
    switch (block) {
        case BLOCK_AIR: return "Air";
        case BLOCK_GRASS: return "Grass Block";
        case BLOCK_DIRT: return "Dirt";
        case BLOCK_STONE: return "Stone";
        case BLOCK_COBBLESTONE: return "Cobblestone";
        case BLOCK_BEDROCK: return "Bedrock";
        case BLOCK_SAND: return "Sand";
        case BLOCK_GRAVEL: return "Gravel";
        case BLOCK_WATER: return "Water";
        case BLOCK_BUCKET: return "Bucket";
        case BLOCK_WATER_BUCKET: return "Water Bucket";
        case BLOCK_OAK_LOG: return "Oak Log";
        case BLOCK_OAK_PLANKS: return "Oak Planks";
        case BLOCK_OAK_LEAVES: return "Oak Leaves";
        case BLOCK_BIRCH_LOG: return "Birch Log";
        case BLOCK_BIRCH_PLANKS: return "Birch Planks";
        case BLOCK_BIRCH_LEAVES: return "Birch Leaves";
        case BLOCK_ACACIA_LOG: return "Acacia Log";
        case BLOCK_ACACIA_PLANKS: return "Acacia Planks";
        case BLOCK_ACACIA_LEAVES: return "Acacia Leaves";
        case BLOCK_DARK_OAK_LOG: return "Dark Oak Log";
        case BLOCK_DARK_OAK_PLANKS: return "Dark Oak Planks";
        case BLOCK_DARK_OAK_LEAVES: return "Dark Oak Leaves";
        case BLOCK_STONE_BRICKS: return "Stone Bricks";
        case BLOCK_MOSSY_STONE_BRICKS: return "Mossy Stone Bricks";
        case BLOCK_CRACKED_STONE_BRICKS: return "Cracked Stone Bricks";
        case BLOCK_MOSSY_COBBLESTONE: return "Mossy Cobblestone";
        case BLOCK_SMOOTH_STONE: return "Smooth Stone";
        case BLOCK_ANDESITE: return "Andesite";
        case BLOCK_GRANITE: return "Granite";
        case BLOCK_DIORITE: return "Diorite";
        case BLOCK_SANDSTONE: return "Sandstone";
        case BLOCK_CHISELED_SANDSTONE: return "Chiseled Sandstone";
        case BLOCK_CUT_SANDSTONE: return "Cut Sandstone";
        case BLOCK_RED_SAND: return "Red Sand";
        case BLOCK_RED_SANDSTONE: return "Red Sandstone";
        case BLOCK_COAL_ORE: return "Coal Ore";
        case BLOCK_IRON_ORE: return "Iron Ore";
        case BLOCK_GOLD_ORE: return "Gold Ore";
        case BLOCK_DIAMOND_ORE: return "Diamond Ore";
        case BLOCK_REDSTONE_ORE: return "Redstone Ore";
        case BLOCK_EMERALD_ORE: return "Emerald Ore";
        case BLOCK_LAPIS_ORE: return "Lapis Ore";
        case BLOCK_IRON_BLOCK: return "Iron Block";
        case BLOCK_GOLD_BLOCK: return "Gold Block";
        case BLOCK_DIAMOND_BLOCK: return "Diamond Block";
        case BLOCK_EMERALD_BLOCK: return "Emerald Block";
        case BLOCK_REDSTONE_BLOCK: return "Redstone Block";
        case BLOCK_LAPIS_BLOCK: return "Lapis Block";
        case BLOCK_COAL_BLOCK: return "Coal Block";
        case BLOCK_WHITE_WOOL: return "White Wool";
        case BLOCK_ORANGE_WOOL: return "Orange Wool";
        case BLOCK_MAGENTA_WOOL: return "Magenta Wool";
        case BLOCK_LIGHT_BLUE_WOOL: return "Light Blue Wool";
        case BLOCK_YELLOW_WOOL: return "Yellow Wool";
        case BLOCK_LIME_WOOL: return "Lime Wool";
        case BLOCK_PINK_WOOL: return "Pink Wool";
        case BLOCK_GRAY_WOOL: return "Gray Wool";
        case BLOCK_LIGHT_GRAY_WOOL: return "Light Gray Wool";
        case BLOCK_CYAN_WOOL: return "Cyan Wool";
        case BLOCK_PURPLE_WOOL: return "Purple Wool";
        case BLOCK_BLUE_WOOL: return "Blue Wool";
        case BLOCK_BROWN_WOOL: return "Brown Wool";
        case BLOCK_GREEN_WOOL: return "Green Wool";
        case BLOCK_RED_WOOL: return "Red Wool";
        case BLOCK_BLACK_WOOL: return "Black Wool";
        case BLOCK_WHITE_CONCRETE: return "White Concrete";
        case BLOCK_ORANGE_CONCRETE: return "Orange Concrete";
        case BLOCK_MAGENTA_CONCRETE: return "Magenta Concrete";
        case BLOCK_LIGHT_BLUE_CONCRETE: return "Light Blue Concrete";
        case BLOCK_YELLOW_CONCRETE: return "Yellow Concrete";
        case BLOCK_LIME_CONCRETE: return "Lime Concrete";
        case BLOCK_PINK_CONCRETE: return "Pink Concrete";
        case BLOCK_GRAY_CONCRETE: return "Gray Concrete";
        case BLOCK_LIGHT_GRAY_CONCRETE: return "Light Gray Concrete";
        case BLOCK_CYAN_CONCRETE: return "Cyan Concrete";
        case BLOCK_PURPLE_CONCRETE: return "Purple Concrete";
        case BLOCK_BLUE_CONCRETE: return "Blue Concrete";
        case BLOCK_BROWN_CONCRETE: return "Brown Concrete";
        case BLOCK_GREEN_CONCRETE: return "Green Concrete";
        case BLOCK_RED_CONCRETE: return "Red Concrete";
        case BLOCK_BLACK_CONCRETE: return "Black Concrete";
        case BLOCK_GLASS: return "Glass";
        case BLOCK_WHITE_STAINED_GLASS: return "White Stained Glass";
        case BLOCK_ORANGE_STAINED_GLASS: return "Orange Stained Glass";
        case BLOCK_MAGENTA_STAINED_GLASS: return "Magenta Stained Glass";
        case BLOCK_LIGHT_BLUE_STAINED_GLASS: return "Light Blue Stained Glass";
        case BLOCK_YELLOW_STAINED_GLASS: return "Yellow Stained Glass";
        case BLOCK_LIME_STAINED_GLASS: return "Lime Stained Glass";
        case BLOCK_PINK_STAINED_GLASS: return "Pink Stained Glass";
        case BLOCK_GRAY_STAINED_GLASS: return "Gray Stained Glass";
        case BLOCK_LIGHT_GRAY_STAINED_GLASS: return "Light Gray Stained Glass";
        case BLOCK_CYAN_STAINED_GLASS: return "Cyan Stained Glass";
        case BLOCK_PURPLE_STAINED_GLASS: return "Purple Stained Glass";
        case BLOCK_BLUE_STAINED_GLASS: return "Blue Stained Glass";
        case BLOCK_BROWN_STAINED_GLASS: return "Brown Stained Glass";
        case BLOCK_GREEN_STAINED_GLASS: return "Green Stained Glass";
        case BLOCK_RED_STAINED_GLASS: return "Red Stained Glass";
        case BLOCK_BLACK_STAINED_GLASS: return "Black Stained Glass";
        case BLOCK_BRICKS: return "Bricks";
        case BLOCK_BOOKSHELF: return "Bookshelf";
        case BLOCK_CRAFTING_TABLE: return "Crafting Table";
        case BLOCK_FURNACE: return "Furnace";
        case BLOCK_CHEST: return "Chest";
        case BLOCK_GLOWSTONE: return "Glowstone";
        case BLOCK_OBSIDIAN: return "Obsidian";
        case BLOCK_NETHERRACK: return "Netherrack";
        case BLOCK_SOUL_SAND: return "Soul Sand";
        case BLOCK_END_STONE: return "End Stone";
        case BLOCK_PURPUR_BLOCK: return "Purpur Block";
        case BLOCK_PRISMARINE: return "Prismarine";
        case BLOCK_SEA_LANTERN: return "Sea Lantern";
        case BLOCK_MAGMA_BLOCK: return "Magma Block";
        case BLOCK_BONE_BLOCK: return "Bone Block";
        case BLOCK_QUARTZ_BLOCK: return "Quartz Block";
        case BLOCK_CHISELED_QUARTZ_BLOCK: return "Chiseled Quartz Block";
        case BLOCK_QUARTZ_PILLAR: return "Quartz Pillar";
        case BLOCK_PACKED_ICE: return "Packed Ice";
        case BLOCK_BLUE_ICE: return "Blue Ice";
        case BLOCK_ICE: return "Ice";
        case BLOCK_SNOW_BLOCK: return "Snow Block";
        case BLOCK_CLAY: return "Clay";
        case BLOCK_HONEYCOMB_BLOCK: return "Honeycomb Block";
        case BLOCK_HAY_BLOCK: return "Hay Block";
        case BLOCK_MELON: return "Melon";
        case BLOCK_PUMPKIN: return "Pumpkin";
        case BLOCK_JACK_O_LANTERN: return "Jack o'Lantern";
        case BLOCK_CACTUS: return "Cactus";
        case BLOCK_SPONGE: return "Sponge";
        case BLOCK_WET_SPONGE: return "Wet Sponge";
        default: return "Unknown Block";
    }
} 
