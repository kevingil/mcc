#include "biome_houses.h"

#include "biomes.h"
#include "world_generation.h"

#include <math.h>
#include <stddef.h>

#define HOUSE_MIN_X 11
#define HOUSE_MAX_X 19
#define HOUSE_MIN_Z 18
#define HOUSE_MAX_Z 26
#define HOUSE_DOOR_X0 14
#define HOUSE_DOOR_X1 15
#define OUTSIDE_END 0.9f
#define WALK_END 2.2f
#define TOUR_END 4.0f

static float houseClock = 0.0f;

static int StripIndex(int worldX)
{
    int index = worldX/BIOME_STRIDE;

    if (worldX < 0) index = 0;
    return index;
}

static int InsideFootprint(int worldX, int worldZ)
{
    int index = StripIndex(worldX);
    int lx = worldX - index*BIOME_STRIDE;

    if ((worldX < 0) || (index >= BiomeCount())) return 0;
    if ((lx < HOUSE_MIN_X) || (lx > HOUSE_MAX_X)) return 0;
    if ((worldZ < HOUSE_MIN_Z) || (worldZ > HOUSE_MAX_Z)) return 0;
    return 1;
}

// Floor block y. Void is a pad at water+4. A column at or above the water
// uses that surface. Oceans and rivers (plateau below the water) get a pad
// whose floor block is WATER_LEVEL+1. Small end islands drop to bedrock off
// the island; those columns keep the plateau so the house stays one floor.
static int FloorY(const Biome *biome, int surfaceY)
{
    int plateau = WATER_LEVEL;

    if (biome == NULL)
    {
        if (surfaceY >= WATER_LEVEL) return surfaceY;
        return WATER_LEVEL + 1;
    }
    if ((biome->flags & BIOME_VOID) != 0) return WATER_LEVEL + 4;
    if (surfaceY >= WATER_LEVEL) return surfaceY;
    plateau = WATER_LEVEL + (int)biome->lift;
    if (plateau >= WATER_LEVEL) return plateau;
    return WATER_LEVEL + 1;
}

static BlockType WallOf(const Biome *biome)
{
    if (biome->log != BLOCK_AIR) return (BlockType)biome->log;
    if (biome->rock != BLOCK_AIR) return (BlockType)biome->rock;
    return BLOCK_STONE;
}

static BlockType RoofOf(const Biome *biome)
{
    if (biome->rock != BLOCK_AIR) return (BlockType)biome->rock;
    return BLOCK_OAK_PLANKS;
}

static BlockType FloorOf(const Biome *biome)
{
    if (biome->rock != BLOCK_AIR) return (BlockType)biome->rock;
    return WallOf(biome);
}

static void Put(Chunk *chunk, int localX, int y, int localZ, BlockType block)
{
    if ((y < 0) || (y >= WORLD_HEIGHT)) return;
    chunk->blocks[localX][y][localZ] = block;
}

static float StandY(int worldX, int worldZ)
{
    float ground = GetSurfaceLevel(worldX, worldZ);
    float onFloor = 0.0f;
    int surfaceY = 0;

    if (!InsideFootprint(worldX, worldZ)) return ground;
    surfaceY = (int)GetTerrainHeight(worldX, worldZ);
    onFloor = (float)FloorY(BiomeAt(worldX, worldZ), surfaceY) + 1.0f;
    if (onFloor > ground) return onFloor;
    return ground;
}

void BiomeHousesStamp(Chunk *chunk, int localX, int localZ, int worldX, int worldZ, int surfaceY)
{
    const Biome *biome = NULL;
    int index = 0;
    int lx = 0;
    int floorY = 0;
    int y = 0;
    int onEdgeX = 0;
    int onEdgeZ = 0;
    int corner = 0;
    int door = 0;
    BlockType wall = BLOCK_STONE;
    BlockType roof = BLOCK_OAK_PLANKS;
    BlockType floor = BLOCK_STONE;

    if (!BiomeTourActive()) return;
    if (chunk == NULL) return;
    if ((localX < 0) || (localX >= CHUNK_SIZE)) return;
    if ((localZ < 0) || (localZ >= CHUNK_SIZE)) return;
    if (!InsideFootprint(worldX, worldZ)) return;

    biome = BiomeAt(worldX, worldZ);
    index = StripIndex(worldX);
    lx = worldX - index*BIOME_STRIDE;
    floorY = FloorY(biome, surfaceY);
    if (floorY < 0) floorY = 0;
    if (floorY > (WORLD_HEIGHT - 5)) floorY = WORLD_HEIGHT - 5;

    wall = WallOf(biome);
    roof = RoofOf(biome);
    floor = FloorOf(biome);

    for (y = floorY + 1; y < WORLD_HEIGHT; y++) Put(chunk, localX, y, localZ, BLOCK_AIR);
    Put(chunk, localX, floorY, localZ, floor);
    if ((biome != NULL) && ((biome->flags & BIOME_VOID) != 0))
    {
        Put(chunk, localX, floorY - 1, localZ, floor);
        Put(chunk, localX, floorY - 2, localZ, floor);
    }

    onEdgeX = (lx == HOUSE_MIN_X) || (lx == HOUSE_MAX_X);
    onEdgeZ = (worldZ == HOUSE_MIN_Z) || (worldZ == HOUSE_MAX_Z);
    corner = (onEdgeX && onEdgeZ);
    door = (worldZ == HOUSE_MIN_Z) && ((lx == HOUSE_DOOR_X0) || (lx == HOUSE_DOOR_X1));

    if (onEdgeX || onEdgeZ)
    {
        if (door)
        {
            Put(chunk, localX, floorY + 1, localZ, BLOCK_AIR);
            Put(chunk, localX, floorY + 2, localZ, BLOCK_AIR);
            Put(chunk, localX, floorY + 3, localZ, wall);
        }
        else
        {
            Put(chunk, localX, floorY + 1, localZ, wall);
            Put(chunk, localX, floorY + 2, localZ, wall);
            Put(chunk, localX, floorY + 3, localZ, wall);
            if (onEdgeX && !corner) Put(chunk, localX, floorY + 1, localZ, BLOCK_GLASS);
        }
    }
    else
    {
        if ((lx == 13) && (worldZ == 25)) Put(chunk, localX, floorY + 1, localZ, BLOCK_CRAFTING_TABLE);
        else if ((lx == 17) && (worldZ == 25)) Put(chunk, localX, floorY + 1, localZ, BLOCK_FURNACE);
        else if ((lx == 13) && (worldZ == 24)) Put(chunk, localX, floorY + 1, localZ, BLOCK_CHEST);
        else if ((lx == 17) && (worldZ == 24)) Put(chunk, localX, floorY + 1, localZ, BLOCK_BOOKSHELF);
        if ((lx == 16) && (worldZ == 24)) Put(chunk, localX, floorY + 3, localZ, BLOCK_GLOWSTONE);
    }

    Put(chunk, localX, floorY + 4, localZ, roof);
}

int BiomeHouseTourStep(Player *player, float dt)
{
    int index = 0;
    int advanced = 0;
    int columnX = 0;
    int columnZ = 0;
    float x = 0.0f;
    float z = 0.0f;
    float y = 0.0f;
    float yaw = 0.0f;
    float pitch = -0.18f;
    float span = 1.0f;
    float t = 0.0f;

    if (player == NULL) return 0;
    if (!BiomeTourActive()) return 0;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    index = BiomeTourIndex();
    houseClock += dt;
    if (houseClock >= TOUR_END)
    {
        if (index >= (BiomeCount() - 1))
        {
            houseClock = TOUR_END;
        }
        else
        {
            int previous = index;
            int tries = 0;

            // BiomeTourTick adds at most 0.1 and the first interval starts
            // at -1.4, so one call never steps. Stop at the first change.
            for (tries = 0; (tries < 40) && (BiomeTourIndex() == previous); tries++)
            {
                BiomeTourTick(0.1f);
            }
            houseClock = 0.0f;
            index = BiomeTourIndex();
            advanced = 1;
        }
    }

    x = (float)(index*BIOME_STRIDE) + 15.5f;
    if (houseClock < OUTSIDE_END)
    {
        z = 14.5f;
        yaw = 0.0f;
        pitch = -0.18f;
    }
    else if (houseClock < WALK_END)
    {
        span = WALK_END - OUTSIDE_END;
        t = (houseClock - OUTSIDE_END)/span;
        z = 14.5f + (22.5f - 14.5f)*t;
        yaw = 0.0f;
        pitch = -0.08f;
    }
    else
    {
        span = TOUR_END - WALK_END;
        t = (houseClock - WALK_END)/span;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        z = 22.5f;
        yaw = -0.8f + (1.6f*t);
        pitch = -0.12f;
    }

    columnX = (int)floorf(x);
    columnZ = (int)floorf(z);
    y = StandY(columnX, columnZ);
    player->position = (Vector3){ x, y, z };
    player->velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    player->onGround = true;
    SetPlayerLook(player, yaw, pitch);
    return advanced;
}
