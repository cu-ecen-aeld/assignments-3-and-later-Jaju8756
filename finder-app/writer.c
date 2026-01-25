/**
* Assignment 2: File Operations and Cross Compiler
* Author: Jacqulin Justin
* Date : 01/24/2026
* References: 
* The GNU C Library (glibc) manual : https://sourceware.org/glibc/manual/latest/html_node/index.html
* Linux manual pages - https://man7.org/linux/man-pages/index.html
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>	// open()
#include <unistd.h>	// read(), write()
#include <syslog.h>	// logging
#include <errno.h>	// error reporting


int main(int argc, char *argv[])
{
	// Setup syslog
	openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
	
	// Check number of arguments
	if (argc != 3) {
	    syslog(LOG_ERR, "Usage: %s <writefile> <writestr>\n", argv[0]);
	    fprintf(stderr, "Usage: %s <writefile> <writestr>\n", argv[0]);
	    return 1;
	}

	const char *writefile = argv[1];
    const char *writestr = argv[2];

	// Open the file
    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open file %s: %s", writefile, strerror(errno));
        closelog();
        return 1;
    }
    
    // Write to the file
    ssize_t bytes_written = write(fd, writestr, strlen(writestr));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Failed to write to file %s: %s", writefile, strerror(errno));
        close(fd);
        closelog();
        return 1;
    }
    
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

	//Cleanup
    close(fd);
    closelog();
    return 0;
}
