#include "game_server.h"

#include "db.h"
#include "protocol.h"
#include "wire.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 8
#define RECV_CAP 65536
#define PAYLOAD_CAP 65536

enum {
    PHASE_FREE = 0,
    PHASE_HELLO = 1,
    PHASE_AUTH = 2,
    PHASE_WORLD = 3
};

typedef struct {
    int fd;
    int phase;
    unsigned accountId;
    int hasPose;
    float px;
    float py;
    float pz;
    float yaw;
    float pitch;
    char name[OC_NAME_MAX + 1];
    unsigned char buf[RECV_CAP];
    int len;
} Conn;

struct GameServer {
    int listenFd;
    int port;
    volatile sig_atomic_t stop;
    unsigned tick;
    OcDb db;
    Conn clients[MAX_CLIENTS];
    pthread_t thread;
    int threadOn;
};

static int SetNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static uint64_t NowMs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec*1000ull + (uint64_t)ts.tv_nsec/1000000ull;
}

static void CloseConn(Conn *conn)
{
    if ((conn != NULL) && (conn->fd >= 0)) close(conn->fd);
    if (conn != NULL) memset(conn, 0, sizeof(*conn));
    if (conn != NULL) conn->fd = -1;
}

static int SendReject(Conn *conn, unsigned code, const char *text)
{
    unsigned char frame[128];
    int n = ProtoBuildReject(frame, (int)sizeof(frame), code, text);

    if (n < 0) return 0;
    return WireSendAll(conn->fd, frame, n);
}

static const char *RejectText(int code)
{
    switch (code)
    {
        case OC_REJECT_VERSION: return "Protocol mismatch";
        case OC_REJECT_TAKEN: return "Username is already taken";
        case OC_REJECT_PASSWORD: return "Wrong password";
        case OC_REJECT_UNKNOWN: return "Unknown username";
        case OC_REJECT_NAME: return "Username must be 3-16 letters, numbers, or underscore";
        case OC_REJECT_EMAIL: return "Email looks wrong";
        case OC_REJECT_AUTH: return "Log in first";
        case OC_REJECT_FULL: return "Server is full";
        default: return "Could not write the account";
    }
}

static void Drop(Conn *conn, unsigned code, const char *text)
{
    if ((conn != NULL) && (conn->fd >= 0) && (text != NULL)) SendReject(conn, code, text);
    CloseConn(conn);
}

static int BroadcastBlock(GameServer *server, int x, int y, int z, unsigned block)
{
    unsigned char frame[32];
    int n = ProtoBuildBlock(frame, (int)sizeof(frame), OC_S2C_BLOCK, x, y, z, block);
    int i = 0;

    if (n < 0) return 0;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        Conn *conn = &server->clients[i];

        if ((conn->fd < 0) || (conn->phase != PHASE_WORLD)) continue;
        if (!WireSendAll(conn->fd, frame, n)) CloseConn(conn);
    }
    return 1;
}

static void WelcomePose(GameServer *server, Conn *conn, float *x, float *y, float *z, float *yaw, float *pitch)
{
    int has = 0;
    int others = 0;
    int i = 0;
    float px = 0.0f;
    float py = -1.0f;
    float pz = 0.0f;
    float pyaw = 0.0f;
    float ppitch = 0.0f;

    has = OcDbLoadPose(&server->db, conn->accountId, &px, &py, &pz, &pyaw, &ppitch);
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        Conn *other = &server->clients[i];

        if (other == conn) continue;
        if ((other->fd < 0) || (other->phase != PHASE_WORLD)) continue;
        others++;
    }
    if (!has)
    {
        *x = (float)others;
        *y = -1.0f;
        *z = 0.0f;
        *yaw = 0.0f;
        *pitch = 0.0f;
        conn->hasPose = 0;
        return;
    }
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        Conn *other = &server->clients[i];
        int guard = 0;
        float dx = 0.0f;
        float dz = 0.0f;

        if (other == conn) continue;
        if ((other->fd < 0) || (other->phase != PHASE_WORLD) || !other->hasPose) continue;
        dx = px - other->px;
        dz = pz - other->pz;
        while (((dx*dx) + (dz*dz) < 2.25f) && (guard < 6))
        {
            px += 2.2f;
            dx = px - other->px;
            dz = pz - other->pz;
            guard++;
        }
    }
    *x = px;
    *y = py;
    *z = pz;
    *yaw = pyaw;
    *pitch = ppitch;
    conn->hasPose = 1;
    conn->px = px;
    conn->py = py;
    conn->pz = pz;
    conn->yaw = pyaw;
    conn->pitch = ppitch;
}

static int SendWelcomePack(GameServer *server, Conn *conn)
{
    OcProfile profile;
    OcEdit *edits = NULL;
    int editCount = 0;
    unsigned char welcome[64];
    unsigned char inventory[512];
    unsigned char *snapshot = NULL;
    int snapshotLen = 0;
    int n = 0;
    int ok = 0;

    memset(&profile, 0, sizeof(profile));
    if (!OcDbLoadProfile(&server->db, conn->accountId, &profile))
    {
        Drop(conn, OC_REJECT_INTERNAL, RejectText(OC_REJECT_INTERNAL));
        return 0;
    }
    edits = (OcEdit *)calloc((size_t)OC_EDIT_MAX, sizeof(OcEdit));
    if (edits == NULL)
    {
        Drop(conn, OC_REJECT_INTERNAL, RejectText(OC_REJECT_INTERNAL));
        return 0;
    }
    if (!OcDbLoadEdits(&server->db, edits, OC_EDIT_MAX, &editCount))
    {
        free(edits);
        Drop(conn, OC_REJECT_INTERNAL, RejectText(OC_REJECT_INTERNAL));
        return 0;
    }

    {
        float wx = 0.0f;
        float wy = -1.0f;
        float wz = 0.0f;
        float wyaw = 0.0f;
        float wpitch = 0.0f;

        WelcomePose(server, conn, &wx, &wy, &wz, &wyaw, &wpitch);
        n = ProtoBuildWelcome(welcome, (int)sizeof(welcome), conn->accountId, (int)OcDbSeed(&server->db), wx, wy, wz, wyaw, wpitch);
    }
    if ((n < 0) || !WireSendAll(conn->fd, welcome, n))
    {
        free(edits);
        CloseConn(conn);
        return 0;
    }
    n = ProtoBuildInventory(inventory, (int)sizeof(inventory), OC_S2C_INVENTORY, &profile);
    if ((n < 0) || !WireSendAll(conn->fd, inventory, n))
    {
        free(edits);
        CloseConn(conn);
        return 0;
    }
    if (ProtoBuildSnapshot(&snapshot, &snapshotLen, edits, editCount) < 0)
    {
        free(edits);
        CloseConn(conn);
        return 0;
    }
    ok = WireSendAll(conn->fd, snapshot, snapshotLen);
    free(snapshot);
    free(edits);
    if (!ok)
    {
        CloseConn(conn);
        return 0;
    }
    conn->phase = PHASE_WORLD;
    printf("join %s id %u seed %u edits %d\n", conn->name, conn->accountId, OcDbSeed(&server->db), editCount);
    fflush(stdout);
    return 1;
}

static void HandlePacket(GameServer *server, Conn *conn, const unsigned char *payload, int len)
{
    unsigned id = ProtoPacketId(payload, len);
    char user[OC_NAME_MAX + 1];
    char pass[OC_PASS_MAX + 1];
    char email[OC_EMAIL_MAX + 1];
    OcProfile profile;
    unsigned protocol = 0;
    int code = 0;
    int x = 0;
    int y = 0;
    int z = 0;
    unsigned block = 0;
    unsigned char frame[256];
    int n = 0;

    memset(&profile, 0, sizeof(profile));
    user[0] = '\0';
    pass[0] = '\0';
    email[0] = '\0';

    if (conn->phase == PHASE_HELLO)
    {
        if (!ProtoParseHello(payload, len, &protocol))
        {
            Drop(conn, OC_REJECT_VERSION, "Expected hello");
            return;
        }
        if (protocol != OC_PROTOCOL_VERSION)
        {
            Drop(conn, OC_REJECT_VERSION, RejectText(OC_REJECT_VERSION));
            return;
        }
        conn->phase = PHASE_AUTH;
        return;
    }

    if (id == OC_C2S_REGISTER)
    {
        if (conn->phase != PHASE_AUTH)
        {
            Drop(conn, OC_REJECT_AUTH, "Say hello first");
            return;
        }
        if (!ProtoParseRegister(payload, len, user, pass, email))
        {
            Drop(conn, OC_REJECT_NAME, RejectText(OC_REJECT_NAME));
            return;
        }
        code = OcDbRegister(&server->db, user, pass, email, &profile);
        if (code != 0)
        {
            const char *text = RejectText(code);

            if (code == OC_REJECT_PASSWORD) text = "Password must be 4-64 characters";
            Drop(conn, (unsigned)code, text);
            return;
        }
        conn->accountId = profile.accountId;
        snprintf(conn->name, sizeof(conn->name), "%s", profile.username);
        conn->phase = PHASE_AUTH;
        n = ProtoBuildAuthOk(frame, (int)sizeof(frame), OC_S2C_REGISTER_OK, profile.accountId, profile.username);
        if ((n < 0) || !WireSendAll(conn->fd, frame, n)) CloseConn(conn);
        else
        {
            printf("registered %s id %u\n", conn->name, conn->accountId);
            fflush(stdout);
        }
        return;
    }

    if (id == OC_C2S_LOGIN)
    {
        if (conn->phase != PHASE_AUTH)
        {
            Drop(conn, OC_REJECT_AUTH, "Say hello first");
            return;
        }
        if (!ProtoParseLogin(payload, len, user, pass))
        {
            Drop(conn, OC_REJECT_NAME, RejectText(OC_REJECT_NAME));
            return;
        }
        code = OcDbLogin(&server->db, user, pass, &profile);
        if (code != 0)
        {
            Drop(conn, (unsigned)code, RejectText(code));
            return;
        }
        conn->accountId = profile.accountId;
        snprintf(conn->name, sizeof(conn->name), "%s", profile.username);
        n = ProtoBuildAuthOk(frame, (int)sizeof(frame), OC_S2C_LOGIN_OK, profile.accountId, profile.username);
        if ((n < 0) || !WireSendAll(conn->fd, frame, n)) CloseConn(conn);
        else
        {
            printf("login %s id %u\n", conn->name, conn->accountId);
            fflush(stdout);
        }
        return;
    }

    if (conn->accountId == 0)
    {
        Drop(conn, OC_REJECT_AUTH, RejectText(OC_REJECT_AUTH));
        return;
    }

    if (id == OC_C2S_JOIN)
    {
        SendWelcomePack(server, conn);
        return;
    }

    if (id == OC_C2S_BLOCK)
    {
        if (conn->phase != PHASE_WORLD)
        {
            Drop(conn, OC_REJECT_AUTH, "Join the world first");
            return;
        }
        if (!ProtoParseBlock(payload, len, &x, &y, &z, &block))
        {
            Drop(conn, OC_REJECT_INTERNAL, "Bad block packet");
            return;
        }
        if (!OcDbPutBlock(&server->db, x, y, z, block))
        {
            Drop(conn, OC_REJECT_INTERNAL, "Could not store that block");
            return;
        }
        printf("block %d %d %d -> %u by %s\n", x, y, z, block, conn->name);
        fflush(stdout);
        BroadcastBlock(server, x, y, z, block);
        return;
    }

    if (id == OC_C2S_INVENTORY)
    {
        if (conn->phase != PHASE_WORLD)
        {
            Drop(conn, OC_REJECT_AUTH, "Join the world first");
            return;
        }
        profile.accountId = conn->accountId;
        if (!ProtoParseInventory(payload, len, &profile))
        {
            Drop(conn, OC_REJECT_INTERNAL, "Bad inventory packet");
            return;
        }
        profile.accountId = conn->accountId;
        if ((profile.selectedSlot < 0) || (profile.selectedSlot >= OC_HOTBAR_SLOTS))
        {
            Drop(conn, OC_REJECT_INTERNAL, "Bad hotbar slot");
            return;
        }
        if (!OcDbSaveProfile(&server->db, &profile))
        {
            Drop(conn, OC_REJECT_INTERNAL, "Could not store the inventory");
            return;
        }
        printf("inventory %s slot %d held %u\n", conn->name, profile.selectedSlot, profile.hotbar[profile.selectedSlot]);
        fflush(stdout);
        return;
    }

    if (id == OC_C2S_POSE)
    {
        float px = 0.0f;
        float py = 0.0f;
        float pz = 0.0f;
        float pyaw = 0.0f;
        float ppitch = 0.0f;

        if (conn->phase != PHASE_WORLD)
        {
            Drop(conn, OC_REJECT_AUTH, "Join the world first");
            return;
        }
        if (!ProtoParsePose(payload, len, &px, &py, &pz, &pyaw, &ppitch))
        {
            Drop(conn, OC_REJECT_INTERNAL, "Bad pose packet");
            return;
        }
        conn->hasPose = 1;
        conn->px = px;
        conn->py = py;
        conn->pz = pz;
        conn->yaw = pyaw;
        conn->pitch = ppitch;
        if (!OcDbSavePose(&server->db, conn->accountId, px, py, pz, pyaw, ppitch))
        {
            Drop(conn, OC_REJECT_INTERNAL, "Could not store the position");
        }
        return;
    }

    if (id == OC_C2S_DISCONNECT)
    {
        printf("disconnect %s\n", conn->name);
        fflush(stdout);
        CloseConn(conn);
        return;
    }

    if (id == OC_C2S_INPUT) return;

    Drop(conn, OC_REJECT_INTERNAL, "Unknown packet");
}

static void Drain(GameServer *server, Conn *conn)
{
    unsigned char payload[PAYLOAD_CAP];

    while (conn->fd >= 0)
    {
        int payloadLen = 0;
        int popped = ProtoPopFrame(conn->buf, &conn->len, payload, PAYLOAD_CAP, &payloadLen);

        if (popped == 0) return;
        if (popped < 0)
        {
            Drop(conn, OC_REJECT_INTERNAL, "Bad frame");
            return;
        }
        HandlePacket(server, conn, payload, payloadLen);
    }
}

static void ReadConn(GameServer *server, Conn *conn)
{
    while (conn->fd >= 0)
    {
        int space = RECV_CAP - conn->len;
        int n = 0;

        if (space <= 0)
        {
            Drop(conn, OC_REJECT_INTERNAL, "Packet too large");
            return;
        }
        n = (int)recv(conn->fd, conn->buf + conn->len, (size_t)space, MSG_DONTWAIT);
        if (n == 0)
        {
            CloseConn(conn);
            return;
        }
        if (n < 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) return;
            CloseConn(conn);
            return;
        }
        conn->len += n;
        Drain(server, conn);
    }
}

static void AcceptClients(GameServer *server)
{
    while (1)
    {
        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        int fd = accept(server->listenFd, (struct sockaddr *)&addr, &addrLen);
        int flags = 1;
        int slot = -1;
        int i = 0;

        if (fd < 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) return;
            return;
        }
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (server->clients[i].fd < 0)
            {
                slot = i;
                break;
            }
        }
        if (slot < 0)
        {
            unsigned char frame[128];
            int n = ProtoBuildReject(frame, (int)sizeof(frame), OC_REJECT_FULL, RejectText(OC_REJECT_FULL));

            if (n > 0) WireSendAll(fd, frame, n);
            close(fd);
            continue;
        }
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
        memset(&server->clients[slot], 0, sizeof(server->clients[slot]));
        server->clients[slot].fd = fd;
        server->clients[slot].phase = PHASE_HELLO;
    }
}

static void ServerLoop(GameServer *server)
{
    uint64_t tickAt = NowMs();

    while (!server->stop)
    {
        struct pollfd pfds[1 + MAX_CLIENTS];
        int slots[1 + MAX_CLIENTS];
        int nfds = 1;
        int i = 0;
        uint64_t now = NowMs();
        int wait = 50 - (int)(now - tickAt);
        int ready = 0;

        if (wait < 0) wait = 0;
        pfds[0].fd = server->listenFd;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;
        slots[0] = -1;
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (server->clients[i].fd < 0) continue;
            pfds[nfds].fd = server->clients[i].fd;
            pfds[nfds].events = POLLIN;
            pfds[nfds].revents = 0;
            slots[nfds] = i;
            nfds++;
        }
        ready = poll(pfds, (nfds_t)nfds, wait);
        if ((ready < 0) && (errno != EINTR)) break;
        if ((ready > 0) && (pfds[0].revents & POLLIN)) AcceptClients(server);
        if (ready > 0)
        {
            for (i = 1; i < nfds; i++)
            {
                Conn *conn = &server->clients[slots[i]];

                if (conn->fd < 0) continue;
                if (conn->fd != pfds[i].fd) continue;
                if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) CloseConn(conn);
                else if (pfds[i].revents & POLLIN) ReadConn(server, conn);
            }
        }

        now = NowMs();
        if (now - tickAt >= 50)
        {
            server->tick++;
            tickAt += 50;
            if (now - tickAt > 250) tickAt = now;
            if ((server->tick % 200u) == 0u)
            {
                int playing = 0;

                for (i = 0; i < MAX_CLIENTS; i++)
                {
                    if (server->clients[i].fd >= 0) playing++;
                }
                printf("tick %u clients %d\n", server->tick, playing);
                fflush(stdout);
            }
        }
    }
}

static void *ThreadMain(void *arg)
{
    ServerLoop((GameServer *)arg);
    return NULL;
}

GameServer *GameServerCreate(void)
{
    GameServer *server = (GameServer *)calloc(1, sizeof(GameServer));
    int i = 0;

    if (server == NULL) return NULL;
    server->listenFd = -1;
    for (i = 0; i < MAX_CLIENTS; i++) server->clients[i].fd = -1;
    return server;
}

void GameServerDestroy(GameServer *server)
{
    if (server == NULL) return;
    GameServerStop(server);
    free(server);
}

int GameServerStart(GameServer *server, const char *dbPath, int port)
{
    struct sockaddr_in addr;
    int fd = -1;
    int on = 1;
    socklen_t len = sizeof(addr);

    if ((server == NULL) || (dbPath == NULL)) return -1;
    if (!OcDbOpen(&server->db, dbPath))
    {
        fprintf(stderr, "opencraft-server: database: %s\n", OcDbLastError(&server->db));
        return -1;
    }
    printf("opencraft-server: opencraft.db seed %u sqlite-vec %s\n",
        OcDbSeed(&server->db), OcDbVecLoaded(&server->db) ? "loaded" : "not installed");

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        OcDbClose(&server->db);
        return -1;
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "opencraft-server: bind %d failed: %s\n", port, strerror(errno));
        close(fd);
        OcDbClose(&server->db);
        return -1;
    }
    if (listen(fd, 8) < 0)
    {
        close(fd);
        OcDbClose(&server->db);
        return -1;
    }
    if (getsockname(fd, (struct sockaddr *)&addr, &len) == 0) server->port = ntohs(addr.sin_port);
    else server->port = port;
    if (SetNonBlock(fd) != 0)
    {
        close(fd);
        OcDbClose(&server->db);
        return -1;
    }
    server->listenFd = fd;
    server->stop = 0;
    server->tick = 0;
    if (pthread_create(&server->thread, NULL, ThreadMain, server) != 0)
    {
        close(fd);
        server->listenFd = -1;
        OcDbClose(&server->db);
        return -1;
    }
    server->threadOn = 1;
    printf("opencraft-server listening on 0.0.0.0:%d\n", server->port);
    fflush(stdout);
    return 0;
}

void GameServerStop(GameServer *server)
{
    int i = 0;

    if (server == NULL) return;
    server->stop = 1;
    if (server->listenFd >= 0)
    {
        shutdown(server->listenFd, SHUT_RDWR);
    }
    if (server->threadOn)
    {
        pthread_join(server->thread, NULL);
        server->threadOn = 0;
    }
    for (i = 0; i < MAX_CLIENTS; i++) CloseConn(&server->clients[i]);
    if (server->listenFd >= 0)
    {
        close(server->listenFd);
        server->listenFd = -1;
    }
    OcDbClose(&server->db);
}

int GameServerPort(const GameServer *server)
{
    if (server == NULL) return 0;
    return server->port;
}

static GameServer *signaledServer = NULL;

static void OnSignal(int signo)
{
    (void)signo;
    if (signaledServer != NULL) signaledServer->stop = 1;
}

void GameServerWait(GameServer *server)
{
    struct sigaction action;

    if (server == NULL) return;
    signaledServer = server;
    memset(&action, 0, sizeof(action));
    action.sa_handler = OnSignal;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    while (!server->stop) usleep(50000);
    signaledServer = NULL;
}
