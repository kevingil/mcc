#include "chunk_codec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTION_HEIGHT 16
#define SECTION_COUNT (SAVE_CHUNK_HEIGHT/SECTION_HEIGHT)
#define SECTION_BLOCKS (SAVE_CHUNK_SIZE*SECTION_HEIGHT*SAVE_CHUNK_SIZE)
#define MAX_PALETTE 512
#define MAX_PACKED_LONGS 1024

static int BlockIndex(int x, int y, int z)
{
    return x + SAVE_CHUNK_SIZE*(z + SAVE_CHUNK_SIZE*y);
}

static int BitsForPalette(int paletteSize)
{
    int bits = 4;

    if (paletteSize < 1) paletteSize = 1;
    while ((bits < 16) && ((1 << bits) < paletteSize)) bits++;
    return bits;
}

static void WriteSection(NbtBuf *out, int sectionY, const int *blocks)
{
    int palette[MAX_PALETTE] = { 0 };
    int paletteOf[SECTION_BLOCKS] = { 0 };
    int paletteSize = 0;
    int i = 0;
    int y0 = sectionY*SECTION_HEIGHT;

    for (i = 0; i < SECTION_BLOCKS; i++)
    {
        int x = i%SAVE_CHUNK_SIZE;
        int z = (i/SAVE_CHUNK_SIZE)%SAVE_CHUNK_SIZE;
        int y = y0 + i/(SAVE_CHUNK_SIZE*SAVE_CHUNK_SIZE);
        int block = blocks[BlockIndex(x, y, z)];
        int found = -1;
        int p = 0;

        for (p = 0; p < paletteSize; p++)
        {
            if (palette[p] == block)
            {
                found = p;
                break;
            }
        }

        if (found < 0)
        {
            if (paletteSize >= MAX_PALETTE) found = 0;
            else
            {
                found = paletteSize;
                palette[paletteSize++] = block;
            }
        }

        paletteOf[i] = found;
    }

    NbtByte(out, "Y", sectionY);
    NbtStartCompound(out, "block_states");
    NbtStartList(out, "palette", NBT_COMPOUND, paletteSize);
    for (i = 0; i < paletteSize; i++)
    {
        char name[32] = { 0 };

        snprintf(name, sizeof(name), "opencraft:%d", palette[i]);
        NbtString(out, "Name", name);
        NbtInt(out, "Id", palette[i]);
        NbtEndCompound(out);
    }

    if (paletteSize > 1)
    {
        int bits = BitsForPalette(paletteSize);
        int perLong = 64/bits;
        int longCount = (SECTION_BLOCKS + perLong - 1)/perLong;
        uint64_t packed[MAX_PACKED_LONGS] = { 0 };

        if (longCount > MAX_PACKED_LONGS) longCount = MAX_PACKED_LONGS;
        for (i = 0; i < SECTION_BLOCKS; i++)
        {
            int longIndex = i/perLong;
            int shift = (i%perLong)*bits;

            if (longIndex >= longCount) break;
            packed[longIndex] |= ((uint64_t)paletteOf[i]) << shift;
        }

        NbtLongArray(out, "data", packed, longCount);
    }

    NbtEndCompound(out);
    NbtEndCompound(out);
}

bool ChunkCodecWrite(int chunkX, int chunkZ, const int *blocks, NbtBuf *out)
{
    int section = 0;

    if ((blocks == NULL) || (out == NULL)) return false;

    NbtStartCompound(out, "");
    NbtInt(out, "xPos", chunkX);
    NbtInt(out, "zPos", chunkZ);
    NbtStartList(out, "sections", NBT_COMPOUND, SECTION_COUNT);
    for (section = 0; section < SECTION_COUNT; section++) WriteSection(out, section, blocks);
    NbtEndCompound(out);
    return out->error == 0;
}

static int IdFromName(const char *name)
{
    const char *number = strrchr(name, ':');

    if (number == NULL) number = name;
    else number++;
    return atoi(number);
}

static void ApplySection(int *blocks, int sectionY, const int *palette, int paletteSize, const uint64_t *packed, int packedCount, bool hasData)
{
    int i = 0;
    int y0 = 0;
    int bits = 0;
    int perLong = 0;
    uint64_t mask = 0;

    if ((sectionY < 0) || (sectionY >= SECTION_COUNT) || (paletteSize <= 0)) return;
    y0 = sectionY*SECTION_HEIGHT;

    if (!hasData)
    {
        for (i = 0; i < SECTION_BLOCKS; i++)
        {
            int x = i%SAVE_CHUNK_SIZE;
            int z = (i/SAVE_CHUNK_SIZE)%SAVE_CHUNK_SIZE;
            int y = y0 + i/(SAVE_CHUNK_SIZE*SAVE_CHUNK_SIZE);
            blocks[BlockIndex(x, y, z)] = palette[0];
        }
        return;
    }

    bits = BitsForPalette(paletteSize);
    perLong = 64/bits;
    mask = (bits >= 64) ? ~0ull : ((1ull << bits) - 1ull);

    for (i = 0; i < SECTION_BLOCKS; i++)
    {
        int longIndex = i/perLong;
        int shift = (i%perLong)*bits;
        int paletteIndex = 0;
        int x = i%SAVE_CHUNK_SIZE;
        int z = (i/SAVE_CHUNK_SIZE)%SAVE_CHUNK_SIZE;
        int y = y0 + i/(SAVE_CHUNK_SIZE*SAVE_CHUNK_SIZE);

        if (longIndex < packedCount) paletteIndex = (int)((packed[longIndex] >> shift) & mask);
        if ((paletteIndex < 0) || (paletteIndex >= paletteSize)) paletteIndex = 0;
        blocks[BlockIndex(x, y, z)] = palette[paletteIndex];
    }
}

static void ReadBlockStates(NbtIn *in, int *palette, int *paletteSize, uint64_t *packed, int *packedCount, bool *hasData)
{
    char name[64] = { 0 };
    int type = 0;

    *paletteSize = 0;
    *packedCount = 0;
    *hasData = false;

    while (NbtInNext(in, name, sizeof(name), &type))
    {
        if ((strcmp(name, "palette") == 0) && (type == NBT_LIST))
        {
            int element = 0;
            int count = 0;
            int i = 0;

            NbtInListHeader(in, &element, &count);
            for (i = 0; (i < count) && !in->error; i++)
            {
                char child[64] = { 0 };
                int childType = 0;
                int id = 0;
                bool sawId = false;
                char blockName[64] = { 0 };

                if (element != NBT_COMPOUND)
                {
                    NbtInSkip(in, element);
                    continue;
                }

                while (NbtInNext(in, child, sizeof(child), &childType))
                {
                    if ((strcmp(child, "Id") == 0) && (childType == NBT_INT))
                    {
                        id = NbtInI32(in);
                        sawId = true;
                    }
                    else if ((strcmp(child, "Name") == 0) && (childType == NBT_STRING))
                    {
                        NbtInReadString(in, blockName, sizeof(blockName));
                    }
                    else NbtInSkip(in, childType);
                }

                if (!sawId) id = IdFromName(blockName);
                if (*paletteSize < MAX_PALETTE) palette[(*paletteSize)++] = id;
            }
        }
        else if ((strcmp(name, "data") == 0) && (type == NBT_LONG_ARRAY))
        {
            int count = NbtInI32(in);
            int i = 0;

            if ((count < 0) || (count > MAX_PACKED_LONGS))
            {
                in->error = 1;
                return;
            }

            for (i = 0; i < count; i++) packed[i] = (uint64_t)NbtInI64(in);
            *packedCount = count;
            *hasData = true;
        }
        else NbtInSkip(in, type);
    }
}

bool ChunkCodecRead(const unsigned char *data, int size, int *chunkX, int *chunkZ, int *blocks)
{
    NbtIn in = { 0 };
    char name[64] = { 0 };
    int type = 0;
    int root = 0;
    bool sawX = false;
    bool sawZ = false;

    if ((data == NULL) || (size <= 0) || (blocks == NULL)) return false;

    NbtInInit(&in, data, size);
    root = NbtInU8(&in);
    NbtInReadString(&in, name, sizeof(name));
    if ((in.error) || (root != NBT_COMPOUND)) return false;

    memset(blocks, 0, sizeof(int)*SAVE_BLOCKS_PER_CHUNK);

    while (NbtInNext(&in, name, sizeof(name), &type))
    {
        if ((strcmp(name, "xPos") == 0) && (type == NBT_INT))
        {
            if (chunkX != NULL) *chunkX = NbtInI32(&in);
            else NbtInI32(&in);
            sawX = true;
        }
        else if ((strcmp(name, "zPos") == 0) && (type == NBT_INT))
        {
            if (chunkZ != NULL) *chunkZ = NbtInI32(&in);
            else NbtInI32(&in);
            sawZ = true;
        }
        else if ((strcmp(name, "sections") == 0) && (type == NBT_LIST))
        {
            int element = 0;
            int count = 0;
            int i = 0;

            NbtInListHeader(&in, &element, &count);
            for (i = 0; (i < count) && !in.error; i++)
            {
                int palette[MAX_PALETTE] = { 0 };
                int paletteSize = 0;
                uint64_t packed[MAX_PACKED_LONGS] = { 0 };
                int packedCount = 0;
                bool hasData = false;
                int sectionY = -1;
                char child[64] = { 0 };
                int childType = 0;

                if (element != NBT_COMPOUND)
                {
                    NbtInSkip(&in, element);
                    continue;
                }

                while (NbtInNext(&in, child, sizeof(child), &childType))
                {
                    if ((strcmp(child, "Y") == 0) && (childType == NBT_BYTE))
                    {
                        sectionY = NbtInU8(&in);
                    }
                    else if ((strcmp(child, "block_states") == 0) && (childType == NBT_COMPOUND))
                    {
                        ReadBlockStates(&in, palette, &paletteSize, packed, &packedCount, &hasData);
                    }
                    else NbtInSkip(&in, childType);
                }

                ApplySection(blocks, sectionY, palette, paletteSize, packed, packedCount, hasData);
            }
        }
        else NbtInSkip(&in, type);
    }

    return (!in.error && sawX && sawZ);
}
