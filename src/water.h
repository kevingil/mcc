#ifndef WATER_H
#define WATER_H

#include "voxel_world.h"

// Source blocks spread 7 steps, fall down holes, and form a new source
// where two sources meet on a solid block. A bucket picks up sources.
void WaterNotify(VoxelWorld *world, BlockPos position);
void WaterWakeChunk(VoxelWorld *world, Chunk *chunk);
void UpdateWater(VoxelWorld *world);

#endif
