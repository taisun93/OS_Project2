#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    char input[1000];
    char *argv[100];
    while (1)
    {
        printf("wish> ");
        fgets(input, 1000, stdin);
        // parse the input command and split it into an array of arguments
        int argc = 0;
        char *token = strtok(input, " \n");
        while (token != NULL)
        {
            argv[argc++] = token;
            token = strtok(NULL, " \n");
        }
        argv[argc] = NULL;
        if (strcmp(argv[0], "exit") == 0)
        {
            // exit the shell program
            exit(0);
        }
        // create child
        pid_t pid = fork();
        if (pid == 0)
        {
            // child process
            execvp(argv[0], argv);
            // if execvp fails, print an error message
            printf("%s: command not found\n", argv[0]);
            exit(1);
        }
        else
        {
            // parent process
            wait(NULL);
        }
    }
    return 0;
}
