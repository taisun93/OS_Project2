#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

int
main(int argc, char **argv)
{
  char **path = malloc(sizeof(char *)*2);
  path[0] = malloc(5);
  strcpy(path[0], "/bin");
  path[1] = NULL;

  char *ptr0 = NULL, *ptr1 = NULL, *ptr2 = NULL, *ptr3 = NULL;
  char delimit[]=" \t\r\n\v\f";
  char *redirect = ">";
  char *amp = "&\n";
  char slash[] = "/";

  if(argc > 1)
  {
    if(argc > 2)
    {
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
    FILE *fp = fopen(argv[1], "r");
    if(fp == NULL)
    {
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
    char *commandAll = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while((linelen = getline(&commandAll, &linecap, fp)) != -1)
    {
      if('\n' == commandAll[0]) continue;
      char *ampc = strtok_r(commandAll, amp, &ptr0);
      while(ampc != NULL)
      {
        char *command = malloc(1+strlen(ampc));
        strcpy(command, ampc);
        if(strstr(command, redirect) != NULL)
        {
          char *t = strtok_r(command, redirect, &ptr1);
          char *commandLeft = malloc(1+strlen(t));
          strcpy(commandLeft, t);
          char *copy = malloc(1+strlen(commandLeft));
          strcpy(copy, commandLeft);

          if(strstr(ptr1, redirect) != NULL)
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          t = strtok_r(NULL, strcat(delimit, redirect), &ptr1);
          if(t == NULL)
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }

          char *output = malloc(1+strlen(t));
          strcpy(output, t);

          if((t = strtok_r(NULL, strcat(delimit, redirect), &ptr1)) != NULL)
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }

          int num_args = 0;
          char *token = strtok_r(commandLeft, delimit, &ptr3);
          if(token == NULL)
          {
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          while((token = strtok_r(NULL, delimit, &ptr3)) != NULL)
              num_args++;

          int out = open(output, O_TRUNC|O_CREAT|O_WRONLY, 0600);
          if (-1 == out)
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }

          int save_out = dup(fileno(stdout));
          int save_err = dup(fileno(stderr));
          if (-1 == dup2(out, fileno(stdout)))
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          if (-1 == dup2(out, fileno(stderr)))
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;

          }

          fflush(stdout);
          fflush(stderr);
          close(out);

          token = strtok_r(copy, delimit, &ptr2);
          if (strcmp(token,"exit") == 0)
          {
            if( num_args != 0 )
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              ampc = strtok_r(NULL, amp, &ptr0);
              continue;
            }
            else
              exit(0);
          }
          else if (strcmp(token,"cd") == 0)
          {
            if(num_args != 1)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              ampc = strtok_r(NULL, amp, &ptr0);
              continue;
            }
            else
            {
              token = strtok_r(NULL, delimit, &ptr2);
              chdir(token);
            }
          }
          else if (strcmp(token,"path") == 0)
          {
            path = malloc(sizeof(char *) * (num_args + 1));
            int i = 0;
            while((token = strtok_r(NULL, delimit, &ptr2)) != NULL)
            {
              path[i] = malloc(1+ strlen(token));
              strcpy(path[i], token);
              i++;
            }
            path[i] = NULL;
          }
          else
          {
            int i = 0;
            int done = 0;
            while(path[i] != NULL)
            {
              done = 0;
              char *bin_path;
              bin_path = malloc(strlen(path[i])+2+strlen(token));
              strcpy(bin_path, path[i]);
              strcat(bin_path, slash);
              strcat(bin_path, token);
              if(access(bin_path, X_OK) == 0)
                done = 1;
              else
              {
                i++;
                continue;
              }

              int status;
              char *args[num_args+2];
              args[0] = malloc(1+strlen(token));
              strcpy(args[0], token);
              int j = 1;
              while((token = strtok_r(NULL, delimit, &ptr2)) != NULL)
              {
                args[j] = malloc(1+strlen(token));
                strcpy(args[j], token);
                j++;
              }
              args[j] = NULL;
              if(fork() == 0)
              {
                execv(bin_path, args);
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
              }
              else
                wait(&status);
              dup2(save_out, fileno(stdout));
              dup2(save_err, fileno(stderr));

              close(save_out);
              close(save_err);
              i++;
              if(done == 1) break;
            }
            if(done == 0)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
            }
          }
        }
        else
        {
          char *copy = malloc(1+strlen(command));
          strcpy(copy, command);
          int num_args = 0;
          char *token = strtok_r(command, delimit, &ptr2);
          if(token == NULL)
          {
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          while((token = strtok_r(NULL, delimit, &ptr2)) != NULL)
              num_args++;
          token = strtok_r(copy, delimit, &ptr3);
          if (strcmp(token,"exit") == 0)
          {
            if(num_args != 0)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
            }
            else
              exit(0);
          }
          else if (strcmp(token,"cd") == 0)
          {
            if(num_args != 1)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
            }
            token = strtok_r(NULL, delimit, &ptr3);
            if(token != NULL)
              chdir(token);
          }
          else if (strcmp(token,"path") == 0)
          {
            path = malloc(sizeof(char *) * (num_args + 1));
            int i = 0;
            while((token = strtok_r(NULL, delimit, &ptr3)) != NULL)
            {
              path[i] = malloc(1+ strlen(token));
              strcpy(path[i], token);
              i++;
            }
            path[i] = NULL;
          }
          else
          {
            int i = 0;
            int done = 0;
            while(path[i] != NULL)
            {
              done = 0;
              char* bin_path;
              bin_path = malloc(strlen(token)+2+strlen(path[i]));
              strcpy(bin_path, path[i]);
              strcat(bin_path, slash);
              strcat(bin_path, token);
              if(access(bin_path, X_OK) == 0)
                done = 1;
              else
              {
                i++;
                continue;
              }

              int status;
              char *args[num_args+2];
              args[0] = malloc(1+strlen(token));
              strcpy(args[0], token);
              int j = 1;
              while((token = strtok_r(NULL, delimit, &ptr3)) != NULL)
              {
                args[j] = malloc(1+strlen(token));
                strcpy(args[j], token);
                j++;
              }
              args[j] = NULL;
              if(fork() == 0)
              {
                execv(bin_path, args);
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
              }
              else
                wait(&status);
              i++;
              if(done == 1) break;
            }
            if(done == 0)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
            }
          }
        }
        ampc = strtok_r(NULL, amp, &ptr0);
      }
    }
  }
  else
  {
    while(1)
    {
      printf("%s", "wish> ");
      fflush(stdout);

      char *commandAll = NULL;
      size_t n = 0;
      size_t c = 0;
      c = getline(&commandAll,&n, stdin);
      if(c == -1) break;
      if(c == 1)
        continue;
      char *ampc = strtok_r(commandAll, amp, &ptr0);
      while(ampc != NULL)
      {
        char *command = malloc(1+strlen(ampc));
        strcpy(command, ampc);
        if(strstr(command, redirect) != NULL)
        {
          char *t = strtok_r(command, redirect, &ptr1);
          if(strstr(ptr1, redirect) != NULL)
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          char *commandLeft = malloc(1+strlen(t));
          strcpy(commandLeft, t);

          char *copy = malloc(1+strlen(commandLeft));
          strcpy(copy, command);

          int num_args = 0;
          char *token = strtok_r(copy, delimit, &ptr2);
          if(token == NULL)
          {
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          while((token = strtok_r(NULL, delimit, &ptr2)) != NULL)
              num_args++;

          t = strtok_r(NULL, strcat(delimit, redirect), &ptr1);
          if(t == NULL)
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          char *output = malloc(1+strlen(t));
          strcpy(output, t);

          if((t = strtok_r(NULL, strcat(delimit, redirect), &ptr1)) != NULL)
          {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }

          int out = open(output, O_TRUNC|O_CREAT|O_WRONLY, 0600);
          if (-1 == out) { //error 

          }

          int save_out = dup(fileno(stdout));
          int save_err = dup(fileno(stderr));
          if (-1 == dup2(out, fileno(stdout))) { //error

          }
          if (-1 == dup2(out, fileno(stderr))) { //error

          }

          fflush(stdout);
          fflush(stderr);
          close(out);

          token = strtok_r(commandLeft, delimit, &ptr1);
          
          if (strcmp(token,"exit") == 0)
          {
            if(num_args != 0)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              ampc = strtok_r(NULL, amp, &ptr0);
              continue;
            }
            else
              exit(0);
          }
          else if (strcmp(token,"cd") == 0)
          {
            if(num_args != 1)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              ampc = strtok_r(NULL, amp, &ptr0);
              continue;
            }
            else
            {
              token = strtok_r(NULL, delimit, &ptr1);
              if(token != NULL)
                chdir(token);
            }
          }
          else if (strcmp(token,"path") == 0)
          {
            path = malloc(sizeof(char *) * (num_args + 1));
            int i = 0;
            while((token = strtok_r(NULL, delimit, &ptr1)) != NULL)
            {
              path[i] = malloc(1+ strlen(token));
              strcpy(path[i], token);
              i++;
            }
            path[i] = NULL;
          }
          else
          {
            int i = 0;
            int done;
            while(path[i] != NULL)
            {
              done = 0;
              char* bin_path;
              bin_path = malloc(strlen(token)+2+strlen(path[i]));
              strcpy(bin_path, path[i]);
              strcat(bin_path, slash);
              strcat(bin_path, token);
              if(access(bin_path, X_OK) == 0)
                done = 1;
              else
              {
                i++;
                continue;
              }

              int status;
              char *args[num_args+2];
              args[0] = malloc(1+strlen(token));
              strcpy(args[0], token);
              int j = 1;
              while((token = strtok_r(NULL, delimit, &ptr1)) != NULL)
              {
                args[j] = malloc(1+strlen(token));
                strcpy(args[j], token);
                j++;
              }
              args[j] = NULL;
              if(fork() == 0)
              {
                execv(bin_path, args);
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
              }
              else
                wait(&status);
              dup2(save_out, fileno(stdout));
              dup2(save_err, fileno(stderr));

              close(save_out);
              close(save_err);
              i++;
              if(done == 1) break;
            }
            if(done == 0)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              ampc = strtok_r(NULL, amp, &ptr0);
              continue;
            }
          }
        }
        else
        {
          char *p1, *p2;
          char *copy = malloc(1+strlen(command));
          strcpy(copy, command);
          int num_args = 0;
          char *token = strtok_r(copy, delimit, &p2);
          if(token == NULL)
          {
            ampc = strtok_r(NULL, amp, &ptr0);
            continue;
          }
          while((token = strtok_r(NULL, delimit, &p2)) != NULL)
            num_args++;
          token = strtok_r(command, delimit, &p1);
          if(c == 1)
          {
          }
          else if (strcmp(token,"exit") == 0)
          {
            if(num_args != 0)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
            }
            else
              exit(0);
          }
          else if (strcmp(token,"cd") == 0)
          {
            if(num_args != 1)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              ampc = strtok_r(NULL, amp, &ptr0);
              continue;
            }
            else
            {
              token = strtok_r(NULL, delimit, &p1);
              if(token != NULL)
                chdir(token);
            }
          }
          else if (strcmp(token,"path") == 0)
          {
            path = malloc(sizeof(char *)*(num_args+1));
            int j = 0;
            while((token = strtok_r(NULL, delimit, &p1)) != NULL)
            {
              path[j] = malloc(1 + strlen(token));
              strcpy(path[j], token);
              j++;
            }
            path[j] = NULL;
          }
          else
          {
            int i = 0;
            int done = 0;
            while(path[i] != NULL)
            {
              done = 0;
              char* bin_path;
              bin_path = malloc(strlen(token)+2+strlen(path[i]));
              strcpy(bin_path, path[i]);
              strcat(bin_path, slash);
              strcat(bin_path, token);
              if(access(bin_path, X_OK) == 0)
                done = 1;
              else
              {
                i++;
                continue;
              }
              int status;
              char *args[num_args+2];
              args[0] = malloc(1+strlen(token));
              strcpy(args[0], token);
              int j = 1;
              while((token = strtok_r(NULL, delimit, &p1)) != NULL)
              {
                args[j] = malloc(1+strlen(token));
                strcpy(args[j], token);
                j++;
              }
              args[j] = NULL;
              if(fork() == 0)
              {
                execv(bin_path, args);
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
              }
              else
                wait(&status);
              i++;
              if(done == 1) break;
            }
            if(done == 0)
            {
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
            }
          }
        }
        ampc = strtok_r(NULL, amp, &ptr0);
      }
    }
  }
}