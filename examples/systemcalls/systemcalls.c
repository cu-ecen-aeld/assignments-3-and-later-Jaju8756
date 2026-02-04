/*
 *	File modified by Jacqulin Justin
 *	Date: 29 Jan 2026	
 */
 
 
#include "systemcalls.h"
#include <stdbool.h>
#include <stdlib.h>	// EXIT_FAILURE
#include <unistd.h>	// STDOUT_FILENO
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>	// va_list, va_start, va_arg, va_end

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
 	// If cmd is NULL, there is no command to run
    if (cmd == NULL) {
        return false;
    }

    int status = system(cmd);

	// system() failure check
    if (status == -1) {
        return false;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    return false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;	// execv() uses NULL to know where arguments end
    
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count]; // became dummy after implementation

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    va_end(args);	// Cleans up va_list
    
    fflush(stdout);
    pid_t pid = fork();

    if (pid == -1) {
        return false;
    }

    if (pid == 0) {
        execv(command[0], command);
        // execv only returns on failure
        exit(EXIT_FAILURE);
    }

    int status;
    
    // Parent waits until child finishes
    if (waitpid(pid, &status, 0) == -1) { 
        return false;
    }
	
	// Checks if child exited normally && exit code 0 for success
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    return false;

}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);

	fflush(stdout);
	
    pid_t pid = fork();

    if (pid == -1) {
        return false;	
    }

    if (pid == 0) {
    	// O_WRONLY → write only
		// O_CREAT → create if it doesn’t exist
		// O_TRUNC → clear file if it exists
    	// 0644 → rw-r--r-- permissions
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        
        // File open failure
        if (fd < 0) {
            exit(EXIT_FAILURE);
        }
		printf("fd value %d", fd);
		
        dup2(fd, STDOUT_FILENO);	// Make STDOUT_FILENO refer to the same open file 'outputfile'
        close(fd);

        execv(command[0], command);
        exit(EXIT_FAILURE);
    }

    int status;
    
    if (waitpid(pid, &status, 0) == -1) {
        return false;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    return false;
}
