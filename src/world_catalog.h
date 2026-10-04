#ifndef WORLD_CATALOG_H
#define WORLD_CATALOG_H

#include <stdbool.h>

// World records live in saves/world_<id>/level.txt.
// The menu reads name, created time, mode, version, and seed.
// Chunk edits, inventory, and player pose are not stored yet.
// Those belong beside this file: player keys in level.txt, and
// modified chunks under region/, so a checkpoint is a copy of the folder.

#define MAX_WORLDS 32
#define WORLD_NAME_LENGTH 32

typedef struct {
    int id;
    char name[48];
    char folder[64];
    long createdUnix;
    char createdText[40];
    char mode[24];
    char version[16];
    unsigned int seed;
} WorldInfo;

void LoadWorldCatalog(const char *version);
int GetWorldCount(void);
const WorldInfo *GetWorld(int index);

// Returns the new world id, or -1 if the catalog is full or the name is empty.
int CreateWorldRecord(const char *name, const char *version);
bool DeleteWorldRecord(int id);
bool RenameWorldRecord(int id, const char *name);
bool RecreateWorldRecord(int id);

void SetActiveWorldId(int id);
int GetActiveWorldId(void);
const char *GetActiveWorldName(void);
const char *GetActiveWorldFolder(void);
unsigned int GetActiveWorldSeed(void);

#endif // WORLD_CATALOG_H
