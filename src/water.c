#include "water.h"

#include <stddef.h>

#define WATER_QUEUE_CAP 8192
#define WATER_UPDATES_PER_TICK 10

static BlockPos waterQueue[WATER_QUEUE_CAP];
static int waterHead = 0;
static int waterCount = 0;

static const int waterDirs[4][2] = {
    { 1, 0 },
    { -1, 0 },
    { 0, 1 },
    { 0, -1 }
};

static bool InWorld(BlockPos pos)
{
    return (pos.y >= 0) && (pos.y < WORLD_HEIGHT);
}

static bool ChunkIsLoaded(VoxelWorld *world, BlockPos pos)
{
    ChunkPos chunkPos = { 0 };
    Vector3 sample = { 0 };

    if (!InWorld(pos)) return false;

    sample = (Vector3){ (float)pos.x, (float)pos.y, (float)pos.z };
    chunkPos = WorldToChunk(sample);
    return GetChunk(world, chunkPos) != NULL;
}

static void Enqueue(BlockPos pos)
{
    int slot = 0;

    if (!InWorld(pos)) return;
    if (waterCount >= WATER_QUEUE_CAP) return;

    slot = (waterHead + waterCount)%WATER_QUEUE_CAP;
    waterQueue[slot] = pos;
    waterCount++;
}

void WaterNotify(VoxelWorld *world, BlockPos position)
{
    (void)world;
    Enqueue(position);
    Enqueue((BlockPos){ position.x + 1, position.y, position.z });
    Enqueue((BlockPos){ position.x - 1, position.y, position.z });
    Enqueue((BlockPos){ position.x, position.y, position.z + 1 });
    Enqueue((BlockPos){ position.x, position.y, position.z - 1 });
    Enqueue((BlockPos){ position.x, position.y + 1, position.z });
    Enqueue((BlockPos){ position.x, position.y - 1, position.z });
}

void WaterWakeChunk(VoxelWorld *world, Chunk *chunk)
{
    int originX = 0;
    int originZ = 0;

    if ((world == NULL) || (chunk == NULL)) return;

    originX = chunk->position.x*CHUNK_SIZE;
    originZ = chunk->position.z*CHUNK_SIZE;

    for (int x = 0; x < CHUNK_SIZE; x++)
    {
        for (int z = 0; z < CHUNK_SIZE; z++)
        {
            for (int y = 0; y < WORLD_HEIGHT; y++)
            {
                BlockType block = chunk->blocks[x][y][z];

                if (IsWaterBlock(block) && (block != BLOCK_WATER))
                {
                    Enqueue((BlockPos){ originX + x, y, originZ + z });
                }
            }
        }
    }
}

static void SetWater(VoxelWorld *world, BlockPos pos, BlockType block)
{
    if (!ChunkIsLoaded(world, pos)) return;
    if (GetBlock(world, pos) == block) return;
    SetBlock(world, pos, block);
}

static int SourceNeighbors(VoxelWorld *world, BlockPos pos)
{
    int count = 0;

    for (int i = 0; i < 4; i++)
    {
        BlockPos next = { pos.x + waterDirs[i][0], pos.y, pos.z + waterDirs[i][1] };
        if (GetBlock(world, next) == BLOCK_WATER) count++;
    }

    return count;
}

static void TrySpread(VoxelWorld *world, BlockPos pos, int level)
{
    BlockType here = BLOCK_AIR;
    int current = 0;

    if (!ChunkIsLoaded(world, pos)) return;

    here = GetBlock(world, pos);
    if (here == BLOCK_AIR)
    {
        SetWater(world, pos, BlockFromWaterLevel(level));
        return;
    }

    if (!IsWaterBlock(here) || (here == BLOCK_WATER)) return;

    current = WaterSpreadLevel(here);
    if ((current != 8) && (current > level)) SetWater(world, pos, BlockFromWaterLevel(level));
}

static void FlowFrom(VoxelWorld *world, BlockPos pos, int level)
{
    BlockPos below = { pos.x, pos.y - 1, pos.z };
    BlockType ground = BLOCK_STONE;
    int next = 0;

    if (below.y >= 0)
    {
        ground = GetBlock(world, below);
        if (ground == BLOCK_AIR) SetWater(world, below, BLOCK_WATER_FALL);
    }

    if ((level == 8) && (ground == BLOCK_AIR)) return;
    if ((level > 0) && (level < 8) && (ground == BLOCK_AIR)) return;

    next = ((level == 0) || (level == 8)) ? 1 : level + 1;
    if (next > 7) return;

    for (int i = 0; i < 4; i++)
    {
        BlockPos side = { pos.x + waterDirs[i][0], pos.y, pos.z + waterDirs[i][1] };
        TrySpread(world, side, next);
    }
}

static void Settle(VoxelWorld *world, BlockPos pos)
{
    BlockType here = BLOCK_AIR;
    BlockPos above = { 0 };
    BlockPos below = { 0 };
    BlockType up = BLOCK_AIR;
    BlockType down = BLOCK_STONE;
    bool canFill = false;
    bool supported = false;
    int best = 99;

    if (!ChunkIsLoaded(world, pos)) return;

    here = GetBlock(world, pos);
    if (here == BLOCK_WATER)
    {
        FlowFrom(world, pos, 0);
        return;
    }

    above = (BlockPos){ pos.x, pos.y + 1, pos.z };
    below = (BlockPos){ pos.x, pos.y - 1, pos.z };
    up = (above.y < WORLD_HEIGHT) ? GetBlock(world, above) : BLOCK_AIR;
    down = (below.y >= 0) ? GetBlock(world, below) : BLOCK_STONE;
    canFill = (here == BLOCK_AIR) || (IsWaterBlock(here) && (here != BLOCK_WATER));
    supported = IsBlockSolid(down) || (down == BLOCK_WATER);

    if (canFill && IsWaterBlock(up))
    {
        if (here != BLOCK_WATER_FALL) SetWater(world, pos, BLOCK_WATER_FALL);
        FlowFrom(world, pos, 8);
        return;
    }

    if (canFill && supported && (SourceNeighbors(world, pos) >= 2))
    {
        SetWater(world, pos, BLOCK_WATER);
        FlowFrom(world, pos, 0);
        return;
    }

    if (!IsWaterBlock(here)) return;

    for (int i = 0; i < 4; i++)
    {
        BlockPos side = { pos.x + waterDirs[i][0], pos.y, pos.z + waterDirs[i][1] };
        int level = WaterSpreadLevel(GetBlock(world, side));

        if (level < 0) continue;
        if ((level == 0) || (level == 8))
        {
            if (best > 1) best = 1;
        }
        else if (level + 1 < best) best = level + 1;
    }

    if (best <= 7)
    {
        BlockType want = BlockFromWaterLevel(best);
        if (here != want) SetWater(world, pos, want);
        FlowFrom(world, pos, best);
    }
    else SetWater(world, pos, BLOCK_AIR);
}

void UpdateWater(VoxelWorld *world)
{
    int updates = WATER_UPDATES_PER_TICK;

    if (world == NULL) return;

    while ((updates > 0) && (waterCount > 0))
    {
        BlockPos pos = waterQueue[waterHead];
        waterHead = (waterHead + 1)%WATER_QUEUE_CAP;
        waterCount--;
        Settle(world, pos);
        updates--;
    }
}
