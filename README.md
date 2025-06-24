# simple-shell
In collaboration with a classmate, this project contains the file of a simple shell that can run bash commands and more.
There are a few custom additions to this shell. This shell has a custom history command addition where if an index is given (so running 'history 0' will run the command at the 0th index in the history), it will run the command at that index in the history.
The shell also manually run some commands instead of using execvp such as changing directory. Piping commands is also supported.
