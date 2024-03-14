#include <arpa/inet.h>     // Declarations for internet operations
#include <netinet/in.h>    // Definitions for internet operations
#include <stdio.h>         // Standard input/output library
#include <stdlib.h>        // Standard library definitions
#include <string.h>        // String manipulation functions
#include <sys/socket.h>    // Socket functions
#include <unistd.h>        // Standard symbolic constants and types

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 10

#ifndef SOCK_CLOEXEC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-macros"
    #define SOCK_CLOEXEC 0
    #pragma GCC diagnostic pop
#endif

// Define a struct to hold socket-related information
struct SocketInfo
{
    int                sockfd;         // Socket file descriptor
    struct sockaddr_in server_addr;    // Server address structure
};

// Function prototypes
int  init_socket(struct SocketInfo *socket_info, const char *server_ip, int server_port);
void cleanup_socket(const struct SocketInfo *socket_info);

// Function to initialize the socket
int init_socket(struct SocketInfo *socket_info, const char *server_ip, int server_port)
{
    // Create a socket
    socket_info->sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(socket_info->sockfd == -1)
    {
        fprintf(stderr, "Socket creation failed\n");
        return -1;
    }

    // Set up server address structure
    memset(&(socket_info->server_addr), 0, sizeof(socket_info->server_addr));
    socket_info->server_addr.sin_family      = AF_INET;
    socket_info->server_addr.sin_addr.s_addr = inet_addr(server_ip);
    socket_info->server_addr.sin_port        = htons(server_port);

    // Connect to the server
    if(connect(socket_info->sockfd, (struct sockaddr *)&(socket_info->server_addr), sizeof(socket_info->server_addr)) == -1)
    {
        fprintf(stderr, "Connection failed\n");
        close(socket_info->sockfd);
        return -1;
    }

    // Print connection message
    printf("Connected to server %s:%d\n", server_ip, server_port);
    return 0;
}

// function to clean up the socket
void cleanup_socket(const struct SocketInfo *socket_info)
{
    close(socket_info->sockfd);    // close the socket
}

// Main function
int main(int argc, const char *argv[])
{
    struct SocketInfo socket_info;
    char              command[BUFFER_SIZE];    // buffer to store user command
    char              buffer[BUFFER_SIZE];     // buffer to store sever response

    // check if the correct no of command-line arguments are provided
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // initialize the socket connection
    if(init_socket(&socket_info, argv[1], (int)strtol(argv[2], NULL, DEFAULT_PORT)) == -1)
    {
        exit(EXIT_FAILURE);
    }

    // Main loop to send commands to the server and receive responses
    while(1)
    {
        ssize_t nread;                // Move this declaration to here
        size_t  len;                  // Declare len here
        printf("Enter command: ");    // Prompt user to enter a command
        fflush(stdout);               // Flush stdout to ensure prompt is displayed

        // Read command from user input
        if(fgets(command, BUFFER_SIZE, stdin) == NULL)
        {
            printf("Error reading command\n");
            break;
        }

        // Get the length of the command
        len = strlen(command);
        if(len > 0 && command[len - 1] == '\n')
        {
            command[len - 1] = '\0';    // Remove newline character
        }

        // Send command to server
        if(write(socket_info.sockfd, command, strlen(command)) < 0)
        {
            fprintf(stderr, "Write error\n");
            break;
        }

        // Read and display the results from the server
        while((nread = read(socket_info.sockfd, buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[nread] = '\0';                       // Null-terminate the string
            printf("Server response: %s\n", buffer);    // Print server response
            break;                                      // Exit the loop after receiving one response
        }

        // Check if server closed the connection
        if(nread == 0)
        {
            printf("Connection closed by server.\n");    // Print message indicating server closure
            break;
        }
        if(nread == -1)
        {
            fprintf(stderr, "Read error\n");    // Check for read error
            break;
        }
    }

    // Cleanup and close the socket
    if(strcmp(command, "exit") == 0)
    {
        // Send a special signal to the server indicating client wants to exit
        // Then close the socket and terminate the client program
        cleanup_socket(&socket_info);
        exit(EXIT_SUCCESS);
    }
}
