#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 512

int cd(char *args[])
{
    if (args[1] == NULL)
    {
        // No arguments specified, print error message
        fprintf(stderr, "An error has occurred\n");
        return 1;
    }
    else if (args[2] != NULL)
    {
        // Too many arguments specified, print error message
        fprintf(stderr, "An error has occurred\n");
        return 1;
    }
    else
    {
        // Change working directory
        if (chdir(args[1]) != 0)
        {
            fprintf(stderr, "cd: %s: No such file or directory\n", args[1]);
            return 1;
        }

        return 0;
    }
}

int path(char *args[])
{
    if (args[1] == NULL)
    {
        // No arguments specified, wipe path
        if (setenv("PATH", "", 1) == -1)
        {
            perror("setenv");
            return 1;
        }
    }
    else
    {
        // Build new path string
        char new_path[1024] = {0};
        int i = 1;
        while (args[i] != NULL)
        {
            strcat(new_path, args[i]);
            strcat(new_path, ":");
            i++;
        }
        // Remove trailing colon
        new_path[strlen(new_path) - 1] = '\0';

        // Set new path
        if (setenv("PATH", new_path, 1) == -1)
        {
            perror("setenv");
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int interactive = (argc == 1) ? 1 : 0;
    FILE *input_file = NULL;
    path("/bin")

        if (!interactive)
    {
        if (argc != 2)
        {
            fprintf(stderr, "batch file fuckery\n");
            exit(EXIT_FAILURE);
        }

        input_file = fopen(argv[1], "r");
        if (input_file == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    // char *path[100] = {"/bin", "/usr/bin", NULL};

    char *input = NULL;
    size_t input_len = 0;

    while (1)
    {
        // Gets next command
        if (interactive)
        {
            printf("wish> ");
            fflush(stdout);
            if (getline(&input, &input_len, stdin) == -1)
            {
                break;
            }
        }
        else
        {
            if (getline(&input, &input_len, input_file) == -1)
            {
                break;
            }
        }

        input[strcspn(input, "\n")] = '\0';

        char *args[MAX_INPUT / 2 + 1];
        int num_args = 0;

        char *token = strtok(input, " ");
        while (token != NULL && num_args < MAX_INPUT / 2 + 1)
        {
            args[num_args++] = token;
            token = strtok(NULL, " ");
        }
        args[num_args] = NULL; // Set last argument to NULL

        if (strcmp(args[0], "exit") == 0)
        {
            if (args[1] != NULL)
            {
                // if args are given for exit
                fprintf(stderr, "An error has occurred\n");
            }
            else
            {
                if (interactive)
                {
                    break;
                }
                exit(0);
            }
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            cd(args);
        }
        else if (strcmp(args[0], "path") == 0)
        {
            path(args);
        }

        // execute shit
        else
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                char *path_env = getenv("PATH");
                char *path = strdup(path_env); // make a copy of the PATH env string
                char *dir = strtok(path, ":"); // split the path string into directories using ":" as delimiter

                while (dir != NULL)
                {
                    char full_path[strlen(dir) + strlen(args[0]) + 2];
                    sprintf(full_path, "%s/%s", dir, args[0]);
                    if (access(full_path, X_OK) == 0)
                    { // check if the file exists and is executable
                        if (execv(full_path, new_args) == -1)
                        {
                            fprintf(stderr, "An error occurred while executing the command\n");
                            exit(EXIT_FAILURE);
                        }
                        break; // exit the loop once the command is found and executed
                    }
                    dir = strtok(NULL, ":"); // get the next directory in PATH
                }

                // if the loop completes without finding the command, print an error message
                fprintf(stderr, "Command not found: %s\n", args[0]);
                exit(EXIT_FAILURE);
            }
            else if (pid < 0)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else
            {
                wait(NULL);
            }
        }
    }

    free(input);

    return 0;
}
