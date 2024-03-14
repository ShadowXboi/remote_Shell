#include <stdio.h>
#include <stdlib.h>

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
}

// Function to handle client Information
static void handle_connection(int client_sockfd)
{
}

static void execute_command(char *command, int client_sockfd)
{
}
