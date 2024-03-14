#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

#ifndef SOCK_CLOEXEC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-macros"
    #define SOCK_CLOEXEC 0
    #pragma GCC diagnostic pop
#endif

// Function prototypes
static void handle_connection(int client_sockfd);
static void execute_command(char *command, int client_sockfd);
static void signal_handler(int signal_number);

// Main Function
int main(int argc, const char *argv[])
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t          client_addr_len;
    int                sockfd;
    pid_t              pid;
    struct sigaction   sa;

    // Set up the signal handler for SIGINT,SIGTSTP, and SIGQUIT
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = signal_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    pid         = getpid();
    printf("My process ID is: %d\n", pid);
    printf("Sending SIGUSR1 signal to my own process...\n");
    // kill(pid, SIGUSR1);
    printf("To send a signal:\n");
    printf("\tCtrl+C: Sends the SIGINT signal to the process.\n");
    printf("\tCtrl+Z: Sends the SIGTSTP signal to the process.\n");
    printf("\tCtrl+\\: Sends the SIGQUIT signal to the process.\n");

    if(sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTSTP, &sa, NULL) < 0 || sigaction(SIGQUIT, &sa, NULL) < 0)
    {
        perror("Error setting signal handler");
        exit(EXIT_FAILURE);
    }
    // Check if the correct no. of arguments are provided
    if(argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
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
    // server_addr.sin_port        = htons(strtol(argv[1], NULL, MAX_CLIENTS));    // Port number provided as command-line argument
    server_addr.sin_port = htons((uint16_t)strtol(argv[1], NULL, MAX_CLIENTS));

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
    // printf("Server listening on port %ld...\n", strtol(argv[1], NULL, MAX_CLIENTS));

    // change the current working directory
    if(chdir("../src") == -1)
    {
        perror(" changing directory failed ");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections and handle them
    while(1)
    {
        int client_sockfd;
        client_addr_len = sizeof(client_addr);    // Using the global declaration
        client_sockfd   = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_sockfd == -1)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Handle client connection in a separate process
        if(fork() == 0)
        {
            close(sockfd);    // Close the listening socket in the child process
            while(1)
            {
                handle_connection(client_sockfd);    // Handle client connection
            }
        }
        else
        {
            close(client_sockfd);    // Close client socket in the parent process
        }
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
    char   output_buffer[BUFFER_SIZE];
    size_t bytes_read;    // Change ssize_t to size_t for send() function

    // Open pipe for command output
    FILE *pipe_fp = popen(command, "r");
    if(pipe_fp == NULL)
    {
        perror("Command execution failed");
        exit(EXIT_FAILURE);
    }

    // Read command output from pipe and send it to client
    while((bytes_read = fread(output_buffer, 1, sizeof(output_buffer), pipe_fp)) > 0)
    {
        if(send(client_sockfd, output_buffer, bytes_read, 0) == -1)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close pipe
    if(pclose(pipe_fp) == -1)
    {
        perror("Error closing pipe");
        exit(EXIT_FAILURE);
    }
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
