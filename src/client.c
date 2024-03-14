#include <stdio.h>     // Standard input/output library
#include <stdlib.h>    // Standard library definitions

#define BUFFER_SIZE 1024

// a struct to hold socket - related information
struct SocketInfo
{
    int               sockfd;        // socket file descriptor
    stuct sockaddr_in sever_addr;    // Sever address structure
};

// Function prototypes
int  init_socket(struct SocketInfo *socket_info, const char *server_ip, int server_port);
void cleanup_socket(const struct SocketInfo *socket_info);

int init_socket(struct SocketInfo *socket_info, const char *server_ip, int server_port)
{
}

void cleanup_socket(const struct SocketInfo *socket_info)
{
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
}
