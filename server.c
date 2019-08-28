#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>
#include <netinet/in.h>
#include <limits.h>
#include <dirent.h>
#include "helper.h"

/**
 *  Creates socket and specified port and starts listening to this socket
 * @param port, the port to listen on
 * Return a File Descriptor of the socket
 * */
int createSocket(int port)
{
    int sock;
    int reuse = 1;

    printf("Server info: Opening Socket \n");
    // Server addess
    struct sockaddr_in server_address = (struct sockaddr_in){
        AF_INET,
        htons(port),
        (struct in_addr){ INADDR_ANY }
    };

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Cannot open socket");
        exit(EXIT_FAILURE);
    }

    // Address can be reused instantly after program exits
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    printf("Server info: Binding to port %d \n", port);
    // Bind socket to port
    if (bind(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        // Cannot bind socket to address
        fprintf(stderr, "Cannot bind socket to address");
        exit(EXIT_FAILURE);
    }

    printf("Server: Listening for requests\n");
    listen(sock, 5);
    return sock;
}

/**
 * Generate two random numbers to be used for the port in passive mode
 * @param port: Port struct pointer to be used
 */
void genPort(Port* port)
{
    srand(time(NULL));
    port->p1 = 131 + (rand() % 64);
    port->p2 = rand() % 317;
}

/**
 * Handles zombies
 * @param signum Signal number
 */
void handleZombie(int signum)
{
    int status;
    printf("Handling zombie process: %d %ld", status, &status);
    wait(&status);
}

/**
 * Get IPv4 address where client connected to.
 * @param sock: File descriptor of socket
 * @param ip:  Resulting Array to store generated address
 * E.g. {127,0,0,1}
 */
void getIP(int sock, int* ip)
{
    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    char host[INET_ADDRSTRLEN];
    getsockname(sock, (struct sockaddr*)&addr, &addr_size);
    inet_ntop(AF_INET, &(addr.sin_addr), host, INET_ADDRSTRLEN);
    sscanf(host, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
}

/**
 * Accept connection from client
 * @param socket: The socket the server is listening to
 * @return int: File descriptor to newly accepted connection
 */
int acceptConnection(int socket)
{
    int addrlen = 0;
    struct sockaddr_in client_address;
    addrlen = sizeof(client_address);
    return accept(socket, (struct sockaddr*)&client_address, &addrlen);
}