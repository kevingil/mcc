#ifndef PLAYER_H
#define PLAYER_H

#include "voxel_types.h"
#include "voxel_world.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------
// Player Functions
//----------------------------------------------------------------------------------
void InitPlayer(Player* player, Vector3 startPosition);
void UpdatePlayer(Player* player, VoxelWorld* world);
void HandlePlayerInput(Player* player);
void UpdatePlayerPhysics(Player* player, VoxelWorld* world);
void UpdatePlayerInteraction(Player* player, VoxelWorld* world);

// Movement functions
void HandlePlayerMovement(Player* player);
void HandlePlayerMouseLook(Player* player);
void ApplyGravity(Player* player);
bool CheckCollision(Player* player, VoxelWorld* world, Vector3 newPosition);

// Block interaction
void UpdateBlockTarget(Player* player, VoxelWorld* world);
void HandleBlockPlacement(Player* player, VoxelWorld* world);
void HandleBlockBreaking(Player* player, VoxelWorld* world);
bool RaycastToBlock(Vector3 origin, Vector3 direction, VoxelWorld* world, BlockPos* hitBlock, Vector3* hitNormal);

// UI functions
void DrawPlayerUI(Player* player);
void DrawCrosshair(void);
void DrawHotbar(Player* player);
void DrawBlockOutline(BlockPos position);

#ifdef __cplusplus
}
#endif

#endif // PLAYER_H 
