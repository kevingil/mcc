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
    
    // Initialize hotbar
    player->hotbar[0] = BLOCK_GRASS;
    player->hotbar[1] = BLOCK_DIRT;
    player->hotbar[2] = BLOCK_STONE;
    player->hotbar[3] = BLOCK_WOOD;
    player->hotbar[4] = BLOCK_LEAVES;
    player->hotbar[5] = BLOCK_WATER;
    player->hotbar[6] = BLOCK_AIR;
    player->hotbar[7] = BLOCK_AIR;
    player->hotbar[8] = BLOCK_AIR;
    
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
    
    // Hotbar selection
    for (int i = 0; i < 9; i++) {
        if (IsKeyPressed(KEY_ONE + i)) {
            player->hotbarSlot = i;
            player->selectedBlock = player->hotbar[i];
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
            cosf(player->yaw),   // X (90 degrees rotated from forward)
            0,                   // Y (always 0 for horizontal movement)
            -sinf(player->yaw)   // Z (90 degrees rotated from forward)
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
