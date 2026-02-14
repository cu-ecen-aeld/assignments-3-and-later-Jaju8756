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

/*  SIGNAL HANDLER  */
static void signal_handler(int signo)
{
	exit_flag = 1;
	
    if(signo == SIGINT){
    	syslog(LOG_INFO, "Caught signal SIGINT, exiting");;
    } else if(signo == SIGTERM){
    	syslog(LOG_INFO, "Caught signal SIGTERM, exiting");;
    }
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    /* Open syslog */
    openlog("aesdsocket", LOG_PID, LOG_USER);

    /* Register signal handlers */
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    
    if(sigaction(SIGTERM, &new_action, NULL) != 0)
    {
    	syslog(LOG_ERR, "Error: %d registering SIGTERM", errno);
    }
    if(sigaction(SIGINT, &new_action, NULL) != 0)
    {
    	syslog(LOG_ERR, "Error: %d registering SIGINT", errno);
    }
    
	/*  Create Socket  */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        syslog(LOG_ERR, "Socket creation failed");
        return -1;
    }

    /*  Bind  to port 9000*/
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        syslog(LOG_ERR, "Bind failed");
        close(server_fd);
        return -1;
    }

    /*  Daemon Mode after ensuring socket bind to port 9000  */
    if (daemon_mode)
    {
        pid_t pid = fork();
        if (pid < 0)
            exit(EXIT_FAILURE);
        if (pid > 0)
            exit(EXIT_SUCCESS);

        umask(0);
        setsid();
        chdir("/");

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    /*  Listen  */
    if (listen(server_fd, 5) < 0)
    {
        syslog(LOG_ERR, "Listen failed");
        close(server_fd);
        return -1;
    }
    
	// socket(), bind(), listen() are onle-time setup operations
    /*  Accept Loop  */
    while (!exit_flag)	// keeps server running until signal interrupts
    {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &addrlen);

        if (client_fd < 0)
		{
			if (errno == EINTR)	// Error interrupt -1
				break;
			else
			{
				syslog(LOG_INFO, "Client accept failed");
				break;
			}
		}
		
		
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr,
                  client_ip, sizeof(client_ip));

        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        /*  Logic: Receive Until Newline  */

        char *packet = NULL;
        size_t total_size = 0;
        char buffer[BUFFER_SIZE];

        while (1) // keeps receiving data from one connected client
        {
            ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);	// reads data from client socket into temporary buffer

            if (bytes <= 0)
                break;

            char *new_packet = realloc(packet, total_size + bytes);	//new ptr because we need to track the old ptr if realloc fails
            if (!new_packet)
            {
                free(packet);
                break;
            }

            packet = new_packet;	// pointers pointing to the same memory
            memcpy(packet + total_size, buffer, bytes);		// appending buffer content into the packet
            total_size += bytes;

            if (memchr(buffer, '\n', bytes))	// newline? packet complete
                break;
        }

        /*  Append buffer content to File  */
        int fd = open(FILE_PATH, O_CREAT | O_APPEND | O_WRONLY, 0644);	

        if (fd >= 0 && packet)
        {
          	ssize_t wr_bytes = write(fd, packet, total_size);
          	if(wr_bytes < total_size)
          	{
          		syslog(LOG_ERR, "Failed write to %s", FILE_PATH);
          	}
            close(fd);
        }

        /*  Send Full File Back  */
        fd = open(FILE_PATH, O_RDONLY);
        if (fd >= 0)
        {
            while ((total_size = read(fd, buffer, sizeof(buffer))) > 0)
            {
                send(client_fd, buffer, total_size, 0);		// Returns full file contents
            }
            close(fd);
        }

        syslog(LOG_INFO,
               "Closed connection from %s",
               client_ip);

        free(packet);
        close(client_fd);
    }
    syslog(LOG_INFO, "Exit flag 1 set");

    /*  Cleanup  */
    close(server_fd);
    remove(FILE_PATH); // deleting the file on exit
    
    closelog();

    return 0;
}

