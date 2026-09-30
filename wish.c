#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARGS 256

static void print_error(void)
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

/* Розбиває рядок на токени за пробілами/табуляціями. Повертає кількість. */
static int tokenize(char *s, char **argv, int max)
{
    int argc = 0;
    char *tok;
    while ((tok = strsep(&s, " \t\r\n")) != NULL) {
        if (*tok == '\0')
            continue;
        if (argc >= max - 1)
            return -1;
        argv[argc++] = tok;
    }
    argv[argc] = NULL;
    return argc;
}

/* ---------- шлях пошуку ---------- */

static char **search_path = NULL;
static size_t path_count = 0;

static void free_path(void)
{
    for (size_t i = 0; i < path_count; i++)
        free(search_path[i]);
    free(search_path);
    search_path = NULL;
    path_count = 0;
}

static void set_path(char **dirs, size_t n)
{
    free_path();
    if (n == 0)
        return;
    search_path = malloc(n * sizeof(char *));
    if (!search_path) {
        print_error();
        return;
    }
    for (size_t i = 0; i < n; i++)
        search_path[i] = strdup(dirs[i]);
    path_count = n;
}

/* Повертає malloc'ований повний шлях до виконуваного файлу або NULL. */
static char *find_executable(const char *cmd)
{
    for (size_t i = 0; i < path_count; i++) {
        size_t len = strlen(search_path[i]) + strlen(cmd) + 2;
        char *full = malloc(len);
        if (!full)
            return NULL;
        snprintf(full, len, "%s/%s", search_path[i], cmd);
        if (access(full, X_OK) == 0)
            return full;
        free(full);
    }
    return NULL;
}

/* ---------- вбудовані команди ---------- */

static int is_builtin(const char *cmd)
{
    return !strcmp(cmd, "exit") || !strcmp(cmd, "cd") || !strcmp(cmd, "path");
}

static void run_builtin(char **argv, int argc)
{
    if (!strcmp(argv[0], "exit")) {
        if (argc != 1) {
            print_error();
            return;
        }
        free_path();
        exit(0);
    } else if (!strcmp(argv[0], "cd")) {
        if (argc != 2 || chdir(argv[1]) != 0)
            print_error();
    } else if (!strcmp(argv[0], "path")) {
        set_path(argv + 1, (size_t)(argc - 1));
    }
}

/* ---------- виконання однієї команди ---------- */

/* Для зовнішніх команд повертає pid дочірнього процесу, інакше -1. */
static pid_t run_segment(char *segment)
{
    char *argv[MAX_ARGS];
    int argc = tokenize(segment, argv, MAX_ARGS);

    if (argc < 0) {
        print_error();
        return -1;
    }
    if (argc == 0)
        return -1; /* порожній рядок */

    if (is_builtin(argv[0])) {
        run_builtin(argv, argc);
        return -1;
    }

    char *full = find_executable(argv[0]);
    if (!full) {
        print_error();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        free(full);
        return -1;
    }
    if (pid == 0) {
        execv(full, argv);
        print_error(); /* execv повернувся - помилка */
        _exit(1);
    }
    free(full);
    return pid;
}

/* Обробляє один рядок вводу і чекає на завершення команди. */
static void process_line(char *line)
{
    pid_t pid = run_segment(line);
    if (pid > 0)
        waitpid(pid, NULL, 0);
}

/* ---------- main ---------- */

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    int interactive = 1;

    if (argc > 2) {
        print_error();
        exit(1);
    }
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
        interactive = 0;
    }

    char *initial[] = { "/bin" };
    set_path(initial, 1);

    char *line = NULL;
    size_t cap = 0;

    while (1) {
        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }
        ssize_t len = getline(&line, &cap, input);
        if (len == -1)
            break; /* EOF */
        process_line(line);
    }

    free(line);
    free_path();
    if (input != stdin)
        fclose(input);
    exit(0);
}