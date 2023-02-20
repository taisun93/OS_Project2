#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

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
            fprintf(stderr, "An error has occurred\n");
            exit(EXIT_FAILURE);
        }

        input_file = fopen(argv[1], "r");
        if (input_file == NULL)
        {
            fprintf(stderr, "An error has occurred\n");
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
            // fprintf(stderr, "reading in \n");
            if (getline(&input, &input_len, input_file) == -1)
            {
                break;
            }
        }

        // tokenization

        while (*input == '\t' || *input == ' ' || *input == '\n')
        {

            input++;
        }

        // fprintf(stderr, "getting line\n");
        char *args[MAX_INPUT];
        int num_args = 0;

        char *token = strtok(input, " ");

        if (token == NULL)
        {
            // continue;
            fprintf(stderr, "null token\n");
        }

        while (token != NULL && num_args < MAX_INPUT)
        {
            // fprintf(stderr, "token here %s \n", token);
            // Trim leading whitespace from token

            if (strcmp(token, ">") != 0 && strstr(token, ">") != NULL)
            {
                char *arg1, *arg2;
                arg1 = strsep(&token, ">");
                arg2 = strsep(&token, ">");
                args[num_args++] = arg1;
                args[num_args++] = ">";
                args[num_args++] = arg2;
            }
            {
                args[num_args++] = token;
            }
            // gets next token
            token = strtok(NULL, " ");
        }

        args[num_args] = NULL; // Set last argument to NULL
        
        if(args[0] == NULL){
            fprintf(stderr, "blah blah \n");
        }
        else if (strcmp(args[0], "exit") == 0)
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
            int redirect = 0;
            // Child process
            char *new_args[MAX_ARGS];
            int i, fd;

            for (i = 0; args[i] != NULL; i++)
            {
                // fprintf(stdout, "start start %s \n", args[i]);
                if (strcmp(args[i], ">") == 0)
                {
                    if (redirect)
                    {
                        redirect = 2;
                    }
                    else
                    {
                        redirect = 1;
                        if (args[i + 1] != NULL)
                        {
                            // char *p = args[i + 1];
                            // while (*p)
                            // {
                            //     if (*p == '/')
                            //     {
                            //         *p = '\0';
                            //         mkdir(args[i + 1], 0777);
                            //         *p = '/';
                            //     }
                            //     p++;
                            // }

                            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        }
                    }
                }
                new_args[i] = args[i];
                // fprintf(stdout, "blah %s \n", args[i]);
            }

            // can't chain redirects
            if (redirect > 1)
            {
                fprintf(stderr, "An error has occurred\n");
                continue;
            }

            new_args[i + 1] = NULL;

            char *path_env = getenv("PATH");
            char *path = strdup(path_env);
            char *dir = strtok(path, ":");
            char full_path[90];
            int saved_stdout = dup(1);

            while (dir != NULL)
            {
                sprintf(full_path, "%s/%s", dir, args[0]);
                if (access(full_path, X_OK) == 0)
                {
                    break;
                }
                dir = strtok(NULL, ":");
            }
            free(path);

            // can't find command
            if (dir == NULL)
            {
                fprintf(stderr, "An error has occurred\n");
                continue;
            }

            // can't end on >
            if (strcmp(args[i - 1], ">") == 0)
            {
                fprintf(stderr, "An error has occurred\n");
                continue;
            }

            // penultimate in a redirect needs to be a >
            if (redirect && strcmp(args[i - 2], ">") != 0)
            {
                fprintf(stderr, "An error has occurred\n");
                continue;
            }
            // redirect here
            if (redirect)
            {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                new_args[i - 2] = NULL;
                new_args[i - 1] = NULL;
            }

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

            if (redirect)
            {
                close(fd);
                // dup2(saved_stdout, 1);
                dup2(saved_stdout, STDOUT_FILENO);
                dup2(saved_stdout, STDERR_FILENO);
            }
        }
    }

    free(input);

    return 0;
}