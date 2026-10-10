#ifndef CLIENT_LOG_H
#define CLIENT_LOG_H

// Client log, same rotation the desktop client uses.
// logs/latest.log is the open session. The next launch gzips it to
// logs/YYYY-MM-DD-N.log.gz and starts a fresh latest.log.

void ClientLogInit(const char *directory);
void ClientLog(const char *fmt, ...);
void ClientLogClose(void);

#endif
