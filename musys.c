// File: musys.c
// Author: Chad Hogg
// Definitions of functions that wrap system calls up with error checking.

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "musys.h"

void
printErrorMessageAndExit (const char* syscall)
{
    int e = errno;
    safePrintString (syscall);
    safePrintString (" error: ");
    safePrintString (strerror (e));
    safePrintString ("\n");
    exit (EXIT_FAILURE);

}


void
safePrintString (const char* s)
{
	Write (STDOUT_FILENO, s, strlen (s));
}


void
safePrintInt (int i)
{
	const unsigned int BUFSIZE = 32;
	char buffer[BUFSIZE];
	int index = BUFSIZE - 1;
	if (i == 0) {
		safePrintString ("0");
	}
	else {
		buffer[index] = '\0';
		while (i > 0 && index >= 0) {
			--index;
			int mod = i % 10;
			buffer[index] = (char)('0' + mod);
			i /= 10;
		}
		safePrintString (buffer + index);
	}
}


int
Close (int fildes)
{
    int result = close (fildes);
    if (result < 0) { printErrorMessageAndExit ("close"); }
    return result;
}


int
Dup2 (int fildes, int fildes2)
{
    int result = dup2 (fildes, fildes2);
    if (result < 0) { printErrorMessageAndExit ("dup2"); }
    return result;
}

void
Execvp (char* file, char* const argv[])
{
	execvp (file, argv);
    printf ("%s: Command not found\n", argv[0]);
    exit (EXIT_FAILURE);
}


pid_t
Fork ()
{
	pid_t result = fork ();
	if (result < 0) { printErrorMessageAndExit ("fork"); }
	return result;
}


int
Fstat (int fd, struct stat* statbuff)
{
    int result = fstat (fd, statbuff);
    if (result < 0) { printErrorMessageAndExit ("fstat"); }
    return result;
}


int
Ftruncate (int fd, off_t length)
{
    int result = ftruncate (fd, length);
    if (result < 0) { printErrorMessageAndExit ("ftruncate"); }
    return result;
}


pid_t
Getpid (void)
{
    int result = getpid ();
    if (result < 0) { printErrorMessageAndExit ("getpid"); }
    return result;
}


pid_t
Getppid (void)
{
    int result = getppid ();
    if (result < 0) { printErrorMessageAndExit ("getppid"); }
    return result;
}


int
Kill (pid_t pid, int sig)
{
    int result = kill (pid, sig);
    if (result < 0) { printErrorMessageAndExit ("kill"); }
    return result;
}


off_t
Lseek (int fd, off_t offset, int whence)
{
    off_t result = lseek (fd, offset, whence);
    if (result == (off_t)-1) { printErrorMessageAndExit ("lseek"); }
    return result;
}


void*
Mmap (void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    void* result = mmap (addr, length, prot, flags, fd, offset);
    if (result == MAP_FAILED) { printErrorMessageAndExit ("mmap"); }
    return result;
}


int
Munmap (void* addr, size_t length)
{
    int result = munmap (addr, length);
    if (result < 0) { printErrorMessageAndExit ("munmap"); }
    return result;
}


int
Open (const char* path, int oflag, mode_t mode)
{
    int result = open (path, oflag, mode);
    if (result < 0) { printErrorMessageAndExit ("open"); }
    return result;
}


int
Pipe (int fildes[2])
{
    int result = pipe (fildes);
    if (result < 0) { printErrorMessageAndExit ("pipe"); }
    return result;
}


ssize_t
Read (int fd, void* buf, size_t count)
{
    ssize_t result = read (fd, buf, count);
    if (result == -1) { printErrorMessageAndExit ("read"); }
    return result;
}


int
Setpgid (pid_t pid, pid_t pgid)
{
    int result = setpgid (pid, pgid);
    if (result < 0) { printErrorMessageAndExit ("setpgid"); }
    return result;
}


int
Setpgrp (void)
{
    int result = setpgrp ();
    if (result < 0) { printErrorMessageAndExit ("setpgrp"); }
    return result;
}


int
Sigaddset (sigset_t* set, int signum)
{
    int result = sigaddset (set, signum);
    if (result < 0) { printErrorMessageAndExit ("sigaddset"); }
    return result;
}


int
Sigdelset (sigset_t* set, int signum)
{
    int result = sigdelset (set, signum);
    if (result < 0) { printErrorMessageAndExit ("sigdelset"); }
    return result;
}


int
Sigemptyset (sigset_t* set)
{
    int result = sigemptyset (set);
    if (result < 0) { printErrorMessageAndExit ("sigemptyset"); }
    return result;
}


int
Sigfillset (sigset_t* set)
{
    int result = sigfillset (set);
    if (result < 0) { printErrorMessageAndExit ("sigfillset"); }
    return result;
}


mu_sighandler_t
Signal (int signum, mu_sighandler_t handler)
{
	struct sigaction action, old_action;

    action.sa_handler = handler;
    Sigemptyset (&action.sa_mask); /* block sigs of type being handled*/
    action.sa_flags = SA_RESTART;  /* restart syscalls if possible*/

    if (sigaction (signum, &action, &old_action) < 0) { printErrorMessageAndExit ("sigaction"); }
    return (old_action.sa_handler);
}


int
Sigprocmask (int how, const sigset_t* set, sigset_t* oldset)
{
    int result = sigprocmask (how, set, oldset);
    if (result < 0) { printErrorMessageAndExit ("sigprocmask"); }
    return result;
}


int
Sigsuspend (const sigset_t* mask)
{
    int result = sigsuspend (mask);
    if (errno != EINTR) { printErrorMessageAndExit ("sigsuspend"); }
    return result;
}


pid_t
Waitpid (pid_t pid, int* wstatus, int options)
{
	pid_t result = waitpid (pid, wstatus, options);
	if (result == -1) { printErrorMessageAndExit ("waitpid"); }
    return result;
}

ssize_t
Write (int fd, const void* buf, size_t count)
{
	ssize_t result = write (fd, buf, count);
	if (result < 0) {
        // Probably can't print anything if this failed ...
		exit (EXIT_FAILURE);
	}
    return result;
}


