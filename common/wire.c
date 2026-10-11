#define _GNU_SOURCE

#include "wire.h"

#include <errno.h>
#include <stddef.h>
#include <sys/socket.h>

int WireSendAll(int fd, const void *data, int n)
{
    const char *bytes = (const char *)data;
    int sent = 0;

    if ((fd < 0) || (data == NULL) || (n < 0)) return 0;
    while (sent < n)
    {
        int wrote = (int)send(fd, bytes + sent, (size_t)(n - sent), MSG_NOSIGNAL);

        if (wrote < 0)
        {
            if (errno == EINTR) continue;
            return 0;
        }
        if (wrote == 0) return 0;
        sent += wrote;
    }
    return 1;
}
