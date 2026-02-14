#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <fcntl.h>

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

int server_fd = -1;
volatile sig_atomic_t exit_flag = 0;
bool caught_sigint = false;
bool caught_sigterm = false;

/* ================= SIGNAL HANDLER ================= */

static void signal_handler(int signo)
{
	exit_flag = 1;
	
    if(signo == SIGINT){
    	syslog(LOG_INFO, "Caught signal SIGINT, exiting");;
    } else if(signo == SIGTERM){
    	syslog(LOG_INFO, "Caught signal SIGTERM, exiting");;
    }
}

/* ================= MAIN ================= */

int main(int argc, char *argv[])
{
    //int daemon_mode = 0;

    //if (argc == 2 && strcmp(argv[1], "-d") == 0)
        //daemon_mode = 1;

    /* Open syslog */
    openlog("aesdsocket", LOG_PID, LOG_USER);

    /* Register signal handlers */
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    
    if(sigaction(SIGTERM, &new_action, NULL) != 0)
    {
    	syslog(LOG_INFO, "Error: %d registering SIGTERM", errno);
    }
    if(sigaction(SIGINT, &new_action, NULL) != 0)
    {
    	syslog(LOG_INFO, "Error: %d registering SIGINT", errno);
    }

    
    closelog();

    return 0;
}

