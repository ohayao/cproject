#include <stdarg.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "log.h"

long timestamp()
{
    time_t t;
    time(&t);
    return (long)t;
}
void timeStr(char *result)
{
    time_t t;
    time(&t);
    struct tm *lt = localtime(&t);
    sprintf(result, "%d-%02d-%d %02d:%02d:%02d", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
}
char *_typeToString(enum Type t)
{
    switch (t)
    {
    case Info:
        return "Info";
    case Warn:
        return "Warn";
    case Debug:
        return "Debug";
    case Error:
        return "Error";
    default:
        return "";
    }
}
void __flagStart(struct Log this, char *name)
{
    char time[100];
    memset(time, 0, sizeof(time));
    timeStr(time);
    printf("%s @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ [%s] In\n", time, name);
    memset(time, 0, sizeof(time));
}
void __flagEnd(struct Log this, char *name)
{
    char time[100];
    memset(time, 0, sizeof(time));
    timeStr(time);
    printf("%s @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ [%s] Exit\n", time, name);
    memset(time, 0, sizeof(time));
}
void __write(struct Log this, enum Type type, char *msg)
{
    char time[100];
    memset(time, 0, sizeof(time));
    timeStr(time);
    printf("[%-5s %s]  [%s] \n", _typeToString(type), time, msg);
    memset(time, 0, sizeof(time));
};
void __writef(struct Log this, enum Type type, char *fmts, ...)
{
    va_list va;
    char logStr[this.msgLen];
    va_start(va, fmts);
    vsnprintf(logStr, sizeof(logStr), fmts, va);
    va_end(va);
    __write(this, type, logStr);
};
void createDirs(char *paths, char *result[], int *lenss)
{
    int len = 1, count = 0;
    char split = '/';
    while (paths[count] != '\0')
    {
        if (paths[count] == split)
            len++;
        count++;
    }
    *lenss = len;
}
struct Log init(int msgLen, char *filePath)
{
    struct Log log;
    log.msgLen = msgLen;
    log.filePath = filePath;
    log.FlagStart = __flagStart;
    log.FlagEnd = __flagEnd;
    log.Write = __write;
    log.Writef = __writef;
    FILE *fp;
    fp = fopen(log.filePath, "r");
    if (fp == NULL)
    {
        int r1 = mkdir("./abcd/123", 0755);
        printf("test res=%d\n", r1);
        printf("create Path:%s\n", log.filePath);
        int res = mkdir(log.filePath, 0755);
        printf("res:%d\n", res);
        if (mkdir(log.filePath, 0755) == 0)
        {
            printf("yes\n");
        }
        fp = fopen(log.filePath, "w");
    }
    fclose(fp);
    return log;
}