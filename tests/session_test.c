#include "db.h"
#include "game_server.h"
#include "protocol.h"
#include "sha256.h"
#include "wire.h"

#include <sqlite3.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int failures = 0;

static void Expect(int cond, const char *message)
{
    if (cond) return;
    fprintf(stderr, "FAIL %s\n", message);
    failures++;
}

static uint64_t NowMs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec*1000ull + (uint64_t)ts.tv_nsec/1000000ull;
}

typedef struct {
    int fd;
    unsigned char buf[256*1024];
    int len;
    unsigned char payload[256*1024];
    int payloadLen;
} TestClient;

static int ConnectLocal(int port)
{
    struct sockaddr_in addr;
    int fd = -1;
    int attempt = 0;

    for (attempt = 0; attempt < 40; attempt++)
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return -1;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((unsigned short)port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) return fd;
        close(fd);
        usleep(50000);
    }
    return -1;
}

static int ReadPacket(TestClient *client, int timeoutMs, unsigned *id)
{
    uint64_t start = NowMs();

    while (1)
    {
        int popped = ProtoPopFrame(client->buf, &client->len, client->payload, (int)sizeof(client->payload), &client->payloadLen);
        struct pollfd pfd;
        int remain = 0;
        int n = 0;

        if (popped < 0) return 0;
        if (popped == 1)
        {
            *id = ProtoPacketId(client->payload, client->payloadLen);
            return 1;
        }
        remain = timeoutMs - (int)(NowMs() - start);
        if (remain <= 0) return 0;
        pfd.fd = client->fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, remain) <= 0) return 0;
        n = (int)recv(client->fd, client->buf + client->len, sizeof(client->buf) - (size_t)client->len, 0);
        if (n <= 0) return 0;
        client->len += n;
    }
}

static int SendFrame(TestClient *client, const unsigned char *frame, int n)
{
    return WireSendAll(client->fd, frame, n);
}

static int OpenClient(TestClient *client, int port, const char *user, const char *pass, const char *email, int create)
{
    unsigned char frame[256];
    unsigned id = 0;
    unsigned accountId = 0;
    char name[OC_NAME_MAX + 1];
    int n = 0;

    memset(client, 0, sizeof(*client));
    client->fd = ConnectLocal(port);
    if (client->fd < 0) return 0;
    n = ProtoBuildHello(frame, (int)sizeof(frame), OC_PROTOCOL_VERSION);
    if ((n < 0) || !SendFrame(client, frame, n)) return 0;
    if (create) n = ProtoBuildRegister(frame, (int)sizeof(frame), user, pass, email);
    else n = ProtoBuildLogin(frame, (int)sizeof(frame), user, pass);
    if ((n < 0) || !SendFrame(client, frame, n)) return 0;
    if (!ReadPacket(client, 3000, &id)) return 0;
    if ((create && (id != OC_S2C_REGISTER_OK)) || (!create && (id != OC_S2C_LOGIN_OK))) return 0;
    if (!ProtoParseAuthOk(client->payload, client->payloadLen, &accountId, name, (int)sizeof(name))) return 0;
    return accountId != 0;
}

static void CloseClient(TestClient *client)
{
    unsigned char frame[16];
    int n = 0;

    if (client->fd >= 0)
    {
        n = ProtoBuildDisconnect(frame, (int)sizeof(frame));
        if (n > 0) WireSendAll(client->fd, frame, n);
        close(client->fd);
    }
    client->fd = -1;
}

static float welcomeX = 0.0f;
static float welcomeY = 0.0f;
static float welcomeZ = 0.0f;
static float welcomeYaw = 0.0f;
static float welcomePitch = 0.0f;

static int Join(TestClient *client, OcProfile *profile, OcEdit *edits, int *editCount, int *seed)
{
    unsigned char frame[32];
    int n = ProtoBuildJoin(frame, (int)sizeof(frame));
    int gotWelcome = 0;
    int gotInventory = 0;
    int gotSnapshot = 0;
    unsigned accountId = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;

    if ((n < 0) || !SendFrame(client, frame, n)) return 0;
    while (!gotWelcome || !gotInventory || !gotSnapshot)
    {
        unsigned id = 0;

        if (!ReadPacket(client, 3000, &id)) return 0;
        if (id == OC_S2C_WELCOME)
        {
            if (!ProtoParseWelcome(client->payload, client->payloadLen, &accountId, seed, &x, &y, &z, &yaw, &pitch)) return 0;
            welcomeX = x;
            welcomeY = y;
            welcomeZ = z;
            welcomeYaw = yaw;
            welcomePitch = pitch;
            gotWelcome = 1;
        }
        else if (id == OC_S2C_INVENTORY)
        {
            if (!ProtoParseInventory(client->payload, client->payloadLen, profile)) return 0;
            gotInventory = 1;
        }
        else if (id == OC_S2C_SNAPSHOT)
        {
            if (!ProtoParseSnapshot(client->payload, client->payloadLen, edits, OC_EDIT_MAX, editCount)) return 0;
            gotSnapshot = 1;
        }
        else return 0;
    }
    return 1;
}

static int FindEdit(const OcEdit *edits, int count, int x, int y, int z)
{
    int i = 0;

    for (i = 0; i < count; i++)
    {
        if ((edits[i].x == x) && (edits[i].y == y) && (edits[i].z == z)) return edits[i].block;
    }
    return -1;
}

static void TestHash(void)
{
    char hex[65];

    Sha256Hex("", 0, hex);
    Expect(strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0, "sha256 empty");
    Sha256Hex("abc", 3, hex);
    Expect(strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0, "sha256 abc");
}

static void TestBadVersion(int port)
{
    TestClient client;
    unsigned char frame[16];
    unsigned id = 0;
    unsigned code = 0;
    char text[96];
    int n = 0;

    memset(&client, 0, sizeof(client));
    client.fd = ConnectLocal(port);
    Expect(client.fd >= 0, "connect for bad version");
    n = ProtoBuildHello(frame, (int)sizeof(frame), 2);
    Expect((n > 0) && SendFrame(&client, frame, n), "send hello 2");
    Expect(ReadPacket(&client, 2000, &id) == 1, "read version reject");
    Expect(id == OC_S2C_REJECT, "version packet is reject");
    Expect(ProtoParseReject(client.payload, client.payloadLen, &code, text, (int)sizeof(text)) == 1, "parse version reject");
    Expect(code == OC_REJECT_VERSION, "version code");
    close(client.fd);
}

static void TestDuplicateAndPassword(int port)
{
    TestClient client;
    unsigned char frame[256];
    unsigned id = 0;
    unsigned code = 0;
    char text[96];
    int n = 0;

    Expect(OpenClient(&client, port, "alice", "secret-pass", "alice@example.com", 1) == 0, "second register is rejected");
    /* OpenClient returns 0 on reject, but it may leave the socket open after a reject packet. */
    if (client.fd >= 0) close(client.fd);

    memset(&client, 0, sizeof(client));
    client.fd = ConnectLocal(port);
    n = ProtoBuildHello(frame, (int)sizeof(frame), OC_PROTOCOL_VERSION);
    SendFrame(&client, frame, n);
    n = ProtoBuildRegister(frame, (int)sizeof(frame), "alice", "secret-pass", "");
    SendFrame(&client, frame, n);
    Expect(ReadPacket(&client, 2000, &id) == 1, "duplicate register reply");
    Expect(id == OC_S2C_REJECT, "duplicate is reject");
    ProtoParseReject(client.payload, client.payloadLen, &code, text, (int)sizeof(text));
    Expect(code == OC_REJECT_TAKEN, "username taken code");
    Expect(strstr(text, "already taken") != NULL, "username taken text");
    close(client.fd);

    memset(&client, 0, sizeof(client));
    client.fd = ConnectLocal(port);
    n = ProtoBuildHello(frame, (int)sizeof(frame), OC_PROTOCOL_VERSION);
    SendFrame(&client, frame, n);
    n = ProtoBuildLogin(frame, (int)sizeof(frame), "alice", "nope-pass");
    SendFrame(&client, frame, n);
    Expect(ReadPacket(&client, 2000, &id) == 1, "bad password reply");
    Expect(id == OC_S2C_REJECT, "bad password is reject");
    ProtoParseReject(client.payload, client.payloadLen, &code, text, (int)sizeof(text));
    Expect(code == OC_REJECT_PASSWORD, "bad password code");
    Expect(strstr(text, "Wrong password") != NULL, "bad password text");
    close(client.fd);
}

static int WaitForBlock(TestClient *client, int x, int y, int z, unsigned block)
{
    uint64_t start = NowMs();

    while (NowMs() - start < 2000)
    {
        unsigned id = 0;
        int bx = 0;
        int by = 0;
        int bz = 0;
        unsigned got = 0;

        if (!ReadPacket(client, 500, &id)) continue;
        if (id != OC_S2C_BLOCK) continue;
        if (!ProtoParseBlock(client->payload, client->payloadLen, &bx, &by, &bz, &got)) return 0;
        if ((bx == x) && (by == y) && (bz == z) && (got == block)) return 1;
    }
    return 0;
}

int main(void)
{
    const char *dir = "/tmp/opencraft-session-b258";
    char dbPath[256];
    GameServer *server = NULL;
    TestClient alice;
    TestClient bob;
    OcProfile profile;
    OcEdit *edits = NULL;
    int editCount = 0;
    int seedA = 0;
    int seedB = 0;
    unsigned char frame[512];
    int n = 0;

    mkdir(dir, 0755);
    snprintf(dbPath, sizeof(dbPath), "%s/opencraft.db", dir);
    unlink(dbPath);
    unlink("/tmp/opencraft-session-b258/opencraft.db-wal");
    unlink("/tmp/opencraft-session-b258/opencraft.db-shm");

    TestHash();
    server = GameServerCreate();
    Expect(server != NULL, "server alloc");
    Expect(GameServerStart(server, dbPath, 0) == 0, "server start");
    Expect(GameServerPort(server) > 0, "server port");

    TestBadVersion(GameServerPort(server));
    Expect(OpenClient(&alice, GameServerPort(server), "alice", "secret-pass", "alice@example.com", 1) == 1, "alice register");
    TestDuplicateAndPassword(GameServerPort(server));

    edits = (OcEdit *)calloc((size_t)OC_EDIT_MAX, sizeof(OcEdit));
    Expect(edits != NULL, "edit buffer");
    memset(&profile, 0, sizeof(profile));
    Expect(Join(&alice, &profile, edits, &editCount, &seedA) == 1, "alice join");
    Expect(editCount == 0, "fresh world has no edits");
    Expect(profile.selectedSlot == 0, "alice starts on slot 0");
    Expect(profile.hotbar[0] == OC_BLOCK_GRASS, "alice starts holding grass");

    n = ProtoBuildBlock(frame, (int)sizeof(frame), OC_C2S_BLOCK, 4, 80, 4, OC_BLOCK_DIAMOND_BLOCK);
    Expect((n > 0) && SendFrame(&alice, frame, n), "alice places diamond");
    Expect(WaitForBlock(&alice, 4, 80, 4, OC_BLOCK_DIAMOND_BLOCK), "alice hears her block");

    OcProfileSetDefault(&profile, profile.accountId, profile.username);
    profile.selectedSlot = 3;
    profile.hotbar[0] = OC_BLOCK_DIAMOND_BLOCK;
    profile.hotbarCount[0] = 64;
    profile.hotbar[3] = OC_BLOCK_BRICKS;
    profile.hotbarCount[3] = 64;
    n = ProtoBuildInventory(frame, (int)sizeof(frame), OC_C2S_INVENTORY, &profile);
    Expect((n > 0) && SendFrame(&alice, frame, n), "alice saves inventory");
    usleep(100000);

    Expect(OpenClient(&bob, GameServerPort(server), "bob", "bob-secret", "", 1) == 1, "bob register");
    editCount = 0;
    memset(&profile, 0, sizeof(profile));
    Expect(Join(&bob, &profile, edits, &editCount, &seedB) == 1, "bob join");
    Expect(seedB == seedA, "both clients share the seed");
    Expect(FindEdit(edits, editCount, 4, 80, 4) == OC_BLOCK_DIAMOND_BLOCK, "bob sees alice's diamond");
    Expect(profile.selectedSlot == 0, "bob has his own slot");
    Expect(profile.hotbar[0] == OC_BLOCK_GRASS, "bob does not inherit alice's hotbar");

    n = ProtoBuildBlock(frame, (int)sizeof(frame), OC_C2S_BLOCK, 5, 80, 4, OC_BLOCK_BRICKS);
    Expect((n > 0) && SendFrame(&bob, frame, n), "bob places bricks");
    Expect(WaitForBlock(&alice, 5, 80, 4, OC_BLOCK_BRICKS), "alice sees bob's bricks");

    CloseClient(&alice);
    CloseClient(&bob);
    usleep(100000);

    Expect(OpenClient(&alice, GameServerPort(server), "alice", "secret-pass", "", 0) == 1, "alice login");
    editCount = 0;
    memset(&profile, 0, sizeof(profile));
    Expect(Join(&alice, &profile, edits, &editCount, &seedA) == 1, "alice rejoin");
    Expect(profile.selectedSlot == 3, "alice held slot persisted");
    Expect(profile.hotbar[3] == OC_BLOCK_BRICKS, "alice held stack persisted");
    Expect(profile.hotbar[0] == OC_BLOCK_DIAMOND_BLOCK, "alice hotbar stack persisted");
    Expect(FindEdit(edits, editCount, 4, 80, 4) == OC_BLOCK_DIAMOND_BLOCK, "diamond persisted");
    Expect(FindEdit(edits, editCount, 5, 80, 4) == OC_BLOCK_BRICKS, "bricks persisted");

    Expect(OpenClient(&bob, GameServerPort(server), "bob", "bob-secret", "", 0) == 1, "bob login");
    editCount = 0;
    memset(&profile, 0, sizeof(profile));
    Expect(Join(&bob, &profile, edits, &editCount, &seedB) == 1, "bob rejoin");
    Expect(profile.selectedSlot == 0, "bob slot unchanged");
    Expect(profile.hotbar[0] == OC_BLOCK_GRASS, "bob hotbar unchanged");
    Expect(FindEdit(edits, editCount, 4, 80, 4) == OC_BLOCK_DIAMOND_BLOCK, "bob still sees the diamond");

    CloseClient(&alice);
    CloseClient(&bob);
    usleep(100000);

    Expect(OpenClient(&alice, GameServerPort(server), "alice", "secret-pass", "", 0) == 1, "alice pose login");
    editCount = 0;
    Expect(Join(&alice, &profile, edits, &editCount, &seedA) == 1, "alice pose join");
    n = ProtoBuildPose(frame, (int)sizeof(frame), 8.0f, 64.0f, 12.0f, 0.4f, -0.25f);
    Expect((n > 0) && SendFrame(&alice, frame, n), "alice saves pose");
    usleep(100000);
    CloseClient(&alice);
    usleep(80000);
    Expect(OpenClient(&alice, GameServerPort(server), "alice", "secret-pass", "", 0) == 1, "alice pose rejoin login");
    Expect(Join(&alice, &profile, edits, &editCount, &seedA) == 1, "alice pose rejoin");
    Expect(fabsf(welcomeX - 8.0f) < 0.2f, "alice pose x persisted");
    Expect(fabsf(welcomeY - 64.0f) < 0.2f, "alice pose y persisted");
    Expect(fabsf(welcomeZ - 12.0f) < 0.2f, "alice pose z persisted");
    Expect(fabsf(welcomeYaw - 0.4f) < 0.05f, "alice look persisted");

    Expect(OpenClient(&bob, GameServerPort(server), "bob", "bob-secret", "", 0) == 1, "bob pose login");
    Expect(Join(&bob, &profile, edits, &editCount, &seedB) == 1, "bob pose join");
    n = ProtoBuildPose(frame, (int)sizeof(frame), 8.0f, 64.0f, 12.0f, 0.4f, -0.25f);
    Expect((n > 0) && SendFrame(&bob, frame, n), "bob saves the same pose");
    usleep(100000);
    CloseClient(&bob);
    usleep(80000);
    Expect(OpenClient(&bob, GameServerPort(server), "bob", "bob-secret", "", 0) == 1, "bob overlap login");
    Expect(Join(&bob, &profile, edits, &editCount, &seedB) == 1, "bob overlap join");
    Expect(((welcomeX - 8.0f)*(welcomeX - 8.0f) + (welcomeZ - 12.0f)*(welcomeZ - 12.0f)) > 2.0f, "bob is not spawned inside alice");

    CloseClient(&alice);
    CloseClient(&bob);
    GameServerStop(server);
    GameServerDestroy(server);
    server = NULL;

    {
        OcDb db;
        char hash[128];
        char email[128];
        sqlite3 *sqlite = NULL;
        sqlite3_stmt *stmt = NULL;

        memset(&db, 0, sizeof(db));
        Expect(OcDbOpen(&db, dbPath) == 1, "reopen opencraft.db");
        Expect(OcDbSeed(&db) == (unsigned)seedA, "seed persisted");
        OcDbClose(&db);

        Expect(sqlite3_open_v2(dbPath, &sqlite, SQLITE_OPEN_READONLY, NULL) == SQLITE_OK, "sqlite open");
        Expect(sqlite3_prepare_v2(sqlite, "SELECT password_hash, email FROM accounts WHERE username = 'alice'", -1, &stmt, NULL) == SQLITE_OK, "account query");
        Expect(sqlite3_step(stmt) == SQLITE_ROW, "alice row");
        snprintf(hash, sizeof(hash), "%s", sqlite3_column_text(stmt, 0));
        snprintf(email, sizeof(email), "%s", sqlite3_column_text(stmt, 1));
        Expect(strcmp(hash, "secret-pass") != 0, "password is not plaintext");
        Expect(strlen(hash) == 64, "password hash is sha256 hex");
        Expect(strcmp(email, "alice@example.com") == 0, "email persisted");
        sqlite3_finalize(stmt);
        Expect(sqlite3_prepare_v2(sqlite, "SELECT loaded FROM extension_status WHERE name = 'sqlite-vec'", -1, &stmt, NULL) == SQLITE_OK, "vec status query");
        Expect(sqlite3_step(stmt) == SQLITE_ROW, "vec status row");
        sqlite3_finalize(stmt);
        sqlite3_close(sqlite);
    }

    free(edits);
    if (failures != 0)
    {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("session_test passed\n");
    return 0;
}
