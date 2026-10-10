#ifndef GAME_SERVER_H
#define GAME_SERVER_H

typedef struct GameServer GameServer;

GameServer *GameServerCreate(void);
void GameServerDestroy(GameServer *server);

/* port 0 asks the OS for a free port. Returns 0 on success. */
int GameServerStart(GameServer *server, const char *dbPath, int port);
void GameServerWait(GameServer *server);
void GameServerStop(GameServer *server);
int GameServerPort(const GameServer *server);

#endif
