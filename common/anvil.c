#include "anvil.h"
#include "nbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
    #include <direct.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#define SECTOR_BYTES 4096
#define HEADER_BYTES (SECTOR_BYTES*2)
#define CHUNKS_PER_REGION 1024
#define MAX_REGION_BYTES (32*1024*1024)

static void MakeDir(const char *path)
{
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static int RegionCoord(int chunk)
{
    int region = chunk/32;

    if ((chunk < 0) && ((chunk%32) != 0)) region--;
    return region;
}

static int LocalCoord(int chunk)
{
    int local = chunk%32;

    if (local < 0) local += 32;
    return local;
}

static uint32_t ReadU32(const unsigned char *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static void WriteU32(unsigned char *bytes, uint32_t value)
{
    bytes[0] = (unsigned char)((value >> 24) & 0xff);
    bytes[1] = (unsigned char)((value >> 16) & 0xff);
    bytes[2] = (unsigned char)((value >> 8) & 0xff);
    bytes[3] = (unsigned char)(value & 0xff);
}

static bool ReadFileAll(const char *path, unsigned char **outData, int *outSize)
{
    FILE *file = NULL;
    long length = 0;
    unsigned char *data = NULL;

    file = fopen(path, "rb");
    if (file == NULL) return false;
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return false;
    }

    length = ftell(file);
    if ((length < 0) || (length > MAX_REGION_BYTES))
    {
        fclose(file);
        return false;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return false;
    }

    data = (unsigned char *)malloc((size_t)length + 1);
    if (data == NULL)
    {
        fclose(file);
        return false;
    }

    if ((length > 0) && (fread(data, 1, (size_t)length, file) != (size_t)length))
    {
        free(data);
        fclose(file);
        return false;
    }

    fclose(file);
    *outData = data;
    *outSize = (int)length;
    return true;
}

static bool WriteFileAtomic(const char *path, const unsigned char *data, int size)
{
    char temp[512] = { 0 };
    FILE *file = NULL;

    if (snprintf(temp, sizeof(temp), "%s.tmp", path) >= (int)sizeof(temp)) return false;
    file = fopen(temp, "wb");
    if (file == NULL) return false;
    if ((size > 0) && (fwrite(data, 1, (size_t)size, file) != (size_t)size))
    {
        fclose(file);
        remove(temp);
        return false;
    }

    if (fclose(file) != 0)
    {
        remove(temp);
        return false;
    }

    if (rename(temp, path) != 0)
    {
        remove(temp);
        return false;
    }

    return true;
}

static void RegionPath(char *path, int pathSize, const char *worldFolder, int regionX, int regionZ)
{
    snprintf(path, (size_t)pathSize, "%s/region/r.%d.%d.mca", worldFolder, regionX, regionZ);
}

bool AnvilWriteChunk(const char *worldFolder, int chunkX, int chunkZ, const unsigned char *nbt, int nbtSize)
{
    int regionX = RegionCoord(chunkX);
    int regionZ = RegionCoord(chunkZ);
    int local = LocalCoord(chunkX) + LocalCoord(chunkZ)*32;
    char folder[160] = { 0 };
    char path[192] = { 0 };
    unsigned char *fileData = NULL;
    int fileSize = 0;
    unsigned char *compressed = NULL;
    int compressedSize = 0;
    int recordSize = 0;
    int sectors = 0;
    int oldOffset = 0;
    int oldSectors = 0;
    int offset = 0;
    uint32_t location = 0;
    bool ok = false;

    if ((worldFolder == NULL) || (worldFolder[0] == '\0') || (nbt == NULL) || (nbtSize <= 0)) return false;
    if ((local < 0) || (local >= CHUNKS_PER_REGION)) return false;
    if (!NbtZlibCompress(nbt, nbtSize, &compressed, &compressedSize)) return false;

    recordSize = 4 + 1 + compressedSize;
    sectors = (recordSize + SECTOR_BYTES - 1)/SECTOR_BYTES;
    if ((sectors < 1) || (sectors > 255))
    {
        NbtFreeBytes(compressed);
        return false;
    }

    snprintf(folder, sizeof(folder), "%s/region", worldFolder);
    MakeDir(worldFolder);
    MakeDir(folder);
    RegionPath(path, sizeof(path), worldFolder, regionX, regionZ);

    if (!ReadFileAll(path, &fileData, &fileSize))
    {
        fileSize = HEADER_BYTES;
        fileData = (unsigned char *)calloc((size_t)fileSize, 1);
        if (fileData == NULL)
        {
            NbtFreeBytes(compressed);
            return false;
        }
    }

    if (fileSize < HEADER_BYTES)
    {
        unsigned char *grown = (unsigned char *)realloc(fileData, HEADER_BYTES);
        if (grown == NULL)
        {
            free(fileData);
            NbtFreeBytes(compressed);
            return false;
        }
        memset(grown + fileSize, 0, (size_t)(HEADER_BYTES - fileSize));
        fileData = grown;
        fileSize = HEADER_BYTES;
    }

    location = ReadU32(fileData + local*4);
    oldOffset = (int)(location >> 8);
    oldSectors = (int)(location & 0xff);

    if ((oldOffset >= 2) && (oldSectors >= sectors) && (oldOffset*SECTOR_BYTES + oldSectors*SECTOR_BYTES <= fileSize))
    {
        offset = oldOffset;
        sectors = oldSectors;
    }
    else
    {
        int padded = (fileSize + SECTOR_BYTES - 1)/SECTOR_BYTES*SECTOR_BYTES;
        unsigned char *grown = NULL;

        if (padded + sectors*SECTOR_BYTES > MAX_REGION_BYTES)
        {
            free(fileData);
            NbtFreeBytes(compressed);
            return false;
        }

        grown = (unsigned char *)realloc(fileData, (size_t)(padded + sectors*SECTOR_BYTES));
        if (grown == NULL)
        {
            free(fileData);
            NbtFreeBytes(compressed);
            return false;
        }

        if (padded > fileSize) memset(grown + fileSize, 0, (size_t)(padded - fileSize));
        fileData = grown;
        fileSize = padded;
        offset = fileSize/SECTOR_BYTES;
        fileSize += sectors*SECTOR_BYTES;
    }

    memset(fileData + offset*SECTOR_BYTES, 0, (size_t)(sectors*SECTOR_BYTES));
    WriteU32(fileData + offset*SECTOR_BYTES, (uint32_t)(1 + compressedSize));
    fileData[offset*SECTOR_BYTES + 4] = 2;
    memcpy(fileData + offset*SECTOR_BYTES + 5, compressed, (size_t)compressedSize);
    WriteU32(fileData + local*4, ((uint32_t)offset << 8) | (uint32_t)sectors);
    WriteU32(fileData + SECTOR_BYTES + local*4, (uint32_t)time(NULL));

    ok = WriteFileAtomic(path, fileData, fileSize);
    free(fileData);
    NbtFreeBytes(compressed);
    return ok;
}

int AnvilReadChunk(const char *worldFolder, int chunkX, int chunkZ, unsigned char **outNbt, int *outSize)
{
    int regionX = RegionCoord(chunkX);
    int regionZ = RegionCoord(chunkZ);
    int local = LocalCoord(chunkX) + LocalCoord(chunkZ)*32;
    char path[192] = { 0 };
    unsigned char *fileData = NULL;
    int fileSize = 0;
    uint32_t location = 0;
    int offset = 0;
    int sectors = 0;
    int recordAt = 0;
    uint32_t length = 0;
    int compression = 0;
    unsigned char *nbt = NULL;
    int nbtSize = 0;

    if ((worldFolder == NULL) || (outNbt == NULL) || (outSize == NULL)) return -1;
    RegionPath(path, sizeof(path), worldFolder, regionX, regionZ);
    if (!ReadFileAll(path, &fileData, &fileSize)) return 0;
    if (fileSize < HEADER_BYTES)
    {
        free(fileData);
        return 0;
    }

    location = ReadU32(fileData + local*4);
    offset = (int)(location >> 8);
    sectors = (int)(location & 0xff);
    if ((offset < 2) || (sectors < 1))
    {
        free(fileData);
        return 0;
    }

    recordAt = offset*SECTOR_BYTES;
    if (recordAt + 5 > fileSize)
    {
        free(fileData);
        return -1;
    }

    length = ReadU32(fileData + recordAt);
    compression = fileData[recordAt + 4];
    if ((length < 1) || (recordAt + 4 + (int)length > fileSize))
    {
        free(fileData);
        return -1;
    }

    if (compression != 2)
    {
        free(fileData);
        return -1;
    }

    if (!NbtZlibDecompress(fileData + recordAt + 5, (int)length - 1, &nbt, &nbtSize, 1024*1024))
    {
        free(fileData);
        return -1;
    }

    free(fileData);
    *outNbt = nbt;
    *outSize = nbtSize;
    return 1;
}

void AnvilRemoveRegions(const char *worldFolder)
{
    char folder[160] = { 0 };

    if ((worldFolder == NULL) || (worldFolder[0] == '\0')) return;
    snprintf(folder, sizeof(folder), "%s/region", worldFolder);

#if !defined(_WIN32)
    {
        DIR *dir = opendir(folder);
        struct dirent *entry = NULL;

        if (dir != NULL)
        {
            while ((entry = readdir(dir)) != NULL)
            {
                char path[320] = { 0 };

                if (entry->d_name[0] == '.') continue;
                snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
                remove(path);
            }
            closedir(dir);
        }
    }
#endif

#if defined(_WIN32)
    _rmdir(folder);
#else
    rmdir(folder);
#endif
}
