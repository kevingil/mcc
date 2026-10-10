#ifndef NBT_H
#define NBT_H

#include <stdbool.h>
#include <stdint.h>

//----------------------------------------------------------------------------------
// Named Binary Tag
// Big-endian compounds, lists, and arrays. level.dat is this payload inside gzip.
// Region chunks are this payload inside zlib.
//----------------------------------------------------------------------------------
enum {
    NBT_END = 0,
    NBT_BYTE = 1,
    NBT_SHORT = 2,
    NBT_INT = 3,
    NBT_LONG = 4,
    NBT_FLOAT = 5,
    NBT_DOUBLE = 6,
    NBT_BYTE_ARRAY = 7,
    NBT_STRING = 8,
    NBT_LIST = 9,
    NBT_COMPOUND = 10,
    NBT_INT_ARRAY = 11,
    NBT_LONG_ARRAY = 12
};

typedef struct {
    unsigned char *data;
    int size;
    int capacity;
    int error;
} NbtBuf;

typedef struct {
    const unsigned char *data;
    int size;
    int pos;
    int error;
    int depth;
} NbtIn;

void NbtBufInit(NbtBuf *buf);
void NbtBufFree(NbtBuf *buf);

void NbtStartCompound(NbtBuf *buf, const char *name);
void NbtEndCompound(NbtBuf *buf);
void NbtStartList(NbtBuf *buf, const char *name, int elementType, int count);
void NbtByte(NbtBuf *buf, const char *name, int value);
void NbtInt(NbtBuf *buf, const char *name, int value);
void NbtLong(NbtBuf *buf, const char *name, int64_t value);
void NbtFloat(NbtBuf *buf, const char *name, float value);
void NbtDouble(NbtBuf *buf, const char *name, double value);
void NbtString(NbtBuf *buf, const char *name, const char *value);
void NbtLongArray(NbtBuf *buf, const char *name, const uint64_t *values, int count);

void NbtRawByte(NbtBuf *buf, int value);
void NbtRawInt(NbtBuf *buf, int value);
void NbtRawLong(NbtBuf *buf, int64_t value);
void NbtRawFloat(NbtBuf *buf, float value);
void NbtRawDouble(NbtBuf *buf, double value);

void NbtInInit(NbtIn *in, const unsigned char *data, int size);
int NbtInU8(NbtIn *in);
int NbtInI32(NbtIn *in);
int64_t NbtInI64(NbtIn *in);
float NbtInF32(NbtIn *in);
double NbtInF64(NbtIn *in);
void NbtInReadString(NbtIn *in, char *out, int cap);
bool NbtInNext(NbtIn *in, char *name, int nameCap, int *type);
void NbtInSkip(NbtIn *in, int type);
void NbtInListHeader(NbtIn *in, int *elementType, int *count);

bool NbtWriteGzip(const char *path, const unsigned char *data, int size);
bool NbtReadGzip(const char *path, unsigned char **outData, int *outSize);
bool NbtZlibCompress(const unsigned char *data, int size, unsigned char **outData, int *outSize);
bool NbtZlibDecompress(const unsigned char *data, int size, unsigned char **outData, int *outSize, int maxOut);
void NbtFreeBytes(void *data);

#endif
