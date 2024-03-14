#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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

//Main Function
int main(int argc, const char *argv[])
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t          client_addr_len;
    int                sockfd;

    //Check if the correct no. of arguments are provided
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
    server_addr.sin_port        = htons(strtol(argv[1], NULL, MAX_CLIENTS));    // Port number provided as command-line argument

    / Bind the socket to the server address
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

    printf("Server listening on port %ld...\n", strtol(argv[1], NULL, MAX_CLIENTS));

    // change the current working directory
    if(chdir("../src") == -1)
    {
        perror(" changing directory failed ");
        exit(EXIT_FAILURE);
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

