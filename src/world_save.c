#include "world_save.h"
#include "anvil.h"
#include "chunk_codec.h"
#include "client_log.h"
#include "level_data.h"
#include "world_catalog.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef char SaveChunkSizeCheck[(CHUNK_SIZE == SAVE_CHUNK_SIZE) ? 1 : -1];
typedef char SaveChunkHeightCheck[(WORLD_HEIGHT == SAVE_CHUNK_HEIGHT) ? 1 : -1];

static char saveFolder[128] = "";
static bool bound = false;
static LevelData currentLevel;

static int BlockIndex(int x, int y, int z)
{
    return x + SAVE_CHUNK_SIZE*(z + SAVE_CHUNK_SIZE*y);
}

void WorldSaveBind(const char *folder)
{
    bound = false;
    saveFolder[0] = '\0';
    LevelDataInit(&currentLevel);

    if ((folder == NULL) || (folder[0] == '\0')) return;

    snprintf(saveFolder, sizeof(saveFolder), "%s", folder);
    bound = true;
    if (!LevelDataRead(saveFolder, &currentLevel))
    {
        LevelDataInit(&currentLevel);
        LevelDataSetMeta(&currentLevel, GetActiveWorldName(), "Survival", "0.1.0", GetActiveWorldSeed(), 0);
    }

    ClientLog("Loaded world '%s'", currentLevel.name);
}

bool WorldSaveHasPlayer(void)
{
    return bound && currentLevel.hasPlayer;
}

Vector3 WorldSavePlayerPosition(void)
{
    return (Vector3){ (float)currentLevel.posX, (float)currentLevel.posY, (float)currentLevel.posZ };
}

void WorldSaveSetSpawn(int x, int y, int z)
{
    if (!bound) return;
    currentLevel.spawnX = x;
    currentLevel.spawnY = y;
    currentLevel.spawnZ = z;
}

void WorldSaveApplyPlayer(Player *player)
{
    int i = 0;
    Vector3 forward = { 0 };

    if ((player == NULL) || !currentLevel.hasPlayer) return;

    player->position = WorldSavePlayerPosition();
    player->velocity = (Vector3){ (float)currentLevel.motionX, (float)currentLevel.motionY, (float)currentLevel.motionZ };
    player->yaw = currentLevel.yaw*(PI/180.0f);
    player->pitch = currentLevel.pitch*(PI/180.0f);

    for (i = 0; i < HOTBAR_SIZE; i++) player->hotbar[i] = BLOCK_AIR;
    for (i = 0; i < INVENTORY_SIZE; i++)
    {
        player->inventory.blocks[i] = BLOCK_AIR;
        player->inventory.quantities[i] = 0;
    }

    for (i = 0; i < currentLevel.itemCount; i++)
    {
        LevelItem *item = &currentLevel.items[i];
        int block = item->block;

        if ((block < 0) || (block >= BLOCK_COUNT)) block = BLOCK_AIR;
        if ((item->slot >= 0) && (item->slot < HOTBAR_SIZE))
        {
            player->hotbar[item->slot] = (BlockType)block;
        }
        else if ((item->slot >= HOTBAR_SIZE) && (item->slot < HOTBAR_SIZE + INVENTORY_SIZE))
        {
            int index = item->slot - HOTBAR_SIZE;
            player->inventory.blocks[index] = (BlockType)block;
            player->inventory.quantities[index] = item->count;
        }
    }

    if ((currentLevel.selectedSlot >= 0) && (currentLevel.selectedSlot < HOTBAR_SIZE))
    {
        player->hotbarSlot = currentLevel.selectedSlot;
    }
    player->selectedBlock = player->hotbar[player->hotbarSlot];

    player->camera.position = Vector3Add(player->position, (Vector3){ 0.0f, 1.62f, 0.0f });
    forward = (Vector3){
        cosf(player->pitch)*sinf(player->yaw),
        sinf(player->pitch),
        cosf(player->pitch)*cosf(player->yaw)
    };
    player->camera.target = Vector3Add(player->camera.position, forward);
}

static void CapturePlayer(const Player *player)
{
    int i = 0;

    currentLevel.hasPlayer = true;
    currentLevel.posX = (double)player->position.x;
    currentLevel.posY = (double)player->position.y;
    currentLevel.posZ = (double)player->position.z;
    currentLevel.motionX = (double)player->velocity.x;
    currentLevel.motionY = (double)player->velocity.y;
    currentLevel.motionZ = (double)player->velocity.z;
    currentLevel.yaw = player->yaw*(180.0f/PI);
    currentLevel.pitch = player->pitch*(180.0f/PI);
    currentLevel.selectedSlot = player->hotbarSlot;
    currentLevel.itemCount = 0;

    for (i = 0; i < HOTBAR_SIZE; i++)
    {
        if (player->hotbar[i] == BLOCK_AIR) continue;
        if (currentLevel.itemCount >= LEVEL_ITEM_MAX) break;
        currentLevel.items[currentLevel.itemCount].slot = i;
        currentLevel.items[currentLevel.itemCount].block = (int)player->hotbar[i];
        currentLevel.items[currentLevel.itemCount].count = 64;
        currentLevel.itemCount++;
    }

    for (i = 0; i < INVENTORY_SIZE; i++)
    {
        if (player->inventory.quantities[i] <= 0) continue;
        if (player->inventory.blocks[i] == BLOCK_AIR) continue;
        if (currentLevel.itemCount >= LEVEL_ITEM_MAX) break;
        currentLevel.items[currentLevel.itemCount].slot = HOTBAR_SIZE + i;
        currentLevel.items[currentLevel.itemCount].block = (int)player->inventory.blocks[i];
        currentLevel.items[currentLevel.itemCount].count = player->inventory.quantities[i];
        currentLevel.itemCount++;
    }
}

bool WorldSaveLoadChunk(Chunk *chunk)
{
    unsigned char *nbt = NULL;
    int nbtSize = 0;
    int status = 0;
    int savedX = 0;
    int savedZ = 0;
    static int blocks[SAVE_BLOCKS_PER_CHUNK];
    int x = 0;
    int y = 0;
    int z = 0;

    if ((!bound) || (chunk == NULL)) return false;

    status = AnvilReadChunk(saveFolder, chunk->position.x, chunk->position.z, &nbt, &nbtSize);
    if (status == 0) return false;
    if (status < 0)
    {
        ClientLog("Chunk %d,%d could not be read", chunk->position.x, chunk->position.z);
        return false;
    }

    if (!ChunkCodecRead(nbt, nbtSize, &savedX, &savedZ, blocks))
    {
        NbtFreeBytes(nbt);
        ClientLog("Chunk %d,%d could not be decoded", chunk->position.x, chunk->position.z);
        return false;
    }
    NbtFreeBytes(nbt);

    if ((savedX != chunk->position.x) || (savedZ != chunk->position.z)) return false;

    for (x = 0; x < CHUNK_SIZE; x++)
    {
        for (y = 0; y < WORLD_HEIGHT; y++)
        {
            for (z = 0; z < CHUNK_SIZE; z++)
            {
                int block = blocks[BlockIndex(x, y, z)];
                if ((block < 0) || (block >= BLOCK_COUNT)) block = BLOCK_AIR;
                chunk->blocks[x][y][z] = (BlockType)block;
            }
        }
    }

    return true;
}

bool WorldSaveStoreChunk(const Chunk *chunk)
{
    static int blocks[SAVE_BLOCKS_PER_CHUNK];
    NbtBuf buf = { 0 };
    int x = 0;
    int y = 0;
    int z = 0;
    bool ok = false;

    if ((!bound) || (chunk == NULL)) return false;

    for (x = 0; x < CHUNK_SIZE; x++)
    {
        for (y = 0; y < WORLD_HEIGHT; y++)
        {
            for (z = 0; z < CHUNK_SIZE; z++)
            {
                blocks[BlockIndex(x, y, z)] = (int)chunk->blocks[x][y][z];
            }
        }
    }

    NbtBufInit(&buf);
    if (!ChunkCodecWrite(chunk->position.x, chunk->position.z, blocks, &buf))
    {
        NbtBufFree(&buf);
        return false;
    }

    ok = AnvilWriteChunk(saveFolder, chunk->position.x, chunk->position.z, buf.data, buf.size);
    NbtBufFree(&buf);
    return ok;
}

void WorldSaveFlush(const Player *player, VoxelWorld *world)
{
    int savedChunks = 0;
    int i = 0;

    if (!bound) return;

    if (world != NULL)
    {
        for (i = 0; i < MAX_CHUNKS; i++)
        {
            Chunk *chunk = &world->chunks[i];

            if (!chunk->isLoaded || !chunk->modified) continue;
            if (WorldSaveStoreChunk(chunk))
            {
                chunk->modified = false;
                savedChunks++;
            }
        }
    }

    if (player != NULL) CapturePlayer(player);
    currentLevel.lastPlayed = 0;
    if (LevelDataWrite(saveFolder, &currentLevel))
    {
        ClientLog("Saved world '%s' at %.1f %.1f %.1f (%d chunks)", currentLevel.name,
            currentLevel.posX, currentLevel.posY, currentLevel.posZ, savedChunks);
    }
}
