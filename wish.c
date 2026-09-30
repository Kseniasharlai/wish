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

/* ---------- шлях пошуку (поки що фіксований) ---------- */

static char *search_path[] = { "/bin" };
static size_t path_count = 1;

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

    if (!strcmp(argv[0], "exit")) {
        if (argc != 1) {
            print_error();
            return -1;
        }
        exit(0);
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

int main(void)
{
    char *line = NULL;
    size_t cap = 0;

    while (1) {
        printf("wish> ");
        fflush(stdout);
        if (getline(&line, &cap, stdin) == -1)
            break; /* EOF */
        process_line(line);
    }
    free(line);
    return 0;
}