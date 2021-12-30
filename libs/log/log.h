#ifndef __log_h__
#define __log_h__
#include <stdio.h>
#include <string.h>
enum Type
{
    Info = 0,
    Warn,
    Debug,
    Error
};
struct Log
{
    int msgLen;
    char *filePath;
    void (*FlagStart)(struct Log this, char *name);
    void (*FlagEnd)(struct Log this, char *name);
    void (*Writef)(struct Log this, enum Type type, char *fmts, ...);
    void (*Write)(struct Log this, enum Type type, char *msg);
};

struct Log init(int msgLen, char *filePath);
long timestamp();
void timeStr(char *result);
char *_typeToString(enum Type t);
void createDirs(char *paths, char *result[], int *lenss);
void __fLagStart(struct Log this, char *name);
void __flagEnd(struct Log this, char *name);
void __write(struct Log this, enum Type type, char *msg);
void __writef(struct Log this, enum Type type, char *fmts, ...);
#endif