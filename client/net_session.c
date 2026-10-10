#include "net_session.h"

#include "player.h"
#include "voxel_world.h"
#include "wire.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

typedef char OcGrassCheck[(OC_BLOCK_GRASS == (int)BLOCK_GRASS) ? 1 : -1];
typedef char OcDirtCheck[(OC_BLOCK_DIRT == (int)BLOCK_DIRT) ? 1 : -1];
typedef char OcStoneCheck[(OC_BLOCK_STONE == (int)BLOCK_STONE) ? 1 : -1];
typedef char OcLogCheck[(OC_BLOCK_OAK_LOG == (int)BLOCK_OAK_LOG) ? 1 : -1];
typedef char OcLeavesCheck[(OC_BLOCK_OAK_LEAVES == (int)BLOCK_OAK_LEAVES) ? 1 : -1];
typedef char OcBucketCheck[(OC_BLOCK_BUCKET == (int)BLOCK_BUCKET) ? 1 : -1];
typedef char OcBricksCheck[(OC_BLOCK_BRICKS == (int)BLOCK_BRICKS) ? 1 : -1];
typedef char OcDiamondCheck[(OC_BLOCK_DIAMOND_BLOCK == (int)BLOCK_DIAMOND_BLOCK) ? 1 : -1];
typedef char OcCountCheck[(OC_BLOCK_COUNT == (int)BLOCK_COUNT) ? 1 : -1];
typedef char OcHotbarCheck[(OC_HOTBAR_SLOTS == HOTBAR_SIZE) ? 1 : -1];
typedef char OcInvCheck[(OC_INV_SLOTS == INVENTORY_SIZE) ? 1 : -1];

static int sock = -1;
static int authed = 0;
static int inWorld = 0;
static int applying = 0;
static unsigned worldSeed = 0;
static char status[160];
static char editLine[160];
static unsigned char recvBuf[1024*1024];
static int recvLen = 0;
static unsigned char payload[1024*1024];
static OcProfile profile;
static OcProfile lastSent;
static int haveSent = 0;
static OcEdit edits[OC_EDIT_MAX];
static int editCount = 0;

static uint64_t NowMs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec*1000ull + (uint64_t)ts.tv_nsec/1000000ull;
}

static void SetStatus(const char *text)
{
    snprintf(status, sizeof(status), "%s", (text != NULL) ? text : "");
}

static void CloseSock(void)
{
    if (sock >= 0) close(sock);
    sock = -1;
    recvLen = 0;
    authed = 0;
    inWorld = 0;
    haveSent = 0;
}

void NetSessionDisconnect(void)
{
    CloseSock();
    editCount = 0;
    editLine[0] = '\0';
}

int NetSessionIsOnline(void)
{
    return (sock >= 0) && authed;
}

int NetSessionIsInWorld(void)
{
    return inWorld && (sock >= 0);
}

const char *NetSessionStatus(void)
{
    return status;
}

const char *NetSessionName(void)
{
    return profile.username;
}

unsigned NetSessionSeed(void)
{
    return worldSeed;
}

const char *NetSessionEditLine(void)
{
    return editLine;
}

static int SetNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int ConnectHost(const char *host, int port)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    char portText[16];
    int fd = -1;
    int status = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portText, sizeof(portText), "%d", port);
    status = getaddrinfo((host != NULL) ? host : "127.0.0.1", portText, &hints, &res);
    if (status != 0) return -1;
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0)
    {
        freeaddrinfo(res);
        return -1;
    }
    if (SetNonBlock(fd) != 0)
    {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    status = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (status < 0 && (errno != EINPROGRESS))
    {
        close(fd);
        return -1;
    }
    if (status < 0)
    {
        struct pollfd pfd;
        int err = 0;
        socklen_t errLen = sizeof(err);

        pfd.fd = fd;
        pfd.events = POLLOUT;
        if (poll(&pfd, 1, 2000) <= 0)
        {
            close(fd);
            return -1;
        }
        if ((getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errLen) < 0) || (err != 0))
        {
            close(fd);
            return -1;
        }
    }
    return fd;
}

static int ReadPacket(int timeoutMs, int *payloadLen, unsigned *id)
{
    uint64_t start = NowMs();

    while (1)
    {
        int popped = ProtoPopFrame(recvBuf, &recvLen, payload, (int)sizeof(payload), payloadLen);
        uint64_t now = 0;
        int remain = 0;
        struct pollfd pfd;
        int n = 0;

        if (popped < 0) return -1;
        if (popped == 1)
        {
            *id = ProtoPacketId(payload, *payloadLen);
            return 1;
        }
        now = NowMs();
        remain = timeoutMs - (int)(now - start);
        if (remain <= 0) return 0;
        pfd.fd = sock;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, remain) <= 0) return 0;
        if (recvLen >= (int)sizeof(recvBuf)) return -1;
        n = (int)recv(sock, recvBuf + recvLen, sizeof(recvBuf) - (size_t)recvLen, 0);
        if (n <= 0) return -1;
        recvLen += n;
    }
}

static int FailReject(unsigned id, int payloadLen)
{
    unsigned code = 0;
    char text[OC_TEXT_MAX + 1];

    text[0] = '\0';
    if (id == OC_S2C_REJECT)
    {
        if (!ProtoParseReject(payload, payloadLen, &code, text, (int)sizeof(text)))
        {
            SetStatus("The server rejected the connection");
        }
        else SetStatus(text);
        CloseSock();
        return 0;
    }
    SetStatus("The server closed the connection");
    CloseSock();
    return 0;
}

static void RememberEdit(int x, int y, int z, unsigned block)
{
    int i = 0;

    for (i = 0; i < editCount; i++)
    {
        if ((edits[i].x == x) && (edits[i].y == y) && (edits[i].z == z))
        {
            edits[i].block = (unsigned short)block;
            return;
        }
    }
    if (editCount >= OC_EDIT_MAX) return;
    edits[editCount].x = x;
    edits[editCount].y = y;
    edits[editCount].z = z;
    edits[editCount].block = (unsigned short)block;
    editCount++;
}

static void NoteEdit(int x, int y, int z, unsigned block)
{
    const char *name = "block";

    if ((block > 0) && (block < (unsigned)BLOCK_COUNT)) name = GetBlockName((BlockType)block);
    if (block == OC_BLOCK_AIR) snprintf(editLine, sizeof(editLine), "Shared air at %d %d %d", x, y, z);
    else snprintf(editLine, sizeof(editLine), "Shared %s at %d %d %d", name, x, y, z);
}

static int StackCount(unsigned block)
{
    if (block == OC_BLOCK_AIR) return 0;
    if ((block == OC_BLOCK_BUCKET) || (block == OC_BLOCK_WATER_BUCKET)) return 1;
    return 64;
}

static void CapturePlayer(const Player *player, OcProfile *out)
{
    int i = 0;

    *out = profile;
    out->selectedSlot = player->hotbarSlot;
    if ((out->selectedSlot < 0) || (out->selectedSlot >= OC_HOTBAR_SLOTS)) out->selectedSlot = 0;
    for (i = 0; i < OC_HOTBAR_SLOTS; i++)
    {
        unsigned block = (unsigned)player->hotbar[i];

        out->hotbar[i] = (unsigned short)block;
        if (block == profile.hotbar[i]) out->hotbarCount[i] = profile.hotbarCount[i];
        else out->hotbarCount[i] = (unsigned short)StackCount(block);
    }
    for (i = 0; i < OC_INV_SLOTS; i++)
    {
        unsigned block = (unsigned)player->inventory.blocks[i];
        int count = player->inventory.quantities[i];

        if ((block == OC_BLOCK_AIR) || (count <= 0))
        {
            out->inv[i] = 0;
            out->invCount[i] = 0;
        }
        else
        {
            if (count > 64) count = 64;
            out->inv[i] = (unsigned short)block;
            out->invCount[i] = (unsigned short)count;
        }
    }
}

static int SendProfile(const OcProfile *out, unsigned packetId)
{
    unsigned char frame[512];
    int n = ProtoBuildInventory(frame, (int)sizeof(frame), packetId, out);

    if ((n < 0) || !WireSendAll(sock, frame, n)) return 0;
    return 1;
}

int NetSessionCreateAccount(const char *host, int port, const char *user, const char *pass, const char *email)
{
    unsigned char hello[16];
    unsigned char body[256];
    int n = 0;
    int payloadLen = 0;
    unsigned id = 0;
    unsigned accountId = 0;
    char name[OC_NAME_MAX + 1];

    NetSessionDisconnect();
    SetStatus("");
    if (!OcNameOk(user))
    {
        SetStatus("Username must be 3-16 letters, numbers, or underscore");
        return 0;
    }
    if (!OcPasswordOk(pass))
    {
        SetStatus("Password must be 4-64 characters");
        return 0;
    }
    if (!OcEmailOk(email))
    {
        SetStatus("Email looks wrong");
        return 0;
    }
    sock = ConnectHost(host, port);
    if (sock < 0)
    {
        SetStatus("Could not reach the server");
        return 0;
    }
    n = ProtoBuildHello(hello, (int)sizeof(hello), OC_PROTOCOL_VERSION);
    if ((n < 0) || !WireSendAll(sock, hello, n))
    {
        SetStatus("Could not reach the server");
        CloseSock();
        return 0;
    }
    n = ProtoBuildRegister(body, (int)sizeof(body), user, pass, (email != NULL) ? email : "");
    if ((n < 0) || !WireSendAll(sock, body, n))
    {
        SetStatus("Could not reach the server");
        CloseSock();
        return 0;
    }
    if (ReadPacket(3000, &payloadLen, &id) != 1) return FailReject(0, 0);
    if (id == OC_S2C_REJECT) return FailReject(id, payloadLen);
    if (!ProtoParseAuthOk(payload, payloadLen, &accountId, name, (int)sizeof(name)))
    {
        SetStatus("The server closed the connection");
        CloseSock();
        return 0;
    }
    OcProfileSetDefault(&profile, accountId, name);
    authed = 1;
    snprintf(status, sizeof(status), "Created account %s", name);
    return 1;
}

int NetSessionLogin(const char *host, int port, const char *user, const char *pass)
{
    unsigned char hello[16];
    unsigned char body[128];
    int n = 0;
    int payloadLen = 0;
    unsigned id = 0;
    unsigned accountId = 0;
    char name[OC_NAME_MAX + 1];

    NetSessionDisconnect();
    SetStatus("");
    if (!OcNameOk(user))
    {
        SetStatus("Username must be 3-16 letters, numbers, or underscore");
        return 0;
    }
    if (!OcPasswordOk(pass))
    {
        SetStatus("Password must be 4-64 characters");
        return 0;
    }
    sock = ConnectHost(host, port);
    if (sock < 0)
    {
        SetStatus("Could not reach the server");
        return 0;
    }
    n = ProtoBuildHello(hello, (int)sizeof(hello), OC_PROTOCOL_VERSION);
    if ((n < 0) || !WireSendAll(sock, hello, n))
    {
        SetStatus("Could not reach the server");
        CloseSock();
        return 0;
    }
    n = ProtoBuildLogin(body, (int)sizeof(body), user, pass);
    if ((n < 0) || !WireSendAll(sock, body, n))
    {
        SetStatus("Could not reach the server");
        CloseSock();
        return 0;
    }
    if (ReadPacket(3000, &payloadLen, &id) != 1) return FailReject(0, 0);
    if (id == OC_S2C_REJECT) return FailReject(id, payloadLen);
    if (!ProtoParseAuthOk(payload, payloadLen, &accountId, name, (int)sizeof(name)))
    {
        SetStatus("The server closed the connection");
        CloseSock();
        return 0;
    }
    OcProfileSetDefault(&profile, accountId, name);
    authed = 1;
    snprintf(status, sizeof(status), "Logged in as %s", name);
    return 1;
}

int NetSessionJoinWorld(void)
{
    unsigned char frame[16];
    int n = 0;
    int gotWelcome = 0;
    int gotInventory = 0;
    int gotSnapshot = 0;
    uint64_t start = NowMs();

    if (!NetSessionIsOnline())
    {
        SetStatus("Log in first");
        return 0;
    }
    editCount = 0;
    editLine[0] = '\0';
    haveSent = 0;
    n = ProtoBuildJoin(frame, (int)sizeof(frame));
    if ((n < 0) || !WireSendAll(sock, frame, n))
    {
        SetStatus("Could not reach the server");
        CloseSock();
        return 0;
    }
    while (!gotWelcome || !gotInventory || !gotSnapshot)
    {
        int payloadLen = 0;
        unsigned id = 0;
        int remain = 5000 - (int)(NowMs() - start);
        unsigned accountId = 0;
        int seed = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
        int read = 0;

        if (remain < 1) remain = 1;
        read = ReadPacket(remain, &payloadLen, &id);
        if (read != 1)
        {
            SetStatus("Could not reach the server");
            CloseSock();
            return 0;
        }
        if (id == OC_S2C_REJECT) return FailReject(id, payloadLen);
        if (id == OC_S2C_WELCOME)
        {
            if (!ProtoParseWelcome(payload, payloadLen, &accountId, &seed, &x, &y, &z, &yaw, &pitch))
            {
                SetStatus("Bad welcome from the server");
                CloseSock();
                return 0;
            }
            worldSeed = (unsigned)seed;
            gotWelcome = 1;
        }
        else if (id == OC_S2C_INVENTORY)
        {
            if (!ProtoParseInventory(payload, payloadLen, &profile))
            {
                SetStatus("Bad inventory from the server");
                CloseSock();
                return 0;
            }
            gotInventory = 1;
        }
        else if (id == OC_S2C_SNAPSHOT)
        {
            if (!ProtoParseSnapshot(payload, payloadLen, edits, OC_EDIT_MAX, &editCount))
            {
                SetStatus("Bad world snapshot");
                CloseSock();
                return 0;
            }
            gotSnapshot = 1;
        }
    }
    inWorld = 1;
    snprintf(status, sizeof(status), "Joined as %s", profile.username);
    return 1;
}

void NetSessionApplySpawnInventory(Player *player)
{
    int i = 0;
    int any = 0;

    if ((player == NULL) || !inWorld) return;
    for (i = 0; i < HOTBAR_SIZE; i++)
    {
        unsigned block = profile.hotbar[i];

        if (block >= (unsigned)BLOCK_COUNT) block = BLOCK_AIR;
        player->hotbar[i] = (BlockType)block;
    }
    if ((profile.selectedSlot >= 0) && (profile.selectedSlot < HOTBAR_SIZE)) player->hotbarSlot = profile.selectedSlot;
    player->selectedBlock = player->hotbar[player->hotbarSlot];
    for (i = 0; i < INVENTORY_SIZE; i++)
    {
        if (profile.invCount[i] > 0) any = 1;
    }
    if (any)
    {
        for (i = 0; i < INVENTORY_SIZE; i++)
        {
            unsigned block = profile.inv[i];

            if ((block >= (unsigned)BLOCK_COUNT) || (profile.invCount[i] <= 0))
            {
                player->inventory.blocks[i] = BLOCK_AIR;
                player->inventory.quantities[i] = 0;
            }
            else
            {
                player->inventory.blocks[i] = (BlockType)block;
                player->inventory.quantities[i] = profile.invCount[i];
            }
        }
    }
}

static void ApplyOne(VoxelWorld *world, int x, int y, int z, unsigned block)
{
    BlockPos pos;
    ChunkPos chunkPos;
    BlockType type = BLOCK_AIR;

    if ((world == NULL) || (y < 0) || (y >= WORLD_HEIGHT)) return;
    if (block >= (unsigned)BLOCK_COUNT) return;
    pos.x = x;
    pos.y = y;
    pos.z = z;
    chunkPos = WorldToChunk((Vector3){ (float)x, (float)y, (float)z });
    if (GetChunk(world, chunkPos) == NULL) return;
    type = (BlockType)block;
    if (GetBlock(world, pos) == type) return;
    applying = 1;
    SetBlock(world, pos, type);
    applying = 0;
}

void NetSessionOverlayEdits(VoxelWorld *world)
{
    int i = 0;

    if (!inWorld || (world == NULL)) return;
    for (i = 0; i < editCount; i++)
    {
        ApplyOne(world, edits[i].x, edits[i].y, edits[i].z, edits[i].block);
    }
}

void NetSessionPoll(VoxelWorld *world)
{
    if (sock < 0) return;
    while (sock >= 0)
    {
        int payloadLen = 0;
        int popped = ProtoPopFrame(recvBuf, &recvLen, payload, (int)sizeof(payload), &payloadLen);
        struct pollfd pfd;
        int n = 0;
        unsigned id = 0;
        int x = 0;
        int y = 0;
        int z = 0;
        unsigned block = 0;

        if (popped < 0)
        {
            SetStatus("The server closed the connection");
            CloseSock();
            return;
        }
        if (popped == 1)
        {
            id = ProtoPacketId(payload, payloadLen);
            if (id == OC_S2C_BLOCK && ProtoParseBlock(payload, payloadLen, &x, &y, &z, &block))
            {
                RememberEdit(x, y, z, block);
                NoteEdit(x, y, z, block);
                ApplyOne(world, x, y, z, block);
            }
            else if (id == OC_S2C_REJECT)
            {
                FailReject(id, payloadLen);
                return;
            }
            continue;
        }
        pfd.fd = sock;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 0) <= 0) return;
        if (recvLen >= (int)sizeof(recvBuf))
        {
            CloseSock();
            return;
        }
        n = (int)recv(sock, recvBuf + recvLen, sizeof(recvBuf) - (size_t)recvLen, MSG_DONTWAIT);
        if (n == 0)
        {
            SetStatus("The server closed the connection");
            CloseSock();
            return;
        }
        if (n < 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) return;
            CloseSock();
            return;
        }
        recvLen += n;
    }
}

void NetSessionLocalBlock(int x, int y, int z, int block)
{
    unsigned char frame[32];
    int n = 0;

    if (!inWorld || applying || (sock < 0)) return;
    if ((block < 0) || (block >= BLOCK_COUNT)) return;
    RememberEdit(x, y, z, (unsigned)block);
    NoteEdit(x, y, z, (unsigned)block);
    n = ProtoBuildBlock(frame, (int)sizeof(frame), OC_C2S_BLOCK, x, y, z, (unsigned)block);
    if ((n < 0) || !WireSendAll(sock, frame, n))
    {
        SetStatus("Could not reach the server");
        CloseSock();
    }
}

void NetSessionSyncInventory(const Player *player)
{
    OcProfile next;

    if (!inWorld || (player == NULL) || (sock < 0)) return;
    CapturePlayer(player, &next);
    if (haveSent && (memcmp(&next, &lastSent, sizeof(next)) == 0)) return;
    if (!SendProfile(&next, OC_C2S_INVENTORY))
    {
        SetStatus("Could not reach the server");
        CloseSock();
        return;
    }
    profile = next;
    lastSent = next;
    haveSent = 1;
}

void NetSessionLeave(const Player *player)
{
    unsigned char frame[16];
    int n = 0;

    if (inWorld && (player != NULL)) NetSessionSyncInventory(player);
    if (sock >= 0)
    {
        n = ProtoBuildDisconnect(frame, (int)sizeof(frame));
        if (n > 0) WireSendAll(sock, frame, n);
    }
    NetSessionDisconnect();
    SetStatus("Left the server");
}
