#ifndef DB_H
#define DB_H

#include "protocol.h"

/*
 * opencraft.db is the one SQLite file for accounts, inventories, and
 * multiplayer block edits. WAL mode. sqlite-vec is loaded when the
 * extension is installed and is not required.
 *
 * This slice calls sqlite3_step on the caller. The server calls it from
 * the same thread that accepts sockets and counts 20 Hz ticks. There is
 * no Kore loop here, so a completion queue would only bounce the same
 * write onto another thread. S31 still owns that writer-thread API.
 */

typedef struct OcDb {
    void *sqlite;
    int vecLoaded;
    unsigned seed;
    char lastError[160];
} OcDb;

int OcDbOpen(OcDb *db, const char *path);
void OcDbClose(OcDb *db);
int OcDbVecLoaded(const OcDb *db);
unsigned OcDbSeed(const OcDb *db);
const char *OcDbLastError(const OcDb *db);

/* 0 on success. Otherwise an OC_REJECT_* code. */
int OcDbRegister(OcDb *db, const char *user, const char *pass, const char *email, OcProfile *out);
int OcDbLogin(OcDb *db, const char *user, const char *pass, OcProfile *out);
int OcDbSaveProfile(OcDb *db, const OcProfile *profile);
int OcDbLoadProfile(OcDb *db, unsigned accountId, OcProfile *out);
int OcDbPutBlock(OcDb *db, int x, int y, int z, unsigned block);
int OcDbLoadEdits(OcDb *db, OcEdit *edits, int cap, int *count);

#endif
