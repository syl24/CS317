#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <limits.h>
#include <dirent.h>
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif
/**
 * Connection mode
 * NORMAL - normal connection mode, nothing to transfer
 * SERVER - passive connection (PASV command), server listens
 */
typedef enum conn_mode
{
  NORMAL,
  SERVER
} conn_mode;

// Enumeration of the commands supported by the server
typedef enum cmdlist
{
  CWD,
  NLST,
  PASV,
  QUIT,
  RETR,
  TYPE,
  USER,
  CDUP,
  STRU,
  MODE,
} cmdlist;

// Mapping of the strings to cmdlist
static const char *cmdlist_str[] =
    {
        "CWD", "NLST", "PASV", "QUIT", "RETR",
        "TYPE", "USER", "CDUP", "STRU", "MODE"};

// Valid usernames for the server
static const char *usernames[] =
    {
        "cs317",
};

typedef struct Command
{
  char command[5]; // size of a supported command (1st input from CLI)
  char arg[BUFFER_SIZE]; // Argument passed in (2nd input from CLI)
  int cmdIndex; // index in cmdlist where cmd is present
} Command;

typedef struct Port
{
  int p1; // 1st number of pasv connection (multiple of 256)
  int p2; // 2nd  number of pasv connection
} Port;

typedef struct State
{
  /* Connection mode: NORMAL, SERVER */
  int mode;
  // Flag if user is logged in
  int logged_in;
  char *username;
  // Response message to client
  char *message;
  // Command connections
  int connection;
  // File Descriptor of socket for Pasv connection
  int sock_pasv;
} State;


static char *welcome_message = "Welcome to my FTP server!";
char start_dir[PATH_MAX]; // Starting directory of server