#include "nbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define NBT_MAX_DEPTH 32
#define NBT_MAX_FILE (8*1024*1024)

//----------------------------------------------------------------------------------
// Buffer
//----------------------------------------------------------------------------------
void NbtBufInit(NbtBuf *buf)
{
    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;
    buf->error = 0;
}

void NbtBufFree(NbtBuf *buf)
{
    free(buf->data);
    NbtBufInit(buf);
}

void NbtFreeBytes(void *data)
{
    free(data);
}

static void WriteBytes(NbtBuf *buf, const void *bytes, int count)
{
    int need = 0;

    if ((buf->error) || (count <= 0)) return;

    need = buf->size + count;
    if (need > buf->capacity)
    {
        int capacity = (buf->capacity > 0) ? buf->capacity : 256;
        unsigned char *grown = NULL;

        while (capacity < need) capacity *= 2;
        grown = (unsigned char *)realloc(buf->data, (size_t)capacity);
        if (grown == NULL)
        {
            buf->error = 1;
            return;
        }

        buf->data = grown;
        buf->capacity = capacity;
    }

    memcpy(buf->data + buf->size, bytes, (size_t)count);
    buf->size += count;
}

static void WriteU8(NbtBuf *buf, unsigned value)
{
    unsigned char byte = (unsigned char)value;
    WriteBytes(buf, &byte, 1);
}

static void WriteU16(NbtBuf *buf, unsigned value)
{
    unsigned char bytes[2];

    bytes[0] = (unsigned char)((value >> 8) & 0xff);
    bytes[1] = (unsigned char)(value & 0xff);
    WriteBytes(buf, bytes, 2);
}

static void WriteU32(NbtBuf *buf, uint32_t value)
{
    unsigned char bytes[4];

    bytes[0] = (unsigned char)((value >> 24) & 0xff);
    bytes[1] = (unsigned char)((value >> 16) & 0xff);
    bytes[2] = (unsigned char)((value >> 8) & 0xff);
    bytes[3] = (unsigned char)(value & 0xff);
    WriteBytes(buf, bytes, 4);
}

static void WriteU64(NbtBuf *buf, uint64_t value)
{
    unsigned char bytes[8];
    int i = 0;

    for (i = 0; i < 8; i++) bytes[i] = (unsigned char)((value >> (8*(7 - i))) & 0xff);
    WriteBytes(buf, bytes, 8);
}

static void WriteName(NbtBuf *buf, const char *name)
{
    int length = 0;

    if (name == NULL) name = "";
    length = (int)strlen(name);
    if (length > 65535) length = 65535;
    WriteU16(buf, (unsigned)length);
    WriteBytes(buf, name, length);
}

static void WriteNamed(NbtBuf *buf, int type, const char *name)
{
    WriteU8(buf, (unsigned)type);
    WriteName(buf, name);
}

void NbtStartCompound(NbtBuf *buf, const char *name)
{
    WriteNamed(buf, NBT_COMPOUND, name);
}

void NbtEndCompound(NbtBuf *buf)
{
    WriteU8(buf, NBT_END);
}

void NbtStartList(NbtBuf *buf, const char *name, int elementType, int count)
{
    WriteNamed(buf, NBT_LIST, name);
    WriteU8(buf, (unsigned)elementType);
    WriteU32(buf, (uint32_t)count);
}

void NbtRawByte(NbtBuf *buf, int value)
{
    WriteU8(buf, (unsigned)(value & 0xff));
}

void NbtRawInt(NbtBuf *buf, int value)
{
    WriteU32(buf, (uint32_t)value);
}

void NbtRawLong(NbtBuf *buf, int64_t value)
{
    WriteU64(buf, (uint64_t)value);
}

void NbtRawFloat(NbtBuf *buf, float value)
{
    uint32_t bits = 0;

    memcpy(&bits, &value, sizeof(bits));
    WriteU32(buf, bits);
}

void NbtRawDouble(NbtBuf *buf, double value)
{
    uint64_t bits = 0;

    memcpy(&bits, &value, sizeof(bits));
    WriteU64(buf, bits);
}

void NbtByte(NbtBuf *buf, const char *name, int value)
{
    WriteNamed(buf, NBT_BYTE, name);
    NbtRawByte(buf, value);
}

void NbtInt(NbtBuf *buf, const char *name, int value)
{
    WriteNamed(buf, NBT_INT, name);
    NbtRawInt(buf, value);
}

void NbtLong(NbtBuf *buf, const char *name, int64_t value)
{
    WriteNamed(buf, NBT_LONG, name);
    NbtRawLong(buf, value);
}

void NbtFloat(NbtBuf *buf, const char *name, float value)
{
    WriteNamed(buf, NBT_FLOAT, name);
    NbtRawFloat(buf, value);
}

void NbtDouble(NbtBuf *buf, const char *name, double value)
{
    WriteNamed(buf, NBT_DOUBLE, name);
    NbtRawDouble(buf, value);
}

void NbtString(NbtBuf *buf, const char *name, const char *value)
{
    int length = 0;

    if (value == NULL) value = "";
    length = (int)strlen(value);
    if (length > 65535) length = 65535;
    WriteNamed(buf, NBT_STRING, name);
    WriteU16(buf, (unsigned)length);
    WriteBytes(buf, value, length);
}

void NbtLongArray(NbtBuf *buf, const char *name, const uint64_t *values, int count)
{
    int i = 0;

    if (count < 0) count = 0;
    WriteNamed(buf, NBT_LONG_ARRAY, name);
    WriteU32(buf, (uint32_t)count);
    for (i = 0; i < count; i++) WriteU64(buf, values[i]);
}

//----------------------------------------------------------------------------------
// Reader
//----------------------------------------------------------------------------------
void NbtInInit(NbtIn *in, const unsigned char *data, int size)
{
    in->data = data;
    in->size = size;
    in->pos = 0;
    in->error = 0;
    in->depth = 0;
}

int NbtInU8(NbtIn *in)
{
    if (in->error) return 0;
    if ((in->pos < 0) || (in->pos >= in->size))
    {
        in->error = 1;
        return 0;
    }

    return in->data[in->pos++];
}

static int ReadU16(NbtIn *in)
{
    int hi = NbtInU8(in);
    int lo = NbtInU8(in);
    return (hi << 8) | lo;
}

int NbtInI32(NbtIn *in)
{
    uint32_t value = 0;
    int i = 0;

    for (i = 0; i < 4; i++) value = (value << 8) | (uint32_t)NbtInU8(in);
    return (int)value;
}

int64_t NbtInI64(NbtIn *in)
{
    uint64_t value = 0;
    int i = 0;

    for (i = 0; i < 8; i++) value = (value << 8) | (uint64_t)NbtInU8(in);
    return (int64_t)value;
}

float NbtInF32(NbtIn *in)
{
    uint32_t bits = 0;
    float value = 0.0f;
    int i = 0;

    for (i = 0; i < 4; i++) bits = (bits << 8) | (uint32_t)NbtInU8(in);
    memcpy(&value, &bits, sizeof(value));
    return value;
}

double NbtInF64(NbtIn *in)
{
    uint64_t bits = 0;
    double value = 0.0;
    int i = 0;

    for (i = 0; i < 8; i++) bits = (bits << 8) | (uint64_t)NbtInU8(in);
    memcpy(&value, &bits, sizeof(value));
    return value;
}

void NbtInReadString(NbtIn *in, char *out, int cap)
{
    int length = ReadU16(in);
    int copy = 0;

    if (in->error) return;
    if ((length < 0) || (in->pos + length > in->size))
    {
        in->error = 1;
        return;
    }

    copy = length;
    if (cap <= 0) copy = 0;
    else if (copy >= cap) copy = cap - 1;

    if (copy > 0) memcpy(out, in->data + in->pos, (size_t)copy);
    if (cap > 0) out[copy] = '\0';
    in->pos += length;
}

bool NbtInNext(NbtIn *in, char *name, int nameCap, int *type)
{
    int tag = 0;

    if (in->error) return false;
    tag = NbtInU8(in);
    if (in->error) return false;
    if (tag == NBT_END) return false;
    if ((tag < NBT_BYTE) || (tag > NBT_LONG_ARRAY))
    {
        in->error = 1;
        return false;
    }

    *type = tag;
    NbtInReadString(in, name, nameCap);
    return !in->error;
}

void NbtInListHeader(NbtIn *in, int *elementType, int *count)
{
    int type = NbtInU8(in);
    int listCount = NbtInI32(in);

    if ((listCount < 0) || (listCount > 1000000)) in->error = 1;
    if (elementType != NULL) *elementType = type;
    if (count != NULL) *count = listCount;
}

void NbtInSkip(NbtIn *in, int type)
{
    int count = 0;
    int i = 0;
    int element = 0;

    if (in->error) return;

    switch (type)
    {
        case NBT_BYTE: NbtInU8(in); break;
        case NBT_SHORT: ReadU16(in); break;
        case NBT_INT: NbtInI32(in); break;
        case NBT_LONG: NbtInI64(in); break;
        case NBT_FLOAT: NbtInF32(in); break;
        case NBT_DOUBLE: NbtInF64(in); break;
        case NBT_BYTE_ARRAY:
        case NBT_STRING:
        {
            count = (type == NBT_STRING) ? ReadU16(in) : NbtInI32(in);
            if ((count < 0) || (in->pos + count > in->size)) in->error = 1;
            else in->pos += count;
        } break;
        case NBT_INT_ARRAY:
        case NBT_LONG_ARRAY:
        {
            int width = (type == NBT_INT_ARRAY) ? 4 : 8;
            count = NbtInI32(in);
            if ((count < 0) || (count > 1000000) || (in->pos + count*width > in->size)) in->error = 1;
            else in->pos += count*width;
        } break;
        case NBT_LIST:
        {
            NbtInListHeader(in, &element, &count);
            for (i = 0; (i < count) && !in->error; i++) NbtInSkip(in, element);
        } break;
        case NBT_COMPOUND:
        {
            char name[64] = { 0 };
            int child = 0;

            if (in->depth >= NBT_MAX_DEPTH)
            {
                in->error = 1;
                return;
            }

            in->depth++;
            while (NbtInNext(in, name, sizeof(name), &child)) NbtInSkip(in, child);
            in->depth--;
        } break;
        default: in->error = 1; break;
    }
}

//----------------------------------------------------------------------------------
// Compression
//----------------------------------------------------------------------------------
bool NbtZlibCompress(const unsigned char *data, int size, unsigned char **outData, int *outSize)
{
    uLongf destSize = 0;
    unsigned char *dest = NULL;
    int result = 0;

    if ((data == NULL) || (size < 0) || (outData == NULL) || (outSize == NULL)) return false;

    destSize = compressBound((uLong)size);
    dest = (unsigned char *)malloc(destSize);
    if (dest == NULL) return false;

    result = compress2(dest, &destSize, data, (uLong)size, Z_DEFAULT_COMPRESSION);
    if (result != Z_OK)
    {
        free(dest);
        return false;
    }

    *outData = dest;
    *outSize = (int)destSize;
    return true;
}

bool NbtZlibDecompress(const unsigned char *data, int size, unsigned char **outData, int *outSize, int maxOut)
{
    uLongf destSize = 0;
    unsigned char *dest = NULL;
    int result = 0;

    if ((data == NULL) || (size <= 0) || (outData == NULL) || (outSize == NULL)) return false;
    if (maxOut <= 0) maxOut = NBT_MAX_FILE;

    destSize = (uLongf)((size*4 < 4096) ? 4096 : size*4);
    while ((int)destSize <= maxOut)
    {
        uLongf capacity = destSize;

        free(dest);
        dest = (unsigned char *)malloc(capacity);
        if (dest == NULL) return false;

        result = uncompress(dest, &capacity, data, (uLong)size);
        if (result == Z_OK)
        {
            *outData = dest;
            *outSize = (int)capacity;
            return true;
        }

        if (result != Z_BUF_ERROR)
        {
            free(dest);
            return false;
        }

        if (destSize > (uLongf)(maxOut/2)) break;
        destSize *= 2;
    }

    free(dest);
    return false;
}

bool NbtWriteGzip(const char *path, const unsigned char *data, int size)
{
    char temp[512] = { 0 };
    gzFile file = NULL;
    int written = 0;

    if ((path == NULL) || (data == NULL) || (size < 0)) return false;
    if (snprintf(temp, sizeof(temp), "%s.tmp", path) >= (int)sizeof(temp)) return false;

    file = gzopen(temp, "wb");
    if (file == NULL) return false;

    written = (size == 0) ? 0 : gzwrite(file, data, (unsigned)size);
    if (gzclose(file) != Z_OK) written = -1;
    if (written != size)
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

bool NbtReadGzip(const char *path, unsigned char **outData, int *outSize)
{
    gzFile file = NULL;
    unsigned char *data = NULL;
    int capacity = 4096;
    int length = 0;

    if ((path == NULL) || (outData == NULL) || (outSize == NULL)) return false;

    file = gzopen(path, "rb");
    if (file == NULL) return false;

    data = (unsigned char *)malloc((size_t)capacity);
    if (data == NULL)
    {
        gzclose(file);
        return false;
    }

    while (length < NBT_MAX_FILE)
    {
        int space = capacity - length;
        int count = 0;
        unsigned char *grown = NULL;

        if (space == 0)
        {
            capacity *= 2;
            grown = (unsigned char *)realloc(data, (size_t)capacity);
            if (grown == NULL)
            {
                free(data);
                gzclose(file);
                return false;
            }
            data = grown;
            space = capacity - length;
        }

        count = gzread(file, data + length, (unsigned)space);
        if (count < 0)
        {
            free(data);
            gzclose(file);
            return false;
        }
        if (count == 0) break;
        length += count;
    }

    gzclose(file);
    *outData = data;
    *outSize = length;
    return true;
}
