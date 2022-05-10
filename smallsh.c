/*
*   Pavan Parimi
*	parimip@oregonstate.edu
*	Smallsh - small shell program
*	Implements a subset of features of well-known shells, such as bash. 
*	
*	Functions besides main listed alphabetically
*/ 

#include "smallsh.h"


/****************************** Main Driver ********************************************/
int main()
{
	// IO vars
	char* input;			// for getline
	char* inputArgs[512];	// Store arguments per prompt loop
	int numArgs = 0;		// Track # of arguments per prompt loop
	size_t len = 0;			// for getline
	size_t characters = 0;	// for getline
	bool inputValid = true;	// flag to avoid newline, comment, or null terminator entered
	pid_t masterPID = getpid();	// for process management & IO $$ expansion
	
	// for redirection
	int redirect;			// flag if redirection needed
	char inRedirect[256];	// hold filname for input redirection
	char outRedirect[256];	// hold filname for output redirection

	//For signals - SIGINT
	struct sigaction SIGINT_action = { 0 };
	SIGINT_action.sa_handler = SIG_IGN;					// Handler for SIGINT set to ignore 
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	//For signals - SIGTSTP
	struct sigaction SIGTSTP_action = { 0 };
	SIGTSTP_action.sa_handler = backgroundModeHandler;	// Handler for SIGTSTP for background
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	// loop until exit command
	while (1)
	{
		// reset inputs and values required for input and redirection
		initializeInputs(inputArgs, inRedirect, outRedirect, &inputValid);
		numArgs = 0;
		input = NULL;

		// get line of input
		while (!inputValid)
		{
			printf(": ");
			fflush(stdout);
			characters = getline(&input, &len, stdin);

			// check if input is valid
			if (input[0] == '\n' || input[0] == '\0' || input[0] == '#')
			{
				// reset inputs and values required for input and redirection if input is invalid
				input = NULL;
				initializeInputs(inputArgs, inRedirect, outRedirect, &inputValid);  
				len = 0;
			}
			else
			{
				inputValid = true;
			}
		}
		input[characters - 1] = '\0'; // remove newline

		// tokenize input returning 1 if redirect needed
		redirect = inputToArgs(input, inputArgs, &numArgs, masterPID, inRedirect, outRedirect);

		// check and run built in if built in present in arguments
		if (isBuiltIn(inputArgs) == true)
		{
			runBuiltIn(inputArgs, &numArgs);
		}
		else
		{
			// execute non-built ins
			execute(inputArgs, redirect, inRedirect, outRedirect, SIGINT_action, &numArgs);
		}

		// reset needed
		resetInputs(inputArgs);
		redirect = 0;
		free(input);
	}
	return EXIT_SUCCESS;
}


/* backgroundModeHandler
 ******************************
 *	Changes between background and foreground mode
 *	Also  prints message
 *   Args:
 * 		N/A
 *   Returns: void
 *******************************/
void backgroundModeHandler()
{
	if (!backgroundOperatorIgnored)
	{
		char* msg = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, msg, 49);
		backgroundOperatorIgnored = true;
	}
	else
	{
		char* msg = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, msg, 29);
		backgroundOperatorIgnored = false;
	}
}


/* execute
 ******************************
 *	Executes all command other than 'cd', 'status',  or 'exit'
 *	
 *   Args:
 * 		inputArgs[]: array of tokenized input arguments
 * 		numArgs: number of arguments to process in inputArgs[]
 *		redirect: flag if redirection is needed (1 means redirection needed)
 *		inRedirect[]: filename for input redirection
 *		outRedirect[]: filename for output redirection
 *		SIGINT_action: sigaction struct to re-enable crtl+C action
 *   Returns: void
 *******************************/
void execute(char* inputArgs[], int redirect, char inRedirect[], char outRedirect[], struct sigaction SIGINT_action, int* numArgs)
{
	pid_t spawnPID = -5;			// junk value assigned
	bool execInBackground = false;	// flag used if & is end of argument to run in background
	int tempNumArgs = *numArgs;		// to be returned back to numArgs

	// determine if '&' is last character of input command
	if ((inputArgs[tempNumArgs - 1][1] == '\0') && (inputArgs[tempNumArgs - 1][0] == '&'))
	{
		inputArgs[tempNumArgs - 1] = NULL;		// remove the & from input Args
		tempNumArgs--;	
		execInBackground = true;
	}

	// fork a child process
	spawnPID = fork();
	switch (spawnPID)
	{
	case -1:

		perror("fork() failed!\n");
		exit(1);
		break;

	case 0:

		// re-enable SIGINT (ctrl + C)
		SIGINT_action.sa_handler = SIG_DFL;			
		sigaction(SIGINT, &SIGINT_action, NULL);

		// set redirection if required
		if (redirect)
		{
			redirectIt(inRedirect, outRedirect);
		}

		//execvp for PATH
		if (execvp(inputArgs[0], inputArgs)) 
		{
			perror("Err: Command Not Found in PATH\n");
			fflush(stdout);
			exit(1); // exit with status 1 if not found
		}
		break;

	default:

		// prints background PID if & was used to run child in background
		if (execInBackground && !backgroundOperatorIgnored)
		{
			pid_t masterPID = waitpid(spawnPID, &wstatus, WNOHANG);
			printf("Background PID is %d\n", spawnPID);
			fflush(stdout);
		}
		// otherwise parent waits for child
		else
		{
			pid_t masterPID = waitpid(spawnPID, &wstatus, 0);

			// saves status depending if child was terminated by signal or not
			if (WIFEXITED(wstatus))
			{
				termedBySig = false;
				lastStatus = WEXITSTATUS(wstatus);
			}
			else if (WIFSIGNALED(wstatus))
			{
				termedBySig = true;
				lastStatus = WTERMSIG(wstatus);
			}
		}

		// keep track of child processes in background mode
		while ((spawnPID = waitpid(-1, &wstatus, WNOHANG)) > 0)
		{
			// print background child process once complete and how completed
			printf("background pid %d is done: ", spawnPID); 
			fflush(stdout);
			if (WIFEXITED(wstatus))
			{
				termedBySig = false;
				lastStatus = WEXITSTATUS(wstatus);
				printf("Exited value %d\n", lastStatus);
				fflush(stdout);
			}
			else if (WIFSIGNALED(wstatus))
			{
				termedBySig = true;
				lastStatus = WTERMSIG(wstatus);
				printf("Terminated by signal %d\n", lastStatus);
				fflush(stdout);
			}

		}
		break;
	}
	*numArgs = tempNumArgs;  // save updated numArgs
}


/* initializeInputs
 ******************************
 *	Initializes input variables in memory 
 *
 *   Args:
 * 		inputArgs[]: array of tokenized input arguments
 *		inRedirect[]: filename for input redirection
 *		outRedirect[]: filename for output redirection
 *		inputValid: flag if input is valid
 *   Returns: void
 *******************************/
void initializeInputs(char* inputArgs[], char inRedirect[], char outRedirect[], bool* inputValid)
{
	// memallocate
	for (int i = 0; i < 512; i++)
	{
		inputArgs[i] = malloc(256 * sizeof(char));
		memset(inputArgs[i], '\0', 256);
	}
	memset(inRedirect, '\0', 256);
	memset(outRedirect, '\0', 256);

	*inputValid = false;
	
}


/* inputToArgs
 ******************************
 *	Tokenizes input into InputArgs
 *
 *   Args:
 *		input: full command from user
 *		numArgs: number of arguments to process in inputArgs[]
 * 		inputArgs[]: array of tokenized input arguments
 *		inRedirect[]: filename for input redirection
 *		outRedirect[]: filename for output redirection
 *		masterPID: shell parent program PID
 *   Returns: redirect integer flag 1 if redirection needed, 0 otherwise
 *******************************/
int inputToArgs(char* input, char* inputArgs[], int* numArgs, pid_t masterPID, char inRedirect[], char outRedirect[])
{

	int argIdx = 0;				// track inputArg index
	int tempNumArgs = 0;		// to be returned back to numArgs
	char* doubleDollarSign;		// to find and replace $$ with masterPID 
	char dollarSignHandler[256];// to help find and replace $$
	int redirect = 0;			// flag to be returned if redirection needed
	char* savePTR;				// to use with strtok_r

	// first token
	char* inputToken = strtok_r(input, " ", &savePTR);  
	// loop through input
	while (inputToken)
	{
		// sets input redirection file name if < found
		if (inputToken[0] == '<')
		{
			inputToken = strtok_r(NULL, " ", &savePTR);
			sprintf(inRedirect, "%s", inputToken);
			inputToken = strtok_r(NULL, " ", &savePTR);
			redirect = 1;
		}
		// sets output redirection file name if < found
		else if (inputToken[0] == '>')
		{
			inputToken = strtok_r(NULL, " ", &savePTR);
			sprintf(outRedirect, "%s", inputToken);
			inputToken = strtok_r(NULL, " ", &savePTR);
			redirect = 1;
		}
		// otherwise proceed as usual
		else
		{
			// use strcpy and strstr to parse $$ into masterPID
			// derived from https://www.delftstack.com/howto/c/string-contains-in-c/
			// and https://www.tutorialspoint.com/c_standard_library/c_function_strstr.htm
			memset(dollarSignHandler, '\0', sizeof(dollarSignHandler));
			strcpy(dollarSignHandler, inputToken);
			doubleDollarSign = strstr(dollarSignHandler, "$$");
			if (doubleDollarSign)
			{
				sprintf(doubleDollarSign, "%d", masterPID);
				sprintf(inputArgs[argIdx], "%s", dollarSignHandler);
			}
			else
			{
				sprintf(inputArgs[argIdx], "%s", inputToken);
			}

			// move to next token/argument
			inputToken = strtok_r(NULL, " ", &savePTR);
			tempNumArgs++;
			argIdx++;
		}

	}
	*numArgs = tempNumArgs;		// save back numArgs
	inputArgs[*numArgs] = NULL; // null terminator last value

	return redirect;
}


/* isBuiltIn
 ******************************
 *	Determines if first argument is built in
 *
 *   Args:
 * 		inputArgs[]: array of tokenized input arguments
 *   Returns: true if built in Argument, false if not
 *******************************/
bool isBuiltIn(char* inputArgs[])
{
	if ((strcmp(inputArgs[0], "cd") == 0) || (strcmp(inputArgs[0], "exit") == 0) || (strcmp(inputArgs[0], "status") == 0))
	{
		return true;
	}
	else
	{
		return false;
	}
}


/* redirectItS
 ******************************
 *	Redirects input and output file
 *
 *   Args:
 *		inRedirect[]: filename for input redirection
 *		outRedirect[]: filename for output redirection
 *   Returns: void
 *******************************/
void redirectIt(char inRedirect[], char outRedirect[])
{
	int result, inputFile, outputFile;	//file descriptions and dup2 result

	if (!(inRedirect[0] == '\0')) //needs to input redirect
	{
		inputFile = open(inRedirect, O_RDONLY);
		if (inputFile == -1)
		{
			perror("Err: No file found for input\n");
			fflush(stdout);
			exit(1);
		}
		result = dup2(inputFile, 0);
		if (result == -1)
		{
			perror("Err: file assignment for input failed\n");
			fflush(stdout);
			exit(1);
		}
		fcntl(inputFile, F_SETFD, FD_CLOEXEC);	// close file
	}
	if (!(outRedirect[0] == '\0')) //needs to output redirect
	{
		outputFile = open(outRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (outputFile == -1)
		{
			perror("Err: No file found for output\n");
			fflush(stdout);
			exit(1);
		}
		result = dup2(outputFile, 1);
		if (result == -1)
		{
			perror("Err: file assignment for output failed\n");
			fflush(stdout);
			exit(1);
		}
		fcntl(outputFile, F_SETFD, FD_CLOEXEC);	// close file
	}
}


/* resetInputs
 ******************************
 *	Frees InputArgs
 *
 *   Args:
 * 		inputArgs[]: array of tokenized input arguments
 *   Returns: void
 *******************************/
void resetInputs(char* inputArgs[])
{
	for (int i = 0; i < 512; i++)
	{
		free(inputArgs[i]);
	}
}


/* runBuiltIn
 ******************************
 *	Runs Built In Arguments
 *
 *   Args:
 * 		inputArgs[]: array of tokenized input arguments
 *		numArgs: number of Arguments of user input
 *   Returns: void
 *******************************/
void runBuiltIn(char* inputArgs[], int* numArgs)
{
	if (strcmp(inputArgs[0], "cd") == 0)
	{
		// cd to HOME if only cd given
		if (*numArgs == 1)
		{
			chdir(getenv("HOME"));
		}
		// otherwise check if valid directory
		else if (chdir(inputArgs[1]) == -1)
		{
			printf("Directory invalid\n");
			fflush(stdout);
		}
	}
	else if (strcmp(inputArgs[0], "exit") == 0)
	{
		exit(0);
	}
	else if (strcmp(inputArgs[0], "status") == 0)  
	{
		if (!termedBySig)
		{
			printf("Exit value %d\n", lastStatus);
			fflush(stdout);
		}
		else
		{
			printf("Terminated by signal %d\n", lastStatus);
			fflush(stdout);
		}
	}
}








