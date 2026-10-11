#include "biomes.h"
#include "client_log.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

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

static const float deepOcean = -0.55f;
static const float oceanEdge = -0.30f;
static const float coastEdge = -0.12f;
static const float riverContinent = -0.15f;
static const float freezeLine = -0.35f;
static const float peakErosion = -0.84f;
static const float riverValley = 0.052f;
static const float riverBed = 0.015f;
static const float hotLine = 0.48f;
static const float warmLine = 0.10f;
static const float coldLine = -0.32f;

static int Count(void)
{
    return (int)(sizeof(biomes)/sizeof(biomes[0]));
}

static unsigned Hash(int x, int z, unsigned salt)
{
    unsigned h = (unsigned)x*374761393u ^ (unsigned)z*668265263u ^ biomeSeed*1442695041u ^ salt*2654435761u;
    h = (h ^ (h >> 13))*1274126177u;
    return h ^ (h >> 16);
}

static float Smooth(float t)
{
    return t*t*(3.0f - 2.0f*t);
}

static float ValueNoise(float x, float z, unsigned salt)
{
    int x0 = (int)floorf(x);
    int z0 = (int)floorf(z);
    float tx = Smooth(x - (float)x0);
    float tz = Smooth(z - (float)z0);
    float a = (float)(Hash(x0, z0, salt) & 255u)/255.0f;
    float b = (float)(Hash(x0 + 1, z0, salt) & 255u)/255.0f;
    float c = (float)(Hash(x0, z0 + 1, salt) & 255u)/255.0f;
    float d = (float)(Hash(x0 + 1, z0 + 1, salt) & 255u)/255.0f;
    float ab = a + (b - a)*tx;
    float cd = c + (d - c)*tx;

    return ab + (cd - ab)*tz;
}

static float Spread(float n)
{
    static const float knot[21] = {
        -1.0f, -0.9f, -0.8f, -0.7f, -0.6f, -0.5f, -0.4f, -0.3f, -0.2f, -0.1f, 0.0f,
        0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f
    };
    static const float cdf[21] = {
        0.000f, 0.006f, 0.023f, 0.050f, 0.089f, 0.139f, 0.198f, 0.265f, 0.339f, 0.418f, 0.500f,
        0.582f, 0.661f, 0.735f, 0.802f, 0.861f, 0.911f, 0.950f, 0.977f, 0.994f, 1.000f
    };
    float s = 0.0f;
    float t = 0.0f;
    float u = 0.0f;
    int i = 0;

    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    s = n*2.0f - 1.0f;
    for (i = 0; i < 20; i++)
    {
        float span = knot[i + 1] - knot[i];

        if (s > knot[i + 1]) continue;
        t = (s - knot[i])/span;
        u = cdf[i] + (cdf[i + 1] - cdf[i])*t;
        return u*2.0f - 1.0f;
    }
    return 1.0f;
}

static float Field(int x, int z, float frequency, unsigned salt)
{
    float broad = ValueNoise((float)x*frequency, (float)z*frequency, salt);
    float detail = ValueNoise((float)x*(frequency*3.0f), (float)z*(frequency*3.0f), salt ^ 0x9E3779B9u);
    float shaped = Spread(broad) + (detail - 0.5f)*0.08f;

    if (shaped < -1.0f) shaped = -1.0f;
    if (shaped > 1.0f) shaped = 1.0f;
    return shaped;
}

static float RiverField(int x, int z)
{
    float n = ValueNoise((float)x*0.0034f, (float)z*0.0034f, 0x51F0C3A5u);
    float wiggle = ValueNoise((float)x*0.0085f, (float)z*0.0085f, 0xD4E2F1C3u);

    return n*0.75f + wiggle*0.25f;
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

static const Biome *ById(const char *id)
{
    int i = 0;
    int count = Count();

    for (i = 0; i < count; i++)
    {
        if (strcmp(biomes[i].id, id) == 0) return &biomes[i];
    }
    return &biomes[0];
}

static const Biome *OceanBiome(float temperature, int deep)
{
    if (temperature < freezeLine)
    {
        if (deep) return ById("deep_frozen_ocean");
        return ById("frozen_ocean");
    }
    if (temperature < -0.05f)
    {
        if (deep) return ById("deep_cold_ocean");
        return ById("cold_ocean");
    }
    if (temperature < 0.32f)
    {
        if (deep) return ById("deep_ocean");
        return ById("ocean");
    }
    if (temperature < 0.62f)
    {
        if (deep) return ById("deep_lukewarm_ocean");
        return ById("lukewarm_ocean");
    }
    if (deep) return ById("deep_lukewarm_ocean");
    return ById("warm_ocean");
}

static const Biome *CoastBiome(float temperature, float erosion)
{
    if (erosion < -0.50f) return ById("stony_shore");
    if (temperature < freezeLine) return ById("snowy_beach");
    return ById("beach");
}

static const Biome *PeakBiome(float temperature, float humidity, float weirdness)
{
    if (temperature < freezeLine)
    {
        if (weirdness > 0.40f) return ById("frozen_peaks");
        if (weirdness < -0.30f) return ById("jagged_peaks");
        return ById("snowy_slopes");
    }
    if (temperature >= hotLine)
    {
        if (weirdness > 0.15f) return ById("stony_peaks");
        return ById("windswept_gravelly_hills");
    }
    if (humidity > 0.25f)
    {
        if (weirdness > 0.45f) return ById("windswept_forest");
        if (weirdness < -0.40f) return ById("windswept_gravelly_hills");
        return ById("windswept_hills");
    }
    if (weirdness > 0.55f) return ById("jagged_peaks");
    if (weirdness < -0.45f) return ById("stony_peaks");
    if (humidity < -0.25f) return ById("windswept_gravelly_hills");
    return ById("windswept_hills");
}

static const Biome *HotBiome(float humidity, float weirdness)
{
    if (humidity < 0.0f)
    {
        if (weirdness > 0.80f) return ById("eroded_badlands");
        if (weirdness > 0.62f) return ById("wooded_badlands");
        if (weirdness > 0.40f) return ById("badlands");
        return ById("desert");
    }
    if (weirdness > 0.62f) return ById("bamboo_jungle");
    if (weirdness < -0.48f) return ById("sparse_jungle");
    return ById("jungle");
}

static const Biome *SavannaBiome(float weirdness)
{
    if (weirdness > 0.70f) return ById("windswept_savanna");
    if (weirdness > 0.42f) return ById("savanna_plateau");
    return ById("savanna");
}

static const Biome *ColdBiome(float temperature, float humidity, float weirdness)
{
    if (temperature < -0.72f)
    {
        if (humidity < 0.0f)
        {
            if (weirdness > 0.82f) return ById("ice_spikes");
            return ById("snowy_plains");
        }
        if (weirdness > 0.55f) return ById("grove");
        return ById("snowy_taiga");
    }
    if (weirdness > 0.72f) return ById("old_growth_pine_taiga");
    if (weirdness < -0.72f) return ById("old_growth_spruce_taiga");
    return ById("taiga");
}

static const Biome *TemperateBiome(float humidity, float weirdness)
{
    if (humidity < -0.02f)
    {
        if (weirdness > 0.68f) return ById("sunflower_plains");
        if (weirdness > 0.40f) return ById("meadow");
        return ById("plains");
    }
    if (weirdness > 0.78f) return ById("flower_forest");
    if (weirdness > 0.64f) return ById("cherry_grove");
    if ((weirdness > 0.52f) && (humidity > 0.40f)) return ById("pale_garden");
    if (weirdness > 0.46f) return ById("dappled_forest");
    if (weirdness < -0.76f) return ById("dark_forest");
    if (weirdness < -0.40f) return ById("birch_forest");
    if ((weirdness < -0.18f) && (humidity > 0.30f)) return ById("old_growth_birch_forest");
    return ById("forest");
}

static const Biome *LandBiome(float temperature, float humidity, float weirdness)
{
    if (temperature >= hotLine) return HotBiome(humidity, weirdness);
    if ((temperature >= warmLine) && (humidity < 0.05f)) return SavannaBiome(weirdness);
    if (temperature < coldLine) return ColdBiome(temperature, humidity, weirdness);
    return TemperateBiome(humidity, weirdness);
}

static int InRiver(float continentalness, float erosion, float river)
{
    if (continentalness <= riverContinent) return 0;
    if (erosion < peakErosion) return 0;
    if (fabsf(river - 0.5f) >= riverBed) return 0;
    return 1;
}

void BiomeClimate(int x, int z, float *temperature, float *humidity, float *continentalness, float *erosion, float *weirdness)
{
    float t = Field(x, z, 0.0010f, 0xA341316Cu);
    float h = Field(x, z, 0.0011f, 0xC8013EA4u);
    float c = Field(x, z, 0.0007f, 0xAD90777Du);
    float e = Field(x, z, 0.0014f, 0x7E95761Eu);
    float w = Field(x, z, 0.0018f, 0x6893DD1Eu);

    if (temperature != NULL) *temperature = t;
    if (humidity != NULL) *humidity = h;
    if (continentalness != NULL) *continentalness = c;
    if (erosion != NULL) *erosion = e;
    if (weirdness != NULL) *weirdness = w;
}

float BiomeRiverMask(int x, int z)
{
    float continentalness = 0.0f;
    float erosion = 0.0f;
    float river = 0.0f;
    float dist = 0.0f;
    float along = 0.0f;
    float mask = 0.0f;
    float mouth = 0.0f;
    float hill = 0.0f;
    const float flat = 0.32f;

    BiomeClimate(x, z, NULL, NULL, &continentalness, &erosion, NULL);
    if (continentalness <= riverContinent) return 0.0f;
    if (erosion < peakErosion) return 0.0f;
    river = RiverField(x, z);
    dist = fabsf(river - 0.5f);
    if (dist >= riverValley) return 0.0f;
    along = dist/riverValley;
    if (along < flat) mask = 1.0f;
    else
    {
        mask = (1.0f - along)/(1.0f - flat);
        mouth = (continentalness - riverContinent)/0.16f;
        if (mouth < 1.0f) mask *= mouth;
        hill = (erosion - peakErosion)/0.20f;
        if (hill < 1.0f) mask *= hill;
    }
    if (mask < 0.0f) mask = 0.0f;
    if (mask > 1.0f) mask = 1.0f;
    return mask;
}

static const Biome *OverworldAt(int x, int z)
{
    float temperature = 0.0f;
    float humidity = 0.0f;
    float continentalness = 0.0f;
    float erosion = 0.0f;
    float weirdness = 0.0f;
    float river = 0.0f;

    BiomeClimate(x, z, &temperature, &humidity, &continentalness, &erosion, &weirdness);
    river = RiverField(x, z);
    if (InRiver(continentalness, erosion, river))
    {
        if (temperature < freezeLine) return ById("frozen_river");
        return ById("river");
    }
    if (continentalness < deepOcean) return OceanBiome(temperature, 1);
    if (continentalness < oceanEdge) return OceanBiome(temperature, 0);
    if (continentalness < coastEdge) return CoastBiome(temperature, erosion);
    if ((weirdness > 0.86f) && (humidity > -0.15f) && (humidity < 0.40f) && (continentalness > -0.04f) && (continentalness < 0.18f) && (erosion > 0.0f))
    {
        return ById("mushroom_fields");
    }
    if (erosion < peakErosion) return PeakBiome(temperature, humidity, weirdness);
    if ((temperature > 0.02f) && (temperature < 0.58f) && (humidity > 0.38f) && (continentalness < 0.18f) && (erosion > 0.05f))
    {
        if (temperature > 0.26f) return ById("mangrove_swamp");
        return ById("swamp");
    }
    return LandBiome(temperature, humidity, weirdness);
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

    return OverworldAt(x, z);
}
