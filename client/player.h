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
void SetPlayerLook(Player* player, float yaw, float pitch);
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
void DrawBlockOutline(BlockPos position);
void SelectHotbarSlot(Player* player, int slot);
int StowItem(Player* player, BlockType block, int count);

// Inventory functions. craftSize 3 opens the crafting table, 2 the player's
// own screen (the creative tabs in creative mode).
void InventoryOpen(Player* player, int craftSize);
void DrawInventory(Player* player);
void InventoryHandleInput(Player* player);
void InventoryClose(Player* player);
void InventorySaveHotbar(Player* player, int row);
void InventoryLoadHotbar(Player* player, int row);
const char* GetBlockName(BlockType block);
// The Java Edition item id without its namespace, such as "grass_block".
const char* GetBlockId(BlockType block);

#ifdef __cplusplus
}
#endif

#endif // PLAYER_H 
