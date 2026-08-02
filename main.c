#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_ARGS 64

int main(int argc, char *argv[])
{
    char *buffer = NULL;
    size_t bufsize = 0;
    ssize_t chars;
    while (1)
    {
        printf("mysh> ");
        // read a line of input using getline()
        chars = getline(&buffer, &bufsize, stdin); // dynamic allocation, must free later
        // checsk if getline returned -1 (e.g. Ctrl+D / EOF) and exit the loop if so
        if (chars == -1)
            break;
        // strips trailing newline character that getline includes
        buffer[chars - 1] = '\0';

        // tokenize the line into an argument array
        char *token = strtok(chars, " "); // returns pointer to start of first token
        int size = 0;
        char **tokens = NULL;
        if (token == NULL)
        {
            free(tokens);
        }
        while (token != NULL)
        {
            size++;
            char **temp = realloc(tokens, size * sizeof(char *));
            if (temp == NULL)
            {
                printf("Out of memory\n");
                free(tokens);
                exit(EXIT_FAILURE);
            }
            tokens = temp;
            tokens[size - 1] = token;
            token = strtok(NULL, " "); // internal static pointer remembers where it was in the string
            // NULL tells strtok we arent using a new string
        }

        if (tokens != NULL)
        {
            pid_t childpid, wait;
            childpid = fork();
            int status, failure;
            if (childpid == -1)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (childpid == 0) // means we're in the child
            {
                failure = execvp(tokens[0], tokens);
                if (failure == -1)
                {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }
            else // means we're in the parent
            {

                wait = waitpid(childpid, &status, 0);
                if (wait == -1)
                {
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    free(buffer);
    return 0;
}
