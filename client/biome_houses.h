#ifndef BIOME_HOUSES_H
#define BIOME_HOUSES_H

#include "voxel_types.h"
#include "player.h"

void BiomeHousesStamp(Chunk *chunk, int localX, int localZ, int worldX, int worldZ, int surfaceY);
int BiomeHouseTourStep(Player *player, float dt);

#endif
