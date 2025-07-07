#include "player.h"
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
    player->hotbar[5] = BLOCK_WATER;
    player->hotbar[6] = BLOCK_COBBLESTONE;
    player->hotbar[7] = BLOCK_SAND;
    player->hotbar[8] = BLOCK_BRICKS;
    
    // Initialize inventory system
    player->inventoryOpen = false;
    player->inventorySelectedSlot = 0;
    player->inventoryScrollOffset = 0;
    
    // Fill inventory with all available blocks
    int slotIndex = 0;
    for (int i = 1; i < BLOCK_COUNT && slotIndex < INVENTORY_SIZE; i++) {
        player->inventory.blocks[slotIndex] = (BlockType)i;
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

void UpdatePlayer(Player* player, VoxelWorld* world) {
    HandlePlayerInput(player);
    UpdatePlayerPhysics(player, world);
    UpdatePlayerInteraction(player, world);
    
    // Update camera position
    player->camera.position = Vector3Add(player->position, (Vector3){0, PLAYER_HEIGHT * 0.9f, 0});
}

void HandlePlayerInput(Player* player) {
    HandlePlayerMouseLook(player);
    HandlePlayerMovement(player);
    
    // Inventory toggle with E key
    if (IsKeyPressed(KEY_E)) {
        player->inventoryOpen = !player->inventoryOpen;
        if (player->inventoryOpen) {
            EnableCursor(); // Show cursor in inventory
        } else {
            DisableCursor(); // Hide cursor when closing inventory
        }
    }
    
    // Inventory navigation (only when inventory is open)
    if (player->inventoryOpen) {
        // Arrow key navigation
        if (IsKeyPressed(KEY_LEFT)) {
            player->inventorySelectedSlot = (player->inventorySelectedSlot - 1 + INVENTORY_SIZE) % INVENTORY_SIZE;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            player->inventorySelectedSlot = (player->inventorySelectedSlot + 1) % INVENTORY_SIZE;
        }
        if (IsKeyPressed(KEY_UP)) {
            player->inventorySelectedSlot = (player->inventorySelectedSlot - INVENTORY_COLS + INVENTORY_SIZE) % INVENTORY_SIZE;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            player->inventorySelectedSlot = (player->inventorySelectedSlot + INVENTORY_COLS) % INVENTORY_SIZE;
        }
        
        // Select block from inventory with Enter
        if (IsKeyPressed(KEY_ENTER) && player->inventory.blocks[player->inventorySelectedSlot] != BLOCK_AIR) {
            player->selectedBlock = player->inventory.blocks[player->inventorySelectedSlot];
            // Place in current hotbar slot (replace whatever is there)
            player->hotbar[player->hotbarSlot] = player->selectedBlock;
        }
        
        // Mouse click selection in inventory
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            int mouseSlot = GetInventorySlotAtMouse(mousePos);
            if (mouseSlot >= 0 && mouseSlot < INVENTORY_SIZE) {
                player->inventorySelectedSlot = mouseSlot;
                if (player->inventory.blocks[mouseSlot] != BLOCK_AIR) {
                    player->selectedBlock = player->inventory.blocks[mouseSlot];
                    // Place in current hotbar slot (replace whatever is there)
                    player->hotbar[player->hotbarSlot] = player->selectedBlock;
                }
            }
        }
    }
    
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

void UpdatePlayerPhysics(Player* player, VoxelWorld* world) {
    float deltaTime = GetFrameTime();
    
    // Apply gravity
    ApplyGravity(player);
    
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
    player->velocity.y -= GRAVITY * deltaTime;
    
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

void UpdatePlayerInteraction(Player* player, VoxelWorld* world) {
    UpdateBlockTarget(player, world);
    
    if (IsCursorHidden()) {
        // Block breaking (left click)
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            HandleBlockBreaking(player, world);
        }
        
        // Block placement (right click)
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            HandleBlockPlacement(player, world);
        }
    }
}

void UpdateBlockTarget(Player* player, VoxelWorld* world) {
    Vector3 rayOrigin = player->camera.position;
    Vector3 rayDirection = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
    
    Vector3 hitNormal;
    player->hasTarget = RaycastToBlock(rayOrigin, rayDirection, world, &player->targetBlock, &hitNormal);
}

void HandleBlockPlacement(Player* player, VoxelWorld* world) {
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
            SetBlock(world, placePos, player->selectedBlock);
        }
    }
}

void HandleBlockBreaking(Player* player, VoxelWorld* world) {
    if (!player->hasTarget) return;
    
    BlockType currentBlock = GetBlock(world, player->targetBlock);
    if (currentBlock != BLOCK_AIR) {
        SetBlock(world, player->targetBlock, BLOCK_AIR);
    }
}

bool RaycastToBlock(Vector3 origin, Vector3 direction, VoxelWorld* world, BlockPos* hitBlock, Vector3* hitNormal) {
    Vector3 rayPos = origin;
    Vector3 rayStep = Vector3Scale(Vector3Normalize(direction), 0.1f);
    
    for (float distance = 0; distance < REACH_DISTANCE; distance += 0.1f) {
        BlockPos currentBlock = WorldToBlock(rayPos);
        BlockType block = GetBlock(world, currentBlock);
        
        if (IsBlockSolid(block)) {
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
void DrawPlayerUI(Player* player) {
    DrawCrosshair();
    DrawHotbar(player);
    
    if (player->hasTarget) {
        DrawBlockOutline(player->targetBlock);
    }
    
    // Draw inventory if open
    if (player->inventoryOpen) {
        DrawInventory(player);
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

void DrawHotbar(Player* player) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int slotSize = 40;
    int hotbarWidth = 9 * slotSize;
    int startX = (screenWidth - hotbarWidth) / 2;
    int startY = screenHeight - slotSize - 20;
    
    for (int i = 0; i < 9; i++) {
        int x = startX + i * slotSize;
        int y = startY;
        
        // Draw slot background
        Color slotColor = (i == player->hotbarSlot) ? YELLOW : GRAY;
        DrawRectangle(x, y, slotSize, slotSize, slotColor);
        DrawRectangleLines(x, y, slotSize, slotSize, WHITE);
        
        // Draw block color
        if (player->hotbar[i] != BLOCK_AIR) {
            Color blockColor = GetBlockColor(player->hotbar[i]);
            DrawRectangle(x + 5, y + 5, slotSize - 10, slotSize - 10, blockColor);
        }
        
        // Draw slot number
        DrawText(TextFormat("%d", i + 1), x + 2, y + 2, 10, WHITE);
    }
}

void DrawBlockOutline(BlockPos position) {
    Vector3 blockPos = {position.x, position.y, position.z};
    Vector3 size = {1.0f, 1.0f, 1.0f};
    DrawCubeWires(Vector3Add(blockPos, Vector3Scale(size, 0.5f)), size.x, size.y, size.z, RED);
}

//----------------------------------------------------------------------------------
// Inventory UI Functions
//----------------------------------------------------------------------------------
void DrawInventory(Player* player) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Inventory background
    int inventoryWidth = 600;
    int inventoryHeight = 400;
    int inventoryX = (screenWidth - inventoryWidth) / 2;
    int inventoryY = (screenHeight - inventoryHeight) / 2;
    
    // Draw semi-transparent background
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
    
    // Draw inventory window
    DrawRectangle(inventoryX, inventoryY, inventoryWidth, inventoryHeight, (Color){50, 50, 50, 240});
    DrawRectangleLines(inventoryX, inventoryY, inventoryWidth, inventoryHeight, WHITE);
    
    // Title
    DrawText("INVENTORY", inventoryX + 20, inventoryY + 15, 24, WHITE);
    DrawText("Use arrow keys to navigate, ENTER to select, E to close", inventoryX + 20, inventoryY + 45, 16, LIGHTGRAY);
    
    // Calculate slot dimensions
    int slotSize = 50;
    int slotSpacing = 5;
    int startX = inventoryX + 50;
    int startY = inventoryY + 80;
    
    // Draw inventory grid
    for (int row = 0; row < INVENTORY_ROWS; row++) {
        for (int col = 0; col < INVENTORY_COLS; col++) {
            int slotIndex = row * INVENTORY_COLS + col;
            int x = startX + col * (slotSize + slotSpacing);
            int y = startY + row * (slotSize + slotSpacing);
            
            // Slot background
            Color slotColor = (slotIndex == player->inventorySelectedSlot) ? YELLOW : GRAY;
            DrawRectangle(x, y, slotSize, slotSize, slotColor);
            DrawRectangleLines(x, y, slotSize, slotSize, WHITE);
            
            // Draw block if not air
            if (player->inventory.blocks[slotIndex] != BLOCK_AIR) {
                Color blockColor = GetBlockColor(player->inventory.blocks[slotIndex]);
                DrawRectangle(x + 5, y + 5, slotSize - 10, slotSize - 10, blockColor);
                
                // Draw quantity if more than 1
                if (player->inventory.quantities[slotIndex] > 1) {
                    DrawText(TextFormat("%d", player->inventory.quantities[slotIndex]), 
                             x + slotSize - 15, y + slotSize - 15, 12, WHITE);
                }
            }
        }
    }
    
    // Draw selected block info
    if (player->inventory.blocks[player->inventorySelectedSlot] != BLOCK_AIR) {
        const char* blockName = GetBlockName(player->inventory.blocks[player->inventorySelectedSlot]);
        DrawText(TextFormat("Selected: %s", blockName), 
                 inventoryX + 20, inventoryY + inventoryHeight - 80, 18, WHITE);
        DrawText(TextFormat("Quantity: %d", player->inventory.quantities[player->inventorySelectedSlot]), 
                 inventoryX + 20, inventoryY + inventoryHeight - 60, 16, LIGHTGRAY);
    }
    
    // Instructions
    DrawText("Click on a block to select it", inventoryX + 20, inventoryY + inventoryHeight - 40, 14, LIGHTGRAY);
    DrawText("Selected blocks will be added to your hotbar", inventoryX + 20, inventoryY + inventoryHeight - 25, 14, LIGHTGRAY);
}

int GetInventorySlotAtMouse(Vector2 mousePos) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    int inventoryWidth = 600;
    int inventoryHeight = 400;
    int inventoryX = (screenWidth - inventoryWidth) / 2;
    int inventoryY = (screenHeight - inventoryHeight) / 2;
    
    int slotSize = 50;
    int slotSpacing = 5;
    int startX = inventoryX + 50;
    int startY = inventoryY + 80;
    
    // Check if mouse is within inventory area
    if (mousePos.x < startX || mousePos.x > startX + INVENTORY_COLS * (slotSize + slotSpacing) ||
        mousePos.y < startY || mousePos.y > startY + INVENTORY_ROWS * (slotSize + slotSpacing)) {
        return -1;
    }
    
    // Calculate which slot the mouse is over
    int col = (mousePos.x - startX) / (slotSize + slotSpacing);
    int row = (mousePos.y - startY) / (slotSize + slotSpacing);
    
    if (col >= 0 && col < INVENTORY_COLS && row >= 0 && row < INVENTORY_ROWS) {
        return row * INVENTORY_COLS + col;
    }
    
    return -1;
}

const char* GetBlockName(BlockType block) {
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
