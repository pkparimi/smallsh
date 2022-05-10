#ifndef SMALLSH_H
#define SMALLSH_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>


/************************************* FLAGS *************************************/


int wstatus = 0;
bool backgroundOperatorIgnored = false;
bool termedBySig = false;
int lastStatus = 0;

/******************************* FUNCTION PROTOYPES *******************************/

// Input/Output
int inputToArgs(char* input, char* inputArgs[], int* numArgs, pid_t masterPID, char inRedirect[], char outRedirect[]);
void initializeInputs(char* inputArgs[], char inRedirect[], char outRedirect[], bool* inputValid);
void resetInputs(char* inputArgs[]);

// Execution
bool isBuiltIn(char* inputArgs[]);
void runBuiltIn(char* inputArgs[], int* numArgs);
void execute(char* inputArgs[], int redirect, char inRedirect[], char outRedirect[], struct sigaction SIGINT_action, int* numArgs);

// Background handling & Redirection
void redirectIt(char inRedirect[], char outRedirect[]);
void backgroundModeHandler();

#endif