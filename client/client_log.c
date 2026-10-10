#include "client_log.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#if defined(_WIN32)
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

#define LOG_MAX_BYTES (8*1024*1024)

static FILE *logFile = NULL;
static char logDir[128] = "logs";

static void MakeDir(const char *path)
{
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static bool FileExists(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) return false;
    fclose(file);
    return true;
}

static void RotateLatest(void)
{
    char latest[192] = { 0 };
    FILE *file = NULL;
    long length = 0;
    unsigned char *data = NULL;
    char dated[192] = { 0 };
    time_t now = time(NULL);
    struct tm *info = localtime(&now);
    char day[16] = "1970-01-01";
    int index = 1;
    gzFile gz = NULL;

    snprintf(latest, sizeof(latest), "%s/latest.log", logDir);
    file = fopen(latest, "rb");
    if (file == NULL) return;

    if (fseek(file, 0, SEEK_END) == 0) length = ftell(file);
    if ((length <= 0) || (length > LOG_MAX_BYTES))
    {
        fclose(file);
        return;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return;
    }

    data = (unsigned char *)malloc((size_t)length);
    if (data == NULL)
    {
        fclose(file);
        return;
    }

    if (fread(data, 1, (size_t)length, file) != (size_t)length)
    {
        free(data);
        fclose(file);
        return;
    }
    fclose(file);

    if (info != NULL) strftime(day, sizeof(day), "%Y-%m-%d", info);
    for (index = 1; index < 1000; index++)
    {
        snprintf(dated, sizeof(dated), "%s/%s-%d.log.gz", logDir, day, index);
        if (!FileExists(dated)) break;
    }

    gz = gzopen(dated, "wb");
    if (gz != NULL)
    {
        gzwrite(gz, data, (unsigned)length);
        gzclose(gz);
        remove(latest);
    }

    free(data);
}

void ClientLogInit(const char *directory)
{
    char path[192] = { 0 };

    ClientLogClose();
    if ((directory != NULL) && (directory[0] != '\0'))
    {
        snprintf(logDir, sizeof(logDir), "%s", directory);
    }

    MakeDir(logDir);
    RotateLatest();
    snprintf(path, sizeof(path), "%s/latest.log", logDir);
    logFile = fopen(path, "w");
}

void ClientLog(const char *fmt, ...)
{
    char message[512] = { 0 };
    char clock[16] = "00:00:00";
    time_t now = 0;
    struct tm *info = NULL;
    va_list args;

    if ((logFile == NULL) || (fmt == NULL)) return;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    now = time(NULL);
    info = localtime(&now);
    if (info != NULL) strftime(clock, sizeof(clock), "%H:%M:%S", info);
    fprintf(logFile, "[%s] [Client thread/INFO]: %s\n", clock, message);
    fflush(logFile);
}

void ClientLogClose(void)
{
    if (logFile != NULL) fclose(logFile);
    logFile = NULL;
}
