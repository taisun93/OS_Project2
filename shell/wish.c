#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char *input = NULL;
    size_t input_len = 0;
    char *argv[100];
    while (1) {
        printf("shell> ");
        ssize_t input_read = getline(&input, &input_len, stdin);
        //parse the input command and split it into an array of arguments
        int argc = 0;
        char *token = strtok(input, " \n");
        while (token != NULL) {
            argv[argc++] = token;
            token = strtok(NULL, " \n");
        }
        argv[argc] = NULL;
        //check if the command is "exit"
        if (strcmp(argv[0], "exit") == 0) {
            //exit the shell program
            exit(0);
        }
        //check if the command is "cd"
        else if (strcmp(argv[0], "cd") == 0) {
            //check if a directory is specified
            if (argc < 2) {
                //print an error message and continue the loop
                printf("cd: missing directory\n");
                continue;
            }
            //try to change the directory using chdir
            if (chdir(argv[1]) != 0) {
                //print an error message if chdir fails and continue the loop
                printf("cd: %s: No such file or directory\n", argv[1]);
                continue;
            }
        }
        //create a child process to execute the command
        pid_t pid = fork();
        if (pid == 0) {
            //child process
            execvp(argv[0], argv);
            //if execvp fails, print an error message
            printf("%s: command not found\n", argv[0]);
            exit(1);
        } else {
            //parent process
            wait(NULL);
        }
    }
    free(input);
    return 0;
}
