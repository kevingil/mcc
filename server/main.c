#include "game_server.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void Usage(void)
{
    fprintf(stderr, "usage: opencraft-server [--port N] [--world DIR] [--db PATH]\n");
}

int main(int argc, char **argv)
{
    const char *worldDir = "world";
    const char *dbPath = NULL;
    char dbBuf[512];
    int port = 25570;
    const char *envPort = getenv("PORT");
    int i = 0;
    GameServer *server = NULL;

    if ((envPort != NULL) && (envPort[0] != '\0')) port = atoi(envPort);

    for (i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "--port") == 0) && (i + 1 < argc))
        {
            port = atoi(argv[++i]);
        }
        else if ((strcmp(argv[i], "--world") == 0) && (i + 1 < argc))
        {
            worldDir = argv[++i];
        }
        else if ((strcmp(argv[i], "--db") == 0) && (i + 1 < argc))
        {
            dbPath = argv[++i];
        }
        else if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0))
        {
            Usage();
            return 0;
        }
        else
        {
            Usage();
            return 1;
        }
    }

    if ((port <= 0) || (port > 65535))
    {
        fprintf(stderr, "opencraft-server: bad port\n");
        return 1;
    }

    if ((mkdir(worldDir, 0755) != 0) && (access(worldDir, W_OK) != 0))
    {
        fprintf(stderr, "opencraft-server: cannot use world directory %s\n", worldDir);
        return 1;
    }

    if (dbPath == NULL)
    {
        snprintf(dbBuf, sizeof(dbBuf), "%s/opencraft.db", worldDir);
        dbPath = dbBuf;
    }

    signal(SIGPIPE, SIG_IGN);
    server = GameServerCreate();
    if (server == NULL) return 1;
    if (GameServerStart(server, dbPath, port) != 0)
    {
        GameServerDestroy(server);
        return 1;
    }

    GameServerWait(server);
    printf("opencraft-server stopping\n");
    fflush(stdout);
    GameServerDestroy(server);
    return 0;
}
