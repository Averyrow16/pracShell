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
        // TODO: read a line of input using getline()
        chars = getline(&buffer, &bufsize, stdin); // dynamic allocation, must free later
        // TODO: check if getline returned -1 (e.g. Ctrl+D / EOF) and exit the loop if so
        if (chars == -1)
            break;
        // TODO: strip the trailing newline character that getline includes
        buffer[chars - 1] = '\0';

        // TODO: tokenize the line into an argument array
        char *token = strtok(chars, " ");
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
            token = strtok(NULL, " ");
        }

        // TODO: skip forking if the line was empty (e.g. user just hit Enter)

        // TODO: fork()
        //   - check for fork failure (-1)
        //   - in the child (return value 0): call execvp() with your args array,
        //     then handle the case where execvp fails (it only returns on error)
        //   - in the parent: call waitpid() on the child's pid
    }

    free(buffer);
    return 0;
}
