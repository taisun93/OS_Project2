#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT 512
#define MAX_ARGS 900

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
        char *path_env = getenv("PATH");
        char new_path[strlen(path_env) + 1];
        strcpy(new_path, path_env);
        int i = 1;
        while (args[i] != NULL)
        {
            strcat(new_path, ":");
            strcat(new_path, args[i]);
            i++;
        }

        // Set new path
        if (setenv("PATH", new_path, 1) == -1)
        {
            perror("setenv");
            return 1;
        }
        else
        {
            // Echo the new path
            // fprintf(stdout, "New PATH=%s\n", new_path);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int interactive = (argc == 1) ? 1 : 0;
    FILE *input_file = NULL;
    setenv("PATH", "/bin", 1);

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
            // fprintf(stdout, "getting line\n");
            if (getline(&input, &input_len, input_file) == -1)
            {
                break;
            }
        }

        input[strcspn(input, "\n")] = '\0';
        // fprintf(stdout, "getting line%s\n", input);
        char *args[MAX_INPUT / 2 + 1];
        int num_args = 0;
        // fprintf(stdout, "-1\n");

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
            int fucking_bother = 1;
            int redirect = 0;
            // Child process
            char *new_args[MAX_ARGS];
            int i;
            for (i = 0; args[i] != NULL; i++)
            {
                fprintf(stderr, "blah blah %s", args[i]);
                if (strcmp(args[i], ">") != 0)
                {
                    if(redirect){
                        //too many redirects
                        fucking_bother = 0;
                        fprintf(stderr, "An error has occurred\n");
                        break;
                    }
                    redirect = 1;
                }
                new_args[i] = args[i];
            }
            new_args[i] = NULL;

            char *path_env = getenv("PATH");
            char *path = strdup(path_env);
            char *dir = strtok(path, ":");
            char full_path[900];
            while (dir != NULL)
            {
                sprintf(full_path, "%s/%s", dir, args[0]);
                if (access(full_path, X_OK) == 0)
                {
                    break;
                }
                dir = strtok(NULL, ":");
            }

            if (dir == NULL)
            {
                fprintf(stderr, "An error has occurred\n");
                continue;
            }

            // can't end on >
            if (strcmp(args[i], ">") != 0)
            {
                fprintf(stderr, "An error has occurred\n");
                fucking_bother = 0;
                break;
            }

            // Check for shell redirection
            int fd = -1;
            if (i >= 2 && args[i - 2] != NULL && strstr(args[i - 2], ">") != NULL)
            {

                int redirIndex = i - 2;
                while (redirIndex > 0 && strcmp(args[redirIndex], ">") != 0)
                {
                    redirIndex--;
                }

                if (strcmp(args[redirIndex], ">") != 0 || args[i - 1] == NULL || strcmp(args[i - 1], ">") != 0)
                {
                    fprintf(stderr, "An error has occurred\n");
                    fucking_bother = 0;
                    break;
                }

                char *filename = args[i - 1];
                fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (fd == -1)
                {
                    fprintf(stderr, "An error has occurred\n");
                    exit(EXIT_FAILURE);
                }

                // Redirect standard output and standard error to file
                if (dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1)
                {
                    fprintf(stderr, "An error has occurred\n");
                    exit(EXIT_FAILURE);
                }

                new_args[i - 2] = NULL;
                new_args[i - 1] = NULL;
                i -= 2;
            }

            fprintf(stderr, "do I bother? %d \n", fucking_bother);
            if (fucking_bother)
            {
                pid_t pid = fork();
                if (pid == 0)
                {
                    if (execv(full_path, new_args) == -1)
                    {
                        fprintf(stderr, "An error has occurred\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (pid < 0)
                {
                    fprintf(stderr, "An error has occurred\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    wait(NULL);
                }

                if (fd != -1)
                {
                    close(fd);
                }
            }
        }

        free(input);

        return 0;
    }
}