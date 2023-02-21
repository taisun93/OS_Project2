#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define MAX_LINE 512
#define MAX_TOKENS 100
#define MAX_PATHS 128

// forward declarations.
void Write(int, const void *, size_t);

// global constants.
const char default_path[] = "/bin";

// global mutables.
char *path[MAX_PATHS];
int num_path;
char *line;
size_t len;

void complain()
{
    fprintf(stderr, "An error has occurred\n");
}

void init_path()
{
    path[0] = malloc(strlen(default_path) + 1);
    strcpy(path[0], default_path);
    num_path++;
}

void free_path()
{
    for (int i = 0; i < MAX_PATHS; i++)
    {
        if (path[i] != NULL)
        {
            free(path[i]);
            path[i] = NULL;
        }
    }
    num_path = 0;
}

int execute_group(char **group, int *pids, int *index)
{
    int redirect = -1;
    int num_redirect_ops = 0;
    int num_redirect = 0;

    int i = 0;
    for (; group[i] != NULL; i++)
    {
        if (num_redirect_ops > 0)
        {
            num_redirect++;
        }

        if (!strcmp(group[i], ">"))
        {
            redirect = i;
            num_redirect_ops++;
        }
    }

    if (num_redirect_ops > 1 || num_redirect > 1 ||
        (num_redirect_ops >= 1 && num_redirect == 0))
    {
        complain();
        return 0;
    }

    char *args[MAX_TOKENS];
    int num_args = 0;
    char *redir_file = NULL;

    int fd = -1;
    if (redirect == -1)
    {
        for (int j = 1; j < i; j++)
        {
            args[j] = group[j];
            num_args++;
        }
        args[i] = NULL;
    }
    else
    {
        for (int j = 1; j < redirect; j++)
        {
            args[j] = group[j];
            num_args++;
        }
        args[redirect] = NULL;
        redir_file = group[redirect + 1];

        if (access(redir_file, F_OK) != -1)
        {
            if (access(redir_file, W_OK) == -1)
            {
                complain();
                return 0;
            }

            struct stat s;
            if (stat(redir_file, &s) == -1)
            {
                complain();
                return 0;
            }
            if (S_ISDIR(s.st_mode))
            {
                complain();
                return 0;
            }

            fd = open(redir_file, O_TRUNC | O_WRONLY);
            if (fd == -1)
            {
                complain();
                return 0;
            }
        }
        else
        {
            fd = open(redir_file, O_CREAT | O_WRONLY);
            if (fd == -1)
            {
                complain();
                return 0;
            }
        }
    }

    char *cmd = group[0];

    if (!strcmp(cmd, "exit"))
    {
        if (num_args > 0)
        {
            complain();
        }
        return 1;
    }

    else if (!strcmp(cmd, "cd"))
    {
        if (num_args == 0 || num_args > 1)
        {
            complain();
            return 0;
        }
        if (chdir(args[1]) == -1)
        {
            complain();
            return 0;
        }
        return 0;
    }

    else if (!strcmp(cmd, "path"))
    {
        free_path();
        for (int i = 0; i < num_args; i++)
        {
            if (num_path > MAX_PATHS)
            {
                complain();
                return 0;
            }
            path[num_path] = malloc(strlen(args[i + 1]) + 1);
            memset(path[num_path], 0, strlen(args[i + 1]) + 1);
            strcpy(path[num_path], args[i + 1]);
            num_path++;
        }
        return 0;
    }

    int not_cwd = 0;
    if (access(cmd, X_OK) == -1)
    {
        not_cwd = 1;
        int good = 0;
        for (int i = 0; i < num_path; i++)
        {
            char *cmd0 = malloc(strlen(path[i]) + strlen(cmd) + 2);
            strcpy(cmd0, path[i]);
            cmd0[strlen(path[i])] = '/';
            strcpy(cmd0 + strlen(path[i]) + 1, cmd);
            if (access(cmd0, X_OK) != -1)
            {
                good = 1;
            }
            if (good)
            {
                cmd = cmd0;
                break;
            }
            else
            {
                free(cmd0);
            }
        }
        if (!good)
        {
            complain();
            return 0;
        }
    }

    args[0] = group[0];

    int pid;
    if ((pid = fork()) == 0)
    {
        if (redirect != -1)
        {
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                complain();
                _exit(1);
            }
        }

        if (execv(cmd, args) == -1)
        {
            complain();
            _exit(1);
        }
    }
    else
    {
        pids[*index] = pid;
        *index = *index + 1;
        if (not_cwd)
        {
            free(cmd);
        }
    }

    return 0;
}

int execute_line(char *line_)
{
    char lineline[MAX_LINE];
    memset(lineline, 0, MAX_LINE);

    char *p = line_;
    char *lp = lineline;
    while (*p != '\n')
    {
        if (*p != '\t')
        {
            *lp = *p;
            lp++;
        }
        p++;
        if (p - line_ >= MAX_LINE)
        {
            break;
        }
    }
    *lp = '\n';

    if (lineline[0] == '\n')
    {
        return 0;
    }

    char line1[MAX_LINE];
    memset(line1, 0, MAX_LINE);
    p = strchr(lineline, '>');
    if (p != NULL)
    {
        const int off = p - lineline;
        strncpy(line1, lineline, off);
        line1[off] = ' ';
        line1[off + 1] = '>';
        line1[off + 2] = ' ';
        strcpy(line1 + off + 3, p + 1);
    }
    else
    {
        strcpy(line1, lineline);
    }

    char l[MAX_LINE];
    memset(l, 0, MAX_LINE);
    strcpy(l, line1);
    char *ll = l;
    while (1)
    {
        p = strchr(ll, '&');
        if (p == NULL)
        {
            break;
        }
        int bad_spacing = 0;
        if (p > ll && *(p - 1) != ' ')
        {
            bad_spacing = 1;
        }
        if (p < ll + strlen(ll) - 2 && *(p + 1) != ' ')
        {
            bad_spacing = 1;
        }

        if (bad_spacing)
        {
            char line2[MAX_LINE];
            memset(line2, 0, MAX_LINE);
            const int off = p - ll;
            strncpy(line2, ll, off);
            line2[off] = ' ';
            line2[off + 1] = '&';
            line2[off + 2] = ' ';
            strcpy(line2 + off + 3, p + 1);
            strcpy(ll, line2);

            ll = ll + off + 2;
        }
        else
        {
            ll = p + 1;
        }

        if (p >= ll + strlen(ll))
        {
            break;
        }
    }
    char line[MAX_LINE];
    memset(line, 0, MAX_LINE);
    strcpy(line, l);

    // tokenization.
    char *tokens[MAX_TOKENS];
    char *token = NULL;
    token = strtok(line, " ");
    int i = 0;
    for (; token != NULL; i++)
    {
        char *p = strchr(token, '\n');
        if (p != NULL)
        {
            token[strlen(token) - 1] = '\0';
            if (token[0] == '\0')
            {
                break;
            }
        }

        tokens[i] = token;
        token = strtok(NULL, " ");
    }

    const int num_tokens = i;

    if (num_tokens == 0)
    {
        // empty command line.
        return 0;
    }

    int num_groups = 0;
    int begin = 0;
    int end = 0;
    while (end < num_tokens)
    {
        while (begin < num_tokens && !strcmp(tokens[begin], "&"))
        {
            begin++;
        }
        if (begin >= num_tokens)
        {
            break;
        }

        end = begin;
        while (end < num_tokens && strcmp(tokens[end], "&"))
        {
            end++;
        }
        num_groups++;

        begin = end;
    }

    if (num_groups == 0)
    {
        return 0;
    }

    char ***groups = malloc(num_groups * sizeof(char **));
    int gi = 0;
    begin = 0;
    end = 0;
    while (end < num_tokens)
    {
        while (begin < num_tokens && !strcmp(tokens[begin], "&"))
        {
            begin++;
        }
        if (begin >= num_tokens)
        {
            break;
        }
        // tokens[begin] = cmd.

        end = begin;
        while (end < num_tokens && strcmp(tokens[end], "&"))
        {
            end++;
        }

        const int sz = end - begin;
        groups[gi] = malloc((sz + 1) * sizeof(char *));
        for (int i = 0; i < sz; i++)
        {
            groups[gi][i] = tokens[begin + i];
        }
        groups[gi][sz] = NULL;
        gi++;

        begin = end;
    }

    // make parallel
    int *pids = malloc(num_groups * sizeof(int));
    memset(pids, 0, num_groups);
    int num_forks = 0;

    for (int i = num_groups - 1; i >= 0; i--)
    {
        execute_group(groups[i], pids, &num_forks)
        // if (execute_group(groups[i], pids, &num_forks))
        // {
        //     for (int j = 0; j < num_groups; j++)
        //     {
        //         free(groups[j]);
        //     }
        //     free(groups);
        //     free(pids);
        //     return 1;
        // }
    }

    for (int i = num_groups - 1; i >= 0; i--)
    {
        if (waitpid(pids[i], NULL, 0) == -1)
        {
            complain();
            for (int j = 0; j < num_groups; j++)
            {
                free(groups[j]);
            }
            free(groups);
            free(pids);
            free_path();
            exit(1);
        }
    }
    for (int j = 0; j < num_groups; j++)
    {
        free(groups[j]);
    }
    free(groups);
    free(pids);

    return 0;
}

void Write(int fd, const void *buf, size_t n)
{
    if (write(fd, buf, n) == -1)
    {
        complain();
    }
}

void interactive()
{
    for (;;)
    {
        fprintf(stdout, "wish> ");

        // read user input.
        if (getline(&line, &len, stdin) == -1)
        {
            // error or eof.
            break;
        }

        if (execute_line(line))
        {
            break;
        }
    }
}

void batch(const char *batch_file)
{
    FILE *f = fopen(batch_file, "r");
    if (f == NULL)
    {
        complain();
        if (line != NULL)
        {
            free(line);
        }
        free_path();
        exit(1);
    }
    int closed = 0;

    while (getline(&line, &len, f) != -1)
    {
        if (execute_line(line))
        {

            if (fclose(f) == -1)
            {
                complain();
            }
            closed = 1;
            break;
        }
    }

    if (!closed)
    {
        if (fclose(f) == -1)
        {
            complain(f);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        complain();
        if (line != NULL)
        {
            free(line);
        }
        free_path();
        exit(1);
    }

    init_path();

    if (argc == 1)
    {
        interactive();
    }
    else
    {
        batch(argv[1]);
    }

    if (line != NULL)
    {
        free(line);
    }
    free_path();
    exit(0);
}