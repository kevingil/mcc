#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <stdbool.h>
#include <stdint.h>

// level.dat is gzip-compressed NBT.
//
// Data holds the world name, seed, spawn, and the player.
// Player.Pos / Rotation / Motion and Player.Inventory follow the usual
// voxel save shape. Hotbar slots are 0-8. Slots 9-53 are the 45-slot pack.
// Item ids look like "opencraft:3".

#define LEVEL_ITEM_MAX 64

typedef struct {
    int slot;
    int block;
    int count;
} LevelItem;

typedef struct {
    char name[48];
    char mode[24];
    char version[16];
    uint32_t seed;
    int64_t createdUnix;
    int64_t lastPlayed;
    int64_t timeTicks;
    int gameType;
    int spawnX;
    int spawnY;
    int spawnZ;
    bool hasPlayer;
    double posX;
    double posY;
    double posZ;
    double motionX;
    double motionY;
    double motionZ;
    float yaw;
    float pitch;
    int selectedSlot;
    LevelItem items[LEVEL_ITEM_MAX];
    int itemCount;
} LevelData;

void LevelDataInit(LevelData *level);
void LevelDataClearPlayer(LevelData *level);
void LevelDataSetMeta(LevelData *level, const char *name, const char *mode, const char *version, uint32_t seed, int64_t createdUnix);
bool LevelDataRead(const char *folder, LevelData *level);
bool LevelDataWrite(const char *folder, const LevelData *level);

#endif
