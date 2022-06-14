# Smallsh

### By Pavan Parimi
#### How To Run
To create and run the program, while in the directory that contains the associated c files, type 'make'. This will create the 'smallsh' executable.

#### Introduction

Smallsh is an implementation of a subset of features of well-known shells, such as bash. It:

1. Provides a prompt for running commands
2. Handles blank lines and comments, which are lines beginning with the `#` character
3. Provides expansion for the variable $$
4. Executes 3 commands `exit`, `cd`, and `status` via code built into the shell
5. Executes other commands by creating new processes using a function from the `exec` family of 6. functions
6. Supports input and output redirection
7. Supports running commands in foreground and background processes
8. Implement custom handlers for 2 signals, SIGINT and SIGTSTP


# Program Functionality
#### 1. The Command Prompt
Use the colon `:` symbol as a prompt for each command line. 

The general syntax of a command line is:

```
command [arg1 arg2 ...] [< input_file] [> output_file] [&]
```
…where items in square brackets are optional.

* A command is made up of words separated by spaces.
The special symbols `<`, `>` and `&` are recognized, but they must be surrounded by spaces like other words.
* If the command is to be executed in the background, the last word must be &. If the & character appears anywhere else, the shell treats this as normal text
* If standard input or output is to be redirected, the > or < words followed by a filename word must appear after all the arguments. Input redirection can appear before or after output redirection.
* The shell does not support quoting; arguements with spaces inside them are not possible. Pipe operator is also not implemented.

#### 2. Comments & Blank Lines
The shell allows blank lines and comments.

* Any line that begins with the `#` character is a comment line and will be ignored. Mid-line comments, such as the C-style //, will not be supported.
* A blank line (one without any commands) will also do nothing.
* The shell re-prompts for another command when it receives either a blank line or a comment line.

#### 3. Expansion of Variable $$
The shell expands any instance of "$$" in a command into the process ID of the smallsh itself. The shell does not otherwise perform varibale expansion. 

#### 4. Built-in Commands
The shell supports three built-in commands: `exit`, `cd`, and `status`. Other built-ins are simply passed on to a member of the `exec()` family of functions.

##### exit
The `exit` command exits theshell. It takes no arguments. When this command is run, the shell kills any other processes or jobs that the shell has started before it terminates itself.

##### cd
The `cd` command changes the working directory of smallsh.

* By itself - with no arguments - it changes to the directory specified in the HOME environment variable
  * This is typically not the location where smallsh was executed from, unless the shell executable is located in the HOME directory, in which case these are the same.
* This command can also take one argument: the path of a directory to change to. The cd command supports both absolute and relative paths.

##### status
The `status` command prints out either the exit status or the terminating signal of the last foreground process ran by the shell.

* If this command is run before any foreground command is run, then it simply returns the exit status 0.
* The three built-in shell commands do not count as foreground processes for the purposes of this built-in command - i.e., `status` should ignore built-in commands.

#### 5. Executing Other Commands
The shell will execute any commands other than the 3 built-in command by using `fork()`, `exec()` and `waitpid()`

* Whenever a non-built in command is received, the parent (i.e., smallsh) will fork off a child.
* The child will use a function from the `exec()` family of functions to run the command.
* The shell uses the PATH variable to look for non-built in commands, and it allows shell scripts to be executed
* If a command fails because the shell could not find the command to run, then the shell will print an error message and set the exit status to 1
* A child process will terminate after running a command (whether the command is successful or it fails).

#### 6. Input & Output Redirection
* An input file redirected via stdin will be opened for reading only; the your shell cannot open the file for reading, it will print an error message and set the exit status to 1 (but won't exit the shell).
* Similarly, an output file redirected via stdout will be opened for writing only; it will be truncated if it already exists or created if it does not exist. If your shell cannot open the output file it will print an error message and set the exit status to 1 (but won't exit the shell).

#### 7. Executing Commands in Foreground & Background
##### Foreground Commands
Any command without an & at the end will be run as a foreground command and the shell will wait for the completion of the command before prompting for the next command. For such commands, the parent shell does NOT return command line access and control to the user until the child terminates.

##### Background Commands
Any non built-in command with an & at the end will be run as a background command and the shell will not wait for such a command to complete. For such commands, the parent will return command line access and control to the user immediately after forking off the child.

* The shell will print the process id of a background process when it begins.
When a background process terminates, a message showing the process id and exit status will be printed. This message must be printed just before the prompt for a new command is displayed.
* If the user doesn't redirect the standard input for a background command, then standard input should be redirected to /dev/null
* If the user doesn't redirect the standard output for a background command, then standard output should be redirected to /dev/null

#### 8. Signals SIGINT & SIGTSTP
##### SIGINT
A CTRL-C command from the keyboard sends a SIGINT signal to the parent process and all children at the same time (this is a built-in part of Linux).

* The shell, i.e., the parent process, will ignore SIGINT
* Any children running as background processes will ignore SIGINT
* A child running as a foreground process will terminate itself when it receives SIGINT
  * The parent will not attempt to terminate the foreground child process; instead the foreground child (if any) willterminate itself on receipt of this signal.
  * If a child foreground process is killed by a signal, the parent will immediately print out the number of the signal that killed it's foreground child process (see the example) before prompting the user for the next command.

##### SIGTSTP
A CTRL-Z command from the keyboard sends a SIGTSTP signal to your parent shell process and all children at the same time (this is a built-in part of Linux).

* A child, if any, running as a foreground process will ignore SIGTSTP.
* Any children running as background process will ignore SIGTSTP.
* When the parent process running the shell receives SIGTSTP
  * The shell will display an informative message (see below) immediately if it's sitting at the prompt, or immediately after any currently running foreground process has terminated
  * The shell then enters a state where subsequent commands can no longer be run in the background.
  * In this state, the & operator will simply be ignored, i.e., all such commands are run as if they were foreground processes.
* If the user sends SIGTSTP again, then the shell will
  * Display another informative message (see below) immediately after any currently running foreground process terminates
  * The shell then returns back to the normal condition where the & operator is once again honored for subsequent commands, allowing them to be executed in the background.

#### Sample Program Execution
Here is an example run using smallsh. Note that CTRL-C has no effect towards the bottom of the example, when it's used while sitting at the command prompt:
```
$ smallsh
: ls
junk   smallsh    smallsh.c
: ls > junk
: status
exit value 0
: cat junk
junk
smallsh
smallsh.c
: wc < junk > junk2
: wc < junk
       3       3      23
: test -f badfile
: status
exit value 1
: wc < badfile
cannot open badfile for input
: status
exit value 1
: badfile
badfile: no such file or directory
: sleep 5
^Cterminated by signal 2
: status &
terminated by signal 2
: sleep 15 &
background pid is 4923
: ps
  PID TTY          TIME CMD
 4923 pts/0    00:00:00 sleep
 4564 pts/0    00:00:03 bash
 4867 pts/0    00:01:32 smallsh
 4927 pts/0    00:00:00 ps
:
: # that was a blank command line, this is a comment line
:
background pid 4923 is done: exit value 0
: # the background sleep finally finished
: sleep 30 &
background pid is 4941
: kill -15 4941
background pid 4941 is done: terminated by signal 15
: pwd
/nfs/stak/users/chaudhrn/CS344/prog3
: cd
: pwd
/nfs/stak/users/chaudhrn
: cd CS344
: pwd
/nfs/stak/users/chaudhrn/CS344
: echo 4867
4867
: echo $$
4867
: ^C^Z
Entering foreground-only mode (& is now ignored)
: date
 Mon Jan  2 11:24:33 PST 2017
: sleep 5 &
: date
 Mon Jan  2 11:24:38 PST 2017
: ^Z
Exiting foreground-only mode
: date
 Mon Jan  2 11:24:39 PST 2017
: sleep 5 &
background pid is 4963
: date
 Mon Jan 2 11:24:39 PST 2017
: exit
$
```
