#include "biomes.h"
#include "client_log.h"

#include <math.h>
#include <stdlib.h>

#define B(id, name, surface, filler, rock, log, leaves, trees, lift, flags) \
    { id, name, (unsigned short)(surface), (unsigned short)(filler), (unsigned short)(rock), \
      (unsigned short)(log), (unsigned short)(leaves), (unsigned short)(trees), (short)(lift), (unsigned)(flags) }

static const Biome biomes[] = {
    B("badlands", "Badlands", BLOCK_TERRACOTTA, BLOCK_ORANGE_TERRACOTTA, BLOCK_TERRACOTTA, BLOCK_AIR, BLOCK_AIR, 0, 8, BIOME_BANDS),
    B("bamboo_jungle", "Bamboo Jungle", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 30, 6, BIOME_POLE | BIOME_MELON),
    B("basalt_deltas", "Basalt Deltas", BLOCK_BLACK_CONCRETE, BLOCK_GRAY_CONCRETE, BLOCK_GRAY_CONCRETE, BLOCK_AIR, BLOCK_AIR, 0, 6, BIOME_MAGMA),
    B("beach", "Beach", BLOCK_SAND, BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_AIR, BLOCK_AIR, 0, 2, 0),
    B("birch_forest", "Birch Forest", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_BIRCH_LOG, BLOCK_BIRCH_LEAVES, 45, 6, 0),
    B("cherry_grove", "Cherry Grove", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_BIRCH_LOG, BLOCK_PINK_CONCRETE, 28, 8, BIOME_HAY),
    B("cold_ocean", "Cold Ocean", BLOCK_GRAVEL, BLOCK_GRAVEL, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -8, BIOME_OCEAN),
    B("crimson_forest", "Crimson Forest", BLOCK_NETHERRACK, BLOCK_NETHERRACK, BLOCK_NETHERRACK, BLOCK_RED_WOOL, BLOCK_RED_CONCRETE, 40, 6, BIOME_GLOW),
    B("dappled_forest", "Dappled Forest", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 36, 6, BIOME_PUMPKIN),
    B("dark_forest", "Dark Forest", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_LEAVES, 70, 6, 0),
    B("deep_cold_ocean", "Deep Cold Ocean", BLOCK_GRAVEL, BLOCK_GRAVEL, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -16, BIOME_OCEAN),
    B("deep_dark", "Deep Dark", BLOCK_BLACK_CONCRETE, BLOCK_GRAY_CONCRETE, BLOCK_OBSIDIAN, BLOCK_AIR, BLOCK_AIR, 0, 4, 0),
    B("deep_frozen_ocean", "Deep Frozen Ocean", BLOCK_GRAVEL, BLOCK_PACKED_ICE, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -16, BIOME_OCEAN | BIOME_ICE),
    B("deep_lukewarm_ocean", "Deep Lukewarm Ocean", BLOCK_SAND, BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_AIR, BLOCK_AIR, 0, -16, BIOME_OCEAN),
    B("deep_ocean", "Deep Ocean", BLOCK_GRAVEL, BLOCK_GRAVEL, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -18, BIOME_OCEAN),
    B("desert", "Desert", BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_SANDSTONE, BLOCK_AIR, BLOCK_AIR, 0, 6, BIOME_CACTUS),
    B("dripstone_caves", "Dripstone Caves", BLOCK_GRANITE, BLOCK_STONE, BLOCK_GRANITE, BLOCK_AIR, BLOCK_AIR, 0, 5, BIOME_SPIKE),
    B("end_barrens", "End Barrens", BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_AIR, BLOCK_AIR, 0, 4, 0),
    B("end_highlands", "End Highlands", BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_PURPUR_BLOCK, BLOCK_PURPUR_BLOCK, 0, 14, BIOME_PURPUR),
    B("end_midlands", "End Midlands", BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_AIR, BLOCK_AIR, 0, 8, BIOME_PURPUR),
    B("eroded_badlands", "Eroded Badlands", BLOCK_RED_TERRACOTTA, BLOCK_ORANGE_TERRACOTTA, BLOCK_TERRACOTTA, BLOCK_AIR, BLOCK_AIR, 0, 16, BIOME_BANDS | BIOME_SPIKE),
    B("flower_forest", "Flower Forest", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 22, 6, BIOME_HAY | BIOME_PUMPKIN),
    B("forest", "Forest", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 48, 6, 0),
    B("frozen_ocean", "Frozen Ocean", BLOCK_GRAVEL, BLOCK_PACKED_ICE, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -8, BIOME_OCEAN | BIOME_ICE),
    B("frozen_peaks", "Frozen Peaks", BLOCK_SNOW_BLOCK, BLOCK_PACKED_ICE, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 22, BIOME_SPIKE),
    B("frozen_river", "Frozen River", BLOCK_ICE, BLOCK_GRAVEL, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -2, BIOME_OCEAN | BIOME_ICE),
    B("grove", "Grove", BLOCK_SNOW_BLOCK, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_SNOW_BLOCK, 24, 12, 0),
    B("ice_caves", "Ice Caves", BLOCK_PACKED_ICE, BLOCK_BLUE_ICE, BLOCK_PACKED_ICE, BLOCK_AIR, BLOCK_AIR, 0, 5, BIOME_SPIKE),
    B("ice_spikes", "Ice Spikes", BLOCK_SNOW_BLOCK, BLOCK_DIRT, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 6, BIOME_SPIKE),
    B("jagged_peaks", "Jagged Peaks", BLOCK_STONE, BLOCK_STONE, BLOCK_ANDESITE, BLOCK_AIR, BLOCK_AIR, 0, 24, BIOME_SPIKE),
    B("jungle", "Jungle", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 60, 6, BIOME_MELON),
    B("lukewarm_ocean", "Lukewarm Ocean", BLOCK_SAND, BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_AIR, BLOCK_AIR, 0, -8, BIOME_OCEAN),
    B("lush_caves", "Lush Caves", BLOCK_MOSSY_COBBLESTONE, BLOCK_CLAY, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 12, 4, BIOME_MELON),
    B("mangrove_swamp", "Mangrove Swamp", BLOCK_DIRT, BLOCK_DIRT, BLOCK_CLAY, BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_LEAVES, 28, 1, BIOME_OCEAN),
    B("meadow", "Meadow", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_BIRCH_LOG, BLOCK_BIRCH_LEAVES, 6, 10, BIOME_HAY),
    B("mushroom_fields", "Mushroom Fields", BLOCK_BROWN_CONCRETE, BLOCK_DIRT, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 6, BIOME_PUMPKIN),
    B("nether_wastes", "Nether Wastes", BLOCK_NETHERRACK, BLOCK_NETHERRACK, BLOCK_NETHERRACK, BLOCK_AIR, BLOCK_AIR, 0, 6, BIOME_GLOW),
    B("ocean", "Ocean", BLOCK_SAND, BLOCK_GRAVEL, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -8, BIOME_OCEAN),
    B("old_growth_birch_forest", "Old Growth Birch Forest", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_BIRCH_LOG, BLOCK_BIRCH_LEAVES, 40, 6, 0),
    B("old_growth_pine_taiga", "Old Growth Pine Taiga", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_LEAVES, 34, 8, 0),
    B("old_growth_spruce_taiga", "Old Growth Spruce Taiga", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_LEAVES, 38, 8, 0),
    B("pale_garden", "Pale Garden", BLOCK_LIGHT_GRAY_TERRACOTTA, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_GRAY_CONCRETE, 32, 6, 0),
    B("plains", "Plains", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 8, 6, 0),
    B("river", "River", BLOCK_SAND, BLOCK_DIRT, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, -2, BIOME_OCEAN),
    B("savanna", "Savanna", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_ACACIA_LOG, BLOCK_ACACIA_LEAVES, 14, 6, 0),
    B("savanna_plateau", "Savanna Plateau", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_ACACIA_LOG, BLOCK_ACACIA_LEAVES, 10, 12, 0),
    B("small_end_islands", "Small End Islands", BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_AIR, BLOCK_AIR, 0, 6, BIOME_ISLAND),
    B("snowy_beach", "Snowy Beach", BLOCK_SNOW_BLOCK, BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_AIR, BLOCK_AIR, 0, 2, 0),
    B("snowy_plains", "Snowy Plains", BLOCK_SNOW_BLOCK, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_SNOW_BLOCK, 6, 6, 0),
    B("snowy_slopes", "Snowy Slopes", BLOCK_SNOW_BLOCK, BLOCK_STONE, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 16, 0),
    B("snowy_taiga", "Snowy Taiga", BLOCK_SNOW_BLOCK, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_LEAVES, 30, 6, 0),
    B("soul_sand_valley", "Soul Sand Valley", BLOCK_SOUL_SAND, BLOCK_SOUL_SAND, BLOCK_SOUL_SAND, BLOCK_AIR, BLOCK_AIR, 0, 5, BIOME_BONE),
    B("sparse_jungle", "Sparse Jungle", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 16, 6, BIOME_MELON),
    B("stony_peaks", "Stony Peaks", BLOCK_STONE, BLOCK_ANDESITE, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 18, 0),
    B("stony_shore", "Stony Shore", BLOCK_STONE, BLOCK_GRAVEL, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 2, 0),
    B("sulfur_caves", "Sulfur Caves", BLOCK_YELLOW_TERRACOTTA, BLOCK_STONE, BLOCK_YELLOW_TERRACOTTA, BLOCK_AIR, BLOCK_AIR, 0, 4, BIOME_GLOW),
    B("sunflower_plains", "Sunflower Plains", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 6, 6, BIOME_HAY),
    B("swamp", "Swamp", BLOCK_GRASS, BLOCK_DIRT, BLOCK_CLAY, BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_LEAVES, 18, 2, 0),
    B("taiga", "Taiga", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_LEAVES, 32, 6, 0),
    B("the_end", "The End", BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_END_STONE, BLOCK_AIR, BLOCK_AIR, 0, 6, 0),
    B("the_void", "The Void", BLOCK_STONE, BLOCK_STONE, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 4, BIOME_VOID),
    B("warm_ocean", "Warm Ocean", BLOCK_SAND, BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_AIR, BLOCK_AIR, 0, -6, BIOME_OCEAN),
    B("warped_forest", "Warped Forest", BLOCK_NETHERRACK, BLOCK_NETHERRACK, BLOCK_NETHERRACK, BLOCK_CYAN_WOOL, BLOCK_CYAN_CONCRETE, 36, 6, 0),
    B("windswept_forest", "Windswept Forest", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 26, 12, 0),
    B("windswept_gravelly_hills", "Windswept Gravelly Hills", BLOCK_GRAVEL, BLOCK_GRAVEL, BLOCK_STONE, BLOCK_AIR, BLOCK_AIR, 0, 12, 0),
    B("windswept_hills", "Windswept Hills", BLOCK_GRASS, BLOCK_STONE, BLOCK_STONE, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 10, 14, 0),
    B("windswept_savanna", "Windswept Savanna", BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_ACACIA_LOG, BLOCK_ACACIA_LEAVES, 12, 14, 0),
    B("wooded_badlands", "Wooded Badlands", BLOCK_TERRACOTTA, BLOCK_ORANGE_TERRACOTTA, BLOCK_TERRACOTTA, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, 16, 8, BIOME_BANDS)
};

static unsigned int biomeSeed = 1;
static int tourIndex = 0;
static float tourTime = -1.4f;
static int tourCached = -1;

static int Count(void)
{
    return (int)(sizeof(biomes)/sizeof(biomes[0]));
}

static unsigned Hash(int x, int z)
{
    unsigned h = (unsigned)x*374761393u ^ (unsigned)z*668265263u ^ biomeSeed*1442695041u;
    h = (h ^ (h >> 13))*1274126177u;
    return h ^ (h >> 16);
}

static float Smooth(float t)
{
    return t*t*(3.0f - 2.0f*t);
}

static float ValueNoise(float x, float z)
{
    int x0 = (int)floorf(x);
    int z0 = (int)floorf(z);
    float tx = Smooth(x - (float)x0);
    float tz = Smooth(z - (float)z0);
    float a = (float)(Hash(x0, z0) & 255u)/255.0f;
    float b = (float)(Hash(x0 + 1, z0) & 255u)/255.0f;
    float c = (float)(Hash(x0, z0 + 1) & 255u)/255.0f;
    float d = (float)(Hash(x0 + 1, z0 + 1) & 255u)/255.0f;
    float ab = a + (b - a)*tx;
    float cd = c + (d - c)*tx;

    return ab + (cd - ab)*tz;
}

void BiomeSetSeed(unsigned int seed)
{
    biomeSeed = (seed == 0) ? 1u : seed;
}

int BiomeTourActive(void)
{
    if (tourCached < 0) tourCached = (getenv("MCC_BIOME_TOUR") != NULL) ? 1 : 0;
    return tourCached;
}

int BiomeCount(void)
{
    return Count();
}

int BiomeTourIndex(void)
{
    if (tourIndex < 0) tourIndex = 0;
    if (tourIndex >= Count()) tourIndex = Count() - 1;
    return tourIndex;
}

void BiomeTourTick(float dt)
{
    int next = 0;

    if (!BiomeTourActive()) return;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;
    tourTime += dt;
    if (tourTime < 1.7f) return;
    tourTime = 0.0f;
    next = tourIndex + 1;
    if (next >= Count()) return;
    tourIndex = next;
    ClientLog("biome tour %d/%d %s", tourIndex + 1, Count(), biomes[tourIndex].name);
}

const Biome *BiomeAt(int x, int z)
{
    int index = 0;
    int count = Count();

    if (BiomeTourActive())
    {
        index = x/BIOME_STRIDE;
        if (x < 0) index = 0;
        if (index >= count) index = count - 1;
        if (index < 0) index = 0;
        return &biomes[index];
    }

    index = (int)(ValueNoise((float)x*0.004f, (float)z*0.004f)*(float)count);
    if (index >= count) index = count - 1;
    if (index < 0) index = 0;
    return &biomes[index];
}
