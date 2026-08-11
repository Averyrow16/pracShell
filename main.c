#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define MAX_ARGS 64
#define HASHSIZE 101

/*
code for a dictionary/hashtable data structure taken from The C Programming Language textbook
*/

/* block in the linked list */
struct nlist
{                       /* table entry: */
    struct nlist *next; /* next entry in chain */
    char *name;         /* defined name */
    char *defn;         /* replacement text */
};

static struct nlist *hashtab[HASHSIZE]; /* pointer table */

/* hash: form hash value for string s */
unsigned hash(char *s)
{
    unsigned hashval;                  // ensures hash value is non negative
    for (hashval = 0; *s != '\0'; s++) // adds each char value to scrambled combinations of previous ones
        hashval = *s + 31 * hashval;
    return hashval % HASHSIZE; // tries to even out how they are assigned to optimize lookup
}

/* lookup: look for s in hashtab */
struct nlist *lookup(char *s)
{
    struct nlist *np;
    for (np = hashtab[hash(s)]; np != NULL; np = np->next) // standard for walking along a linked list
        if (strcmp(s, np->name) == 0)                      // if any of them match the name we're looking for
            return np;                                     // found
    return NULL;                                           // not found
}

/* install: put (name, defn) in hashtab */
struct nlist *install(char *name, char *defn)
{
    struct nlist *np;
    unsigned hashval;
    if ((np = lookup(name)) == NULL) // not found (we also assign np in this line if found)
    {
        np = (struct nlist *)malloc(sizeof(*np));            // creates a new struct nlist the size of one struct nlist
        if (np == NULL || (np->name = strdup(name)) == NULL) // if mem allocation failed
            return NULL;

        /* inserts at front of linked list */
        hashval = hash(name);
        np->next = hashtab[hashval];
        hashtab[hashval] = np;
    }
    else                                   // already there
        free((void *)np->defn);            // free previous defn
    if ((np->defn = strdup(defn)) == NULL) // means theres no room for a new entry
        return NULL;
    // else the if statement assigns the value anyways so theres nothing else to do
    return np;
}

int main(int argc, char *argv[])
{

    char *buffer = NULL;
    size_t bufsize = 0;
    ssize_t chars;
    while (1)
    {
        int size = 0;
        char **tokens = NULL;
        printf("mysh> ");
        // read a line of input using getline()
        chars = getline(&buffer, &bufsize, stdin); // dynamic allocation, must free later
        // checsk if getline returned -1 (e.g. Ctrl+D / EOF) and exit the loop if so
        if (chars == -1)
            break;
        // strips trailing newline character that getline includes
        buffer[chars - 1] = '\0';

        // tokenize the line into an argument array
        char *token = strtok(buffer, " "); // returns pointer to start of first token
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
        if (size != 0)
        {
            char **temp = realloc(tokens, (size + 1) * sizeof(char *));
            tokens = temp;
            tokens[size] = NULL;
        }
        if (tokens != NULL)
        {
            pid_t childpid, wait;
            const char *parent_processes[] = {"cd", "pwd", "exit", "export", "set"};
            bool parent = false;
            for (int i = 0; i < 5; i++)
                if (strcmp(tokens[0], parent_processes[i]) == 0)
                    parent = true;
            if (parent == false)
            {

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
                    }
                }
            }
            else
            {
                if (strcmp(tokens[0], "cd") == 0)
                {
                    int chdir_fail = chdir(tokens[1]); // returns whether or not change directory worked
                    if (chdir_fail == -1)
                    {
                        perror("chdir");
                    }
                }
                if (strcmp(tokens[0], "pwd") == 0)
                {
                    char cwd[100];
                    char *cwd_ = getcwd(cwd, 100);
                    if (cwd_ == NULL)
                    {
                        perror("getcwd");
                    }
                    printf("%s\n", cwd);
                }
                if (strcmp(tokens[0], "export") == 0)
                {
                    char *env_var_name = strtok(tokens[1], "="); // gets name of env variable to be added
                    char *env_var_value = strtok(NULL, "=");     // gets everything past the = aka the value
                    // this overwrites tokens[1] but thats ok cuz tokens get reper after this anyways
                    int export_fail = setenv(env_var_name, env_var_value, 1);
                    if (export_fail == -1)
                    {
                        perror("export");
                    }
                }
                if (strcmp(tokens[0], "set") == 0)
                {
                    char *local_var_name = strtok(tokens[1], "="); // gets name of env variable to be added
                    char *local_var_value = strtok(NULL, "=");     // gets everything past the = aka the value
                    if (local_var_name == NULL || local_var_value == NULL)
                    {
                        printf("set failure\n");
                    }
                    else
                    {
                        install(local_var_name, local_var_value);
                    }
                }
            }
        }
    }

    free(buffer);
    return 0;
}
