#define _POSIX_C_SOURCE 200809L

#include "parse.h"

#include <ctype.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "helpers.h"
#include "microstruct.h"

static void parse_var_line(struct ms *res, char *line, int var_index)
{
    char *key = line;
    char *value = NULL;
    char *eq = strchr(line, '=');

    if (eq)
    {
        *eq = '\0';
        eq++;
        value = eq;
        while (*value && (*value == ' ' || *value == '\t'))
            value++;
    }
    else
    {
        value = "";
    }

    je_vais_te_tailler(key);
    je_vais_te_tailler(value);

    strcpy(res->var_name[var_index], key);
    char *token = strtok(value, "#");
    if (token)
        strcpy(res->var_value[var_index], token);
    else
        res->var_value[var_index][0] = '\0';
        
    res->vnsize = var_index + 1;
}

static void parse_dep_line(struct ms *res, char *token, int rule)
{
    int count = 0;
    while ((token = strtok(NULL, " :\t")) != NULL)
    {
        if (!is_valid(token))
        {
            if (contain_sharp(token))
                break;
            else
                errx(2, "Syntax error: Invalid dependencies name %s", token);
        }
        strcpy(res->dep[rule][count++], token);
    }
    res->dsize[rule] = count;
}

static int handling_error(char *token)
{
    if (!token)
        errx(2, "Syntax error: Missing rule name");
    if (!is_valid(token))
    {
        if (contain_sharp(token))
            return 1;
        else
            errx(2, "Syntax error: Invalid rule name %s", token);
    }
    return 0;
}

void parse(struct ms *res, char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
        errx(2, "Unable to open file %s", path);

    char *line = NULL;
    size_t size = 0;
    ssize_t nread;

    int rule = -1;
    size_t var = 0;

    while ((nread = getline(&line, &size, file)) != -1)
    {
        remove_backline(line);
        if (strlen(line) == 0 || line[0] == '#')
            continue;

        if (line[0] == '\t')
        {
            if (rule == -1)
                errx(2, "Syntax error: command without rule");

            je_vais_te_tailler(line);
            if (strlen(line) > 0)
                strcpy(res->command[rule][res->csize[rule]++], line);
            continue;
        }

        char *colon = strchr(line, ':');
        char *eq = strchr(line, '=');

        if (eq && (!colon || eq < colon))
        {
            parse_var_line(res, line, var++);
            continue;
        }

        char *token = strtok(line, " :\t");
        if (handling_error(token))
            continue;

        rule++;
        strcpy(res->rule_name[rule], token);
        parse_dep_line(res, token, rule);
        res->csize[rule] = 0;
    }
    
    res->rsize = (rule == -1) ? 0 : rule + 1;
    free(line);
    fclose(file);
}

void pretty_print(struct ms res)
{
    replace_var(&res);
    printf("# variables\n");
    for (size_t i = 0; i < res.vnsize; i++)
        printf("'%s' = '%s'\n", res.var_name[i], res.var_value[i]);

    printf("# rules\n");
    for (size_t i = 0; i < res.rsize; i++)
    {
        printf("(%s):", res.rule_name[i]);
        for (size_t j = 0; j < res.dsize[i]; j++)
            printf(" [%s]", res.dep[i][j]);
        printf("\n");

        for (size_t j = 0; j < res.csize[i]; j++)
            printf("\t'%s'\n", res.command[i][j]);
    }
}

void replace_var(struct ms *res)
{
    if (!res)
        return;
    for (size_t i = 0; i < res->rsize; i++)
    {
        char new_deps[64][256];
        size_t new_count = 0;
        for (size_t j = 0; j < res->dsize[i]; j++)
        {
            char *dep = res->dep[i][j];
            if (dep[0] == '$')
            {
                if (is_var(dep))
                {
                    char var_name[256];
                    strncpy(var_name, dep, sizeof(var_name));
                    var_name[sizeof(var_name) - 1] = '\0';
                    trim_var(var_name);
                    for (size_t k = 0; k < res->vnsize; k++)
                    {
                        if (strcmp(var_name, res->var_name[k]) == 0)
                        {
                            char value_copy[512];
                            strncpy(value_copy, res->var_value[k], sizeof(value_copy));
                            value_copy[sizeof(value_copy) - 1] = '\0';

                            char *tok = strtok(value_copy, " \t");
                            while (tok)
                            {
                                strcpy(new_deps[new_count++], tok);
                                tok = strtok(NULL, " \t");
                            }
                            break;
                        }
                    }
                }
                else
                    errx(2, "Invalid Syntax: wrong variable name %s", dep);
            }
            else
                strcpy(new_deps[new_count++], dep);
        }
        res->dsize[i] = new_count;
        for (size_t j = 0; j < new_count; j++)
            strcpy(res->dep[i][j], new_deps[j]);
    }
}


void replace_var_commands(struct ms *res)
{
    if (!res) return;
    for (size_t i = 0; i < res->rsize; i++)
    {
        for (size_t j = 0; j < res->csize[i]; j++)
        {
            char *cmd = res->command[i][j];
            char buffer[2048] = "";
            size_t pos = 0;

            while (*cmd)
            {
                if (*cmd == '$' && *(cmd + 1) != '\0')
                {
                    if (*(cmd + 1) == '$') 
                    {
                        buffer[pos++] = *cmd++;
                        buffer[pos++] = *cmd++;
                        continue;
                    }

                    char var_name[256] = "";
                    size_t k = 0;
                    int is_valid_var = 0;
                    char *temp_cmd = cmd;

                    if (cmd[1] == '(' || cmd[1] == '{')
                    {
                        char close_char = (cmd[1] == '(') ? ')' : '}';
                        cmd += 2;
                        while (*cmd && *cmd != close_char && k < sizeof(var_name) - 1)
                            var_name[k++] = *cmd++;
                        var_name[k] = '\0';
                        
                        if (*cmd == close_char) {
                            is_valid_var = 1;
                            cmd++;
                        } else {
                            cmd = temp_cmd;
                        }
                    }
                    else
                    {
                        var_name[0] = cmd[1];
                        var_name[1] = '\0';
                        is_valid_var = 1;
                        cmd += 2;
                    }

                    if (is_valid_var)
                    {
                        for (size_t v = 0; v < res->vnsize; v++)
                        {
                            if (strcmp(var_name, res->var_name[v]) == 0)
                            {
                                char *val = res->var_value[v];
                                while (*val && pos < sizeof(buffer) - 1)
                                    buffer[pos++] = *val++;
                                break;
                            }
                        }
                    }
                    else
                    {
                        buffer[pos++] = *cmd++;
                    }
                }
                else
                {
                    if (pos < sizeof(buffer) - 1)
                        buffer[pos++] = *cmd;
                    cmd++;
                }
            }
            buffer[pos] = '\0';
            strcpy(res->command[i][j], buffer);
        }
    }
}


void replace_var_rule(struct ms *res)
{
    if (!res) return;
    for (size_t i = 0; i < res->rsize; i++)
    {
        if (res->rule_name[i][0] == '$')
        {
            if (is_var(res->rule_name[i]))
            {
                char var_name[256];
                strncpy(var_name, res->rule_name[i], sizeof(var_name));
                var_name[sizeof(var_name) - 1] = '\0';
                trim_var(var_name);
                int found = 0;
                for (size_t k = 0; k < res->vnsize; k++)
                {
                    if (strcmp(var_name, res->var_name[k]) == 0)
                    {
                        strcpy(res->rule_name[i], res->var_value[k]);
                        found = 1;
                        break;
                    }
                }
                if (!found)
                    res->rule_name[i][0] = '\0';
            }
            else
                errx(2, "Invalid Syntax: wrong variable name %s", res->rule_name[i]);
        }
    }
}
