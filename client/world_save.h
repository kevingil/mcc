#ifndef WORLD_SAVE_H
#define WORLD_SAVE_H

#include "player.h"

#include <stdbool.h>

// Binds the active world folder, then reads level.dat.
// Edited chunks go to region/r.<x>.<z>.mca. Unedited chunks stay generated.
void WorldSaveBind(const char *folder);
unsigned int WorldSaveSeed(void);
bool WorldSaveHasPlayer(void);
Vector3 WorldSavePlayerPosition(void);
void WorldSaveSetSpawn(int x, int y, int z);
void WorldSaveApplyPlayer(Player *player);
bool WorldSaveLoadChunk(Chunk *chunk);
bool WorldSaveStoreChunk(const Chunk *chunk);
void WorldSaveFlush(const Player *player, VoxelWorld *world);

#endif
