#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
    int                sockfd;
    struct sockaddr_in server_addr;
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
    socket_info->server_addr.sin_port        = htons((uint16_t)server_port);

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
    // close the socket
    close(socket_info->sockfd);
}

// Main function
int main(int argc, const char *argv[])
{
    struct SocketInfo socket_info;
    char              command[BUFFER_SIZE];
    char              buffer[BUFFER_SIZE];

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
        ssize_t nread;
        size_t  len;
        printf("Enter command: ");
        fflush(stdout);

        // Read command from user input
        if(fgets(command, BUFFER_SIZE, stdin) == NULL)
        {
            printf("Error reading command\n");
            break;
        }

        // Remove the newline character at the end of the command, if present
        len = strlen(command);
        if(len > 0 && command[len - 1] == '\n')
        {
            command[len - 1] = '\0';
        }

        // Check for the "exit" command before sending anything to the server
        if(strcmp(command, "exit") == 0)
        {
            cleanup_socket(&socket_info);
            printf("Disconnecting from server...\n");
            exit(EXIT_SUCCESS);
        }

        // Send command to server
        if(write(socket_info.sockfd, command, strlen(command)) < 0)
        {
            fprintf(stderr, "Write error\n");
            break;
        }

        // Read and display the results from the server
        nread = read(socket_info.sockfd, buffer, sizeof(buffer) - 1);
        if(nread > 0)
        {
            buffer[nread] = '\0';    // Null-terminate the string
            printf("Server response: %s\n", buffer);
        }
        else if(nread == 0)
        {
            printf("Connection closed by server.\n");
            break;    // Exit the loop if the connection is closed
        }
        else    // nread < 0
        {
            fprintf(stderr, "Read error\n");
            break;    // Exit the loop on read error
        }
    }
}
