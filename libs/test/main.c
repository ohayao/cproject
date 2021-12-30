#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "../log/log.h"
#include "../str/str.h"

void testLog(struct Log log);
void testCount(char *str, char *sub);
void testSplit(char *str, char *split);
int main()
{
    struct Log log = init(1024, "./txt/log1");
    struct Log log2 = init(2048, "./txt/log2");

    testLog(log);
    testLog(log2);

    testCount("/user/root/home", "/");
    testCount("aaaaa", "aa");
    testCount("abababbabb", "ab");

    testSplit("/user/root/home", "/");
    testSplit("|A|B|C|D|", "|");
    testSplit("192.168.1.1", ".");
    return 0;
}
void testLog(struct Log log)
{
    log.FlagStart(log, "DEBUG XXXXX IN");
    log.Writef(log, Info, "log type %s %d", "info", 0);
    log.Writef(log, Warn, "log type %s %d", "warn", 1);
    log.Write(log, Error, "error message 。。。");
    log.FlagEnd(log, "DEBUG XXXXX OUT");
}
void testCount(char *str, char *sub)
{
    int len = count(str, sub);
    printf("[%s] [%s] Total=%d\n", str, sub, len);
}
void testSplit(char *str, char *_split)
{
    int len = 0;
    char *result[100] = {0};
    split(str, _split, (char **)&result, &len);
    printf("[%s].split(%s).Len=[%d]\n", str, _split, len);
    for (int i = 0; i < len; i++)
        printf("\t%d=[%s]", i, result[i]);
    printf("\n");
}