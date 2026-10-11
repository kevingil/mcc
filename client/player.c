#include "player.h"
#include <stddef.h>
#include "gui.h"
#include "net_session.h"
#include "voxel_renderer.h"
#include "biomes.h"
#include "world_generation.h"
#include "raymath.h"
#include "rlgl.h"
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

typedef struct {
    BlockType block;
    int count;
} StarterStack;

// Broken blocks do not drop anything yet, so survival starts with building
// stock in the storage rows instead of an empty inventory.
static const StarterStack starterKit[] = {
    { BLOCK_OAK_PLANKS, 64 }, { BLOCK_OAK_LOG, 32 }, { BLOCK_BIRCH_LOG, 32 },
    { BLOCK_STONE, 64 }, { BLOCK_STONE_BRICKS, 64 }, { BLOCK_SANDSTONE, 48 },
    { BLOCK_GLASS, 32 }, { BLOCK_CRAFTING_TABLE, 1 }, { BLOCK_FURNACE, 1 },
    { BLOCK_CHEST, 2 }, { BLOCK_BOOKSHELF, 8 }, { BLOCK_GLOWSTONE, 16 },
    { BLOCK_WHITE_WOOL, 24 }, { BLOCK_RED_WOOL, 24 }, { BLOCK_BLUE_WOOL, 24 },
    { BLOCK_YELLOW_WOOL, 24 }, { BLOCK_GRAVEL, 64 }, { BLOCK_QUARTZ_BLOCK, 16 },
    { BLOCK_OBSIDIAN, 10 }, { BLOCK_IRON_BLOCK, 9 }, { BLOCK_GOLD_BLOCK, 9 },
    { BLOCK_DIAMOND_BLOCK, 3 }
};

static bool clickGrabbedMouse = false;

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
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        player->hotbarCount[i] = GuiMaxStack(player->hotbar[i]);
    }
    player->offhand = BLOCK_AIR;
    player->offhandCount = 0;

    player->gameMode = GAME_MODE_SURVIVAL;
    player->airSupply = 300;
    player->attackTicks = 0;
    
    // Initialize inventory system
    player->inventoryOpen = false;
    player->craftSize = 2;
    player->cursorBlock = BLOCK_AIR;
    player->cursorCount = 0;
    player->recipeBookOpen = 0;
    player->creativeTab = 0;
    player->creativeScroll = 0.0f;
    player->itemSearch[0] = '\0';
    player->searchFocused = 0;
    for (int craftIndex = 0; craftIndex < 9; craftIndex++) {
        player->craft[craftIndex] = BLOCK_AIR;
        player->craftCount[craftIndex] = 0;
    }
    
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        player->inventory.blocks[i] = BLOCK_AIR;
        player->inventory.quantities[i] = 0;
    }
    for (int i = 0; i < (int)(sizeof(starterKit)/sizeof(starterKit[0])); i++) {
        player->inventory.blocks[i] = starterKit[i].block;
        player->inventory.quantities[i] = starterKit[i].count;
    }
    
    DisableCursor(); // Lock cursor for first-person view
}

void SelectHotbarSlot(Player* player, int slot) {
    if ((slot < 0) || (slot >= HOTBAR_SIZE)) return;
    player->hotbarSlot = slot;
    player->selectedBlock = (player->hotbarCount[slot] > 0)? player->hotbar[slot] : BLOCK_AIR;
}

static int StowInto(BlockType *slot, int *count, BlockType block, int amount) {
    int room = GuiMaxStack(block) - *count;

    if ((*slot != block) || (*count <= 0) || (room <= 0)) return amount;
    if (room > amount) room = amount;
    *count += room;
    return amount - room;
}

static int StowEmpty(BlockType *slot, int *count, BlockType block, int amount) {
    int placed = GuiMaxStack(block);

    if ((*count > 0) && (*slot != BLOCK_AIR)) return amount;
    if (placed > amount) placed = amount;
    *slot = block;
    *count = placed;
    return amount - placed;
}

// Same order as returning items to the inventory in vanilla: the selected
// slot, the offhand, then every slot from the hotbar onward, first topping up
// matching stacks and then filling empty ones.
int StowItem(Player* player, BlockType block, int count) {
    int i = 0;

    if ((block == BLOCK_AIR) || (count <= 0)) return 0;
    count = StowInto(&player->hotbar[player->hotbarSlot], &player->hotbarCount[player->hotbarSlot], block, count);
    count = StowInto(&player->offhand, &player->offhandCount, block, count);
    for (i = 0; (i < HOTBAR_SIZE) && (count > 0); i++) {
        count = StowInto(&player->hotbar[i], &player->hotbarCount[i], block, count);
    }
    for (i = 0; (i < STORAGE_SIZE) && (count > 0); i++) {
        count = StowInto(&player->inventory.blocks[i], &player->inventory.quantities[i], block, count);
    }
    for (i = 0; (i < HOTBAR_SIZE) && (count > 0); i++) {
        count = StowEmpty(&player->hotbar[i], &player->hotbarCount[i], block, count);
    }
    for (i = 0; (i < STORAGE_SIZE) && (count > 0); i++) {
        count = StowEmpty(&player->inventory.blocks[i], &player->inventory.quantities[i], block, count);
    }
    SelectHotbarSlot(player, player->hotbarSlot);
    return count;
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

    clickGrabbedMouse = false;
    HandlePlayerInput(player);
    UpdatePlayerPhysics(player, world);
    UpdatePlayerInteraction(player, world);
    
    // Update camera position
    player->camera.position = Vector3Add(player->position, (Vector3){0, PLAYER_HEIGHT * 0.9f, 0});
}

void HandlePlayerInput(Player* player) {
    float wheel = 0.0f;
    bool creative = player->gameMode == GAME_MODE_CREATIVE;

    HandlePlayerMouseLook(player);
    HandlePlayerMovement(player);

    if (player->inventoryOpen) {
        InventoryHandleInput(player);
        return;
    }
    if (!IsCursorHidden() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        DisableCursor();
        clickGrabbedMouse = true;
        return;
    }
    if (IsKeyPressed(KEY_E)) {
        InventoryOpen(player, 2);
        return;
    }

    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if (!IsKeyPressed(KEY_ONE + i)) continue;
        if (creative && IsKeyDown(KEY_X)) InventoryLoadHotbar(player, i);
        else if (creative && IsKeyDown(KEY_C)) InventorySaveHotbar(player, i);
        else SelectHotbarSlot(player, i);
    }

    wheel = GetMouseWheelMove();
    if (wheel > 0.0f) SelectHotbarSlot(player, (player->hotbarSlot + HOTBAR_SIZE - 1)%HOTBAR_SIZE);
    else if (wheel < 0.0f) SelectHotbarSlot(player, (player->hotbarSlot + 1)%HOTBAR_SIZE);

    if (IsKeyPressed(KEY_F)) {
        int slot = player->hotbarSlot;
        BlockType held = player->hotbar[slot];
        int heldCount = player->hotbarCount[slot];

        player->hotbar[slot] = player->offhand;
        player->hotbarCount[slot] = player->offhandCount;
        player->offhand = held;
        player->offhandCount = heldCount;
        SelectHotbarSlot(player, slot);
    }
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
    BlockType target = BLOCK_AIR;

    UpdateBlockTarget(player, world);
    if (!IsCursorHidden() || player->inventoryOpen || clickGrabbedMouse) return;
    if (player->hasTarget) target = GetBlock(world, player->targetBlock);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Swinging at air or water restarts the attack cooldown.
        if (IsBlockSolid(target)) HandleBlockBreaking(player, world);
        else player->attackTicks = 0;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        bool sneaking = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if ((target == BLOCK_CRAFTING_TABLE) && !sneaking) InventoryOpen(player, 3);
        else HandleBlockPlacement(player, world);
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

static bool HasItem(Player* player, BlockType block) {
    if ((player->offhand == block) && (player->offhandCount > 0)) return true;
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if ((player->hotbar[i] == block) && (player->hotbarCount[i] > 0)) return true;
    }
    for (int i = 0; i < STORAGE_SIZE; i++) {
        if ((player->inventory.blocks[i] == block) && (player->inventory.quantities[i] > 0)) return true;
    }
    return false;
}

static bool HasRoomFor(Player* player, BlockType block) {
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if ((player->hotbarCount[i] <= 0) || (player->hotbar[i] == BLOCK_AIR)) return true;
        if ((player->hotbar[i] == block) && (player->hotbarCount[i] < GuiMaxStack(block))) return true;
    }
    for (int i = 0; i < STORAGE_SIZE; i++) {
        if ((player->inventory.quantities[i] <= 0) || (player->inventory.blocks[i] == BLOCK_AIR)) return true;
        if ((player->inventory.blocks[i] == block) && (player->inventory.quantities[i] < GuiMaxStack(block))) return true;
    }
    return false;
}

// Creative keeps the empty bucket and adds one filled bucket if there is none.
// Survival turns a single bucket into the filled one, or splits one off a stack.
static void UseBucket(Player* player, VoxelWorld* world) {
    int slot = player->hotbarSlot;
    BlockType target = BLOCK_AIR;

    if (!player->hasTarget) return;
    target = GetBlock(world, player->targetBlock);
    if (target != BLOCK_WATER) return;

    if (player->gameMode == GAME_MODE_CREATIVE) {
        CommitBlock(world, player->targetBlock, BLOCK_AIR);
        if (!HasItem(player, BLOCK_WATER_BUCKET)) StowItem(player, BLOCK_WATER_BUCKET, 1);
        return;
    }
    if (player->hotbarCount[slot] > 1) {
        if (!HasRoomFor(player, BLOCK_WATER_BUCKET)) return;
        CommitBlock(world, player->targetBlock, BLOCK_AIR);
        player->hotbarCount[slot]--;
        StowItem(player, BLOCK_WATER_BUCKET, 1);
        return;
    }
    CommitBlock(world, player->targetBlock, BLOCK_AIR);
    player->hotbar[slot] = BLOCK_WATER_BUCKET;
    player->hotbarCount[slot] = 1;
    SelectHotbarSlot(player, slot);
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
    if (player->gameMode == GAME_MODE_CREATIVE) return;
    player->hotbar[player->hotbarSlot] = BLOCK_BUCKET;
    player->hotbarCount[player->hotbarSlot] = 1;
    SelectHotbarSlot(player, player->hotbarSlot);
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
            int slot = player->hotbarSlot;

            CommitBlock(world, placePos, player->selectedBlock);
            if (player->gameMode != GAME_MODE_CREATIVE) {
                player->hotbarCount[slot]--;
                if (player->hotbarCount[slot] <= 0) {
                    player->hotbarCount[slot] = 0;
                    player->hotbar[slot] = BLOCK_AIR;
                }
                SelectHotbarSlot(player, slot);
            }
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
// Black at 40% alpha, a hair larger than the block so it does not z-fight.
void DrawBlockOutline(BlockPos position) {
    Vector3 center = { position.x + 0.5f, position.y + 0.5f, position.z + 0.5f };

    rlDrawRenderBatchActive();
    rlSetLineWidth(2.5f);
    DrawCubeWires(center, 1.004f, 1.004f, 1.004f, (Color){ 0, 0, 0, 102 });
    rlDrawRenderBatchActive();
    rlSetLineWidth(1.0f);
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
        case BLOCK_LAPIS_ORE: return "Lapis Lazuli Ore";
        case BLOCK_IRON_BLOCK: return "Block of Iron";
        case BLOCK_GOLD_BLOCK: return "Block of Gold";
        case BLOCK_DIAMOND_BLOCK: return "Block of Diamond";
        case BLOCK_EMERALD_BLOCK: return "Block of Emerald";
        case BLOCK_REDSTONE_BLOCK: return "Block of Redstone";
        case BLOCK_LAPIS_BLOCK: return "Block of Lapis Lazuli";
        case BLOCK_COAL_BLOCK: return "Block of Coal";
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
        case BLOCK_TERRACOTTA: return "Terracotta";
        case BLOCK_WHITE_TERRACOTTA: return "White Terracotta";
        case BLOCK_ORANGE_TERRACOTTA: return "Orange Terracotta";
        case BLOCK_MAGENTA_TERRACOTTA: return "Magenta Terracotta";
        case BLOCK_LIGHT_BLUE_TERRACOTTA: return "Light Blue Terracotta";
        case BLOCK_YELLOW_TERRACOTTA: return "Yellow Terracotta";
        case BLOCK_LIME_TERRACOTTA: return "Lime Terracotta";
        case BLOCK_PINK_TERRACOTTA: return "Pink Terracotta";
        case BLOCK_GRAY_TERRACOTTA: return "Gray Terracotta";
        case BLOCK_LIGHT_GRAY_TERRACOTTA: return "Light Gray Terracotta";
        case BLOCK_CYAN_TERRACOTTA: return "Cyan Terracotta";
        case BLOCK_PURPLE_TERRACOTTA: return "Purple Terracotta";
        case BLOCK_BLUE_TERRACOTTA: return "Blue Terracotta";
        case BLOCK_BROWN_TERRACOTTA: return "Brown Terracotta";
        case BLOCK_GREEN_TERRACOTTA: return "Green Terracotta";
        case BLOCK_RED_TERRACOTTA: return "Red Terracotta";
        case BLOCK_BLACK_TERRACOTTA: return "Black Terracotta";
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
        case BLOCK_QUARTZ_BLOCK: return "Block of Quartz";
        case BLOCK_CHISELED_QUARTZ_BLOCK: return "Chiseled Quartz Block";
        case BLOCK_QUARTZ_PILLAR: return "Quartz Pillar";
        case BLOCK_PACKED_ICE: return "Packed Ice";
        case BLOCK_BLUE_ICE: return "Blue Ice";
        case BLOCK_ICE: return "Ice";
        case BLOCK_SNOW_BLOCK: return "Snow Block";
        case BLOCK_CLAY: return "Clay";
        case BLOCK_HONEYCOMB_BLOCK: return "Honeycomb Block";
        case BLOCK_HAY_BLOCK: return "Hay Bale";
        case BLOCK_MELON: return "Melon";
        case BLOCK_PUMPKIN: return "Pumpkin";
        case BLOCK_JACK_O_LANTERN: return "Jack o'Lantern";
        case BLOCK_CACTUS: return "Cactus";
        case BLOCK_SPONGE: return "Sponge";
        case BLOCK_WET_SPONGE: return "Wet Sponge";
        default: return "Unknown Block";
    }
}

static const char* BlockIdOverride(BlockType block) {
    switch (block) {
        case BLOCK_IRON_BLOCK: return "iron_block";
        case BLOCK_GOLD_BLOCK: return "gold_block";
        case BLOCK_DIAMOND_BLOCK: return "diamond_block";
        case BLOCK_EMERALD_BLOCK: return "emerald_block";
        case BLOCK_REDSTONE_BLOCK: return "redstone_block";
        case BLOCK_LAPIS_BLOCK: return "lapis_block";
        case BLOCK_COAL_BLOCK: return "coal_block";
        case BLOCK_LAPIS_ORE: return "lapis_ore";
        case BLOCK_QUARTZ_BLOCK: return "quartz_block";
        case BLOCK_HAY_BLOCK: return "hay_block";
        case BLOCK_JACK_O_LANTERN: return "jack_o_lantern";
        default: return NULL;
    }
}

// Most ids are the English name in lower case with underscores.
const char* GetBlockId(BlockType block) {
    static char ids[BLOCK_COUNT][40] = { { 0 } };
    static bool ready = false;

    if (!ready) {
        for (int i = 0; i < BLOCK_COUNT; i++) {
            const char *name = BlockIdOverride((BlockType)i);
            int length = 0;
            bool literal = (name != NULL);

            if (!literal) name = GetBlockName((BlockType)i);
            for (int k = 0; (name[k] != '\0') && (length < (int)sizeof(ids[i]) - 1); k++) {
                char c = name[k];

                if (literal) ids[i][length++] = c;
                else if (c == ' ') ids[i][length++] = '_';
                else if (c != '\'') ids[i][length++] = (char)(((c >= 'A') && (c <= 'Z'))? c - 'A' + 'a' : c);
            }
            ids[i][length] = '\0';
        }
        ready = true;
    }
    if ((block < 0) || (block >= BLOCK_COUNT)) return "air";
    return ids[block];
}
