#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include "shell.h"
int main(int argc, char **argv)
{
    // RUN COMMAND LOOP
    lsh_loop();
    return EXIT_SUCCESS;
}

void lsh_loop(void)
{
    char *line;
    char **args;
    int status;
    char cwd[1024];
    do
    {
        getcwd(cwd, sizeof(cwd));
        printf("\033[1;31m");
        printf("lsh:");
        printf("\033[0m");
        printf("\033[1;34m");
        printf("%s",cwd);
        printf("\033[0m");
        printf("$ ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        if (args)
        {
            status = lsh_execute(args);
        }
        free(line);
        free(args);
    } while (status);
}

// #define LSH_RL_BUFFER_SIZE 1024
// char *lsh_read_line(void)
// {
//     int buffer_size = LSH_RL_BUFFER_SIZE;
//     int position = 0;
//     char *buffer = malloc(sizeof(char) * buffer_size);
//     if (!buffer)
//     {
//         fprintf(stderr, "lsh: allocation error\n");
//         exit(EXIT_FAILURE);
//     }
//     int c;
//     while (1)
//     {
//         c = getchar();
//         if (c == EOF || c == '\n')
//         {
//             buffer[position] = '\0';
//             return buffer;
//         }
//         else
//         {
//             buffer[position] = c;
//         }
//         position++;

//         if (position >= buffer_size)
//         {
//             buffer_size += LSH_RL_BUFFER_SIZE;
//             buffer = realloc(buffer, sizeof(char) * buffer_size);
//             if (!buffer)
//             {
//                 fprintf(stderr, "lsh: allocation error\n");
//                 exit(EXIT_FAILURE);
//             }
//         }
//     }
// }

char *lsh_read_line(void)
{
    char *line = NULL;
    size_t bufsiz = 0;
    if (getline(&line, &bufsiz, stdin) == -1)
    {
        if (feof(stdin))
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("lsh: readline");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

// #define LSH_TOK_BUF_SIZE 64
// #define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUF_SIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    char *token;
    char *saveptr;
    token = strtok_r(line, LSH_TOK_DELIM, &saveptr);
    while (token)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUF_SIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok_r(NULL, LSH_TOK_DELIM, &saveptr);
    }
    tokens[position] = NULL;
    return tokens;
}

int lsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
        {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("lsh");
    }
    else
    {

        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}


/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "q"};

int (*builtin_func[])(char **) = {

    &lsh_cd,
    &lsh_help,
    &lsh_exit,
    &lsh_exit};

int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("lsh: cd failed");
        }
    }
    return 1;
}

int lsh_help(char **args)
{
    int i;
    printf("LSH Builtin Commands:\n");
    for (i = 0; i < lsh_num_builtins(); i++)
    {
        printf("%s\n", builtin_str[i]);
    }
    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args)
{
    return 0;
}

int lsh_execute(char **args)
{
    if (args[0] == NULL)
    {
        return 1;
    }
    int i;
    for (i = 0; i < lsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
}