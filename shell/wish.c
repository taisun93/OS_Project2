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
        // No arguments specified, print current path
        char *path = getenv("PATH");
        printf("The PATH variable is now %s\n", path);
    }
    else
    {
        // Combine arguments into colon-separated string
        char *new_path = args[1];
        for (int i = 2; args[i] != NULL; i++)
        {
            new_path = strcat(new_path, ":");
            new_path = strcat(new_path, args[i]);
        }

        // Set new path
        if (setenv("PATH", new_path, 1) != 0)
        {
            fprintf(stderr, "path: Failed to set PATH variable\n");
            return 1;
        }

        printf("The PATH variable is now %s\n", new_path);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int interactive = (argc == 1) ? 1 : 0;
    FILE *input_file = NULL;

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
        else
        {
            pid_t pid = fork();

            if (pid == 0)
            {
                char *file = find_executable(args[0], path);
                if (file != NULL)
                {
                    if (execv(file, args) == -1)
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
    }

    free(input);

    return 0;
}
