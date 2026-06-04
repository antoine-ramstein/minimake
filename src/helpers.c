#define _POSIX_C_SOURCE 200809L

#include "helpers.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void remove_backline(char *str)
{
    int count = 0;
    for (int i = 0; str[i]; i++)
    {
        if (str[i] != '\n' && str[i] != '\r')
            str[count++] = str[i];
    }
    str[count] = '\0';
}

int is_valid(char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        if (isblank(str[i]) || str[i] == ':' || str[i] == '=' || str[i] == '#')
            return 0;
    }
    return 1;
}

int contain_sharp(const char *str)
{
    while (*str)
    {
        if (*str == '#')
            return 1;
        str++;
    }
    return 0;
}

void je_vais_te_tailler(char *str)
{
    if (!str)
        return;
    char *start = str;
    char *end;

    while (*start && isblank(*start))
        start++;
    if (!*start)
    {
        str[0] = '\0';
        return;
    }

    memmove(str, start, strlen(start) + 1);
    end = str + strlen(str) - 1;
    while (end > str && isblank(*end))
        end--;
    *(end + 1) = '\0';
}

int is_var(const char *str)
{
    size_t len = strlen(str);
    if (len == 2 && str[0] == '$')
        return 1;
        
    return len >= 4 && str[0] == '$'
        && ((str[1] == '(' && str[len - 1] == ')')
            || (str[1] == '{' && str[len - 1] == '}'));
}

void trim_var(char *str)
{
    size_t len = strlen(str);

    if (len == 2 && str[0] == '$')
    {
        str[0] = str[1];
        str[1] = '\0';
    }
    else if (len >= 4 && str[0] == '$')
    {
        if (str[1] == '(' && str[len - 1] == ')')
        {
            memmove(str, str + 2, len - 3);
            str[len - 3] = '\0';
        }
        else if (str[1] == '{' && str[len - 1] == '}')
        {
            memmove(str, str + 2, len - 3);
            str[len - 3] = '\0';
        }
    }
}
