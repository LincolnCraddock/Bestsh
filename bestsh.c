// mushc.c
// Simple shell for Linux
// By Lincoln Craddock
/*
  Author     : Lincoln Craddock
  Description: A simple shell for Linux
  Compile    : gcc bestsh.c musys.c -D USE_READLINE -lreadline -lhistory -o bestsh
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "musys.h"

#define BLOCK_SIGNALS sigset_t mask, oldmask; \
                      Sigemptyset(&mask); \
                      Sigaddset(&mask, SIGCHLD); \
                      Sigaddset(&mask, SIGINT); \
                      Sigaddset(&mask, SIGTSTP); \
                      Sigprocmask(SIG_BLOCK, &mask, &oldmask);
#define UNBLOCK_SIGNALS Sigprocmask(SIG_SETMASK, &oldmask, NULL);

#define MAX_INPUT_LEN (1024)
#define MAX_NUM_TOKS (128)

#define GRAY_ESC "\033[38;5;244m"
#define DEFAULT_COLOR_ESC "\033[39m"

#define CLEAR_LINE_ESC "\033[2K"

#ifdef USE_READLINE
#define YELLOW_ESC "\033[33m"
#define BOLD_ESC "\033[1m"
#define DEFAULT_STYLE_ESC "\033[22m"
#endif

/* FUNCTION HEADERS */

void readInput(char *buf);
int tokenizeInput(char *buf, char *tok[]);
int groupTokens(char *tok[], int (*cmdidxs)[2], char seps[]);

void childReaper(int sig);
void interruptHandler(int sig);
void stopHandler(int sig);

#ifdef USE_READLINE
void redisplay(void);

bool commandExists(const char *cmd);
#endif

/* GLOBAL VARS */

const char *builtins[] = {"quit", "fg", "debug", NULL};

volatile pid_t g_foregroundPid = 0;
volatile int g_numChildren = 0;
volatile pid_t g_stoppedPid = 0;

char g_foregroundName[MAX_INPUT_LEN]; // if g_foregroundPid != 0, contains name of foreground task
char g_stoppedName[MAX_INPUT_LEN]; // if g_stoppedPid != 0, contains name of stopped task
bool g_awaitingInput = false;

bool g_debugEnabled = false;

int
main(void)
{
    char line[1024];

    #ifdef USE_READLINE
    rl_redisplay_function = redisplay;
    #endif


    Signal (SIGCHLD, childReaper);
    Signal (SIGINT, interruptHandler);
    Signal (SIGTSTP, stopHandler);

    while (true) {
        g_awaitingInput = true;
        readCommand(line);
        g_awaitingInput = false;

        if (!*line)
            continue;

        char *linecpy = strdup(line);
        char *tok[MAX_NUM_TOKS]; // TODO: could this cause buffer overflow?
        int len = tokenizeCommand(linecpy, tok);
        
        if (len == 0) // blank line
            continue;
        
        // TODO: detect extra arguments after quit, debug, fg

        if (strcmp(tok[0], "quit") == 0)
            break;
        
        #ifdef USE_READLINE
        add_history(line);
        #endif

        if (strcmp(tok[0], "debug") == 0)
        {
            g_debugEnabled = !g_debugEnabled;
            printf(GRAY_ESC "Debug mode %s" DEFAULT_COLOR_ESC "\n", g_debugEnabled ? "activated" : "deactivated");

            continue;
        }

        if (strcmp(tok[0], "fg") == 0)
        {
            BLOCK_SIGNALS
            if (g_stoppedPid)
            {   
                g_foregroundPid = g_stoppedPid;
                strcpy(g_foregroundName, g_stoppedName);
                g_stoppedPid = 0;

                kill(-g_foregroundPid, SIGCONT);

                if (g_debugEnabled)
                    printf(GRAY_ESC "Job (%d) %s resumed" DEFAULT_COLOR_ESC "\n", g_foregroundPid, g_foregroundName);

                while (g_foregroundPid)
                    Sigsuspend (&oldmask);  
            }
            UNBLOCK_SIGNALS

            continue;
        }

        int cmdidxs[(MAX_NUM_TOKS + 1) / 2][2];
        int numpipes, numseps;
        int numcmds = groupTokens(tok, cmdidxs, &numpipes, &numseps);

        int pipes[MAX_NUM_TOKS / 2][2];
        for (int i = 0; i < numpipes; ++i)
            Pipe(pipes[i]);

        if (strcmp(tok[len - 1], "&") == 0)
        {
            tok[len - 1] = NULL;

            BLOCK_SIGNALS
            pid_t pid = Fork();
            if (pid)
            {
                if (g_debugEnabled)
                    printf(GRAY_ESC "Background job (%d)" DEFAULT_COLOR_ESC "\n", pid);
                
                ++g_numChildren;
                UNBLOCK_SIGNALS
            }
            else
            {
                Execvp(tok[0], tok);
                UNBLOCK_SIGNALS
            }
        }
        else
        {
            BLOCK_SIGNALS
            for (int childnum = 0; childnum < numcmds; ++childnum)
            {
                pid_t pid = Fork();
                if (pid)
                {
                    if (g_debugEnabled)
                        printf(GRAY_ESC "Job (%d) %s" DEFAULT_COLOR_ESC "\n", pid, tok[0]);

                    setpgid(pid, pid);
                    ++g_numChildren;
                    g_foregroundPid = pid;
                    strcpy(g_foregroundName, tok[0]);

                    while (g_foregroundPid)
                        Sigsuspend (&oldmask);
                    UNBLOCK_SIGNALS
                }
                else
                {
                    setpgid(0, 0);
                    UNBLOCK_SIGNALS

                    

                    Execvp(tok[0], tok);
                }
            }
        }
        free(linecpy);
    }

    return 0;
}

/* HELPERS */

void
readInput (char *buf)
{
    #ifdef USE_READLINE
    rl_num_chars_to_read = MAX_INPUT_LEN; // TODO: does this need done every call?
    char *line = readline("bestsh> ");
    if (!line)
        exit(EXIT_SUCCESS); // eof
    strncpy(buf, line, MAX_INPUT_LEN - 1);
    buf[MAX_INPUT_LEN - 1] = '\0';
    free(line);
    #else
    printf("bestsh> ");
    fflush(stdout);
    if (fgets(buf, MAX_INPUT_LEN, stdin) == NULL)
        exit(EXIT_SUCCESS); // eof // TODO: should return smth instead
    #endif
}

int
tokenizeInput(char *buf, char *tok[])
{
    int i = 0;
    do {
        tok[i] = strtok(i == 0 ? buf : NULL, " \t\n");
    } while(tok[i++]);
    return i - 1;
}

int
groupTokens(char *tok[], int (*cmdidxs)[2], char seps[])
{
    int idx1 = 0;
    int idx2 = 0;
    int i = 0;
    for (int j = 0; j < MAX_NUM_TOKS; ++j)
    {
        if (strcmp(tok[j], "|") == 0 || strcmp(tok[j], ";"))
        {
            // TODO: detect two separators in a row            
            idx2 = j;
            cmdidxs[i][0] = idx1;
            cmdidxs[i][1] = idx2;
            idx1 = j + 1;

            seps[i++] = tok[j][0];
        }
    }
    if () // TODO
    return i;
}

/* SIGNAL HANDLERS */

void
childReaper (int sig)
{
    BLOCK_SIGNALS
    int olderrno = errno;

    pid_t pid;
    int status;
    while (g_numChildren > 0 && (pid = Waitpid (-1, &status, WNOHANG)) > 0)
    {
        --g_numChildren;

        if (g_debugEnabled)
        {
            if (g_awaitingInput)
                safePrintString(CLEAR_LINE_ESC "\r");

            if (pid == g_foregroundPid)
            {
                safePrintString(GRAY_ESC "Job (");
                safePrintInt(pid);
                safePrintString(") ");
                safePrintString(g_foregroundName);
                safePrintString(" ");
            }
            else
            {
                safePrintString(GRAY_ESC "Background job (");
                safePrintInt(pid);
                safePrintString(") ");
            }

            if (WIFSIGNALED(status))
            {
                safePrintString("terminated by signal ");
                safePrintInt(WTERMSIG(status));
            }
            else
            {
                safePrintString("finished");
            }
            safePrintString(DEFAULT_COLOR_ESC "\n");

            if (g_awaitingInput)
            {
                rl_on_new_line();
                rl_redisplay();
            }
        }

        if (pid == g_foregroundPid)
            g_foregroundPid = 0;
        else if (pid == g_stoppedPid)
            g_stoppedPid = 0;
    }

    errno = olderrno;
    UNBLOCK_SIGNALS
}

void
interruptHandler (int sig)
{
    BLOCK_SIGNALS
    int olderrno = errno;

    kill(-g_foregroundPid, SIGINT);

    safePrintString("\n"); // insert \n after the ^C in terminal

    errno = olderrno;
    UNBLOCK_SIGNALS
}

void
stopHandler (int sig)
{
    BLOCK_SIGNALS
    int olderrno = errno;

    kill(-g_foregroundPid, SIGTSTP);

    g_stoppedPid = g_foregroundPid;
    strcpy(g_stoppedName, g_foregroundName);
    g_foregroundPid = 0;

    safePrintString("\n"); // insert \n after the ^Z in terminal
    if (g_debugEnabled)
    {
        safePrintString(GRAY_ESC "Job (");
        safePrintInt(g_stoppedPid);
        safePrintString(") ");
        safePrintString(g_stoppedName);
        safePrintString(" ");
        safePrintString("stopped by signal ");
        safePrintInt(SIGTSTP);
        safePrintString(DEFAULT_COLOR_ESC "\n");
    }

    errno = olderrno;
    UNBLOCK_SIGNALS
}

#ifdef USE_READLINE
/* READLINE HANDLERS */

void
redisplay(void)
{
    char *bufcpy = strdup(rl_line_buffer);
    if (bufcpy)
    {
        char *saveptr;
        char *tok = strtok_r(bufcpy, " ", &saveptr);
        if (tok && strcmp(rl_line_buffer, tok) == 0)
        {
            if (commandExists(tok))
                rl_set_prompt("\001" BOLD_ESC "\002bestsh\001" DEFAULT_STYLE_ESC "\002> ");
            else
                rl_set_prompt("bestsh> ");
        }
    }
    free(bufcpy);

    rl_redisplay();
}

/* READLINE HELPERS */

bool
commandExists(const char *cmd)
{
    // check built-in commands
    for (int i = 0; builtins[i]; ++i)
        if (strcmp(cmd, builtins[i]) == 0)
            return true;
    
    // check external commands
    char *paths = getenv("PATH");
    if (!paths)
        return false;
    
    char *pathscpy = strdup(paths); // modifying return value of getenv() is undefined
    char *saveptr;
    char *tok = strtok_r(pathscpy, ":", &saveptr);
    while (tok)
    {
        char fullpath[PATH_MAX] = "";
        int ret = snprintf(fullpath, PATH_MAX, "%s/%s", tok, cmd);
        if (ret > 0 && ret < PATH_MAX)
        {
            struct stat sb;
            if (stat(fullpath, &sb) == 0 && S_ISREG(sb.st_mode) && access(fullpath, X_OK) == 0)
            {
                free(pathscpy);
                return true;
            }
        }
        tok = strtok_r(NULL, ":", &saveptr);
    }
    free(pathscpy);

    return false;
}
#endif