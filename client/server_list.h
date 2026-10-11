#ifndef SERVER_LIST_H
#define SERVER_LIST_H

#define SERVER_NAME_MAX 32
#define SERVER_HOST_MAX 64
#define SERVER_LIST_MAX 16

typedef struct ServerEntry {
    char name[SERVER_NAME_MAX];
    char host[SERVER_HOST_MAX];
    int port;
} ServerEntry;

void ServerListLoad(void);
int ServerListCount(void);
ServerEntry *ServerListAt(int index);
int ServerListAdd(const char *name, const char *host, int port);
void ServerListUpdate(int index, const char *name, const char *host, int port);
void ServerListRemove(int index);

#endif
