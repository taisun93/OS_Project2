#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 512

int main(int argc, char *argv[]) {
    char *input = NULL;
    size_t input_len = 0;

    if (argc < 2) {
        // No filename specified, read input from stdin
        printf("wish> ");
        fflush(stdout);
        getline(&input, &input_len, stdin);
    } else {
        // Filename specified, read input from file
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        getline(&input, &input_len, file);
        fclose(file);
    }

    input[strcspn(input, "\n")] = '\0'; // Remove trailing newline

    char *args[MAX_INPUT/2 + 1];
    int num_args = 0;

    char *token = strtok(input, " ");
    while (token != NULL && num_args < MAX_INPUT/2 + 1) {
        args[num_args++] = token;
        token = strtok(NULL, " ");
    }
    args[num_args] = NULL; // Set last argument to NULL

    if (strcmp(args[0], "exit") == 0) {
        // Handle exit command
        exit(0);
    }

    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "An error has occurred");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL);
    }

    return 0;
}
