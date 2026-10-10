#include "db.h"
#include "sha256.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void SetError(OcDb *db, const char *text)
{
    if (db == NULL) return;
    snprintf(db->lastError, sizeof(db->lastError), "%s", (text != NULL) ? text : "");
}

static int Exec(OcDb *db, const char *sql)
{
    char *err = NULL;
    int status = 0;

    status = sqlite3_exec((sqlite3 *)db->sqlite, sql, NULL, NULL, &err);
    if (status != SQLITE_OK)
    {
        SetError(db, (err != NULL) ? err : sqlite3_errmsg((sqlite3 *)db->sqlite));
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

static void ToHex(const unsigned char *in, int n, char *out)
{
    static const char *hex = "0123456789abcdef";
    int i = 0;

    for (i = 0; i < n; i++)
    {
        out[i*2] = hex[in[i] >> 4];
        out[i*2 + 1] = hex[in[i] & 15];
    }
    out[n*2] = '\0';
}

static int HashEqual(const char *a, const char *b)
{
    unsigned diff = 0;
    int i = 0;

    for (i = 0; i < 64; i++) diff |= (unsigned char)a[i] ^ (unsigned char)b[i];
    return (diff == 0) && (a[64] == '\0') && (b[64] == '\0');
}

static void HashPassword(const unsigned char salt[16], const char *pass, char hex[65])
{
    unsigned char buf[16 + OC_PASS_MAX];
    int passLen = (int)strlen(pass);

    memcpy(buf, salt, 16);
    memcpy(buf + 16, pass, (size_t)passLen);
    Sha256Hex(buf, (size_t)(16 + passLen), hex);
}

static int ReadSalt(unsigned char salt[16])
{
    FILE *file = fopen("/dev/urandom", "rb");

    if (file == NULL) return 0;
    if (fread(salt, 1, 16, file) != 16)
    {
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

static int TryVec(sqlite3 *sqlite)
{
    const char *paths[5];
    int i = 0;
    const char *env = getenv("SQLITE_VEC_PATH");

    paths[0] = (env != NULL) ? env : "";
    paths[1] = "vec0";
    paths[2] = "/usr/lib/sqlite3/vec0";
    paths[3] = "/usr/local/lib/vec0";
    paths[4] = "sqlite-vec";

    sqlite3_enable_load_extension(sqlite, 1);
    for (i = 0; i < 5; i++)
    {
        char *err = NULL;

        if ((paths[i] == NULL) || (paths[i][0] == '\0')) continue;
        if (sqlite3_load_extension(sqlite, paths[i], NULL, &err) == SQLITE_OK)
        {
            sqlite3_free(err);
            return 1;
        }
        sqlite3_free(err);
    }
    sqlite3_enable_load_extension(sqlite, 0);
    return 0;
}

static unsigned NewSeed(void)
{
    unsigned char bytes[4] = { 0 };
    unsigned seed = 0;
    FILE *file = fopen("/dev/urandom", "rb");

    if ((file != NULL) && (fread(bytes, 1, 4, file) == 4))
    {
        seed = (unsigned)bytes[0] | ((unsigned)bytes[1] << 8) | ((unsigned)bytes[2] << 16) | ((unsigned)bytes[3] << 24);
    }
    if (file != NULL) fclose(file);
    seed &= 0x7fffffffu;
    if (seed == 0) seed = 1337u;
    return seed;
}

static int EnsureSeed(OcDb *db)
{
    sqlite3_stmt *stmt = NULL;
    int status = 0;
    unsigned seed = 0;

    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite, "SELECT seed FROM world_meta WHERE id = 1", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    status = sqlite3_step(stmt);
    if (status == SQLITE_ROW)
    {
        db->seed = (unsigned)sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return 1;
    }
    sqlite3_finalize(stmt);

    seed = NewSeed();
    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite, "INSERT INTO world_meta(id, seed) VALUES (1, ?)", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)seed);
    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (status != SQLITE_DONE) return 0;
    db->seed = seed;
    return 1;
}

int OcDbOpen(OcDb *db, const char *path)
{
    sqlite3 *sqlite = NULL;
    int status = 0;

    if ((db == NULL) || (path == NULL) || (path[0] == '\0')) return 0;
    memset(db, 0, sizeof(*db));
    status = sqlite3_open_v2(path, &sqlite, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (status != SQLITE_OK)
    {
        SetError(db, (sqlite != NULL) ? sqlite3_errmsg(sqlite) : "could not open opencraft.db");
        if (sqlite != NULL) sqlite3_close(sqlite);
        return 0;
    }
    db->sqlite = sqlite;
    sqlite3_busy_timeout(sqlite, 2000);
    if (!Exec(db, "PRAGMA journal_mode=WAL;") || !Exec(db, "PRAGMA foreign_keys=ON;"))
    {
        OcDbClose(db);
        return 0;
    }
    if (!Exec(db,
        "CREATE TABLE IF NOT EXISTS accounts ("
        " id INTEGER PRIMARY KEY,"
        " username TEXT NOT NULL UNIQUE COLLATE NOCASE,"
        " password_hash TEXT NOT NULL,"
        " salt TEXT NOT NULL,"
        " email TEXT,"
        " created_unix INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS profiles ("
        " account_id INTEGER PRIMARY KEY REFERENCES accounts(id),"
        " selected_slot INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS profile_slots ("
        " account_id INTEGER NOT NULL REFERENCES accounts(id),"
        " slot INTEGER NOT NULL,"
        " block_id INTEGER NOT NULL,"
        " count INTEGER NOT NULL,"
        " PRIMARY KEY(account_id, slot));"
        "CREATE TABLE IF NOT EXISTS world_meta ("
        " id INTEGER PRIMARY KEY CHECK (id = 1),"
        " seed INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS block_edits ("
        " x INTEGER NOT NULL,"
        " y INTEGER NOT NULL,"
        " z INTEGER NOT NULL,"
        " block_id INTEGER NOT NULL,"
        " PRIMARY KEY(x, y, z));"
        "CREATE TABLE IF NOT EXISTS extension_status ("
        " name TEXT PRIMARY KEY,"
        " loaded INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS profile_pose ("
        " account_id INTEGER PRIMARY KEY REFERENCES accounts(id),"
        " pos_x REAL NOT NULL,"
        " pos_y REAL NOT NULL,"
        " pos_z REAL NOT NULL,"
        " yaw REAL NOT NULL,"
        " pitch REAL NOT NULL);"))
    {
        OcDbClose(db);
        return 0;
    }

    /* sqlite-vec stays optional. extension_status is the seam a later slice uses. */
    db->vecLoaded = TryVec(sqlite);
    {
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(sqlite, "INSERT OR REPLACE INTO extension_status(name, loaded) VALUES ('sqlite-vec', ?)", -1, &stmt, NULL) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, db->vecLoaded ? 1 : 0);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    if (!EnsureSeed(db))
    {
        SetError(db, "could not store the world seed");
        OcDbClose(db);
        return 0;
    }
    SetError(db, "");
    return 1;
}

void OcDbClose(OcDb *db)
{
    if ((db == NULL) || (db->sqlite == NULL)) return;
    sqlite3_close((sqlite3 *)db->sqlite);
    db->sqlite = NULL;
}

int OcDbVecLoaded(const OcDb *db)
{
    if (db == NULL) return 0;
    return db->vecLoaded;
}

unsigned OcDbSeed(const OcDb *db)
{
    if (db == NULL) return 0;
    return db->seed;
}

const char *OcDbLastError(const OcDb *db)
{
    if ((db == NULL) || (db->lastError[0] == '\0')) return "";
    return db->lastError;
}

static int WriteSlots(OcDb *db, const OcProfile *profile)
{
    sqlite3_stmt *stmt = NULL;
    int i = 0;
    int status = 0;

    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite, "DELETE FROM profile_slots WHERE account_id = ?", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)profile->accountId);
    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (status != SQLITE_DONE) return 0;

    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "INSERT INTO profile_slots(account_id, slot, block_id, count) VALUES (?, ?, ?, ?)", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    for (i = 0; i < OC_HOTBAR_SLOTS + OC_INV_SLOTS; i++)
    {
        unsigned block = 0;
        unsigned count = 0;

        if (i < OC_HOTBAR_SLOTS)
        {
            block = profile->hotbar[i];
            count = profile->hotbarCount[i];
        }
        else
        {
            block = profile->inv[i - OC_HOTBAR_SLOTS];
            count = profile->invCount[i - OC_HOTBAR_SLOTS];
        }
        if ((block == OC_BLOCK_AIR) || (count == 0)) continue;
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, (int)profile->accountId);
        sqlite3_bind_int(stmt, 2, i);
        sqlite3_bind_int(stmt, 3, (int)block);
        sqlite3_bind_int(stmt, 4, (int)count);
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            return 0;
        }
    }
    sqlite3_finalize(stmt);

    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "INSERT INTO profiles(account_id, selected_slot) VALUES (?, ?) "
        "ON CONFLICT(account_id) DO UPDATE SET selected_slot = excluded.selected_slot", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)profile->accountId);
    sqlite3_bind_int(stmt, 2, profile->selectedSlot);
    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return status == SQLITE_DONE;
}

static int ReadSlots(OcDb *db, OcProfile *profile)
{
    sqlite3_stmt *stmt = NULL;
    int status = 0;

    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite, "SELECT selected_slot FROM profiles WHERE account_id = ?", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)profile->accountId);
    status = sqlite3_step(stmt);
    if (status == SQLITE_ROW) profile->selectedSlot = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if ((profile->selectedSlot < 0) || (profile->selectedSlot >= OC_HOTBAR_SLOTS)) profile->selectedSlot = 0;

    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "SELECT slot, block_id, count FROM profile_slots WHERE account_id = ?", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)profile->accountId);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int slot = sqlite3_column_int(stmt, 0);
        int block = sqlite3_column_int(stmt, 1);
        int count = sqlite3_column_int(stmt, 2);

        if ((block < 0) || (block >= OC_BLOCK_COUNT)) continue;
        if (count < 0) count = 0;
        if (count > 64) count = 64;
        if ((slot >= 0) && (slot < OC_HOTBAR_SLOTS))
        {
            profile->hotbar[slot] = (unsigned short)block;
            profile->hotbarCount[slot] = (unsigned short)count;
        }
        else if ((slot >= OC_HOTBAR_SLOTS) && (slot < OC_HOTBAR_SLOTS + OC_INV_SLOTS))
        {
            int index = slot - OC_HOTBAR_SLOTS;
            profile->inv[index] = (unsigned short)block;
            profile->invCount[index] = (unsigned short)count;
        }
    }
    sqlite3_finalize(stmt);
    return 1;
}

int OcDbRegister(OcDb *db, const char *user, const char *pass, const char *email, OcProfile *out)
{
    sqlite3_stmt *stmt = NULL;
    unsigned char salt[16];
    char saltHex[33];
    char hash[65];
    int status = 0;
    const char *storedEmail = NULL;

    if ((db == NULL) || (db->sqlite == NULL) || (out == NULL)) return OC_REJECT_INTERNAL;
    if (!OcNameOk(user)) return OC_REJECT_NAME;
    if (!OcPasswordOk(pass)) return OC_REJECT_PASSWORD;
    if (!OcEmailOk(email)) return OC_REJECT_EMAIL;
    if (!ReadSalt(salt))
    {
        SetError(db, "could not salt the password");
        return OC_REJECT_INTERNAL;
    }
    ToHex(salt, 16, saltHex);
    HashPassword(salt, pass, hash);
    storedEmail = ((email != NULL) && (email[0] != '\0')) ? email : NULL;

    if (!Exec(db, "BEGIN IMMEDIATE")) return OC_REJECT_INTERNAL;
    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "INSERT INTO accounts(username, password_hash, salt, email, created_unix) VALUES (?, ?, ?, ?, ?)",
        -1, &stmt, NULL);
    if (status != SQLITE_OK)
    {
        Exec(db, "ROLLBACK");
        return OC_REJECT_INTERNAL;
    }
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, saltHex, -1, SQLITE_TRANSIENT);
    if (storedEmail != NULL) sqlite3_bind_text(stmt, 4, storedEmail, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(stmt, 4);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)time(NULL));
    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (status == SQLITE_CONSTRAINT)
    {
        Exec(db, "ROLLBACK");
        return OC_REJECT_TAKEN;
    }
    if (status != SQLITE_DONE)
    {
        Exec(db, "ROLLBACK");
        return OC_REJECT_INTERNAL;
    }

    OcProfileSetDefault(out, (unsigned)sqlite3_last_insert_rowid((sqlite3 *)db->sqlite), user);
    if (!WriteSlots(db, out))
    {
        Exec(db, "ROLLBACK");
        return OC_REJECT_INTERNAL;
    }
    if (!Exec(db, "COMMIT")) return OC_REJECT_INTERNAL;
    return 0;
}

int OcDbLogin(OcDb *db, const char *user, const char *pass, OcProfile *out)
{
    sqlite3_stmt *stmt = NULL;
    int status = 0;
    const char *hashCol = NULL;
    const char *saltCol = NULL;
    unsigned char salt[16];
    char actual[65];
    int i = 0;

    if ((db == NULL) || (db->sqlite == NULL) || (out == NULL)) return OC_REJECT_INTERNAL;
    if (!OcNameOk(user)) return OC_REJECT_NAME;
    if (!OcPasswordOk(pass)) return OC_REJECT_PASSWORD;

    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "SELECT id, password_hash, salt FROM accounts WHERE username = ? COLLATE NOCASE", -1, &stmt, NULL);
    if (status != SQLITE_OK) return OC_REJECT_INTERNAL;
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_TRANSIENT);
    status = sqlite3_step(stmt);
    if (status == SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return OC_REJECT_UNKNOWN;
    }
    if (status != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return OC_REJECT_INTERNAL;
    }

    OcProfileSetDefault(out, (unsigned)sqlite3_column_int(stmt, 0), user);
    hashCol = (const char *)sqlite3_column_text(stmt, 1);
    saltCol = (const char *)sqlite3_column_text(stmt, 2);
    if ((hashCol == NULL) || (saltCol == NULL) || (strlen(saltCol) != 32) || (strlen(hashCol) != 64))
    {
        sqlite3_finalize(stmt);
        return OC_REJECT_INTERNAL;
    }
    for (i = 0; i < 16; i++)
    {
        unsigned byte = 0;
        if (sscanf(saltCol + i*2, "%2x", &byte) != 1)
        {
            sqlite3_finalize(stmt);
            return OC_REJECT_INTERNAL;
        }
        salt[i] = (unsigned char)byte;
    }
    HashPassword(salt, pass, actual);
    if (!HashEqual(actual, hashCol))
    {
        sqlite3_finalize(stmt);
        return OC_REJECT_PASSWORD;
    }
    sqlite3_finalize(stmt);
    snprintf(out->username, sizeof(out->username), "%s", user);
    if (!ReadSlots(db, out)) return OC_REJECT_INTERNAL;
    return 0;
}

int OcDbSaveProfile(OcDb *db, const OcProfile *profile)
{
    if ((db == NULL) || (db->sqlite == NULL) || (profile == NULL) || (profile->accountId == 0)) return 0;
    if ((profile->selectedSlot < 0) || (profile->selectedSlot >= OC_HOTBAR_SLOTS)) return 0;
    if (!Exec(db, "BEGIN IMMEDIATE")) return 0;
    if (!WriteSlots(db, profile))
    {
        Exec(db, "ROLLBACK");
        return 0;
    }
    return Exec(db, "COMMIT");
}

int OcDbLoadProfile(OcDb *db, unsigned accountId, OcProfile *out)
{
    sqlite3_stmt *stmt = NULL;
    int status = 0;

    if ((db == NULL) || (db->sqlite == NULL) || (out == NULL) || (accountId == 0)) return 0;
    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite, "SELECT username FROM accounts WHERE id = ?", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)accountId);
    status = sqlite3_step(stmt);
    if (status != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return 0;
    }
    OcProfileSetDefault(out, accountId, (const char *)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return ReadSlots(db, out);
}

int OcDbPutBlock(OcDb *db, int x, int y, int z, unsigned block)
{
    sqlite3_stmt *stmt = NULL;
    int status = 0;

    if ((db == NULL) || (db->sqlite == NULL)) return 0;
    if ((y < 0) || (y >= 128) || (block >= OC_BLOCK_COUNT)) return 0;
    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "INSERT INTO block_edits(x, y, z, block_id) VALUES (?, ?, ?, ?) "
        "ON CONFLICT(x, y, z) DO UPDATE SET block_id = excluded.block_id", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, x);
    sqlite3_bind_int(stmt, 2, y);
    sqlite3_bind_int(stmt, 3, z);
    sqlite3_bind_int(stmt, 4, (int)block);
    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return status == SQLITE_DONE;
}

int OcDbLoadEdits(OcDb *db, OcEdit *edits, int cap, int *count)
{
    sqlite3_stmt *stmt = NULL;
    int n = 0;

    if ((db == NULL) || (db->sqlite == NULL) || (count == NULL)) return 0;
    *count = 0;
    if (sqlite3_prepare_v2((sqlite3 *)db->sqlite, "SELECT x, y, z, block_id FROM block_edits ORDER BY y, z, x", -1, &stmt, NULL) != SQLITE_OK)
    {
        return 0;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (n >= cap) break;
        edits[n].x = sqlite3_column_int(stmt, 0);
        edits[n].y = sqlite3_column_int(stmt, 1);
        edits[n].z = sqlite3_column_int(stmt, 2);
        edits[n].block = (unsigned short)sqlite3_column_int(stmt, 3);
        n++;
    }
    sqlite3_finalize(stmt);
    *count = n;
    return 1;
}

int OcDbSavePose(OcDb *db, unsigned accountId, float x, float y, float z, float yaw, float pitch)
{
    sqlite3_stmt *stmt = NULL;
    int status = 0;

    if ((db == NULL) || (db->sqlite == NULL) || (accountId == 0)) return 0;
    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "INSERT INTO profile_pose(account_id, pos_x, pos_y, pos_z, yaw, pitch) VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(account_id) DO UPDATE SET pos_x = excluded.pos_x, pos_y = excluded.pos_y, "
        "pos_z = excluded.pos_z, yaw = excluded.yaw, pitch = excluded.pitch", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)accountId);
    sqlite3_bind_double(stmt, 2, (double)x);
    sqlite3_bind_double(stmt, 3, (double)y);
    sqlite3_bind_double(stmt, 4, (double)z);
    sqlite3_bind_double(stmt, 5, (double)yaw);
    sqlite3_bind_double(stmt, 6, (double)pitch);
    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return status == SQLITE_DONE;
}

int OcDbLoadPose(OcDb *db, unsigned accountId, float *x, float *y, float *z, float *yaw, float *pitch)
{
    sqlite3_stmt *stmt = NULL;
    int status = 0;

    if ((db == NULL) || (db->sqlite == NULL) || (accountId == 0)) return 0;
    if ((x == NULL) || (y == NULL) || (z == NULL) || (yaw == NULL) || (pitch == NULL)) return 0;
    status = sqlite3_prepare_v2((sqlite3 *)db->sqlite,
        "SELECT pos_x, pos_y, pos_z, yaw, pitch FROM profile_pose WHERE account_id = ?", -1, &stmt, NULL);
    if (status != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, (int)accountId);
    status = sqlite3_step(stmt);
    if (status != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return 0;
    }
    *x = (float)sqlite3_column_double(stmt, 0);
    *y = (float)sqlite3_column_double(stmt, 1);
    *z = (float)sqlite3_column_double(stmt, 2);
    *yaw = (float)sqlite3_column_double(stmt, 3);
    *pitch = (float)sqlite3_column_double(stmt, 4);
    sqlite3_finalize(stmt);
    return 1;
}
