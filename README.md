# smallsh

A bash-like shell application written in C.
- Provides a prompt for running commands
- Handles blank lines and comments, which are lines beginning 
  with the # character
- Provides expansion for the variable $$
- Executes 3 commands: exit, cd, and status via code built into 
  the shell
- Executes other commands by creating a new process and searching the PATH environment variable for the command
- Supports input and output redirection
- Supports running commands in foreground and background processes
- Implements custom handlers for 2 signals, SIGINT and SIGTSTP

Basic command structure is: 

	command [arg1 arg2 ...] [< input_file] [> output_file] [&]

Items in square brackets are optional. If the & is included at the end of the input, the command will run in the background. ^Z will enable foreground only mode. Run "exit" to exit shell. 


## Installation


How to compile and run smallsh:

Go to the directory containing smallsh.c and compile the program using the command

	gcc -std=c99 -o smallsh smallsh.c

And then to run the shell, enter:

	./smallsh

#	

Written by: Brendan MacIntyre