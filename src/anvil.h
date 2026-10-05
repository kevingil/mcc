#ifndef ANVIL_H
#define ANVIL_H

#include <stdbool.h>

// Anvil region files: saves/world_<id>/region/r.<x>.<z>.mca
// 8192-byte header (1024 locations, 1024 timestamps), then 4 KiB sectors.
// A chunk record is a big-endian length, a compression byte (2 = zlib),
// and the compressed NBT from ChunkCodecWrite.

bool AnvilWriteChunk(const char *worldFolder, int chunkX, int chunkZ, const unsigned char *nbt, int nbtSize);
int AnvilReadChunk(const char *worldFolder, int chunkX, int chunkZ, unsigned char **outNbt, int *outSize);
void AnvilRemoveRegions(const char *worldFolder);

#endif
