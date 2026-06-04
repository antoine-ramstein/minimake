#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <err.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "microstruct.h"

static void remove_backline(char *str)
{
    int count = 0;
    for (int i = 0; str[i]; i++)
    {
        if (str[i] != '\n')
        {
            str[count++] = str[i];
        }
    }
    str[count] = '\0';
}

static int is_valid(char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        if (isblank(str[i]) || str[i] == ':' || str[i] == '=' || str[i] == '#')
        {
            return 0;
        }
    }
    return 1;
}

static int contain_sharp(const char *str)
{
    while (*str != '\0')
    {
        if (*str == '#')
        {
            return 1;
        }
        str++;
    }
    return 0;
}

static void je_vais_te_tailler(char *str)
{
    if (str == NULL)
        return;
    char *start = str;
    char *end;

    while (*start && isblank(*start))
        start++;
    if (*start == 0)
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

static void parse_var_line(struct ms *res, char *line, int var_index)
{
    if (line == NULL)
        return;

    char *key = line;
    char *value = NULL;

    char *eq = strchr(line, '=');
    if (eq != NULL)
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
    strcpy(res->var_value[var_index], token);
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
            {
                errx(2, "Syntax error: Invalid dependencies name %s", token);
            }
        }
        strcpy(res->dep[rule][count], token);
        count++;
    }
    res->dsize[rule] = count;
}

static int parse_command_line(struct ms *res, int *command, int rule,
                              char *line)
{
    remove_backline(line);
    if (line[0] != '\t')
    {
        return (strlen(line) == 0);
    }
    je_vais_te_tailler(line);
    if (strlen(line) > 0)
    {
        strcpy(res->command[rule][*command], line);
        (*command)++;
    }
    return 1;
}

static int handling_error(char *token)
{
    if (token == NULL)
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

static struct ms end(struct ms *res, char *line, FILE *file, int rule)
{
    res->rsize = rule;
    free(line);
    fclose(file);
    return *res;
}

static int is_var(const char *str)
{
    size_t len = strlen(str);
    return len >= 4 && str[0] == '$' && str[1] == '(' && str[len - 1] == ')';
}

static void trim_var(char *str)
{
    size_t len = strlen(str);
    if (len >= 3 && str[0] == '$' && str[1] == '(' && str[len - 1] == ')')
    {
        memmove(str, str + 2, len - 3);
        str[len - 3] = '\0';
    }
}

static void replace_var(struct ms *res)
{
    if (!res)
        return;
    for (size_t i = 0; i < res->rsize; i++)
    {
        for (size_t j = 0; j < res->dsize[i]; j++)
        {
            if (res->dep[i][j][0] == '$')
            {
                if (is_var(res->dep[i][j]))
                {
                    char var_name[256];
                    strncpy(var_name, res->dep[i][j], sizeof(var_name));
                    var_name[sizeof(var_name) - 1] = '\0';
                    trim_var(var_name);
                    int found = 0;
                    for (size_t k = 0; k < res->vnsize; k++)
                    {
                        if (strcmp(var_name, res->var_name[k]) == 0)
                        {
                            strcpy(res->dep[i][j], res->var_value[k]);
                            found = 1;
                            break;
                        }
                    }
                    if (!found)
                        errx(2, "Error: Varibale %s is not defined", var_name);
                }
                else
                {
                    errx(2, "Invalid Syntax: wrong variable name %s",
                         res->dep[i][j]);
                }
            }
        }
    }
}

static void replace_var_commands(struct ms *res)
{
    if (!res)
        return;

    for (size_t i = 0; i < res->rsize; i++)
    {
        for (size_t j = 0; j < res->csize[i]; j++)
        {
            char *cmd = res->command[i][j];
            char buffer[1024] = "";
            char temp[1024];
            size_t pos = 0;

            while (*cmd)
            {
                if (*cmd == '$' && *(cmd + 1) == '(')
                {
                    char var_name[256];
                    size_t k = 0;
                    cmd += 2;
                    while (*cmd && *cmd != ')' && k < sizeof(var_name) - 1)
                        var_name[k++] = *cmd++;
                    var_name[k] = '\0';
                    if (*cmd == ')')
                        cmd++;

                    int found = 0;
                    for (size_t v = 0; v < res->vnsize; v++)
                    {
                        if (strcmp(var_name, res->var_name[v]) == 0)
                        {
                            snprintf(temp, sizeof(temp), "%s",
                                     res->var_value[v]);
                            found = 1;
                            break;
                        }
                    }

                    if (!found)
                        errx(2, "Error: variable %s is not defined", var_name);

                    if (pos + strlen(temp) < sizeof(buffer))
                    {
                        strcat(buffer, temp);
                        pos += strlen(temp);
                    }
                }
                else
                {
                    size_t len = strlen(buffer);
                    if (len < sizeof(buffer) - 1)
                        buffer[len] = *cmd, buffer[len + 1] = '\0';
                    cmd++;
                }
            }

            strcpy(res->command[i][j], buffer);
        }
    }
}

void replace_var_rule(struct ms *res)
{
    {
        if (!res)
            return;
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
                        errx(2, "Error: Varibale %s is not defined", var_name);
                }
                else
                {
                    errx(2, "Invalid Syntax: wrong variable name %s",
                         res->rule_name[i]);
                }
            }
        }
    }
}

void parse(struct ms *res, char *path)
{
    FILE *file;
    char *line = NULL;
    size_t size = 0;
    ssize_t nread;

    file = fopen(path, "r");
    if (file == NULL)
        errx(2, "Unable to open file %s", path);

    size_t rule = 0;
    size_t var = 0;
    while ((nread = getline(&line, &size, file)) != -1)
    {
        remove_backline(line);
        if (strlen(line) == 0 || line[0] == '#')
            continue;
        if (line[0] == '\t')
            errx(2, "Syntax error: command without rule");
        if (strchr(line, '='))
        {
            parse_var_line(res, line, var);
            var++;
            continue;
        }
        char *token = strtok(line, " :\t");
        if (handling_error(token))
            continue;
        strcpy(res->rule_name[rule], token);
        parse_dep_line(res, token, rule);
        int command = 0;
        while ((nread = getline(&line, &size, file)) != -1)
        {
            if (!parse_command_line(res, &command, rule, line))
                break;
        }
        res->csize[rule] = command;
        rule++;

        if (nread == -1)
            break;
        if (line[0] != '\t')
            fseek(file, -nread, SEEK_CUR);
    }
    end(res, line, file, rule);
}

void pretty_print(struct ms res)
{
    replace_var(&res);
    size_t var = 0;
    printf("# variables\n");
    while (var != res.vnsize)
    {
        printf("'%s' = '%s'\n", res.var_name[var], res.var_value[var]);
        var++;
    }

    size_t count = 0;
    printf("# rules\n");
    while (count != res.rsize)
    {
        printf("(%s):", res.rule_name[count]);
        size_t c2 = 0;
        while (c2 != res.dsize[count])
        {
            if (c2 == res.dsize[count] - 1)
                printf(" [%s]", res.dep[count][c2]);
            else
            {
                printf(" [%s]", res.dep[count][c2]);
            }
            c2++;
        }
        printf("\n");
        size_t c3 = 0;
        while (c3 != res.csize[count])
        {
            printf("\t'%s'\n", res.command[count][c3]);
            c3++;
        }
        count++;
    }
}

int print_help(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            printf("%s:\n", argv[0]);
            printf("-h: display this help message\n");
            printf("-f: <file>: define the name of the Makefile to use\n");
            printf("-p: printf the Makefile organized\n");
            return 1;
        }
    }
    return 0;
}

int print_p(int argc, char *argv[], struct ms res)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            pretty_print(res);
            return 1;
        }
    }
    return 0;
}

int microshell(char *command)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        err(1, "fork failed");
    }

    if (pid == 0)
    {
        execlp("/bin/sh", "supershell", "-c", command, NULL);
        err(1, "execlp failed");
    }
    else
    {
        int wstatus;
        pid_t child_pid = waitpid(pid, &wstatus, 0);
        if (child_pid == -1)
        {
            err(1, "waitpid failed");
        }
        return WEXITSTATUS(wstatus);
    }

    return 0;
}

char *file_and_target(int argc, char *argv[], struct ms *r)
{
    char *res = NULL;
    r->tsize = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp("-f", argv[i]) == 0)
        {
            if (i + 1 >= argc)
                errx(2, "No file specified after -f");
            res = strdup(argv[i + 1]);
            i++;
            continue;
        }
        if (strcmp("-p", argv[i]) == 0 || strcmp("-h", argv[i]) == 0)
            continue;

        strcpy(r->target[r->tsize], argv[i]);
        r->tsize++;
    }
    return res;
}

int find_index(struct ms *res, char *rule)
{
    for (size_t i = 0; i < res->rsize; i++)
    {
        if (strcmp(res->rule_name[i], rule) == 0)
        {
            return i;
        }
    }
    return -1;
}

char *make_dollar_escape(const char *src)
{
    size_t len = strlen(src);
    char *res = malloc(len + 1);
    size_t j = 0;

    for (size_t i = 0; src[i]; i++)
    {
        if (src[i] == '$' && src[i + 1] == '$')
        {
            res[j++] = '$';
            i++;
        }
        else
        {
            res[j++] = src[i];
        }
    }
    res[j] = '\0';
    return res;
}

void exec_no_target(struct ms *res, int r_index)
{
    struct stat st;
    if (res->visited[r_index])
        return;
    res->visited[r_index] = 1;
    if (res->rsize == 0 || stat(res->rule_name[r_index], &st) == 0)
        return;
    for (size_t i = 0; i < res->dsize[r_index]; i++)
    {
        char *dep_name = res->dep[r_index][i];

        if (stat(dep_name, &st) && (S_ISREG(st.st_mode)))
            continue;

        int idx = find_index(res, dep_name);
        if (idx != -1)
            exec_no_target(res, idx);
        else
            errx(2, "rule %s not found", dep_name);
    }

    for (size_t i = 0; i < res->csize[r_index]; i++)
    {
        if (res->command[r_index][i][0] != '@')
        {
            printf("%s\n", res->command[r_index][i]);
            char *command = make_dollar_escape(res->command[r_index][i]);
            int wstatus = microshell(command);
            if (wstatus != 0)
                errx(2, "Wrong command %s", command);
            free(command);
        }
        else
        {
            char *command = make_dollar_escape(res->command[r_index][i]);
            command++;
            int wstatus = microshell(command--);
            if (wstatus != 0)
                errx(2, "Wrong command %s", command);
            free(command);
        }
    }
}

void execution(struct ms *res)
{
    replace_var(res);
    replace_var_commands(res);
    replace_var_rule(res);
    if (res->tsize == 0)
    {
        exec_no_target(res, 0);
    }
    else
    {
        for (size_t i = 0; i < res->tsize; i++)
        {
            int idx = find_index(res, res->target[i]);
            if (idx != -1)
            {
                exec_no_target(res, idx);
            }
            else
            {
                errx(2, "rule %s not found", res->target[i]);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        errx(2, "Wrong use of %s. Use -h for help", argv[0]);
        return 2;
    }
    if (print_help(argc, argv))
        return 0;

    struct ms res;
    char *path = file_and_target(argc, argv, &res);
    struct stat st;
    if (!path)
    {
        if (stat("Makefile", &st) == 0 && S_ISREG(st.st_mode))
        {
            path = strdup("Makefile");
        }
        else if (stat("makefile", &st) == 0 && S_ISREG(st.st_mode))
        {
            path = strdup("makefile");
        }
        else
        {
            errx(2, "No Makefile found and no file was given with -f");
        }
    }
    if (stat(path, &st) != 0)
        errx(2, "Thie file does not exist: %s", path);
    if (S_ISDIR(st.st_mode))
        errx(2, "%s is a directory", path);

    res.rsize = 0;
    parse(&res, path);
    if (print_p(argc, argv, res))
    {
        free(path);
        return 0;
    }

    execution(&res);
    free(path);
    return 0;
}
