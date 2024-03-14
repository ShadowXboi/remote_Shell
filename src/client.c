#include <stdio.h>     // Standard input/output library
#include <stdlib.h>    // Standard library definitions
#include <string.h>        // String manipulation functions
#include <sys/socket.h>    // Socket functions
#include <unistd.h>        // Standard symbolic constants and types
#include <arpa/inet.h>     // Declarations for internet operations
#include <netinet/in.h>    // Definitions for internet operations

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 10


#ifndef SOCK_CLOEXEC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"
#define SOCK_CLOEXEC 0
#pragma GCC diagnostic pop
#endif

// a struct to hold socket - related information
struct SocketInfo
{
    int               sockfd;        // socket file descriptor
    struct sockaddr_in sever_addr;    // Sever address structure
};

// Function prototypes
int  init_socket(struct SocketInfo *socket_info, const char *server_ip, int server_port);
void cleanup_socket(const struct SocketInfo *socket_info);

//Function to initialize the socket
int init_socket(struct SocketInfo *socket_info, const char *server_ip, int server_port)
{
    //create a socket
    socket_info->sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXIC, 0);
    if(socket_info->sockfd == -1)
    {
        fprintf(stderr, "Socket creation failed\n");
        return  -1;
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

//function to clean up the socket
void cleanup_socket(const struct SocketInfo *socket_info)
{
    close(socket_info->sockfd);     //close the socket
}

// Main function
int main(int argc, const char *argv[])
{
    struct SocketInfo socket_info;
    char              command[BUFFER_SIZE];    // buffer to store user command
    char              buffer[BUFFER_SIZE];     // buffer to store sever response

    //check if the correct no of command-line arguments are provided
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //initialize the socket connection
    if(init_socket(&socket_info, argv[1],(int) strtol(argv[2], NULL, DEFAULT_PORT)) == -1)
    {
        exit(EXIT_FAILURE);
    }


}
