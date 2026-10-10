#include "level_data.h"
#include "nbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

static void MakeDir(const char *path)
{
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void CopyText(char *dst, int dstSize, const char *src)
{
    int i = 0;

    if (dstSize <= 0) return;
    if (src == NULL) src = "";
    while ((src[i] != '\0') && (i < dstSize - 1))
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int BlockFromId(const char *text)
{
    const char *number = strrchr(text, ':');

    if (number == NULL) number = text;
    else number++;
    return atoi(number);
}

void LevelDataInit(LevelData *level)
{
    memset(level, 0, sizeof(*level));
    CopyText(level->name, sizeof(level->name), "World");
    CopyText(level->mode, sizeof(level->mode), "Survival");
    CopyText(level->version, sizeof(level->version), "0.1.0");
    level->spawnY = 64;
}

void LevelDataClearPlayer(LevelData *level)
{
    level->hasPlayer = false;
    level->itemCount = 0;
    level->posX = 0.0;
    level->posY = 0.0;
    level->posZ = 0.0;
    level->motionX = 0.0;
    level->motionY = 0.0;
    level->motionZ = 0.0;
    level->yaw = 0.0f;
    level->pitch = 0.0f;
    level->selectedSlot = 0;
    level->timeTicks = 0;
    level->spawnX = 0;
    level->spawnY = 64;
    level->spawnZ = 0;
}

void LevelDataSetMeta(LevelData *level, const char *name, const char *mode, const char *version, uint32_t seed, int64_t createdUnix)
{
    if ((name != NULL) && (name[0] != '\0')) CopyText(level->name, sizeof(level->name), name);
    if ((mode != NULL) && (mode[0] != '\0')) CopyText(level->mode, sizeof(level->mode), mode);
    if ((version != NULL) && (version[0] != '\0')) CopyText(level->version, sizeof(level->version), version);
    level->seed = seed;
    if (createdUnix != 0) level->createdUnix = createdUnix;
    level->gameType = (level->mode[0] == 'C') ? 1 : 0;
}

static void AddItem(LevelData *level, int slot, int block, int count)
{
    LevelItem *item = NULL;

    if (level->itemCount >= LEVEL_ITEM_MAX) return;
    if (count <= 0) return;
    item = &level->items[level->itemCount++];
    item->slot = slot;
    item->block = block;
    item->count = count;
}

static void ReadDoubles(NbtIn *in, double *values, int want)
{
    int element = 0;
    int count = 0;
    int i = 0;

    NbtInListHeader(in, &element, &count);
    for (i = 0; (i < count) && !in->error; i++)
    {
        if (element == NBT_DOUBLE)
        {
            double value = NbtInF64(in);
            if (i < want) values[i] = value;
        }
        else if (element == NBT_FLOAT)
        {
            float value = NbtInF32(in);
            if (i < want) values[i] = (double)value;
        }
        else NbtInSkip(in, element);
    }
}

static void ReadFloats(NbtIn *in, float *values, int want)
{
    int element = 0;
    int count = 0;
    int i = 0;

    NbtInListHeader(in, &element, &count);
    for (i = 0; (i < count) && !in->error; i++)
    {
        if (element == NBT_FLOAT)
        {
            float value = NbtInF32(in);
            if (i < want) values[i] = value;
        }
        else if (element == NBT_DOUBLE)
        {
            double value = NbtInF64(in);
            if (i < want) values[i] = (float)value;
        }
        else NbtInSkip(in, element);
    }
}

static void ReadInventory(NbtIn *in, LevelData *level)
{
    int element = 0;
    int count = 0;
    int i = 0;

    level->itemCount = 0;
    NbtInListHeader(in, &element, &count);
    for (i = 0; (i < count) && !in->error; i++)
    {
        char name[64] = { 0 };
        int type = 0;
        int slot = -1;
        int block = 0;
        int itemCount = 1;
        char id[64] = { 0 };

        if (element != NBT_COMPOUND)
        {
            NbtInSkip(in, element);
            continue;
        }

        while (NbtInNext(in, name, sizeof(name), &type))
        {
            if ((strcmp(name, "Slot") == 0) && (type == NBT_BYTE)) slot = NbtInU8(in);
            else if ((strcmp(name, "Count") == 0) && (type == NBT_BYTE)) itemCount = NbtInU8(in);
            else if ((strcmp(name, "id") == 0) && (type == NBT_STRING)) NbtInReadString(in, id, sizeof(id));
            else NbtInSkip(in, type);
        }

        if (id[0] != '\0') block = BlockFromId(id);
        if (slot >= 0) AddItem(level, slot, block, itemCount);
    }
}

static void ReadPlayer(NbtIn *in, LevelData *level)
{
    char name[64] = { 0 };
    int type = 0;
    double pos[3] = { 0.0, 0.0, 0.0 };
    double motion[3] = { 0.0, 0.0, 0.0 };
    float rotation[2] = { 0.0f, 0.0f };

    level->hasPlayer = true;
    while (NbtInNext(in, name, sizeof(name), &type))
    {
        if ((strcmp(name, "Pos") == 0) && (type == NBT_LIST)) ReadDoubles(in, pos, 3);
        else if ((strcmp(name, "Motion") == 0) && (type == NBT_LIST)) ReadDoubles(in, motion, 3);
        else if ((strcmp(name, "Rotation") == 0) && (type == NBT_LIST)) ReadFloats(in, rotation, 2);
        else if ((strcmp(name, "SelectedItemSlot") == 0) && (type == NBT_INT)) level->selectedSlot = NbtInI32(in);
        else if ((strcmp(name, "Inventory") == 0) && (type == NBT_LIST)) ReadInventory(in, level);
        else NbtInSkip(in, type);
    }

    level->posX = pos[0];
    level->posY = pos[1];
    level->posZ = pos[2];
    level->motionX = motion[0];
    level->motionY = motion[1];
    level->motionZ = motion[2];
    level->yaw = rotation[0];
    level->pitch = rotation[1];
}

static void ReadVersion(NbtIn *in, LevelData *level)
{
    char name[64] = { 0 };
    int type = 0;

    while (NbtInNext(in, name, sizeof(name), &type))
    {
        if ((strcmp(name, "Name") == 0) && (type == NBT_STRING)) NbtInReadString(in, level->version, sizeof(level->version));
        else NbtInSkip(in, type);
    }
}

static void ReadData(NbtIn *in, LevelData *level)
{
    char name[64] = { 0 };
    int type = 0;

    while (NbtInNext(in, name, sizeof(name), &type))
    {
        if ((strcmp(name, "LevelName") == 0) && (type == NBT_STRING)) NbtInReadString(in, level->name, sizeof(level->name));
        else if ((strcmp(name, "RandomSeed") == 0) && (type == NBT_LONG)) level->seed = (uint32_t)NbtInI64(in);
        else if ((strcmp(name, "GameType") == 0) && (type == NBT_INT))
        {
            level->gameType = NbtInI32(in);
            CopyText(level->mode, sizeof(level->mode), (level->gameType == 1) ? "Creative" : "Survival");
        }
        else if ((strcmp(name, "SpawnX") == 0) && (type == NBT_INT)) level->spawnX = NbtInI32(in);
        else if ((strcmp(name, "SpawnY") == 0) && (type == NBT_INT)) level->spawnY = NbtInI32(in);
        else if ((strcmp(name, "SpawnZ") == 0) && (type == NBT_INT)) level->spawnZ = NbtInI32(in);
        else if ((strcmp(name, "Time") == 0) && (type == NBT_LONG)) level->timeTicks = NbtInI64(in);
        else if ((strcmp(name, "LastPlayed") == 0) && (type == NBT_LONG)) level->lastPlayed = NbtInI64(in);
        else if ((strcmp(name, "Created") == 0) && (type == NBT_LONG)) level->createdUnix = NbtInI64(in);
        else if ((strcmp(name, "Version") == 0) && (type == NBT_COMPOUND)) ReadVersion(in, level);
        else if ((strcmp(name, "Player") == 0) && (type == NBT_COMPOUND)) ReadPlayer(in, level);
        else NbtInSkip(in, type);
    }
}

bool LevelDataRead(const char *folder, LevelData *level)
{
    char path[192] = { 0 };
    unsigned char *data = NULL;
    int size = 0;
    NbtIn in = { 0 };
    char name[64] = { 0 };
    int type = 0;
    int root = 0;
    bool sawData = false;

    if ((folder == NULL) || (level == NULL)) return false;
    LevelDataInit(level);
    snprintf(path, sizeof(path), "%s/level.dat", folder);
    if (!NbtReadGzip(path, &data, &size)) return false;

    NbtInInit(&in, data, size);
    root = NbtInU8(&in);
    NbtInReadString(&in, name, sizeof(name));
    if ((!in.error) && (root == NBT_COMPOUND))
    {
        while (NbtInNext(&in, name, sizeof(name), &type))
        {
            if ((strcmp(name, "Data") == 0) && (type == NBT_COMPOUND))
            {
                ReadData(&in, level);
                sawData = true;
            }
            else NbtInSkip(&in, type);
        }
    }

    NbtFreeBytes(data);
    return (!in.error && sawData);
}

static void WriteItems(NbtBuf *buf, const LevelData *level)
{
    int i = 0;

    NbtStartList(buf, "Inventory", NBT_COMPOUND, level->itemCount);
    for (i = 0; i < level->itemCount; i++)
    {
        char id[32] = { 0 };
        const LevelItem *item = &level->items[i];

        snprintf(id, sizeof(id), "opencraft:%d", item->block);
        NbtByte(buf, "Slot", item->slot);
        NbtString(buf, "id", id);
        NbtByte(buf, "Count", item->count);
        NbtEndCompound(buf);
    }
}

bool LevelDataWrite(const char *folder, const LevelData *level)
{
    NbtBuf buf = { 0 };
    char path[192] = { 0 };
    bool ok = false;
    int64_t now = (int64_t)time(NULL)*1000;

    if ((folder == NULL) || (level == NULL)) return false;

    NbtBufInit(&buf);
    NbtStartCompound(&buf, "");
    NbtStartCompound(&buf, "Data");
    NbtString(&buf, "LevelName", level->name);
    NbtLong(&buf, "RandomSeed", (int64_t)level->seed);
    NbtInt(&buf, "GameType", level->gameType);
    NbtInt(&buf, "SpawnX", level->spawnX);
    NbtInt(&buf, "SpawnY", level->spawnY);
    NbtInt(&buf, "SpawnZ", level->spawnZ);
    NbtLong(&buf, "Time", level->timeTicks);
    NbtLong(&buf, "LastPlayed", (level->lastPlayed != 0) ? level->lastPlayed : now);
    NbtLong(&buf, "Created", level->createdUnix);
    NbtInt(&buf, "DataVersion", 1);
    NbtStartCompound(&buf, "Version");
    NbtString(&buf, "Name", level->version);
    NbtInt(&buf, "Id", 1);
    NbtEndCompound(&buf);

    if (level->hasPlayer)
    {
        NbtStartCompound(&buf, "Player");
        NbtStartList(&buf, "Pos", NBT_DOUBLE, 3);
        NbtRawDouble(&buf, level->posX);
        NbtRawDouble(&buf, level->posY);
        NbtRawDouble(&buf, level->posZ);
        NbtStartList(&buf, "Rotation", NBT_FLOAT, 2);
        NbtRawFloat(&buf, level->yaw);
        NbtRawFloat(&buf, level->pitch);
        NbtStartList(&buf, "Motion", NBT_DOUBLE, 3);
        NbtRawDouble(&buf, level->motionX);
        NbtRawDouble(&buf, level->motionY);
        NbtRawDouble(&buf, level->motionZ);
        NbtInt(&buf, "SelectedItemSlot", level->selectedSlot);
        WriteItems(&buf, level);
        NbtEndCompound(&buf);
    }

    NbtEndCompound(&buf);
    NbtEndCompound(&buf);

    MakeDir(folder);
    snprintf(path, sizeof(path), "%s/level.dat", folder);
    ok = (buf.error == 0) && NbtWriteGzip(path, buf.data, buf.size);
    NbtBufFree(&buf);
    return ok;
}
