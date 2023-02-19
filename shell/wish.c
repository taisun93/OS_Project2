#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 512

int cd(char *args[]) {
    if (args[1] == NULL) {
        // No arguments specified, print error message
        fprintf(stderr, "cd: missing argument\n");
        return 1;
    } else if (args[2] != NULL) {
        // Too many arguments specified, print error message
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    } else {
        // Concatenate arguments into a single path string
        char *path = malloc(strlen(args[1]) + strlen("/bin/") + 1);
        if (path == NULL) {
            fprintf(stderr, "cd: memory allocation error\n");
            return 1;
        }
        strcpy(path, "/bin/");
        strcat(path, args[1]);

        // Change working directory
        if (chdir(path) != 0) {
            fprintf(stderr, "cd: %s: No such file or directory\n", path);
            free(path);
            return 1;
        }

        free(path);
        return 0;
    }
}


int main(int argc, char *argv[])
{
    char *input = NULL;
    size_t input_len = 0;

    while (1) {
        printf("wish> ");
        fflush(stdout);
        getline(&input, &input_len, stdin);

        input[strcspn(input, "\n")] = '\0'; // Remove trailing newline

        char *args[MAX_INPUT / 2 + 1];
        int num_args = 0;

        char *token = strtok(input, " ");
        while (token != NULL && num_args < MAX_INPUT / 2 + 1)
        {
            args[num_args++] = token;
            token = strtok(NULL, " ");
        }
        args[num_args] = NULL; // Set last argument to NULL

        if (args[0] == NULL) {
            // Empty input line, prompt again
            continue;
        }

        if (strcmp(args[0], "exit") == 0)
        {
            exit(0);
        }

        if (strcmp(args[0], "cd") == 0)
        {
            cd(args);
            continue;
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            char path[512];
            snprintf(path, sizeof(path), "/bin/%s", args[0]);
            if (access(path, X_OK) == 0)
            {
                if (execv(path, args) == -1)
                {
                    fprintf(stderr, "An error has occurred\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                fprintf(stderr, "An error has occurred\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid < 0)
        {
            // Fork failed
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent process
            wait(NULL);
        }
    }

    return 0;
}
