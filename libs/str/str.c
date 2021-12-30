#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "str.h"
int count(char *input, char *target)
{
    int total = 0, il = strlen(input), tl = strlen(target);
    if (il < tl || tl < 1)
        return 0;
    for (int i = 0; i < il; i++)
    {
        int flag = 1;
        for (int j = 0; j < tl; j++)
        {
            if (target[j] == '\0' || input[i + j] != target[j])
            {
                flag = 0;
                break;
            }
        }
        if (flag == 1)
            total++;
    }
    return total;
}
void split(char *input, char *split, char **output, int *len)
{
    int MaxLen = 100;
    char *result[MaxLen];
    int i = 0, il = strlen(input), sl = strlen(split);
    int flag = 0, total = -1;
    char prev[100] = {0};
    while (i < il)
    {
        flag = 0;
        for (int j = 0; j < sl && i + j < il; j++)
        {
            if (split[j] == input[i + j])
            {
                flag = 1;
            }
            else
            {
                flag = 0;
                sprintf(prev, "%s%c", prev, input[i + j]);
                break;
            }
        }
        if (flag == 1)
        {
            total += 1;
            i += sl;
            result[total] = (char *)malloc(100 * sizeof(char));
            strcpy(result[total], prev);
            output[total] = result[total];
            memset(prev, 0, sizeof(prev));
        }
        else
            i++;
    }
    total += 1;
    result[total] = (char *)malloc(100 * sizeof(char));
    strcpy(result[total], prev);
    output[total] = result[total];
    *len = total + 1;
}