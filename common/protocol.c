#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void PutU16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)(v & 255u);
    p[1] = (unsigned char)((v >> 8) & 255u);
}

static void PutU32(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)(v & 255u);
    p[1] = (unsigned char)((v >> 8) & 255u);
    p[2] = (unsigned char)((v >> 16) & 255u);
    p[3] = (unsigned char)((v >> 24) & 255u);
}

static void PutI32(unsigned char *p, int v)
{
    PutU32(p, (unsigned)v);
}

static void PutF32(unsigned char *p, float v)
{
    unsigned bits = 0;

    memcpy(&bits, &v, 4);
    PutU32(p, bits);
}

static int StrLenCap(const char *text, int cap)
{
    int n = 0;

    if (text == NULL) return 0;
    while ((text[n] != '\0') && (n < cap)) n++;
    if (text[n] != '\0') return -1;
    return n;
}

int ProtoFrame(unsigned char *out, int cap, unsigned packetId, const unsigned char *body, int bodyLen)
{
    unsigned n = 0;

    if ((out == NULL) || (bodyLen < 0)) return -1;
    n = 2u + (unsigned)bodyLen;
    if ((4 + (int)n) > cap) return -1;
    PutU32(out, n);
    PutU16(out + 4, packetId);
    if ((bodyLen > 0) && (body != NULL)) memcpy(out + 6, body, (size_t)bodyLen);
    return 4 + (int)n;
}

int ProtoPopFrame(unsigned char *buf, int *len, unsigned char *payload, int payloadCap, int *payloadLen)
{
    unsigned n = 0;

    if ((buf == NULL) || (len == NULL) || (payload == NULL) || (payloadLen == NULL)) return -1;
    if (*len < 4) return 0;
    n = (unsigned)buf[0] | ((unsigned)buf[1] << 8) | ((unsigned)buf[2] << 16) | ((unsigned)buf[3] << 24);
    if ((n < 2u) || ((int)n > payloadCap)) return -1;
    if (*len < 4 + (int)n) return 0;
    memcpy(payload, buf + 4, n);
    *payloadLen = (int)n;
    memmove(buf, buf + 4 + (int)n, (size_t)(*len - 4 - (int)n));
    *len -= 4 + (int)n;
    return 1;
}

unsigned ProtoPacketId(const unsigned char *payload, int len)
{
    if ((payload == NULL) || (len < 2)) return 0;
    return (unsigned)payload[0] | ((unsigned)payload[1] << 8);
}

static int ReaderLeft(const ProtoReader *reader)
{
    return reader->size - reader->pos;
}

static int ReadBytes(ProtoReader *reader, void *dst, int n)
{
    if ((reader == NULL) || (n < 0) || (ReaderLeft(reader) < n)) return 0;
    if ((n > 0) && (dst != NULL)) memcpy(dst, reader->data + reader->pos, (size_t)n);
    reader->pos += n;
    return 1;
}

static int ReadU8(ProtoReader *reader, unsigned *value)
{
    unsigned char b = 0;

    if (!ReadBytes(reader, &b, 1)) return 0;
    *value = b;
    return 1;
}

static int ReadU16(ProtoReader *reader, unsigned *value)
{
    unsigned char b[2] = { 0 };

    if (!ReadBytes(reader, b, 2)) return 0;
    *value = (unsigned)b[0] | ((unsigned)b[1] << 8);
    return 1;
}

static int ReadU32(ProtoReader *reader, unsigned *value)
{
    unsigned char b[4] = { 0 };

    if (!ReadBytes(reader, b, 4)) return 0;
    *value = (unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24);
    return 1;
}

static int ReadI32(ProtoReader *reader, int *value)
{
    unsigned u = 0;

    if (!ReadU32(reader, &u)) return 0;
    *value = (int)u;
    return 1;
}

static int ReadF32(ProtoReader *reader, float *value)
{
    unsigned char b[4] = { 0 };
    unsigned bits = 0;

    if (!ReadBytes(reader, b, 4)) return 0;
    bits = (unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24);
    memcpy(value, &bits, 4);
    return 1;
}

static int ReadStr(ProtoReader *reader, char *dst, int dstCap)
{
    unsigned n = 0;

    if ((dst == NULL) || (dstCap <= 0)) return 0;
    if (!ReadU8(reader, &n)) return 0;
    if ((int)n >= dstCap) return 0;
    if (!ReadBytes(reader, dst, (int)n)) return 0;
    dst[n] = '\0';
    return 1;
}

static int ExpectId(const unsigned char *payload, int len, unsigned id, ProtoReader *reader)
{
    unsigned got = 0;

    if ((payload == NULL) || (len < 2) || (reader == NULL)) return 0;
    reader->data = payload;
    reader->size = len;
    reader->pos = 0;
    if (!ReadU16(reader, &got)) return 0;
    return got == id;
}

static int AppendStr(unsigned char *body, int *len, int cap, const char *text, int textLen)
{
    if ((*len + 1 + textLen) > cap) return 0;
    body[*len] = (unsigned char)textLen;
    *len += 1;
    if (textLen > 0) memcpy(body + *len, text, (size_t)textLen);
    *len += textLen;
    return 1;
}

int OcNameOk(const char *name)
{
    int n = 0;
    int i = 0;

    n = StrLenCap(name, OC_NAME_MAX + 1);
    if ((n < 3) || (n > OC_NAME_MAX)) return 0;
    for (i = 0; i < n; i++)
    {
        char c = name[i];
        int ok = ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || (c == '_');
        if (!ok) return 0;
    }
    return 1;
}

int OcPasswordOk(const char *pass)
{
    int n = 0;
    int i = 0;

    n = StrLenCap(pass, OC_PASS_MAX + 1);
    if ((n < 4) || (n > OC_PASS_MAX)) return 0;
    for (i = 0; i < n; i++)
    {
        unsigned char c = (unsigned char)pass[i];
        if ((c < 32) || (c > 126)) return 0;
    }
    return 1;
}

int OcEmailOk(const char *email)
{
    int n = 0;
    int i = 0;
    int at = -1;

    if ((email == NULL) || (email[0] == '\0')) return 1;
    n = StrLenCap(email, OC_EMAIL_MAX + 1);
    if ((n < 3) || (n > OC_EMAIL_MAX)) return 0;
    for (i = 0; i < n; i++)
    {
        unsigned char c = (unsigned char)email[i];
        if ((c <= 32) || (c >= 127)) return 0;
        if (c == '@')
        {
            if (at >= 0) return 0;
            at = i;
        }
    }
    if ((at <= 0) || (at >= n - 1)) return 0;
    for (i = at + 1; i < n; i++)
    {
        if (email[i] == '.') return 1;
    }
    return 0;
}

void OcProfileSetDefault(OcProfile *profile, unsigned accountId, const char *username)
{
    static const unsigned short hotbar[OC_HOTBAR_SLOTS] = {
        OC_BLOCK_GRASS,
        OC_BLOCK_DIRT,
        OC_BLOCK_STONE,
        OC_BLOCK_OAK_LOG,
        OC_BLOCK_OAK_LEAVES,
        OC_BLOCK_BUCKET,
        OC_BLOCK_COBBLESTONE,
        OC_BLOCK_SAND,
        OC_BLOCK_BRICKS
    };
    int i = 0;

    if (profile == NULL) return;
    memset(profile, 0, sizeof(*profile));
    profile->accountId = accountId;
    profile->selectedSlot = 0;
    if (username != NULL) snprintf(profile->username, sizeof(profile->username), "%s", username);
    for (i = 0; i < OC_HOTBAR_SLOTS; i++)
    {
        profile->hotbar[i] = hotbar[i];
        profile->hotbarCount[i] = (hotbar[i] == OC_BLOCK_BUCKET) ? 1 : 64;
    }
}

int ProtoBuildHello(unsigned char *out, int cap, unsigned protocol)
{
    unsigned char body[2];

    PutU16(body, protocol);
    return ProtoFrame(out, cap, OC_C2S_HELLO, body, 2);
}

int ProtoBuildRegister(unsigned char *out, int cap, const char *user, const char *pass, const char *email)
{
    unsigned char body[1 + OC_NAME_MAX + 1 + OC_PASS_MAX + 1 + OC_EMAIL_MAX];
    int len = 0;
    int userLen = StrLenCap(user, OC_NAME_MAX + 1);
    int passLen = StrLenCap(pass, OC_PASS_MAX + 1);
    int emailLen = StrLenCap(email, OC_EMAIL_MAX + 1);

    if ((userLen < 0) || (passLen < 0) || (emailLen < 0)) return -1;
    if (!AppendStr(body, &len, (int)sizeof(body), user, userLen)) return -1;
    if (!AppendStr(body, &len, (int)sizeof(body), pass, passLen)) return -1;
    if (!AppendStr(body, &len, (int)sizeof(body), email, emailLen)) return -1;
    return ProtoFrame(out, cap, OC_C2S_REGISTER, body, len);
}

int ProtoBuildLogin(unsigned char *out, int cap, const char *user, const char *pass)
{
    unsigned char body[1 + OC_NAME_MAX + 1 + OC_PASS_MAX];
    int len = 0;
    int userLen = StrLenCap(user, OC_NAME_MAX + 1);
    int passLen = StrLenCap(pass, OC_PASS_MAX + 1);

    if ((userLen < 0) || (passLen < 0)) return -1;
    if (!AppendStr(body, &len, (int)sizeof(body), user, userLen)) return -1;
    if (!AppendStr(body, &len, (int)sizeof(body), pass, passLen)) return -1;
    return ProtoFrame(out, cap, OC_C2S_LOGIN, body, len);
}

int ProtoBuildJoin(unsigned char *out, int cap)
{
    return ProtoFrame(out, cap, OC_C2S_JOIN, NULL, 0);
}

int ProtoBuildBlock(unsigned char *out, int cap, unsigned packetId, int x, int y, int z, unsigned block)
{
    unsigned char body[14];

    PutI32(body, x);
    PutI32(body + 4, y);
    PutI32(body + 8, z);
    PutU16(body + 12, block);
    return ProtoFrame(out, cap, packetId, body, 14);
}

static int WriteInventoryBody(unsigned char *body, int cap, const OcProfile *profile)
{
    int len = 0;
    int i = 0;

    if ((profile == NULL) || (cap < 2 + OC_HOTBAR_SLOTS*4 + OC_INV_SLOTS*4)) return -1;
    body[len++] = (unsigned char)profile->selectedSlot;
    body[len++] = OC_HOTBAR_SLOTS;
    for (i = 0; i < OC_HOTBAR_SLOTS; i++)
    {
        PutU16(body + len, profile->hotbar[i]);
        PutU16(body + len + 2, profile->hotbarCount[i]);
        len += 4;
    }
    body[len++] = OC_INV_SLOTS;
    for (i = 0; i < OC_INV_SLOTS; i++)
    {
        PutU16(body + len, profile->inv[i]);
        PutU16(body + len + 2, profile->invCount[i]);
        len += 4;
    }
    return len;
}

int ProtoBuildInventory(unsigned char *out, int cap, unsigned packetId, const OcProfile *profile)
{
    unsigned char body[2 + OC_HOTBAR_SLOTS*4 + OC_INV_SLOTS*4];
    int n = WriteInventoryBody(body, (int)sizeof(body), profile);

    if (n < 0) return -1;
    return ProtoFrame(out, cap, packetId, body, n);
}

int ProtoBuildDisconnect(unsigned char *out, int cap)
{
    return ProtoFrame(out, cap, OC_C2S_DISCONNECT, NULL, 0);
}

int ProtoBuildReject(unsigned char *out, int cap, unsigned code, const char *text)
{
    unsigned char body[2 + OC_TEXT_MAX];
    int textLen = StrLenCap(text, OC_TEXT_MAX + 1);
    int len = 0;

    if (textLen < 0) textLen = OC_TEXT_MAX;
    body[len++] = (unsigned char)code;
    if (!AppendStr(body, &len, (int)sizeof(body), text, textLen)) return -1;
    return ProtoFrame(out, cap, OC_S2C_REJECT, body, len);
}

int ProtoBuildAuthOk(unsigned char *out, int cap, unsigned packetId, unsigned accountId, const char *name)
{
    unsigned char body[4 + 1 + OC_NAME_MAX];
    int nameLen = StrLenCap(name, OC_NAME_MAX + 1);
    int len = 4;

    if (nameLen < 0) return -1;
    PutU32(body, accountId);
    if (!AppendStr(body, &len, (int)sizeof(body), name, nameLen)) return -1;
    return ProtoFrame(out, cap, packetId, body, len);
}

int ProtoBuildWelcome(unsigned char *out, int cap, unsigned accountId, int seed, float x, float y, float z, float yaw, float pitch)
{
    unsigned char body[2 + 4 + 4 + 1 + 24];
    int len = 0;

    PutU16(body + len, OC_PROTOCOL_VERSION);
    len += 2;
    PutU32(body + len, accountId);
    len += 4;
    PutI32(body + len, seed);
    len += 4;
    body[len++] = 0;
    PutF32(body + len, x);
    len += 4;
    PutF32(body + len, y);
    len += 4;
    PutF32(body + len, z);
    len += 4;
    PutF32(body + len, yaw);
    len += 4;
    PutF32(body + len, pitch);
    len += 4;
    return ProtoFrame(out, cap, OC_S2C_WELCOME, body, len);
}

int ProtoBuildSnapshot(unsigned char **out, int *outLen, const OcEdit *edits, int count)
{
    int bytes = 0;
    unsigned char *frame = NULL;
    int i = 0;
    int pos = 6;

    if ((out == NULL) || (outLen == NULL) || (count < 0) || (count > OC_EDIT_MAX)) return -1;
    if ((count > 0) && (edits == NULL)) return -1;
    bytes = 4 + 2 + 4 + count*14;
    frame = (unsigned char *)malloc((size_t)bytes);
    if (frame == NULL) return -1;
    PutU32(frame, (unsigned)(2 + 4 + count*14));
    PutU16(frame + 4, OC_S2C_SNAPSHOT);
    PutU32(frame + 6, (unsigned)count);
    pos = 10;
    for (i = 0; i < count; i++)
    {
        PutI32(frame + pos, edits[i].x);
        PutI32(frame + pos + 4, edits[i].y);
        PutI32(frame + pos + 8, edits[i].z);
        PutU16(frame + pos + 12, edits[i].block);
        pos += 14;
    }
    *out = frame;
    *outLen = bytes;
    return bytes;
}

int ProtoParseHello(const unsigned char *payload, int len, unsigned *protocol)
{
    ProtoReader reader = { 0 };

    if (!ExpectId(payload, len, OC_C2S_HELLO, &reader)) return 0;
    if (!ReadU16(&reader, protocol)) return 0;
    return reader.pos == reader.size;
}

int ProtoParseRegister(const unsigned char *payload, int len, char *user, char *pass, char *email)
{
    ProtoReader reader = { 0 };

    if (!ExpectId(payload, len, OC_C2S_REGISTER, &reader)) return 0;
    if (!ReadStr(&reader, user, OC_NAME_MAX + 1)) return 0;
    if (!ReadStr(&reader, pass, OC_PASS_MAX + 1)) return 0;
    if (!ReadStr(&reader, email, OC_EMAIL_MAX + 1)) return 0;
    return reader.pos == reader.size;
}

int ProtoParseLogin(const unsigned char *payload, int len, char *user, char *pass)
{
    ProtoReader reader = { 0 };

    if (!ExpectId(payload, len, OC_C2S_LOGIN, &reader)) return 0;
    if (!ReadStr(&reader, user, OC_NAME_MAX + 1)) return 0;
    if (!ReadStr(&reader, pass, OC_PASS_MAX + 1)) return 0;
    return reader.pos == reader.size;
}

int ProtoParseBlock(const unsigned char *payload, int len, int *x, int *y, int *z, unsigned *block)
{
    ProtoReader reader = { 0 };

    if (!ExpectId(payload, len, OC_C2S_BLOCK, &reader) && !ExpectId(payload, len, OC_S2C_BLOCK, &reader)) return 0;
    if (!ReadI32(&reader, x)) return 0;
    if (!ReadI32(&reader, y)) return 0;
    if (!ReadI32(&reader, z)) return 0;
    if (!ReadU16(&reader, block)) return 0;
    return reader.pos == reader.size;
}

int ProtoParseInventory(const unsigned char *payload, int len, OcProfile *profile)
{
    ProtoReader reader = { 0 };
    unsigned id = 0;
    unsigned selected = 0;
    unsigned hotbarN = 0;
    unsigned invN = 0;
    int i = 0;
    unsigned block = 0;
    unsigned count = 0;

    if ((payload == NULL) || (len < 2) || (profile == NULL)) return 0;
    reader.data = payload;
    reader.size = len;
    reader.pos = 0;
    if (!ReadU16(&reader, &id)) return 0;
    if ((id != OC_C2S_INVENTORY) && (id != OC_S2C_INVENTORY)) return 0;
    if (!ReadU8(&reader, &selected)) return 0;
    if (!ReadU8(&reader, &hotbarN) || (hotbarN != OC_HOTBAR_SLOTS)) return 0;
    for (i = 0; i < OC_HOTBAR_SLOTS; i++)
    {
        if (!ReadU16(&reader, &block) || !ReadU16(&reader, &count)) return 0;
        profile->hotbar[i] = (unsigned short)block;
        profile->hotbarCount[i] = (unsigned short)count;
    }
    if (!ReadU8(&reader, &invN) || (invN != OC_INV_SLOTS)) return 0;
    for (i = 0; i < OC_INV_SLOTS; i++)
    {
        if (!ReadU16(&reader, &block) || !ReadU16(&reader, &count)) return 0;
        profile->inv[i] = (unsigned short)block;
        profile->invCount[i] = (unsigned short)count;
    }
    profile->selectedSlot = (int)selected;
    return reader.pos == reader.size;
}

int ProtoParseReject(const unsigned char *payload, int len, unsigned *code, char *text, int textCap)
{
    ProtoReader reader = { 0 };

    if (!ExpectId(payload, len, OC_S2C_REJECT, &reader)) return 0;
    if (!ReadU8(&reader, code)) return 0;
    if (!ReadStr(&reader, text, textCap)) return 0;
    return 1;
}

int ProtoParseAuthOk(const unsigned char *payload, int len, unsigned *accountId, char *name, int nameCap)
{
    ProtoReader reader = { 0 };
    unsigned id = 0;

    if ((payload == NULL) || (len < 2)) return 0;
    reader.data = payload;
    reader.size = len;
    reader.pos = 0;
    if (!ReadU16(&reader, &id)) return 0;
    if ((id != OC_S2C_LOGIN_OK) && (id != OC_S2C_REGISTER_OK)) return 0;
    if (!ReadU32(&reader, accountId)) return 0;
    if (!ReadStr(&reader, name, nameCap)) return 0;
    return 1;
}

int ProtoParseWelcome(const unsigned char *payload, int len, unsigned *accountId, int *seed, float *x, float *y, float *z, float *yaw, float *pitch)
{
    ProtoReader reader = { 0 };
    unsigned protocol = 0;
    unsigned dimension = 0;

    if (!ExpectId(payload, len, OC_S2C_WELCOME, &reader)) return 0;
    if (!ReadU16(&reader, &protocol) || (protocol != OC_PROTOCOL_VERSION)) return 0;
    if (!ReadU32(&reader, accountId)) return 0;
    if (!ReadI32(&reader, seed)) return 0;
    if (!ReadU8(&reader, &dimension)) return 0;
    if (!ReadF32(&reader, x)) return 0;
    if (!ReadF32(&reader, y)) return 0;
    if (!ReadF32(&reader, z)) return 0;
    if (!ReadF32(&reader, yaw)) return 0;
    if (!ReadF32(&reader, pitch)) return 0;
    return 1;
}

int ProtoParseSnapshot(const unsigned char *payload, int len, OcEdit *edits, int cap, int *count)
{
    ProtoReader reader = { 0 };
    unsigned n = 0;
    unsigned i = 0;

    if (!ExpectId(payload, len, OC_S2C_SNAPSHOT, &reader)) return 0;
    if (!ReadU32(&reader, &n)) return 0;
    if ((int)n > cap) return 0;
    for (i = 0; i < n; i++)
    {
        unsigned block = 0;
        if (!ReadI32(&reader, &edits[i].x)) return 0;
        if (!ReadI32(&reader, &edits[i].y)) return 0;
        if (!ReadI32(&reader, &edits[i].z)) return 0;
        if (!ReadU16(&reader, &block)) return 0;
        edits[i].block = (unsigned short)block;
    }
    *count = (int)n;
    return reader.pos == reader.size;
}
