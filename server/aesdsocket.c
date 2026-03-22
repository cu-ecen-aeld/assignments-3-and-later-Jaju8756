/* References: 	https://stackoverflow.com/questions/14320041/pthread-mutex-initializer-vs-pthread-mutex-init-mutex-param
				https://man7.org/linux/man-pages/man3/strftime.3.html
*/

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
#include <pthread.h>
#include <time.h>
#include "../aesd-char-driver/aesd_ioctl.h"

#define PORT 9000
#define BUFFER_SIZE 1024

#ifndef USE_AESD_CHAR_DEVICE
#define USE_AESD_CHAR_DEVICE 1
#endif

#if USE_AESD_CHAR_DEVICE
#define FILE_PATH "/dev/aesdchar"
#else
#define FILE_PATH "/var/tmp/aesdsocketdata"
#endif

int server_fd = -1;
bool caught_sigint = false;
bool caught_sigterm = false;
volatile sig_atomic_t exit_flag = 0;
timer_t timerid;

// Mutex initializtion to keep multithreading safe
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
struct thread_data *head = NULL;

    
struct thread_data{
    pthread_t thread_id;
    int client_fd;
    struct sockaddr_in client_addr;
    bool thread_complete;
    struct thread_data *next;	// singly linked list
};

#if !USE_AESD_CHAR_DEVICE
void timer_handler(union sigval arg)
{
    time_t t = time(NULL);	//current time in seconds since epoch
    struct tm *tmp = localtime(&t); 	//converts it to human-readable local time
    char timebuf[128];

    ssize_t len = strftime(timebuf, sizeof(timebuf), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", tmp);
    if (len == 0){
        printf("strftime failed (buffer too small)\n");
    } else{
        printf("Formatted: %s (Bytes: %zu)\n", timebuf, len);
    }    

    pthread_mutex_lock(&file_mutex);

    int fd = open(FILE_PATH, O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (fd >= 0)
    {
    	syslog(LOG_INFO, "Open %s success", FILE_PATH);
        ssize_t write_t = write(fd, timebuf, strlen(timebuf));
        if(write_t < strlen(timebuf)){
            syslog(LOG_ERR, "Failed write timestamp to %s", FILE_PATH);
        } else{
            syslog(LOG_INFO, "Timestamp appended to %s", FILE_PATH);
        }
        close(fd);
    }

    pthread_mutex_unlock(&file_mutex);
}

void init_timer()
{
    struct sigevent sev;	//describes how the timer notifies you
    struct itimerspec its;	//describes initial expiration & interval

    memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify = SIGEV_THREAD;	//timer expiration runs handler in a new thread
    sev.sigev_notify_function = timer_handler;

    if(timer_create(CLOCK_REALTIME, &sev, &timerid) == -1){
    	syslog(LOG_ERR, "timer_create failed: %s\n", strerror(errno));
	} else{
		syslog(LOG_INFO, "Timer created successfully\n");
	}

    its.it_value.tv_sec = 10;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 10;
    its.it_interval.tv_nsec = 0;

    if(timer_settime(timerid, 0, &its, NULL) == -1){
    	syslog(LOG_ERR, "timer_settime failed: %s\n", strerror(errno));
	} else{
		syslog(LOG_INFO, "Timer set successfully\n");
	}
}
#endif

static void signal_handler(int signo)
{
    exit_flag = 1;
    if (signo == SIGINT || signo == SIGTERM)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
        if (server_fd != -1)
        {
        	syslog(LOG_INFO, "Shutdown server");
            shutdown(server_fd, SHUT_RDWR);
        }
    }
}

void init_syslog()
{
    openlog("aesdsocket", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Syslog initialized");
}

void register_signals()
{
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

    syslog(LOG_INFO, "Signal handlers registered");
}

int create_socket()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        syslog(LOG_ERR, "Socket creation failed");
        return -1;
    }

    syslog(LOG_INFO, "Socket created successfully");

    // SO_REUSEADDR allows to immediately reuse the port even if it's in TIME_WAIT
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    syslog(LOG_INFO, "SO_REUSEADDR set on socket");

    return 0;
}

int bind_socket()
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        syslog(LOG_ERR, "Bind failed %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    syslog(LOG_INFO, "Bind successful on port %d", PORT);
    return 0;
}

void start_daemon()
{
    pid_t pid = fork();
    
    if (pid < 0)
	{
		syslog(LOG_ERR, "Fork failed daemon process: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

    if (pid > 0)
    {
		syslog(LOG_INFO, "Fork succeeded for daemon process: %s", strerror(errno));
        exit(EXIT_SUCCESS);
	}
	if (setsid() < 0)
	{
		syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

    umask(0);


	if (chdir("/") < 0)
	{
		syslog(LOG_ERR, "chdir failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

    int fd = open("/dev/null", O_RDWR);

    if (fd >= 0)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        syslog(LOG_INFO, "Daemon success");
    }
}

int start_listen()
{
    if (listen(server_fd, 5) < 0)
    {
        syslog(LOG_ERR, "Listen failed");
        close(server_fd);
        return -1;
    }

    syslog(LOG_INFO, "Server listening on port %d", PORT);
    return 0;
}

void *handle_client(void *thread_param)
{
	struct thread_data *data = (struct thread_data *)thread_param;
	
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->client_addr.sin_addr), client_ip, sizeof(client_ip));

    syslog(LOG_INFO, "Accepted connection from %s", client_ip);

    /*  Logic: Receive Until Newline  */
    char *packet = NULL;
    size_t total_size = 0;
    char buffer[BUFFER_SIZE]; // temporary chunk receiver 

	
    while (1) // keeps receiving data from one connected client
    {
        ssize_t bytes = recv(data->client_fd, buffer, sizeof(buffer), 0); // reads data from client socket into temporary buffer

        if (bytes <= 0)
            break;

        char *new_packet = realloc(packet, total_size + bytes); //new ptr because we need to track the old ptr if realloc fails
        if (!new_packet)
        {
            syslog(LOG_ERR, "Memory allocation failed");
            free(packet);
            packet = NULL;
            break;
        }

        packet = new_packet; // pointers pointing to the same memory
        memcpy(packet + total_size, buffer, bytes); // appending buffer content into the packet
        total_size += bytes;
		syslog(LOG_INFO, "packet sending started");
        if (memchr(buffer, '\n', bytes)) // newline? packet complete
            break;
    }
	
	pthread_mutex_lock(&file_mutex);
	
#if USE_AESD_CHAR_DEVICE
	if (packet && strncmp(packet, "AESDCHAR_IOCSEEKTO:", 19) == 0)
    {
        /*  IOCTL PATH  */
        unsigned int x = 0, y = 0;

        /* Ensure null-terminated copy */
        char *cmd = strndup(packet, total_size);
        if (!cmd)
        {
            syslog(LOG_ERR, "strndup failed");
            pthread_mutex_unlock(&file_mutex);
            goto cleanup;
        }

        if (sscanf(cmd + 19, "%u,%u", &x, &y) == 2)
        {
            struct aesd_seekto seekto;
            seekto.write_cmd = x;
            seekto.write_cmd_offset = y;

            int fd = open(FILE_PATH, O_RDWR);
            if (fd < 0)
            {
                syslog(LOG_ERR, "open failed: %s", strerror(errno));
                free(cmd);
                pthread_mutex_unlock(&file_mutex);
                goto cleanup;
            }

            /*  Perform IOCTL */
            if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) != 0)
            {
                syslog(LOG_ERR, "ioctl failed: %s", strerror(errno));
            }

            ssize_t bytes_read;
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
            {
                if (send(data->client_fd, buffer, bytes_read, 0) < 0)
                {
                    syslog(LOG_ERR, "send failed: %s", strerror(errno));
                    break;
                }
            }
            close(fd);
        }
        else
        {
            syslog(LOG_ERR, "Invalid IOCTL format");
        }
        free(cmd);
    }
    else
    {
        /*  NORMAL WRITE PATH  */

        int fd = open(FILE_PATH, O_RDWR);
        if (fd < 0)
        {
            syslog(LOG_ERR, "open failed: %s", strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            goto cleanup;
        }

        /* Write packet */
        ssize_t written = 0;
        while (written < (ssize_t)total_size)
        {
            ssize_t w = write(fd, packet + written, total_size - written);
            if (w <= 0)
            {
                syslog(LOG_ERR, "write failed: %s", strerror(errno));
                break;
            }
            written += w;
        }

        /* Reset file offset to beginning for read */
        lseek(fd, 0, SEEK_SET);

        /* Read back entire content */
        ssize_t bytes_read;
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
        {
            if (send(data->client_fd, buffer, bytes_read, 0) < 0)
            {
                syslog(LOG_ERR, "send failed: %s", strerror(errno));
                break;
            }
        }

        close(fd);
    }
#else
    int fd = open(FILE_PATH, O_CREAT | O_APPEND | O_WRONLY, 0644);

	if (fd < 0) {
		syslog(LOG_ERR, "Failed to open %s: %s", FILE_PATH, strerror(errno));
	}

    if (fd >= 0 && packet)
    {
    	syslog(LOG_INFO, "Open %s success", FILE_PATH);
        ssize_t wr_bytes = write(fd, packet, total_size);
        if(wr_bytes < total_size){
            syslog(LOG_ERR, "Failed write to %s", FILE_PATH);
        } else{
            syslog(LOG_INFO, "Data appended to %s", FILE_PATH);
        }
        close(fd);
    }

    /*  Send Full File Back  */
    fd = open(FILE_PATH, O_RDONLY);
    if (fd >= 0)
    {
        ssize_t bytes_read;

		while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
		{
			ssize_t bytes_sent = send(data->client_fd, buffer, bytes_read, 0);
			if (bytes_sent < 0)
			{
				syslog(LOG_ERR, "Send failed: %s", strerror(errno));
				break;
			}
		}

		if (bytes_read < 0)
		{
			syslog(LOG_ERR, "Read failed: %s", strerror(errno));
		}
        close(fd);
    }
#endif
	pthread_mutex_unlock(&file_mutex);

cleanup:
    syslog(LOG_INFO, "Closed connection from %s", client_ip);
    free(packet);
    close(data->client_fd);
 
    data->thread_complete = true;
	return NULL;
}

void accept_loop()
{
    // socket(), bind(), listen() are onle-time setup operations
    /*  Accept Loop  */
    while (!exit_flag) // keeps server running until signal interrupts
    {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);

        if (client_fd < 0)
        {
            if (errno == EINTR) // Error interrupt -1
                break;
            else
            {
                syslog(LOG_INFO, "Client accept failed");
                break;
            }
        }
        
		/* Handle client communication */
        struct thread_data *new_thread = malloc(sizeof(struct thread_data));
		new_thread->client_fd = client_fd;
		new_thread->client_addr = client_addr;
		new_thread->thread_complete = false;
		new_thread->next = head;
		head = new_thread;

		int pCreate = pthread_create(&new_thread->thread_id, NULL, handle_client, new_thread);
		if (pCreate != 0)
		{
			syslog(LOG_ERR, "Thread creation failed: %s", strerror(errno));
		}
		
		struct thread_data *curr = head;
		struct thread_data *prev = NULL;

		while (curr != NULL)
		{
			if (curr->thread_complete)
			{
				pthread_join(curr->thread_id, NULL);

				if (prev == NULL)
				    head = curr->next;
				else
				    prev->next = curr->next;

				struct thread_data *temp = curr;
				curr = curr->next;
				free(temp);
			}
			else
			{
				prev = curr;
				curr = curr->next;
			}
		} 
    }
}

/*  Cleanup  */
void cleanup()
{
    syslog(LOG_INFO, "Exit flag 1 set");

    // Stop timer
#if !USE_AESD_CHAR_DEVICE
    timer_delete(timerid);
#endif

    // Stop accepting connections
    close(server_fd);

    // Join ALL remaining/incomplete threads
    struct thread_data *curr = head;
    while (curr != NULL)
    {
        pthread_join(curr->thread_id, NULL);

        struct thread_data *temp = curr;
        curr = curr->next;
        free(temp);
    }

    // Destroy shared resources
    pthread_mutex_destroy(&file_mutex);

#if !USE_AESD_CHAR_DEVICE
	remove(FILE_PATH);
#endif

    syslog(LOG_INFO, "Server cleanup complete");
    closelog();
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    init_syslog();
    register_signals();

    if (create_socket() != 0)
        return -1;
        
	/*  Bind  to port 9000*/
    if (bind_socket() != 0)
        return -1;
        
	/*  Daemon Mode after ensuring socket bind to port 9000  */
    if (daemon_mode)
	{
    	start_daemon();
    }
    
#if !USE_AESD_CHAR_DEVICE
	init_timer();
#endif
    if (start_listen() != 0)
        return -1;

    accept_loop();	// keeps server running until signal interrupts: while (!exit_flag)
    				// Also handles client communication
    cleanup();

    return 0;
}

