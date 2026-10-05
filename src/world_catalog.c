#include "world_catalog.h"
#include "anvil.h"
#include "level_data.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

//----------------------------------------------------------------------------------
// Module Variables
//----------------------------------------------------------------------------------
static WorldInfo worlds[MAX_WORLDS] = { 0 };
static int worldCount = 0;
static char catalogVersion[16] = "0.1.0";

static int activeId = -1;
static char activeName[48] = "";
static char activeFolder[64] = "";
static unsigned int activeSeed = 0;

//----------------------------------------------------------------------------------
// Local Functions
//----------------------------------------------------------------------------------
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

static void CopyName(char *dst, int dstSize, const char *src)
{
    int j = 0;

    if (dstSize <= 0) return;
    if (src == NULL) src = "";

    for (int i = 0; (src[i] != '\0') && (j < dstSize - 1); i++)
    {
        unsigned char c = (unsigned char)src[i];
        if ((c < 32) || (c == 127)) continue;
        dst[j] = (char)c;
        j++;
    }

    dst[j] = '\0';
}

static void MakeDir(const char *path)
{
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void RemoveDir(const char *path)
{
#if defined(_WIN32)
    _rmdir(path);
#else
    rmdir(path);
#endif
}

static void TrimLine(char *text)
{
    int length = (int)strlen(text);

    while ((length > 0) && ((text[length - 1] == '\n') || (text[length - 1] == '\r') || (text[length - 1] == ' ')))
    {
        text[length - 1] = '\0';
        length--;
    }
}

static void FormatCreated(long unixTime, char *out, int outSize)
{
    time_t when = (time_t)unixTime;
    struct tm *info = localtime(&when);
    int hour = 0;
    const char *ampm = "AM";

    if (info == NULL)
    {
        CopyText(out, outSize, "unknown");
        return;
    }

    hour = info->tm_hour;
    if (hour >= 12)
    {
        ampm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;

    snprintf(out, outSize, "%d/%d/%02d %d:%02d %s",
        info->tm_mon + 1, info->tm_mday, info->tm_year%100,
        hour, info->tm_min, ampm);
}

static unsigned int MakeSeed(int id)
{
    unsigned int seed = (unsigned int)time(NULL);

    seed = seed*1664525u + (unsigned int)id*1013904223u;
    if (seed == 0) seed = 1;
    return seed;
}

static int FindWorldIndexById(int id)
{
    for (int i = 0; i < worldCount; i++)
    {
        if (worlds[i].id == id) return i;
    }

    return -1;
}

static int NextId(void)
{
    int maxId = 0;

    for (int i = 0; i < worldCount; i++)
    {
        if (worlds[i].id > maxId) maxId = worlds[i].id;
    }

    return maxId + 1;
}

static void FillFolder(WorldInfo *world)
{
    snprintf(world->folder, sizeof(world->folder), "saves/world_%d", world->id);
}

static bool WriteLevel(const WorldInfo *world, bool resetPlayer)
{
    char path[96] = { 0 };
    FILE *file = NULL;
    LevelData level;
    bool textOk = false;

    MakeDir("saves");
    MakeDir(world->folder);
    snprintf(path, sizeof(path), "%s/level.txt", world->folder);

    file = fopen(path, "w");
    if (file == NULL) return false;

    fprintf(file, "id=%d\n", world->id);
    fprintf(file, "name=%s\n", world->name);
    fprintf(file, "created=%ld\n", world->createdUnix);
    fprintf(file, "mode=%s\n", world->mode);
    fprintf(file, "version=%s\n", world->version);
    fprintf(file, "seed=%u\n", world->seed);
    fclose(file);
    textOk = true;

    if (resetPlayer) AnvilRemoveRegions(world->folder);
    if (resetPlayer || !LevelDataRead(world->folder, &level)) LevelDataInit(&level);
    if (resetPlayer) LevelDataClearPlayer(&level);
    LevelDataSetMeta(&level, world->name, world->mode, world->version, world->seed, (int64_t)world->createdUnix);
    return textOk && LevelDataWrite(world->folder, &level);
}

static bool ReadLevel(const char *folder, int folderId, WorldInfo *world)
{
    char path[96] = { 0 };
    FILE *file = NULL;

    memset(world, 0, sizeof(*world));
    snprintf(path, sizeof(path), "%s/level.txt", folder);
    file = fopen(path, "r");
    if (file == NULL) return false;

    world->id = folderId;
    CopyText(world->mode, sizeof(world->mode), "Survival");
    CopyText(world->version, sizeof(world->version), catalogVersion);

    char line[128] = { 0 };
    while (fgets(line, sizeof(line), file) != NULL)
    {
        TrimLine(line);
        if (strncmp(line, "id=", 3) == 0) world->id = atoi(line + 3);
        else if (strncmp(line, "name=", 5) == 0) CopyText(world->name, sizeof(world->name), line + 5);
        else if (strncmp(line, "created=", 8) == 0) world->createdUnix = strtol(line + 8, NULL, 10);
        else if (strncmp(line, "mode=", 5) == 0) CopyText(world->mode, sizeof(world->mode), line + 5);
        else if (strncmp(line, "version=", 8) == 0) CopyText(world->version, sizeof(world->version), line + 8);
        else if (strncmp(line, "seed=", 5) == 0) world->seed = (unsigned int)strtoul(line + 5, NULL, 10);
    }

    fclose(file);

    if (world->id <= 0) world->id = folderId;
    if (world->name[0] == '\0') CopyText(world->name, sizeof(world->name), "World");
    if (world->createdUnix == 0) world->createdUnix = (long)time(NULL);
    FormatCreated(world->createdUnix, world->createdText, sizeof(world->createdText));
    FillFolder(world);
    return true;
}

static void SortWorlds(void)
{
    for (int i = 1; i < worldCount; i++)
    {
        WorldInfo key = worlds[i];
        int j = i - 1;

        while ((j >= 0) && (worlds[j].id > key.id))
        {
            worlds[j + 1] = worlds[j];
            j--;
        }

        worlds[j + 1] = key;
    }
}

static bool NameIsBlank(const char *name)
{
    if (name == NULL) return true;

    for (int i = 0; name[i] != '\0'; i++)
    {
        if (!isspace((unsigned char)name[i])) return false;
    }

    return true;
}

//----------------------------------------------------------------------------------
// Catalog
//----------------------------------------------------------------------------------
void LoadWorldCatalog(const char *version)
{
    worldCount = 0;
    if ((version != NULL) && (version[0] != '\0')) CopyText(catalogVersion, sizeof(catalogVersion), version);

    for (int id = 1; (id <= 512) && (worldCount < MAX_WORLDS); id++)
    {
        char folder[64] = { 0 };

        snprintf(folder, sizeof(folder), "saves/world_%d", id);
        if (ReadLevel(folder, id, &worlds[worldCount]))
        {
            char datPath[96] = { 0 };
            FILE *dat = NULL;

            snprintf(datPath, sizeof(datPath), "%s/level.dat", worlds[worldCount].folder);
            dat = fopen(datPath, "rb");
            if (dat == NULL) WriteLevel(&worlds[worldCount], false);
            else fclose(dat);
            worldCount++;
        }
    }

    SortWorlds();

    if (worldCount == 0) CreateWorldRecord("New World", catalogVersion, 0, false);
}

int GetWorldCount(void)
{
    return worldCount;
}

const WorldInfo *GetWorld(int index)
{
    if ((index < 0) || (index >= worldCount)) return NULL;
    return &worlds[index];
}

int CreateWorldRecord(const char *name, const char *version, unsigned int seed, bool chooseSeed)
{
    WorldInfo *world = NULL;
    int id = 0;

    if (NameIsBlank(name)) return -1;
    if (worldCount >= MAX_WORLDS) return -1;

    world = &worlds[worldCount];
    memset(world, 0, sizeof(*world));
    world->id = NextId();
    id = world->id;
    CopyName(world->name, sizeof(world->name), name);
    if (world->name[0] == '\0') return -1;

    world->createdUnix = (long)time(NULL);
    FormatCreated(world->createdUnix, world->createdText, sizeof(world->createdText));
    CopyText(world->mode, sizeof(world->mode), "Survival");
    if ((version != NULL) && (version[0] != '\0')) CopyText(world->version, sizeof(world->version), version);
    else CopyText(world->version, sizeof(world->version), catalogVersion);
    if (chooseSeed) world->seed = seed;
    else world->seed = MakeSeed(world->id);
    FillFolder(world);

    if (!WriteLevel(world, false)) return -1;

    worldCount++;
    SortWorlds();
    return id;
}

bool DeleteWorldRecord(int id)
{
    int index = FindWorldIndexById(id);
    char path[96] = { 0 };

    if (index < 0) return false;

    snprintf(path, sizeof(path), "%s/level.txt", worlds[index].folder);
    remove(path);
    snprintf(path, sizeof(path), "%s/level.dat", worlds[index].folder);
    remove(path);
    AnvilRemoveRegions(worlds[index].folder);
    RemoveDir(worlds[index].folder);

    for (int i = index; i < worldCount - 1; i++) worlds[i] = worlds[i + 1];
    worldCount--;

    if (activeId == id)
    {
        activeId = -1;
        activeName[0] = '\0';
        activeFolder[0] = '\0';
        activeSeed = 0;
    }

    return true;
}

bool RenameWorldRecord(int id, const char *name)
{
    int index = FindWorldIndexById(id);

    if (index < 0) return false;
    if (NameIsBlank(name)) return false;

    CopyName(worlds[index].name, sizeof(worlds[index].name), name);
    if (worlds[index].name[0] == '\0') return false;
    if (activeId == id) CopyText(activeName, sizeof(activeName), name);
    return WriteLevel(&worlds[index], false);
}

bool RecreateWorldRecord(int id)
{
    int index = FindWorldIndexById(id);

    if (index < 0) return false;

    worlds[index].createdUnix = (long)time(NULL);
    FormatCreated(worlds[index].createdUnix, worlds[index].createdText, sizeof(worlds[index].createdText));
    worlds[index].seed = MakeSeed(worlds[index].id + (int)worlds[index].createdUnix);
    if (activeId == id) activeSeed = worlds[index].seed;
    return WriteLevel(&worlds[index], true);
}

void SetActiveWorldId(int id)
{
    int index = FindWorldIndexById(id);

    if (index < 0)
    {
        activeId = -1;
        activeName[0] = '\0';
        activeFolder[0] = '\0';
        activeSeed = 0;
        return;
    }

    activeId = id;
    CopyText(activeName, sizeof(activeName), worlds[index].name);
    CopyText(activeFolder, sizeof(activeFolder), worlds[index].folder);
    activeSeed = worlds[index].seed;
}

int GetActiveWorldId(void)
{
    return activeId;
}

const char *GetActiveWorldName(void)
{
    return activeName;
}

const char *GetActiveWorldFolder(void)
{
    return activeFolder;
}

unsigned int GetActiveWorldSeed(void)
{
    return activeSeed;
}
