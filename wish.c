#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int main(void)
{
    char *line = NULL;
    size_t cap = 0;

    while (1) {
        printf("wish> ");
        fflush(stdout);
        if (getline(&line, &cap, stdin) == -1)
            break; /* EOF */

        char *argv[MAX_ARGS];
        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc <= 0)
            continue;
        if (!strcmp(argv[0], "exit")) {
            if (argc != 1) {
                print_error();
                continue;
            }
            exit(0);
        }
        /* тимчасово: просто показуємо, що розібрали */
        printf("команда: %s, аргументів: %d\n", argv[0], argc - 1);
    }
    free(line);
    return 0;
}