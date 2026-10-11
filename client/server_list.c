#include "server_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ServerEntry servers[SERVER_LIST_MAX];
static int serverCount = 0;
static int loaded = 0;

static void CopyField(char *dst, int cap, const char *src)
{
    int i = 0;

    if (cap <= 0) return;
    if (src == NULL) src = "";
    while ((src[i] != '\0') && (i < cap - 1))
    {
        char c = src[i];
        if ((c == '\t') || (c == '\n') || (c == '\r')) c = ' ';
        dst[i] = c;
        i++;
    }
    dst[i] = '\0';
}

static void AddDefault(void)
{
    serverCount = 0;
    CopyField(servers[0].name, SERVER_NAME_MAX, "Local");
    CopyField(servers[0].host, SERVER_HOST_MAX, "127.0.0.1");
    servers[0].port = 25570;
    serverCount = 1;
}

static int Save(void)
{
    FILE *file = fopen("servers.txt", "w");
    int i = 0;

    if (file == NULL) return 0;
    for (i = 0; i < serverCount; i++)
    {
        fprintf(file, "%s\t%s\t%d\n", servers[i].name, servers[i].host, servers[i].port);
    }
    fclose(file);
    return 1;
}

void ServerListLoad(void)
{
    FILE *file = NULL;
    char line[160];

    if (loaded) return;
    loaded = 1;
    serverCount = 0;
    file = fopen("servers.txt", "r");
    if (file == NULL)
    {
        AddDefault();
        Save();
        return;
    }
    while ((serverCount < SERVER_LIST_MAX) && (fgets(line, (int)sizeof(line), file) != NULL))
    {
        char *tab1 = strchr(line, '\t');
        char *tab2 = NULL;
        int port = 0;

        if (tab1 == NULL) continue;
        *tab1 = '\0';
        tab2 = strchr(tab1 + 1, '\t');
        if (tab2 == NULL) continue;
        *tab2 = '\0';
        port = atoi(tab2 + 1);
        if ((port <= 0) || (port > 65535)) continue;
        if ((line[0] == '\0') || (tab1[1] == '\0')) continue;
        CopyField(servers[serverCount].name, SERVER_NAME_MAX, line);
        CopyField(servers[serverCount].host, SERVER_HOST_MAX, tab1 + 1);
        servers[serverCount].port = port;
        serverCount++;
    }
    fclose(file);
    if (serverCount == 0) AddDefault();
}

int ServerListCount(void)
{
    ServerListLoad();
    return serverCount;
}

ServerEntry *ServerListAt(int index)
{
    ServerListLoad();
    if ((index < 0) || (index >= serverCount)) return NULL;
    return &servers[index];
}

int ServerListAdd(const char *name, const char *host, int port)
{
    ServerListLoad();
    if (serverCount >= SERVER_LIST_MAX) return 0;
    if ((port <= 0) || (port > 65535)) return 0;
    if ((name == NULL) || (name[0] == '\0')) return 0;
    if ((host == NULL) || (host[0] == '\0')) return 0;
    CopyField(servers[serverCount].name, SERVER_NAME_MAX, name);
    CopyField(servers[serverCount].host, SERVER_HOST_MAX, host);
    servers[serverCount].port = port;
    serverCount++;
    Save();
    return 1;
}

void ServerListUpdate(int index, const char *name, const char *host, int port)
{
    ServerEntry *entry = ServerListAt(index);

    if (entry == NULL) return;
    if ((port <= 0) || (port > 65535)) return;
    if ((name == NULL) || (name[0] == '\0')) return;
    if ((host == NULL) || (host[0] == '\0')) return;
    CopyField(entry->name, SERVER_NAME_MAX, name);
    CopyField(entry->host, SERVER_HOST_MAX, host);
    entry->port = port;
    Save();
}

void ServerListRemove(int index)
{
    int i = 0;

    ServerListLoad();
    if ((index < 0) || (index >= serverCount)) return;
    for (i = index; i < serverCount - 1; i++) servers[i] = servers[i + 1];
    serverCount--;
    if (serverCount == 0) AddDefault();
    Save();
}
