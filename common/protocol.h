#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * OpenCraft session protocol.
 *
 * A frame is little-endian u32 payload_length, then that many bytes.
 * The payload starts with little-endian u16 packet_id. Protocol number is 1.
 * Layout matches docs/parity/SERVER.md. Account, inventory, and the
 * block-edit snapshot are extra ids this slice needs. Chunk sections and
 * entity movement are not sent yet.
 */

#define OC_PROTOCOL_VERSION 1

#define OC_C2S_HELLO 1
#define OC_C2S_INPUT 2
#define OC_C2S_REGISTER 3
#define OC_C2S_LOGIN 4
#define OC_C2S_JOIN 5
#define OC_C2S_BLOCK 6
#define OC_C2S_INVENTORY 7
#define OC_C2S_DISCONNECT 8
#define OC_C2S_POSE 9

#define OC_S2C_WELCOME 1
#define OC_S2C_CHUNK 2
#define OC_S2C_BLOCK 3
#define OC_S2C_ENTITY 4
#define OC_S2C_REJECT 5
#define OC_S2C_INVENTORY 6
#define OC_S2C_SNAPSHOT 7
#define OC_S2C_LOGIN_OK 8
#define OC_S2C_REGISTER_OK 9

#define OC_HOTBAR_SLOTS 9
#define OC_INV_SLOTS 45
#define OC_NAME_MAX 16
#define OC_PASS_MAX 64
#define OC_EMAIL_MAX 64
#define OC_EDIT_MAX 8192
#define OC_TEXT_MAX 96

#define OC_REJECT_VERSION 1
#define OC_REJECT_TAKEN 2
#define OC_REJECT_PASSWORD 3
#define OC_REJECT_UNKNOWN 4
#define OC_REJECT_NAME 5
#define OC_REJECT_EMAIL 6
#define OC_REJECT_AUTH 7
#define OC_REJECT_FULL 8
#define OC_REJECT_INTERNAL 9

/* Block ids match client/voxel_types.h BlockType. common/ cannot include raylib. */
#define OC_BLOCK_AIR 0
#define OC_BLOCK_GRASS 1
#define OC_BLOCK_DIRT 2
#define OC_BLOCK_STONE 3
#define OC_BLOCK_COBBLESTONE 4
#define OC_BLOCK_SAND 6
#define OC_BLOCK_OAK_LOG 9
#define OC_BLOCK_OAK_LEAVES 11
#define OC_BLOCK_DIAMOND_BLOCK 43
#define OC_BLOCK_BRICKS 114
#define OC_BLOCK_BUCKET 153
#define OC_BLOCK_WATER_BUCKET 154
#define OC_BLOCK_COUNT 155

typedef struct {
    unsigned accountId;
    char username[OC_NAME_MAX + 1];
    int selectedSlot;
    unsigned short hotbar[OC_HOTBAR_SLOTS];
    unsigned short hotbarCount[OC_HOTBAR_SLOTS];
    unsigned short inv[OC_INV_SLOTS];
    unsigned short invCount[OC_INV_SLOTS];
} OcProfile;

typedef struct {
    int x;
    int y;
    int z;
    unsigned short block;
} OcEdit;

typedef struct {
    const unsigned char *data;
    int size;
    int pos;
} ProtoReader;

int ProtoFrame(unsigned char *out, int cap, unsigned packetId, const unsigned char *body, int bodyLen);
int ProtoPopFrame(unsigned char *buf, int *len, unsigned char *payload, int payloadCap, int *payloadLen);
unsigned ProtoPacketId(const unsigned char *payload, int len);

int ProtoBuildHello(unsigned char *out, int cap, unsigned protocol);
int ProtoBuildRegister(unsigned char *out, int cap, const char *user, const char *pass, const char *email);
int ProtoBuildLogin(unsigned char *out, int cap, const char *user, const char *pass);
int ProtoBuildJoin(unsigned char *out, int cap);
int ProtoBuildBlock(unsigned char *out, int cap, unsigned packetId, int x, int y, int z, unsigned block);
int ProtoBuildInventory(unsigned char *out, int cap, unsigned packetId, const OcProfile *profile);
int ProtoBuildDisconnect(unsigned char *out, int cap);
int ProtoBuildPose(unsigned char *out, int cap, float x, float y, float z, float yaw, float pitch);
int ProtoBuildReject(unsigned char *out, int cap, unsigned code, const char *text);
int ProtoBuildAuthOk(unsigned char *out, int cap, unsigned packetId, unsigned accountId, const char *name);
int ProtoBuildWelcome(unsigned char *out, int cap, unsigned accountId, int seed, float x, float y, float z, float yaw, float pitch);
int ProtoBuildSnapshot(unsigned char **out, int *outLen, const OcEdit *edits, int count);

int ProtoParseHello(const unsigned char *payload, int len, unsigned *protocol);
int ProtoParseRegister(const unsigned char *payload, int len, char *user, char *pass, char *email);
int ProtoParseLogin(const unsigned char *payload, int len, char *user, char *pass);
int ProtoParseBlock(const unsigned char *payload, int len, int *x, int *y, int *z, unsigned *block);
int ProtoParsePose(const unsigned char *payload, int len, float *x, float *y, float *z, float *yaw, float *pitch);
int ProtoParseInventory(const unsigned char *payload, int len, OcProfile *profile);
int ProtoParseReject(const unsigned char *payload, int len, unsigned *code, char *text, int textCap);
int ProtoParseAuthOk(const unsigned char *payload, int len, unsigned *accountId, char *name, int nameCap);
int ProtoParseWelcome(const unsigned char *payload, int len, unsigned *accountId, int *seed, float *x, float *y, float *z, float *yaw, float *pitch);
int ProtoParseSnapshot(const unsigned char *payload, int len, OcEdit *edits, int cap, int *count);

void OcProfileSetDefault(OcProfile *profile, unsigned accountId, const char *username);
int OcNameOk(const char *name);
int OcPasswordOk(const char *pass);
int OcEmailOk(const char *email);

#endif
