#ifndef BIOMES_H
#define BIOMES_H

#include "voxel_types.h"

#define BIOME_STRIDE 32

enum {
    BIOME_OCEAN = 1,
    BIOME_BANDS = 2,
    BIOME_CACTUS = 4,
    BIOME_SPIKE = 8,
    BIOME_MELON = 16,
    BIOME_PUMPKIN = 32,
    BIOME_HAY = 64,
    BIOME_GLOW = 128,
    BIOME_PURPUR = 256,
    BIOME_BONE = 512,
    BIOME_MAGMA = 1024,
    BIOME_VOID = 2048,
    BIOME_ISLAND = 4096,
    BIOME_POLE = 8192,
    BIOME_ICE = 16384
};

typedef struct Biome {
    const char *id;
    const char *name;
    unsigned short surface;
    unsigned short filler;
    unsigned short rock;
    unsigned short log;
    unsigned short leaves;
    unsigned short trees;
    short lift;
    unsigned flags;
} Biome;

void BiomeSetSeed(unsigned int seed);
int BiomeTourActive(void);
int BiomeCount(void);
int BiomeTourIndex(void);
void BiomeTourTick(float dt);
const Biome *BiomeAt(int x, int z);
void BiomeClimate(int x, int z, float *temperature, float *humidity, float *continentalness, float *erosion, float *weirdness);
float BiomeRiverMask(int x, int z);

#endif
