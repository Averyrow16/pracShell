#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_ARGS 64
#define HASHSIZE 101

int EXIT_STATUS = 0; // temporary holder of exit status
// Reminder: make sure exit_status affects all code including cd, export and set
// but ill work on it later this is just temp

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
    char **tokens = NULL;
    char **commands = NULL;
    char **pipe_commands = NULL;
    while (1)
    {

        free(tokens);
        free(commands);
        free(pipe_commands);
        tokens = NULL;
        commands = NULL;
        pipe_commands = NULL;
        int tok_size = 0, com_size = 0, pipe_com_size = 0;
        printf("mysh> ");
        // read a line of input using getline()
        chars = getline(&buffer, &bufsize, stdin); // dynamic allocation, must free later
        // checsk if getline returned -1 (e.g. Ctrl+D / EOF) and exit the loop if so
        if (chars == -1)
        {
            break;
        }
        // strips trailing newline character that getline includes
        buffer[chars - 1] = '\0';
        char *command = strtok(buffer, "&&");
        while (command != NULL)
        {
            com_size++;
            char **temp = realloc(commands, com_size * sizeof(char *));
            if (temp == NULL)
            {
                printf("Out of memory\n");
                free(commands);
                exit(EXIT_FAILURE);
            }
            commands = temp;
            commands[com_size - 1] = command;
            command = strtok(NULL, "&&"); // internal static pointer remembers where it was in the string
            // NULL tells strtok we arent using a new string
        }
        if (com_size != 0) // if there are actual commands
        {
            char **temp = realloc(commands, (com_size + 1) * sizeof(char *));
            commands = temp;
            commands[com_size] = NULL; // terminates with null
        }
        for (int com = 0; com < com_size; com++) // for each command
        {

            char *cur_buffer = commands[com];

            char *pipe_command = strtok(cur_buffer, "|"); // splits pipe commands
            while (pipe_command != NULL)
            {
                pipe_com_size++;
                char **temp = realloc(pipe_commands, pipe_com_size * sizeof(char *));
                if (temp == NULL)
                {
                    printf("Out of memory\n");
                    free(pipe_commands);
                    exit(EXIT_FAILURE);
                }
                pipe_commands = temp;
                pipe_commands[pipe_com_size - 1] = pipe_command;
                pipe_command = strtok(NULL, "|"); // internal static pointer remembers where it was in the string
                // NULL tells strtok we arent using a new string
            }
            if (pipe_com_size != 0) // if there are pipe commands (basically if the string wasnt null)
            {
                char **temp = realloc(pipe_commands, (pipe_com_size + 1) * sizeof(char *));
                pipe_commands = temp;
                pipe_commands[pipe_com_size] = NULL; // terminates with null
            }
            // k so now we have pipe_commands which is an array of all pipe commands
            // now we need a for loop that checks the current command and the next one
            int pipe_fail, dup_fail;
            int pipefd[2], prevpipe[2];
            for (int pipe_com = 0; pipe_com < pipe_com_size; pipe_com++)
            {
                // resetting tokens
                tok_size = 0;
                free(tokens);
                tokens = NULL;

                cur_buffer = pipe_commands[pipe_com];
                // tokenize the line into an argument array
                char *token = strtok(cur_buffer, " "); // returns pointer to start of first token
                if (token == NULL)
                {
                    free(tokens);
                }
                while (token != NULL)
                {
                    tok_size++;
                    char **temp = realloc(tokens, tok_size * sizeof(char *));
                    if (temp == NULL)
                    {
                        printf("Out of memory\n");
                        free(tokens);
                        exit(EXIT_FAILURE);
                    }
                    tokens = temp;
                    tokens[tok_size - 1] = token;
                    token = strtok(NULL, " "); // internal static pointer remembers where it was in the string
                    // NULL tells strtok we arent using a new string
                }
                if (tok_size != 0)
                {
                    char **temp = realloc(tokens, (tok_size + 1) * sizeof(char *));
                    tokens = temp;
                    tokens[tok_size] = NULL;
                }
                if (tokens != NULL)
                {
                    pid_t childpid, wait;
                    const char *parent_processes[] = {"cd", "pwd", "exit", "export", "set"};
                    const char *redirect_symbols[] = {"<", ">", ">>", "2>"};
                    char **redirections = NULL;
                    char **symbols = NULL;
                    int *indexes = NULL;
                    int redirect_count = 0;
                    bool parent = false;
                    for (int i = 0; i < 5; i++)
                    {
                        if (strcmp(tokens[0], parent_processes[i]) == 0)
                        {
                            parent = true;
                        }
                    }
                    for (int i = 0; i < 4; i++)
                    {
                        for (int j = 1; j < tok_size; j++)
                        {
                            // this whole if statement just adds to the redirect and symbol lists and reallocs them
                            if (strcmp(tokens[j], redirect_symbols[i]) == 0)
                            {
                                if (tokens[j + 1] != NULL)
                                {
                                    redirect_count++;
                                    char **temp_red = realloc(redirections, redirect_count * sizeof(char **));
                                    if (temp_red == NULL)
                                    {
                                        printf("Mem. realloc. failed");
                                        free(redirections);
                                    }
                                    redirections = temp_red;
                                    redirections[redirect_count - 1] = tokens[j + 1]; // adds the file that will redirect
                                    char **temp_sym = realloc(symbols, redirect_count * sizeof(char **));
                                    if (temp_sym == NULL)
                                    {
                                        printf("Mem. realloc. failed");
                                        free(symbols);
                                    }
                                    symbols = temp_sym;
                                    symbols[redirect_count - 1] = tokens[j];                              // adds the symbol                                                               // skips the filename that would come right after
                                    int *temp_idx = realloc(indexes, redirect_count * 2 * sizeof(int *)); // reallocs for indexes of symbols and redirections
                                    if (temp_idx == NULL)
                                    {
                                        printf("Mem. realloc. failed");
                                        free(indexes);
                                    }
                                    indexes = temp_idx;
                                    indexes[(redirect_count - 1) * 2] = j;
                                    indexes[(redirect_count - 1) * 2 + 1] = j + 1;
                                    j++; // skips the filename that comes after
                                }
                                else
                                { // if the command ended with <, >, >> or 2>
                                    printf("Error: no token after redirection symbol\n");
                                }
                            }
                        }
                    }
                    for (int i = 0; i < redirect_count * 2; i++)
                    {
                        tokens[indexes[i]] = NULL; // sets all symbols and filenames to null, leaving only the original command
                    }
                    char **new_tokens = malloc((tok_size - (redirect_count * 2) + 1) * sizeof(char **)); // gives enough space for commands without redirections
                    int p = 0;
                    for (int i = 0; i < tok_size; i++)
                    {
                        if (tokens[i] != NULL)
                        {
                            new_tokens[p] = tokens[i];
                            p++;
                        }
                    }
                    new_tokens[p] = NULL; // ends with NULL-terminator
                    // by now new_tokens contains the command without redirections
                    tokens = new_tokens; // makes it point to new_tokens so i dont have to change code that comes after
                    if (parent == false)
                    {
                        // we dont need to check for last command bc we don't pipe on last command
                        if (pipe_com_size > 1)
                        {                                            // if there acc is a pipe
                                                                     //  we dont need to check for last command bc we don't pipe on last command
                            if (pipe_commands[pipe_com + 1] != NULL) // if not on last command
                            {
                                if (pipe_com == 0) // first command so we dont change stdin
                                {
                                    pipe_fail = pipe(pipefd); // pipe pipe pipe
                                    // now pipefd contains start and end file descriptors of pipe
                                    if (pipe_fail == -1)
                                    {
                                        printf("Error: pipe failure");
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else // middle of the pipe
                                {
                                    pipe_fail = pipe(pipefd); // pipe pipe pipe
                                    // now pipefd contains start and end file descriptors of pipe
                                    if (pipe_fail == -1)
                                    {
                                        printf("Error: pipe failure");
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                        }
                        childpid = fork();
                        int status, failure;
                        if (childpid == -1)
                        {
                            perror("fork");
                            exit(EXIT_FAILURE);
                        }

                        else if (childpid == 0) // means we're in the child
                        {
                            if (pipe_com_size > 1)
                            { // if there acc is a pipe
                                // ok so pipe() returns 2 file descriptors that refer to ends of the pipe (pipefd[0] read and pipefd[1] write)
                                // so in theory if i get those 2 and then use dup2() to change the stdin and stdout to refer to these
                                // then the read end will read from stdin and the write end will write to stdout (if needed)

                                // dup2(oldfd, newfd) makes the old point to the new. new is destination, which is overwritten and old is the source which is where we copy from

                                if (pipe_commands[pipe_com + 1] == NULL) // we're on the last command of pipes array
                                {
                                    // then we dont change stdout
                                    // now pipefd contains start and end file descriptors of pipe
                                    dup_fail = dup2(prevpipe[0], 0); // means we get input from reading whats in the pipe
                                    if (dup_fail == -1)
                                    {
                                        printf("Error: dup failure1");
                                        exit(EXIT_FAILURE);
                                    }
                                    close(prevpipe[0]);
                                    close(prevpipe[1]);
                                    close(pipefd[0]);
                                    close(pipefd[1]);
                                }
                                else // we can assume this pipe is either the beginning or in the middle
                                {
                                    if (pipe_com == 0) // first command so we dont change stdin
                                    {
                                        dup_fail = dup2(pipefd[1], 1); // means we make the output write into the pipe
                                        if (dup_fail == -1)
                                        {
                                            printf("Error: dup failure2");
                                            exit(EXIT_FAILURE);
                                        }
                                        close(pipefd[0]);
                                        close(pipefd[1]);
                                    }
                                    else // middle of the pipe
                                    {
                                        dup_fail = dup2(prevpipe[0], 0); // means we get input from reading whats in the pipe
                                        if (dup_fail == -1)
                                        {
                                            printf("Error: dup failure3");
                                            exit(EXIT_FAILURE);
                                        }
                                        dup_fail = dup2(pipefd[1], 1); // means we get output to write into the pipe
                                        if (dup_fail == -1)
                                        {
                                            printf("Error: dup failure4");
                                            exit(EXIT_FAILURE);
                                        }
                                        close(prevpipe[0]);
                                        close(prevpipe[1]);
                                        close(pipefd[0]);
                                        close(pipefd[1]);
                                    } // beforehand i tried dup2(prevpipe[0]. pipefd[1]) which didn't work bc that just makes the pointers point to the same read end
                                    // by connecting them to stdin and stdout we make the data go through the reading and writing that is required once the command is ran
                                    // also i keep mixing up oldfd and newfd so heres how it works:
                                    // dup2(oldfd, newfd) means the newfd is a copy of the oldfd
                                    // so we put stdout and stdin as newfd so that the fds 1 and 0 become copies of the other fds, meaning anything meant to be read from them or written to them instead gets read or written from/to the new fds
                                }
                            }
                            // now run through all redirections
                            int file_descriptor;
                            if (redirect_count > 0)
                            {
                                for (int i = 0; i < redirect_count; i++)
                                {
                                    // redirections[i]
                                    if (strcmp(symbols[i], "<") == 0) // redirects stdin (0) to read from file instead of keyboard
                                    {
                                        file_descriptor = open(redirections[i], O_RDONLY);
                                        if (file_descriptor == -1)
                                        {
                                            perror("open");
                                            exit(EXIT_FAILURE);
                                        }
                                        dup_fail = dup2(file_descriptor, 0); // makes stdin point to the file we are reading from
                                    }
                                    else if (strcmp(symbols[i], ">") == 0) // redirects stdout (1) into file, overwriting it if it exists
                                    {
                                        file_descriptor = open(redirections[i], O_WRONLY | O_CREAT, S_IRWXU);
                                        if (file_descriptor == -1)
                                        {
                                            perror("open");
                                            exit(EXIT_FAILURE);
                                        }
                                        dup_fail = dup2(file_descriptor, 1); // makes stdout point to the file we are reading from
                                    }
                                    else if (strcmp(symbols[i], ">>") == 0) // redirects stdout (1) into file, appending to end if it exists
                                    {
                                        file_descriptor = open(redirections[i], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
                                        if (file_descriptor == -1)
                                        {
                                            perror("open");
                                            exit(EXIT_FAILURE);
                                        }
                                        dup_fail = dup2(file_descriptor, 1); // makes stdout point to the file we are reading from
                                    }
                                    else if (strcmp(symbols[i], "2>") == 0) // redirects stderr (2) into file
                                    {
                                        file_descriptor = open(redirections[i], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
                                        if (file_descriptor == -1)
                                        {
                                            perror("open");
                                            exit(EXIT_FAILURE);
                                        }
                                        dup_fail = dup2(file_descriptor, 2); // makes stdout point to the file we are reading from
                                    }
                                    if (dup_fail == -1)
                                    {
                                        perror("dup2");
                                        exit(EXIT_FAILURE);
                                    }
                                    failure = close(file_descriptor);
                                    if (failure == -1)
                                    {
                                        perror("close");
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                            failure = execvp(tokens[0], tokens); // tokens [0] is the command, tokens is the args
                            // so if i get an error like ls: cannot access 'wc': no such file or directory
                            // that means the tokens array looks like ["ls", "wc", "-l"] meaning it wasn't properly reset
                            if (failure == -1)
                            {
                                perror("execvp");
                                exit(EXIT_FAILURE);
                            }
                        }
                        else // means we're in the parent
                        {
                            if (pipe_com == pipe_com_size - 1)
                            {
                                close(pipefd[0]);
                                close(pipefd[1]);
                                close(prevpipe[0]);
                                close(prevpipe[1]);
                            }
                            wait = waitpid(childpid, &status, 0);
                            if (wait == -1)
                            {
                                perror("waitpid");
                            }
                            else
                            {
                                if (WIFEXITED(status))
                                {
                                    EXIT_STATUS = WEXITSTATUS(status);
                                }
                            }
                            if (pipe_com != 0 && pipe_com != pipe_com_size - 1)
                            {
                                close(prevpipe[0]);
                                close(prevpipe[1]);
                            }
                            prevpipe[0] = pipefd[0];
                            prevpipe[1] = pipefd[1];
                        }
                    }
                    else
                    {
                        if (strcmp(tokens[0], "cd") == 0)
                        {
                            if (tokens[1] == NULL) // change to go backwards in directory later
                            {
                                printf("Error: no path given\n");
                            }
                            else
                            {
                                int chdir_fail = chdir(tokens[1]); // returns whether or not change directory worked
                                if (chdir_fail == -1)
                                {
                                    perror("chdir");
                                }
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
                            if (tokens[1] == NULL)
                            {
                                printf("Error: no variable given\n");
                            }
                            else
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
                        }
                        if (strcmp(tokens[0], "set") == 0)
                        {
                            if (tokens[1] == NULL)
                            {
                                printf("Error: no variable given\n");
                            }
                            else
                            {
                                char *local_var_name = strtok(tokens[1], "="); // gets name of env variable to be added
                                char *local_var_value = strtok(NULL, "=");     // gets everything past the = aka the value
                                if (local_var_name == NULL || local_var_value == NULL)
                                {
                                    printf("set failure\n");
                                }
                                else
                                {
                                    install(local_var_name, local_var_value); // adds to hashtable
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    free(buffer);
    return 0;
}
