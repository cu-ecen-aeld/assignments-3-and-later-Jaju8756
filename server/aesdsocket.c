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

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

int server_fd = -1;
bool caught_sigint = false;
bool caught_sigterm = false;
volatile sig_atomic_t exit_flag = 0;


struct thread_data *head = NULL;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_data{
    pthread_t thread_id;
    int client_fd;
    struct sockaddr_in client_addr;
    bool thread_complete;
    struct thread_data *next;
};

static void signal_handler(int signo)
{
    exit_flag = 1;
    if (signo == SIGINT || signo == SIGTERM)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
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
    char buffer[BUFFER_SIZE];

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
            break;
        }

        packet = new_packet; // pointers pointing to the same memory
        memcpy(packet + total_size, buffer, bytes); // appending buffer content into the packet
        total_size += bytes;
		syslog(LOG_INFO, "packet sending started");
        if (memchr(buffer, '\n', bytes)) // newline? packet complete
            break;
    }

    int fd = open(FILE_PATH, O_CREAT | O_APPEND | O_WRONLY, 0644);

    if (fd >= 0 && packet)
    {
    	syslog(LOG_INFO, "Open %s success", FILE_PATH);
        ssize_t wr_bytes = write(fd, packet, total_size);
        if(wr_bytes < total_size)
        {
            syslog(LOG_ERR, "Failed write to %s", FILE_PATH);
        }
        else
        {
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

		pthread_create(&new_thread->thread_id, NULL, handle_client, new_thread);
		
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

    close(server_fd);
    remove(FILE_PATH); // deleting the file on exit

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
        start_daemon();

    if (start_listen() != 0)
        return -1;

    accept_loop();	// keeps server running until signal interrupts: while (!exit_flag)
    				// Also handles client communication
    cleanup();

    return 0;
}

