/* Name: smallsh
 *
 * Synopsis: smallsh
 * 			 In shell: command [arg1 arg2 ...] [< input_file] [> output_file] [&]
 *
 * Description: Basic shell that contains a subset of features of some well-known shells,
 * 				
 * 				such as bash. See README for a more thorough description. 
 *
 * Author: Brendan MacIntyre
 *
 */

#define _POSIX_SOURCE
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define MAXINPUT 2048
#define MAXARGS 512

int foreground_only = 0;


/**
 * Prompts user for input and then parses command and stores arguments in appropriate vars/buffers.
 *
 *  @param argc Integer buffer to store # of arguments inputted
 *  @param args Buffer to store command and arguments
 *  @param pid Integer containing pid of the smallsh process
 *  @param inFile Buffer to store filename of redirected input
 *  @param outFile Buffer to store filename of redirected output
 *  @param background Integer buffer to store background command option
 *
 *  @return 1 if error with input or blank line, 0 if input successfuly stored in buffers 
 */
int promptCommand(int *argc, char *args[], int pid, char inFile[], char outFile[], int *background)
{
	// Get user input
	char input[MAXINPUT];
	printf(": ");
	fflush(stdout);
	fgets(input, MAXINPUT, stdin);
	input[strlen(input)-1] = '\0';
		
	// Check if comment
	if (input[0] == '#' || input[0] == '\0' || input[0] == ' ') return 1;
	
	if (strlen(input) > MAXINPUT) {
		fprintf(stderr, "Too many characters inputted");
		return 1;	
	}

	*argc = 0;
	int i = 0;
	
	// Parse input
	char *token = strtok(input, " ");
	while(token != NULL) {
		if (*argc > 512) {
			fprintf(stderr, "Too many arguments inputted");
			return 1;
		}	
		
		// Redirects
		if (strcmp(token, "<") == 0) {
			token = strtok(NULL, " ");
			strcpy(inFile, token);
			
		}
		else if (strcmp(token, ">") == 0) {
			token = strtok(NULL, " ");
			strcpy(outFile, token);
		}
		else { // Command or args - check for $$ and store	
			char* pch = strstr(token, "$$");
			if (pch != NULL) {   // Need to perform variable expansion
				char tmpToken[MAXINPUT];
				strcpy(tmpToken, token);			
				while (pch != NULL) {
					tmpToken[strlen(tmpToken)-strlen(pch)] = '\0'; 
					pch += 2;
					char expandedToken[MAXINPUT];
					snprintf(expandedToken, MAXINPUT, "%s%d%s", tmpToken, pid, pch);
					strcpy(tmpToken, expandedToken);
					pch = strstr(tmpToken, "$$");
				}
				args[i] = malloc(strlen(tmpToken)+1);
				strcpy(args[i++],tmpToken);
				*argc += 1;
			} else {
				args[i] = malloc(strlen(token)+1);	
				strcpy(args[i++],token);
				*argc += 1;
			}
		}
		
		token = strtok(NULL, " ");
		
	}
	
	// Check if background option set
	if (strcmp(args[*argc-1], "&") == 0) {
		*background = 1;
		args[*argc-1] = NULL;
	}	
	
	return 0;
}


/**
 * Prints exit/termination status
 *
 * @param estatus Integer representing the exit code status of process
 */
void printStatus(int estatus) {
	if (WIFEXITED(estatus)) {
		printf("exit value %d\n", WEXITSTATUS(estatus));
	} else {
		printf("terminated by signal %d\n", WTERMSIG(estatus));
	}
} 

/**
 * Executes non-built in command
 *
 * @param args Array containing command and associated arguments to execute
 * @param inFile Filename to redirect input to
 * @param outFile Filename to redirect output to
 * @param estatus Integer buffer to store exit value code of process in
 * @param background Option to set command to run in background (0 if false)
 * @param saction The sigaction struct for SIGINT  
 */
void execCom(char *args[], char inFile[], char outFile[], int *estatus, int background, struct sigaction saction) {
	
	pid_t spawnPid = fork();
	
	if (background) { //Background option checked, check if file redirection needed
		if (strcmp(inFile, "")) inFile = "/dev/null";
		if (strcmp(outFile, "")) outFile ="/dev/null";
	}
	
	switch(spawnPid){
		case -1:	// Error forking process
			perror("fork()\n");
			exit(1);
			break;
		case 0:		// Child Process
			// Accept ^C
			saction.sa_handler = SIG_DFL;
			sigaction(SIGINT, &saction, NULL);
			
			// Set file redirects
			if (strcmp(inFile, "")) {
				int fdIn = open(inFile, O_RDONLY);
				if (fdIn == -1) {
					perror("Error trying to open input file\n");
					exit(1);
				}
				dup2(fdIn, STDIN_FILENO);
				fcntl(fdIn, F_SETFD, FD_CLOEXEC);
			}
			if (strcmp(outFile, "")) {
				int fdOut = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if (fdOut == -1) {
					perror("Error trying to open output file\n");
					exit(1);
				}
				dup2(fdOut, STDOUT_FILENO);
				fcntl(fdOut, F_SETFD, FD_CLOEXEC);

			}
			// Execute command	
			if (execvp(args[0], args)) {
				perror("execvp");
				exit(1);
			}
			break;
		default:	// Parent process
			if(background & !foreground_only) {	// Background option set to true
				printf("Background process id: %d\n", spawnPid);
				waitpid(spawnPid, estatus, WNOHANG);
				fflush(stdout);

			} else {	// Run in foreground (wait for child process to end)
				waitpid(spawnPid, estatus, 0);
			}
		// Check for background process termination
		while ((spawnPid = waitpid(-1, estatus, WNOHANG)) > 0) {
			printf("background pid %d is done:", spawnPid);
			printStatus(*estatus);
			fflush(stdout);
		}
	}

}


/**
 * Signal handler for SIGTSTP, sets whether or not background processes are allowed
 * 
 * @param foreground_only A global variable representing the option for commands to run in the
 * 			  foreground only (0 if false)
 */
void handle_SIGTSTP(){
	if (foreground_only) {
		char *message = "\nExiting foreground-only mode.\n";
		write (1, message, 31);
		fflush(stdout);
		foreground_only = 0;
	} else {
		char *message = "\nEntering foreground-only mode (& is now ignored).\n";
		write(1, message, 51);
		fflush(stdout);
		foreground_only = 1;
	}
}

int main(void) 
{
	// Initialize buffers and vars
	int argc = 0;
	char *args[MAXARGS];
	for (int i=0; i<MAXARGS; i++) {
		args[i] = NULL;
	}
	
	int pid = getpid();
	char inFile[256] = "";
	char outFile[256] = "";
	int background = 0;
	int estatus = 0;
	
	// Handler for SIGINT
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	sigaction(SIGINT, &SIGINT_action, NULL);
	
	// Handler for SIGTSTP
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	
	// Prompt for commands until program is exited
	while(1) {	
	
		// Get input
		if(promptCommand(&argc, args, pid, inFile, outFile, &background)) continue;	
		
		// exit command
		if (strcmp(args[0], "exit") == 0)  exit(0);
		
		// cd command
		else if (strcmp(args[0], "cd") == 0) {
			if (argc == 1) {
				chdir(getenv("HOME"));
			}
			else {
				chdir(args[1]);
			}	
		}
	
		// status command
		else if (strcmp(args[0], "status") == 0) {
			printStatus(estatus);
		}
		
		// command is not built-in
		else {
			execCom(args, inFile, outFile, &estatus, background, SIGINT_action);
		}
	
		// Reset buffers and vars	
		for (int i=0; args[i]; i++) {
			args[i] = NULL;
		}
		argc = 0;
		inFile[0] = '\0';
		outFile[0] = '\0';
		background = 0;
	}
		
	return 0;
}



