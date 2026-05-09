// File: musys.h
// Author: Chad Hogg
// Prototypes of functions that wrap system calls up with error checking.
// This should, in theory, include every system call that you need to complete
//   CSCI380 labs at Millersville University.
// If you wish to use other system calls you may do so, but you will have to
//   do the error checking yourself.

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/syscall.h>

//// Basic functions for printing that are async-signal-safe. ////

void
safePrintString (const char* s);

void
safePrintInt (int i);

//// Wrappers around system calls that print an error message and exit on failure. ////

int
Close (int fildes);

int
Dup2 (int fildes, int fildes2);

void
Execvp (char* file, char* const argv[]);

pid_t
Fork ();

int
Fstat (int fd, struct stat* statbuff);

int
Ftruncate (int fd, off_t length);

pid_t
Getpid (void);

pid_t
Getppid (void);

int
Kill (pid_t pid, int sig);

off_t
Lseek (int fd, off_t offset, int whence);

void*
Mmap (void* addr, size_t length, int prot, int flags, int fd, off_t offset);

int
Munmap (void* addr, size_t length);

int
Open (const char* path, int oflag, mode_t mode);

int
Pipe (int fildes[2]);

ssize_t
Read (int fd, void* buf, size_t count);

int
Setpgid (pid_t pid, pid_t pgid);

int
Setpgrp (void);

int
Sigaddset (sigset_t* set, int signum);

int
Sigdelset (sigset_t* set, int signum);

int
Sigemptyset (sigset_t* set);

int
Sigfillset (sigset_t* set);

// We do this because the signal() system call isn't fully portable.
typedef void (*mu_sighandler_t) (int);
mu_sighandler_t
Signal (int signum, mu_sighandler_t handler);

int
Sigprocmask (int how, const sigset_t* set, sigset_t* oldset);

int
Sigsuspend (const sigset_t* mask);

pid_t
Waitpid (pid_t pid, int* wstatus, int options);

ssize_t
Write (int fd, const void* buf, size_t count);
