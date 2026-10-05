#include "anvil.h"
#include "chunk_codec.h"
#include "client_log.h"
#include "level_data.h"
#include "nbt.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

static void TestNbtRoundTrip(void)
{
    NbtBuf buf = { 0 };
    NbtIn in = { 0 };
    unsigned char *gzipped = NULL;
    int gzipSize = 0;
    unsigned char *plain = NULL;
    int plainSize = 0;
    char name[64] = { 0 };
    int type = 0;
    int root = 0;
    char text[32] = { 0 };
    int number = 0;
    uint64_t longs[2] = { 0 };
    int longCount = 0;
    char path[] = "/tmp/opencraft-nbt-test.dat";

    NbtBufInit(&buf);
    NbtStartCompound(&buf, "");
    NbtString(&buf, "Name", "New World");
    NbtInt(&buf, "Count", 7);
    NbtStartList(&buf, "Pos", NBT_DOUBLE, 2);
    NbtRawDouble(&buf, 1.5);
    NbtRawDouble(&buf, -2.0);
    NbtLongArray(&buf, "data", (uint64_t[]){ 0x11ull, 0x22ull }, 2);
    NbtEndCompound(&buf);
    CHECK(buf.error == 0);

    CHECK(NbtWriteGzip(path, buf.data, buf.size));
    CHECK(NbtReadGzip(path, &plain, &plainSize));
    CHECK(plainSize == buf.size);
    CHECK(memcmp(plain, buf.data, (size_t)buf.size) == 0);

    NbtInInit(&in, plain, plainSize);
    root = NbtInU8(&in);
    NbtInReadString(&in, name, sizeof(name));
    CHECK(root == NBT_COMPOUND);
    while (NbtInNext(&in, name, sizeof(name), &type))
    {
        if (strcmp(name, "Name") == 0)
        {
            NbtInReadString(&in, text, sizeof(text));
        }
        else if (strcmp(name, "Count") == 0)
        {
            number = NbtInI32(&in);
        }
        else if (strcmp(name, "data") == 0)
        {
            longCount = NbtInI32(&in);
            longs[0] = (uint64_t)NbtInI64(&in);
            longs[1] = (uint64_t)NbtInI64(&in);
        }
        else NbtInSkip(&in, type);
    }

    CHECK(!in.error);
    CHECK(strcmp(text, "New World") == 0);
    CHECK(number == 7);
    CHECK(longCount == 2);
    CHECK(longs[0] == 0x11ull);
    CHECK(longs[1] == 0x22ull);
    (void)gzipped;
    (void)gzipSize;

    NbtFreeBytes(plain);
    NbtBufFree(&buf);
    remove(path);
}

static void FillPattern(int *blocks, int value)
{
    int i = 0;

    for (i = 0; i < SAVE_BLOCKS_PER_CHUNK; i++) blocks[i] = value;
}

static void TestChunkCodec(void)
{
    int *blocks = (int *)calloc(SAVE_BLOCKS_PER_CHUNK, sizeof(int));
    int *readBack = (int *)calloc(SAVE_BLOCKS_PER_CHUNK, sizeof(int));
    NbtBuf buf = { 0 };
    int chunkX = 0;
    int chunkZ = 0;
    int index = 0;

    CHECK(blocks != NULL);
    CHECK(readBack != NULL);
    FillPattern(blocks, 5);
    NbtBufInit(&buf);
    CHECK(ChunkCodecWrite(3, -4, blocks, &buf));
    CHECK(ChunkCodecRead(buf.data, buf.size, &chunkX, &chunkZ, readBack));
    CHECK(chunkX == 3);
    CHECK(chunkZ == -4);
    CHECK(memcmp(blocks, readBack, sizeof(int)*SAVE_BLOCKS_PER_CHUNK) == 0);
    NbtBufFree(&buf);

    blocks[0] = 1;
    index = 1 + SAVE_CHUNK_SIZE*(0 + SAVE_CHUNK_SIZE*0);
    blocks[index] = 2;
    index = 0 + SAVE_CHUNK_SIZE*(0 + SAVE_CHUNK_SIZE*16);
    blocks[index] = 9;
    NbtBufInit(&buf);
    CHECK(ChunkCodecWrite(-2, 8, blocks, &buf));
    memset(readBack, 0, sizeof(int)*SAVE_BLOCKS_PER_CHUNK);
    CHECK(ChunkCodecRead(buf.data, buf.size, &chunkX, &chunkZ, readBack));
    CHECK(chunkX == -2);
    CHECK(chunkZ == 8);
    CHECK(memcmp(blocks, readBack, sizeof(int)*SAVE_BLOCKS_PER_CHUNK) == 0);

    NbtBufFree(&buf);
    free(blocks);
    free(readBack);
}

static void TestAnvil(void)
{
    const char *folder = "/tmp/opencraft-anvil-test";
    int *blocks = (int *)calloc(SAVE_BLOCKS_PER_CHUNK, sizeof(int));
    int *readBack = (int *)calloc(SAVE_BLOCKS_PER_CHUNK, sizeof(int));
    NbtBuf first = { 0 };
    NbtBuf second = { 0 };
    unsigned char *nbt = NULL;
    int nbtSize = 0;
    int chunkX = 0;
    int chunkZ = 0;

    mkdir(folder, 0755);
    FillPattern(blocks, 4);
    blocks[0] = 8;
    NbtBufInit(&first);
    CHECK(ChunkCodecWrite(0, 0, blocks, &first));
    CHECK(AnvilWriteChunk(folder, 0, 0, first.data, first.size));
    CHECK(AnvilReadChunk(folder, 0, 0, &nbt, &nbtSize) == 1);
    CHECK(ChunkCodecRead(nbt, nbtSize, &chunkX, &chunkZ, readBack));
    CHECK(memcmp(blocks, readBack, sizeof(int)*SAVE_BLOCKS_PER_CHUNK) == 0);
    NbtFreeBytes(nbt);

    blocks[0] = 12;
    NbtBufInit(&second);
    CHECK(ChunkCodecWrite(0, 0, blocks, &second));
    CHECK(AnvilWriteChunk(folder, 0, 0, second.data, second.size));
    CHECK(AnvilReadChunk(folder, 0, 0, &nbt, &nbtSize) == 1);
    CHECK(ChunkCodecRead(nbt, nbtSize, &chunkX, &chunkZ, readBack));
    CHECK(readBack[0] == 12);
    NbtFreeBytes(nbt);

    CHECK(AnvilReadChunk(folder, 1, 0, &nbt, &nbtSize) == 0);

    blocks[3] = 15;
    NbtBufFree(&first);
    NbtBufInit(&first);
    CHECK(ChunkCodecWrite(-1, -40, blocks, &first));
    CHECK(AnvilWriteChunk(folder, -1, -40, first.data, first.size));
    CHECK(AnvilReadChunk(folder, -1, -40, &nbt, &nbtSize) == 1);
    memset(readBack, 0, sizeof(int)*SAVE_BLOCKS_PER_CHUNK);
    CHECK(ChunkCodecRead(nbt, nbtSize, &chunkX, &chunkZ, readBack));
    CHECK(chunkX == -1);
    CHECK(chunkZ == -40);
    CHECK(readBack[3] == 15);
    NbtFreeBytes(nbt);

    AnvilRemoveRegions(folder);
    CHECK(AnvilReadChunk(folder, 0, 0, &nbt, &nbtSize) == 0);
    rmdir(folder);
    NbtBufFree(&first);
    NbtBufFree(&second);
    free(blocks);
    free(readBack);
}

static void TestLevelData(void)
{
    const char *folder = "/tmp/opencraft-level-test";
    LevelData level;
    LevelData loaded;

    mkdir(folder, 0755);
    LevelDataInit(&level);
    LevelDataSetMeta(&level, "New World", "Survival", "0.1.0", 99u, 1700000000);
    level.hasPlayer = true;
    level.posX = 4.5;
    level.posY = 70.0;
    level.posZ = -3.25;
    level.yaw = 90.0f;
    level.pitch = -10.0f;
    level.selectedSlot = 2;
    level.spawnX = 1;
    level.spawnY = 64;
    level.spawnZ = 2;
    level.itemCount = 2;
    level.items[0].slot = 0;
    level.items[0].block = 3;
    level.items[0].count = 64;
    level.items[1].slot = 9;
    level.items[1].block = 5;
    level.items[1].count = 12;

    CHECK(LevelDataWrite(folder, &level));
    CHECK(LevelDataRead(folder, &loaded));
    CHECK(strcmp(loaded.name, "New World") == 0);
    CHECK(loaded.seed == 99u);
    CHECK(loaded.createdUnix == 1700000000);
    CHECK(loaded.hasPlayer);
    CHECK(loaded.posX == 4.5);
    CHECK(loaded.posZ == -3.25);
    CHECK(loaded.yaw == 90.0f);
    CHECK(loaded.selectedSlot == 2);
    CHECK(loaded.spawnX == 1);
    CHECK(loaded.itemCount == 2);
    CHECK(loaded.items[0].slot == 0);
    CHECK(loaded.items[0].block == 3);
    CHECK(loaded.items[1].slot == 9);
    CHECK(loaded.items[1].count == 12);

    remove("/tmp/opencraft-level-test/level.dat");
    rmdir(folder);
}

static int FileContains(const char *path, const char *needle)
{
    FILE *file = fopen(path, "rb");
    char data[1024] = { 0 };
    size_t count = 0;

    if (file == NULL) return 0;
    count = fread(data, 1, sizeof(data) - 1, file);
    fclose(file);
    data[count] = '\0';
    return strstr(data, needle) != NULL;
}

static void TestClientLog(void)
{
    const char *dir = "/tmp/opencraft-log-test";
    char latest[128] = { 0 };

    mkdir(dir, 0755);
    snprintf(latest, sizeof(latest), "%s/latest.log", dir);
    remove(latest);

    ClientLogInit(dir);
    ClientLog("hello save");
    ClientLogClose();
    CHECK(FileContains(latest, "hello save"));
    CHECK(FileContains(latest, "[Client thread/INFO]"));

    ClientLogInit(dir);
    ClientLog("second session");
    ClientLogClose();
    CHECK(FileContains(latest, "second session"));
    CHECK(!FileContains(latest, "hello save"));

    {
        DIR *directory = opendir(dir);
        struct dirent *entry = NULL;
        int found = 0;

        CHECK(directory != NULL);
        if (directory != NULL)
        {
            while ((entry = readdir(directory)) != NULL)
            {
                char path[256] = { 0 };
                gzFile gz = NULL;
                char data[1024] = { 0 };

                if (strstr(entry->d_name, ".log.gz") == NULL) continue;
                snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
                gz = gzopen(path, "rb");
                if (gz == NULL) continue;
                gzread(gz, data, sizeof(data) - 1);
                gzclose(gz);
                if (strstr(data, "hello save") != NULL) found = 1;
            }
            closedir(directory);
        }
        CHECK(found == 1);
    }
}

int main(void)
{
    TestNbtRoundTrip();
    TestChunkCodec();
    TestAnvil();
    TestLevelData();
    TestClientLog();

    if (failures == 0)
    {
        printf("save format tests passed\n");
        return 0;
    }

    printf("%d save format checks failed\n", failures);
    return 1;
}
