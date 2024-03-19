#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 65535

// Ensure SOCK_CLOEXEC is defined, using conditional compilation
#ifndef SOCK_CLOEXEC
    #define SOCK_CLOEXEC 0
#endif

// Function prototypes
static void handle_connection(int client_sockfd);
static void execute_command(char *command, int client_sockfd);
static void signal_handler(int signal_number);
static void sigchld_handler(int sig);

// Main Function
int main(int argc, const char *argv[])
{
    long               psort;
    char              *endptr;
    int                sockfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct sigaction   sa;
    socklen_t          client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));    // zero initialize to client addr before use

    printf("To send a signal:\n");
    printf("\tCtrl+C: Sends the SIGINT signal to the process.\n");
    printf("\tCtrl+Z: Sends the SIGTSTP signal to the process.\n");
    printf("\tCtrl+\\: Sends the SIGQUIT signal to the process.\n");

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;               // or sa.sa_sigaction = ... for advanced handling
    sa.sa_flags   = SA_RESTART | SA_NOCLDSTOP;    // plus any other flags you need

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("Error setting signal handler for SIGINT");
        exit(EXIT_FAILURE);
    }

    // Repeat for other signals you're handling, e.g., SIGCHLD
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sa.sa_flags   = SA_RESTART | SA_NOCLDSTOP;

    if(sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("Error setting signal handler for SIGCHLD");
        exit(EXIT_FAILURE);
    }
    // Validate command line arguments
    if(argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Validate the port number

    psort = strtol(argv[1], &endptr, MAX_CLIENTS);
    if(*endptr != '\0' || psort <= 0 || psort > PORT)
    {
        fprintf(stderr, "Invalid port number. Please specify a port number between 1 and 65535.\n");
        exit(EXIT_FAILURE);
    }

    // Create a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons((uint16_t)strtoul(argv[1], NULL, MAX_CLIENTS));

    // Bind the socket to the server address
    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if(listen(sockfd, MAX_CLIENTS) == -1)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %s...\n", argv[1]);

    // change the current working directory
    if(chdir("../src") == -1)
    {
        perror(" changing directory failed ");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections and handle them
    while(1)
    {
        pid_t pid;
        int   client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_sockfd == -1)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid = fork();
        if(pid == 0)
        {
            // In child process
            close(sockfd);
            while(1)
            {
                // Handle client connection
                handle_connection(client_sockfd);
                // Ensure we close the socket descriptor before exiting
            }
        }
        else if(pid > 0)
        {
            // In parent process
            close(client_sockfd);    // Parent doesn't need this
        }
        else
        {
            perror("Fork failed");
        }
    }

    // Handle client connection in a separate process
    //        if(fork() == 0)
    //        {
    //            // Close the listening socket in the child process
    //            close(sockfd);
    //            while(1)
    //            {
    //                // Handle client connection
    //                handle_connection(client_sockfd);
    //            }
    //        }
    //        else
    //        {
    //            // Close client socket in the parent process
    //            close(client_sockfd);
    //        }
}

static void sigchld_handler(int sig)
{
    (void)sig;    // Mark the parameter as unused.
    // Use waitpid to clean up the child processes.
    while(waitpid(-1, NULL, WNOHANG) > 0)
    {
        // No body needed; loop just reaps zombies.
    }
}

// Function to handle client Information
static void handle_connection(int client_sockfd)
{
    char    buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive command from client
    bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
    if(bytes_received == -1)
    {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }
    else if(bytes_received == 0)
    {
        printf("Client disconnected\n");
        exit(EXIT_SUCCESS);
    }

    buffer[bytes_received] = '\0';    // Null-terminate the received data

    // Execute the command and send the output back to the client
    execute_command(buffer, client_sockfd);
}

static void execute_command(char *command, int client_sockfd)
{
    char             output_buffer[BUFFER_SIZE];
    FILE            *output_fp = NULL;
    int              input_fd  = -1;
    FILE            *pipe_fp;
    size_t           bytes_read;
    char            *append_redirect_pos;
    char            *output_redirect_pos;
    char            *input_redirect_pos;
    char            *filename;
    struct sigaction sa_temp;
    struct sigaction sa_old;
    int              is_gcc_command;

    is_gcc_command = strncmp(command, "gcc", 3) == 0 ? 1 : 0;

    memset(&sa_temp, 0, sizeof(sa_temp));
    sa_temp.sa_handler = SIG_DFL;             // Default handler
    sigaction(SIGCHLD, &sa_temp, &sa_old);    // Save old handler

    // Initialize pointers for redirection detection
    append_redirect_pos = strstr(command, ">>");
    output_redirect_pos = strchr(command, '>');
    input_redirect_pos  = strchr(command, '<');

    // Handle appending (>>) and simple output redirection ('>')
    if(append_redirect_pos != NULL || output_redirect_pos != NULL)
    {
        char *redirect_pos = append_redirect_pos ? append_redirect_pos : output_redirect_pos;
        int   append       = append_redirect_pos != NULL;

        *redirect_pos = '\0';
        filename      = redirect_pos + (append ? 2 : 1);
        while(*filename == ' ')
        {
            filename++;
        }

        output_fp = fopen(filename, append ? "a" : "w");
        if(output_fp == NULL)
        {
            perror("File opening failed for output redirection");
            exit(EXIT_FAILURE);
        }
    }

    // Handle input redirection ('<')
    if(input_redirect_pos != NULL)
    {
        *input_redirect_pos = '\0';
        filename            = input_redirect_pos + 1;
        while(*filename == ' ')
        {
            filename++;
        }

        input_fd = open(filename, O_RDONLY | O_CLOEXEC);
        if(input_fd == -1)
        {
            perror("File opening failed for input redirection");
            exit(EXIT_FAILURE);
        }
    }

    // Execute the command
    pipe_fp = popen(command, "r");
    if(pipe_fp == NULL)
    {
        perror("Command execution failed");
        exit(EXIT_FAILURE);
    }

    // Special handling for gcc commands
    if(is_gcc_command)
    {
        int execution_status = 0;
        execution_status     = pclose(pipe_fp);    // Close the compiler process and get status
        pipe_fp              = NULL;               // Avoid double closing

        if(WIFEXITED(execution_status) && WEXITSTATUS(execution_status) == 0)
        {
            // Compilation successful; execute the compiled program
            pipe_fp = popen("./test.out", "r");
        }
        else
        {
            const char *error_msg = "GCC compilation failed.\n";
            send(client_sockfd, error_msg, strlen(error_msg), 0);
            goto cleanup;
        }
    }

    // Read and handle command output
    //    while((bytes_read = fread(output_buffer, 1, sizeof(output_buffer), pipe_fp)) > 0)
    //    {
    //        if(output_fp != NULL)
    //        {
    //            // Redirect output to file
    //            fwrite(output_buffer, 1, bytes_read, output_fp);
    //        }
    //        else
    //        {
    //            // Send output back to the client
    //            if(send(client_sockfd, output_buffer, bytes_read, 0) == -1)
    //            {
    //                perror("Send failed");
    //                exit(EXIT_FAILURE);
    //            }
    //        }
    //    }
    // Read and send command or compilation output back to the client
    while((bytes_read = fread(output_buffer, 1, sizeof(output_buffer) - 1, pipe_fp)) > 0)
    {
        // Ensure the buffer is null-terminated to safely work with functions expecting strings
        output_buffer[bytes_read] = '\0';

        // Send the read data to the client
        if(send(client_sockfd, output_buffer, bytes_read, 0) == -1)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
    }

cleanup:
    if(output_fp != NULL)
    {
        fclose(output_fp);
    }
    if(input_fd != -1)
    {
        close(input_fd);
    }
    if(pipe_fp != NULL && pclose(pipe_fp) == -1)
    {
        perror("Error closing pipe");
    }

    // Restore the original SIGCHLD handler
    sigaction(SIGCHLD, &sa_old, NULL);
}

// Signal handler function
static void signal_handler(int signal_number)
{
    switch(signal_number)
    {
        case SIGINT:
            printf("\nReceived SIGINT, server shutting down...\n");
            // Add cleanup and shutdown code here
            exit(EXIT_SUCCESS);
        case SIGTSTP:
            // Handle SIGTSTP if needed
            printf("\nReceived SIGTSTP, action not implemented.\n");
            break;
        case SIGQUIT:
            printf("\nReceived SIGQUIT, server shutting down...\n");
            // Add cleanup and shutdown code here
            exit(EXIT_SUCCESS);
        default:
            printf("Unhandled signal: %d\n", signal_number);
    }
}
