#ifndef CHUNK_CODEC_H
#define CHUNK_CODEC_H

#include "nbt.h"

#include <stdbool.h>

// One saved chunk is an NBT compound inside a region sector.
// sections[] hold 16-block slices. block_states uses a palette and,
// when the palette has more than one entry, a long array packed from
// the low bits of each long, 4 bits minimum.
#define SAVE_CHUNK_SIZE 16
#define SAVE_CHUNK_HEIGHT 128
#define SAVE_BLOCKS_PER_CHUNK (SAVE_CHUNK_SIZE*SAVE_CHUNK_HEIGHT*SAVE_CHUNK_SIZE)

// blocks[x + 16*(z + 16*y)] is the block id.
bool ChunkCodecWrite(int chunkX, int chunkZ, const int *blocks, NbtBuf *out);
bool ChunkCodecRead(const unsigned char *data, int size, int *chunkX, int *chunkZ, int *blocks);

#endif
