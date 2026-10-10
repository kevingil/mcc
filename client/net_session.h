#ifndef NET_SESSION_H
#define NET_SESSION_H

#include "player.h"
#include "protocol.h"

void NetSessionDisconnect(void);
int NetSessionIsOnline(void);
int NetSessionIsInWorld(void);
const char *NetSessionStatus(void);
const char *NetSessionName(void);
unsigned NetSessionSeed(void);
const char *NetSessionEditLine(void);

int NetSessionCreateAccount(const char *host, int port, const char *user, const char *pass, const char *email);
int NetSessionLogin(const char *host, int port, const char *user, const char *pass);
int NetSessionJoinWorld(void);

void NetSessionApplySpawnInventory(Player *player);
void NetSessionOverlayEdits(VoxelWorld *world);
void NetSessionPoll(VoxelWorld *world);
void NetSessionLocalBlock(int x, int y, int z, int block);
void NetSessionSyncInventory(const Player *player);
void NetSessionLeave(const Player *player);

#endif
