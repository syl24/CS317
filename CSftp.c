#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "dir.h"
#include "usage.h"
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <string.h>
#include <strings.h>
#include "helper.h"
#include "server.h"
#include "handles.h"
#include <netdb.h>
#include <limits.h>
#include <arpa/inet.h>
#include "parsecmd.h"

/**
 * Initialize and run the Concurrent server
 * @param port to start the server on
 */
void initServer(int port)
{
  int sock = createSocket(port);
  struct sockaddr_in client_address;
  int len = sizeof(client_address);
  int connection, pid, bytes_read;

  while (1)
  {
    connection = accept(sock, (struct sockaddr *)&client_address, &len);
    int ip[4]; // ipv4 address
    char clientIP[256];
    getIP(connection, ip);
    sprintf(clientIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    printf("Server info: Client accepted connection from: %s\n", clientIP);
    memset(ip, 0, 4);
    memset(clientIP, 0, 256);
    char buffer[BUFFER_SIZE];
    Command *cmd = malloc(sizeof(Command));
    State *state = malloc(sizeof(State));
    pid = fork(); // fork the main process to create child process for each request
    memset(buffer, 0, BUFFER_SIZE);

    if (pid < 0)
    {
      fprintf(stderr, "Cannot create child process.");
      exit(EXIT_SUCCESS);
    }
    if (!pid)
    {
      // Child process handles the client requests
      printf("Server info: Created a child process for the new incoming request\n");
      close(sock);               // child does not need listener
      char initWelcome[BUFFER_SIZE] = "220"; // initialize connection with 220 code
      strcat(initWelcome, " - ");
      strcat(initWelcome, welcome_message);
      strcat(initWelcome, "\n");
      write(connection, initWelcome, strlen(initWelcome));
      memset(initWelcome, 0, BUFFER_SIZE);
      // Read command from Client
      while (bytes_read = read(connection, buffer, BUFFER_SIZE))
      {
        signal(SIGCHLD, handleZombie);
        if (bytes_read <= BUFFER_SIZE)
        {
          buffer[BUFFER_SIZE - 1] = '\0';
          printf("User %s sent command: %s\n", (state->username == 0) ? "unknown" : state->username, buffer);
          state->logged_in = state->logged_in == 1? 1: 0;
          state->connection = connection;
          int res = parseCommand(buffer, cmd);
          if (res == -1)
          {
            printf("Unable to parse command\n");
            // If unable to Parse command
            state->message = "500 - Unknown Command\n";
            writeState(state);
          } else {
            response(cmd, state);
          }
        }
        else
        {
          // Read error 
          perror("Server error");
        }
        printf("Server info: Freeing command args\n");
        memset(cmd->command, 0, 5);
        memset(cmd->command, 0, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE);
      }
      printf("Server info: Client disconnected.\n");
      exit(0);
    }
    else
    {
      // Created a new child process to handle client connection, Parent can close the accepted connection
      printf("Server info: Parent process closing connection.\n");
      close(connection); // Parent does not need this anymore
    }
  }
}

/**
 * Writes to the socket (state->connection), with the message state->message
 * @param State struct
 */
void writeState(State *state)
{
  write(state->connection, state->message, strlen(state->message));
  state->message = '\0'; // clear the message
}
//-------------------------------------------------------------------------------------------//


int main(int argc, char *argv[])
{

  int port;
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc

  // Check the command line arguments
  if (argc != 2)
  {
    usage(argv[0]);
    return -1;
  }
    if (getcwd(start_dir, sizeof(start_dir)) != NULL) {
        printf("Server info: Init working dir: %s\n", start_dir);
    }
  port = atoi(argv[1]);
  if (port < 1024 || port > 65535)
  {
    // port has to be in range
    usage(argv[0]);
    return -1;
  }

  // Initialize the server
  initServer(port);
  // This is how to call the function in dir.c to get a listing of a directory.
  // It requires a file descriptor, so in your code you would pass in the file descriptor
  // returned for the ftp server's data connection

  //printf("Printed %d directory entries\n", listFiles(1, "."));
  return 0;
}
