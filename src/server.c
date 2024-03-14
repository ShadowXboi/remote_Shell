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

}

// Function to handle client Information
static void handle_connection(int client_sockfd)
{
}

static void execute_command(char *command, int client_sockfd)
{
}
