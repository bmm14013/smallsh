
How to compile and run smallsh:

In the command line run:

gcc -std=c99 -o smallsh smallsh.c

And then to run, enter:

./smallsh


Description: smallsh is a basic shell that contains a subset of features of some well-known shells,
 	     such as bash. It supports the built-in commands, exit, cd, and status. It also supports
	     non built-in commands by searching the PATH environment variable. Finally, the shell
	     supports input and output redirection, as well as executing commands in the foreground
	     and background. To set foreground commands only press ^Z. Basic command structure is:
	     
	     command [arg1 arg2 ...] [< input_file] [> output_file] [&]
	     	
	     ...where items in square brackets are optional. If & is included, background option is
	     set to true 

Written by: Brendan MacIntyre
