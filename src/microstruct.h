#ifndef MICROSTRUCT_H
#define MICROSTRUCT_H

#include <stddef.h>

struct ms
{
    char rule_name[64][64];
    char dep[64][32][64];
    char command[128][64][128];
    char var_name[32][64];
    char var_value[32][128];
    size_t rsize;
    size_t dsize[64];
    size_t csize[64];
    size_t vnsize;
    char target[32][64];
    size_t tsize;
    int visited[256];
};

#endif /* ! MICROSTRUCT_H */
