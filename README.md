# Bestsh
The best command-line shell.

## Run
Download the files and call ``make`` to generate an executable. If GNU Readline is available, the program will be compiled with extra features.

## Features
- Execute Linux commands like ``ls`` and ``grep``
- End commands with an ``&`` to run them as a background process
- Stop or terminate the current running process with ``^Z`` and ``^C``
- Type ``fg`` to bring a stopped process back to the foreground
- Type ``debug`` to enter Debugging Mode, where all processes print their PIDs when they start/finish

### Readline Only
- Enter tab twice to see the contents of your current directory
- Tab through previous commands with the up and down arrow keys
- The prompt becomes **bolded** when you're entering a valid command

## Author
Lincoln Craddock
