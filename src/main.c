#define _POSIX_C_SOURCE 200809L

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "helpers.h"
#include "microstruct.h"
#include "parse.h"

int is_utd(struct ms *res, int r_index);

static int microshell(char *command)
{
    pid_t pid = fork();
    if (pid == -1)
        err(1, "fork failed");

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
            err(1, "waitpid failed");
        return WEXITSTATUS(wstatus);
    }
}

static int find_index(struct ms *res, char *rule)
{
    for (size_t i = 0; i < res->rsize; i++)
        if (strcmp(res->rule_name[i], rule) == 0)
            return i;
    return -1;
}

static char *make_dollar_escape(const char *src)
{
    size_t len = strlen(src);
    char *res = malloc(len + 1);
    size_t j = 0;

    for (size_t i = 0; i < len; i++)
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

int is_ntbd(struct ms *res, int r_index)
{
    if (res->csize[r_index] > 0)
        return 0;

    struct stat st;
    for (size_t i = 0; i < res->dsize[r_index]; i++)
    {
        char *dep_name = res->dep[r_index][i];
        int idx = find_index(res, dep_name);
        if (idx != -1)
        {
            if (!is_ntbd(res, idx) && !is_utd(res, idx))
                return 0;
        }
        else
        {
            if (stat(dep_name, &st) != 0 || !S_ISREG(st.st_mode))
                return 0;
        }
    }
    return 1;
}

int is_utd(struct ms *res, int r_index)
{
    struct stat st_target;
    if (stat(res->rule_name[r_index], &st_target) != 0)
        return 0;

    for (size_t i = 0; i < res->dsize[r_index]; i++)
    {
        char *dep_name = res->dep[r_index][i];
        int idx = find_index(res, dep_name);

        if (idx != -1)
        {
            if (!is_ntbd(res, idx) && !is_utd(res, idx))
                return 0;
            struct stat st_dep_rule;
            if (stat(res->rule_name[idx], &st_dep_rule) == 0)
            {
                if (st_dep_rule.st_mtime > st_target.st_mtime)
                    return 0;
            }
            else
                return 0;
        }
        else
        {
            struct stat st_dep;
            if (stat(dep_name, &st_dep) != 0)
                return 0;
            if (st_dep.st_mtime > st_target.st_mtime)
                return 0;
        }
    }
    return 1;
}

void exec_no_target(struct ms *res, int r_index)
{
    struct stat st;
    if (res->visited[r_index])
        return;
    res->visited[r_index] = 1;

    if (is_ntbd(res, r_index))
    {
        printf("minimake: Nothing to be done for '%s'.\n", res->rule_name[r_index]);
        return;
    }

    if (is_utd(res, r_index))
    {
        printf("minimake: '%s' is up to date.\n", res->rule_name[r_index]);
        return;
    }

    for (size_t i = 0; i < res->dsize[r_index]; i++)
    {
        char *dep_name = res->dep[r_index][i];
        int idx = find_index(res, dep_name);

        if (idx != -1)
        {
            exec_no_target(res, idx);
        }
        else
        {
            if (stat(dep_name, &st) != 0)
                errx(2, "minimake: *** No rule to make target '%s'. Stop.", dep_name);
        }
    }

    for (size_t i = 0; i < res->csize[r_index]; i++)
    {
        char *command = res->command[r_index][i];
        int silent = 0;
        if (command[0] == '@')
        {
            silent = 1;
            command++;
        }
        if (!silent)
        {
            printf("%s\n", command);
            fflush(stdout);
        }
        char *cmd_exec = make_dollar_escape(command);
        int wstatus = microshell(cmd_exec);
        free(cmd_exec);
        if (wstatus != 0)
            errx(2, "Command failed");
    }
}

static int was_done(struct ms *res, int index)
{
    if (!res)
        return -1;
    for (int i = 0; i < index; i++)
    {
        if (strcmp(res->target[i], res->target[index]) == 0)
            return 1;
    }
    return 0;
}

void execution(struct ms *res)
{
    replace_var(res);
    replace_var_commands(res);
    replace_var_rule(res);

    if (res->tsize == 0)
    {
        if (res->rsize > 0)
            exec_no_target(res, 0);
        else
            errx(2, "minimake: *** No targets. Stop.");
    }
    else
    {
        for (size_t i = 0; i < res->tsize; i++)
        {
            if (was_done(res, i))
            {
                printf("minimake: '%s' is up to date.\n", res->target[i]);
                continue;
            }
            int idx = find_index(res, res->target[i]);
            if (idx != -1)
                exec_no_target(res, idx);
            else
                errx(2, "minimake: *** No rule to make target '%s'. Stop.", res->target[i]);
        }
    }
}

int print_help(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            printf("%s:\n", argv[0]);
            printf("  -h: display this help message\n");
            printf("  -f <file>: define the name of the Makefile to use\n");
            printf("  -p: pretty print the Makefile organized\n");
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

static char *file_and_target(int argc, char *argv[], struct ms *r)
{
    char *res = NULL;
    r->tsize = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            if (i + 1 >= argc)
                errx(2, "No file specified after -f");
            res = strdup(argv[i + 1]);
            i++;
            continue;
        }
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "-h") == 0)
            continue;

        strcpy(r->target[r->tsize], argv[i]);
        r->tsize++;
    }
    return res;
}

int main(int argc, char *argv[])
{
    if (print_help(argc, argv))
        return 0;

    struct ms res = { 0 };
    char *path = file_and_target(argc, argv, &res);
    struct stat st;

    if (!path)
    {
        if (stat("makefile", &st) == 0 && S_ISREG(st.st_mode))
            path = strdup("makefile");
        else if (stat("Makefile", &st) == 0 && S_ISREG(st.st_mode))
            path = strdup("Makefile");
        else
        {
            fprintf(stderr, "minimake: *** No targets specified and no makefile found. Stop.\n");
            exit(2);
        }
    }

    if (stat(path, &st) != 0)
        errx(2, "This file does not exist: %s", path);
    if (S_ISDIR(st.st_mode))
        errx(2, "%s is a directory", path);

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