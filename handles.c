#include "helper.h"
#include "server.h"
#include "handles.h"
#include "parsecmd.h"
#include "dir.h"
#include <strings.h>
#include <sys/sendfile.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>

/**
 * Handles client request and generates response message for client
 * @param Command: Current cmd from client
 * @param State: Current connection state from client
 */
void response(Command* cmd, State* state)
{
    printf("Parsed command is: %s\n", cmd->command);
    switch (cmd->cmdIndex) {
    case USER:
        ftpUser(cmd, state);
        break;
    case PASV:
        ftpPasv(cmd, state);
        break;
    case CWD:
        ftpCwd(cmd, state);
        break;
    case CDUP:
        ftpCdup(state);
        break;
    case NLST:
        ftpNlst(cmd, state);
        break;
    case STRU:
        ftpStru(cmd, state);
        break;
    case MODE:
        ftpMode(cmd, state);
        break;
    case RETR:
        ftpRetr(cmd, state);
        break;
    case QUIT:
        ftpQuit(state);
        break;
    case TYPE:
        ftpType(cmd, state);
        break;
    default:
        // Shouldn't reach here
        state->message = "500 - Unknown command.\n";
        writeState(state);
        break;
    }
}

/** 
 * Handle STRU command.
 */
void ftpStru(Command* cmd, State* state)
{
    printf("Server info: STRU command\n");
    if (state->logged_in) {
        if (strcasecmp(cmd->arg, "F") == 0) {
            state->message = "200 - Successfully set structure to FILE.\n";
        }
        else {
            if (strcasecmp(cmd->arg, "P") == 0) {
                state->message = "504 - Page structure is not supported, only File.\n";
            }
            else {
                if (strcasecmp(cmd->arg, "R") == 0) {
                    state->message = "504 - Record structure is not supported, only File.\n";
                }
                else {
                    state->message = "501 - Bad command argument supplied.\n";
                }
            }
        }
    }
    else {
        state->message = "530 - Please login with USER.\n";
    }
    writeState(state);
}

/** 
 * Handle CDUP command.
 *  Only need state as no arguments are needed
 */
void ftpCdup(State* state)
{
    printf("Server info: CDUP command\n");
    if (state->logged_in) {
        char cwd[PATH_MAX];
        memset(cwd, 0, sizeof(cwd));
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Server info: Current working dir: %s\n", cwd);
            printf("Server info: Start dir: %s \n", start_dir);
            if (strcasecmp(start_dir, cwd) == 0) {
                // if start dir is equal to the current directory don't allow
                state->message = "550 - Requested action is not allowed. Unauthorized to access parent of root directory.\n";
            }
            else {
                if (chdir("..") == 0) {
                    state->message = "200 - Directory successfully changed to parent directory.\n";
                }
                else {
                    state->message = "550 - Requested action could not be taken. Failed to change to parent directory.\n";
                }
            }
        }
        else {
            state->message = "550 - Requestion action could not be taken. Failed to determine current directory.\n";
        }
        memset(cwd, 0, sizeof(cwd));
    }
    else {
        state->message = "530 - Please login with USER.\n";
    }
    writeState(state);
}

/** 
 * Handle NLST command.
 */
void ftpNlst(Command* cmd, State* state)
{
    printf("Server info: NLST Command\n");
    char curDir[BUFFER_SIZE];
    memset(curDir, 0, BUFFER_SIZE);
    if (state->logged_in) {
        if (state->mode == SERVER) {
            if (cmd->arg != NULL && cmd->arg[0] != '\0') {
                // argument passed in is  non null or non empty
                state->message = "501 - Requested action could not be taken. Server does not support parameter for NLST.\n";
            }
            else {
                if (cmd->arg != NULL && cmd->arg[0] == '\0') {
                    // must have no argument supplied
                    int connection; // pasv connection
                    connection = acceptConnection(state->sock_pasv);
                    printf("Server info: Successfully accepted pasv connection\n");
                    state->message = "150 - File status is valid. Here comes the directory listing.\n";
                    writeState(state);
                    printf("Server info: Preparing to print names of files of current directory.\n");
                    getcwd(curDir, BUFFER_SIZE);
                    listFiles(connection, curDir);
                    close(connection);
                    close(state->sock_pasv);
                    printf("Server info: Files sent successfully to pasv connection. Successfully Closed connection.\n");
                    state->mode = NORMAL; // change back to normal mode
                    printf("Server info: Set mode back to NORMAL.\n");
                    state->message = "226 - Directory send successful. Closing connection.\n";
                }
                else {
                    state->message = "501 - Requested action could not be taken. Bad command.\n";
                }
            }
        }
        else {
            state->message = "425 - Data connection not open. Please Use PASV first.\n";
        }
    }
    else {
        state->message = "503 - Please login with USER first\n";
    }
    writeState(state);
}

/** 
 * Handle MODE command.
 */
void ftpMode(Command* cmd, State* state)
{
    printf("Server info: MODE command\n");
    if (state->logged_in) {
        if (strcasecmp(cmd->arg, "S") == 0) {
            state->message = "200 - Mode is valid. Mode set to S.\n";
        }
        else {
            if (strcasecmp(cmd->arg, "C") == 0) {
                state->message = "504 - MODE Compressed is not supported.\n";
            }
            else {
                if (strcasecmp(cmd->arg, "B") == 0) {
                    state->message = "504 - MODE Block is not supported.\n";
                }
                else {
                    state->message = "501 - Bad command. Unknown MODE.\n";
                }
            }
        }
    }
    else {
        state->message = "530 - Please login with USER\n";
    }
    writeState(state);
}

/** 
 * Handle USER command.
 */
void ftpUser(Command* cmd, State* state)
{
    //usernames[0] is cs317, refer to helper.h usernames[]
    printf("Server info: Called USER\n");
    printf("User loggin state %d \n", state->logged_in);
    if (state->logged_in) {
        state->message = "530 - Already logged in. Cannot change user when logged in.\n";
    }
    else {
        int res = strcasecmp(cmd->arg, usernames[0]); // case insensitive compare
        if (res == 0) {
            state->username = malloc(40); // max size of a username
            memset(state->username, 0, 40);
            strcpy(state->username, cmd->arg);
            state->logged_in = 1;
            state->message = "230 - Login successful.\n";
        }
        else {
            state->message = "530 - Invalid username. cs317 is the only valid username.\n";
        }
    }
    writeState(state);
}


/** 
 * Handle PASV command.
 */
void ftpPasv(Command* cmd, State* state)
{
    int ip[4]; // ipv4 address
    char messageBuf[256];
    memset(ip, 0, 4);
    memset(messageBuf, 0, 256);
    int isSockCreate;
    Port* port = malloc(sizeof(Port));
    printf("Server info: PASV command\n");
    if (state->logged_in) {
        char* response = "227 - Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n";
        genPort(port);
        getIP(state->connection, ip);

        // Close previous passive socket if already created
        if (state->sock_pasv) {
            close(state->sock_pasv);
        }
        // Start listening here, but don't accept the connection
        printf("Server info: Random generated port 1: %d\n", port->p1);
        printf("Server info: Random generated port 2: %d\n", port->p2);
        state->sock_pasv = createSocket((256 * port->p1) + port->p2);
        printf("Server info: LISTENING on PASV port: %d\n", 256 * port->p1 + port->p2);
        sprintf(messageBuf, response, ip[0], ip[1], ip[2], ip[3], port->p1, port->p2);
        state->message = messageBuf;
        state->mode = SERVER; // set mode to SERVER (pasv)
        printf("Server info: %s\n", state->message);
    }
    else {
        state->message = "530 - Please login with USER.\n";
    }
    writeState(state);
    memset(ip, 0, 4);
    memset(messageBuf, 0, 256);
    free(port);
}

void ftpQuit(State* state)
{
    printf("Server info: Called QUIT\n");
    state->message = "221 - Goodbye.\n";
    writeState(state);
    close(state->connection);
    printf("Server info: Client disconnected.\n");
    exit(0);
}

/**
 * Checks the param if it is a valid path argument according to spec. E.g ./, ../, 
 * absolute path that does not contain starting dir, or blank arg is considered invalid
 * @param arg: Path to validate
 * @return a negative number if invalid, else positive
 * */

int checkPath(char* arg)
{
    printf("Checking the path arg\n");
    char* p1 = "./";
    char* p2 = "../";
    char* p3 = "..";
    char* p4 = ".";
    char* temp;
      if (arg != NULL && (arg[0] == '\0')) {
            printf("Server info: Command arg is blank\n");
            return -6;
                        // Failed to change directory
        }
    temp = strstr(arg, p2);
    if (temp) {
        printf("Arg contains ../\n");
        return -1;
    }
    else {
        // arg does not contain ../, check if it begins with ./ or is only . or ..
        temp = strstr(arg, p1);
        if (temp && temp == &arg[0]) {
            printf("Arg begins with ./\n");
            return -2;
        }
        if (strcmp(arg, p3) == 0) {
            printf("Arg is ..\n");
            return -3;
        }
        if (strcmp(arg, p4) == 0) {
            printf("Arg is .\n");
            return -4;
        }
        // check if arg is an absolute path
        temp = strstr(arg, "/");
        if (temp && temp == &arg[0]) {
            printf("Arg is an abosulte path\n");
            printf("Startin dir is %s\n", start_dir);
            // if arg is an absolute path check if it begins with the start_dir

            if (strcmp(start_dir, arg) == 0) {
                // if the path is the same as start dir
                return 1;
            }
            char start_dir_buff[256];
            memset(start_dir_buff, 0, 256);
            strcpy(start_dir_buff, start_dir);
            strcat(start_dir_buff, "/"); // append / at the end of start_dir
            printf("This is the start dir buff: %s\n", start_dir_buff);
            temp = strstr(arg, start_dir_buff);
            memset(start_dir_buff, 0, 256);
            if (temp && temp == &arg[0]) {
                printf("Valid Arg is an absolute path that starts with start_dir\n");
                return 1;
            }
            else {
                printf("Invalid Arg is an absolute path that does not start with start_dir\n");
                return -5;
            }
        }
        printf("Arg is valid\n");
        return 1; // valid cwd arg
    }
}

/** 
 * Handle CWD command.
 */
void ftpCwd(Command* cmd, State* state)
{
    if (state->logged_in) {
        printf("Server info: CWD called\n");
        printf("Server info: This the command arg cwd: %s\n", cmd->arg);
            printf("Server info: Checking the validity of command arg\n");
            int res = checkPath(cmd->arg); // check if the path supplied is valid or not

            switch (res) {
            case -1:
                state->message = "550 - Failed to change directory. Directory path cannot contain ../ \n";
                break;
            case -2:
                state->message = "550 - Failed to change directory. Directory path cannot begin with ./ \n";
                break;
            case -3:
                state->message = "550 - Failed to change directory. Directory Path cannot be ..\n";
                break;
            case -4:
                state->message = "550 - Failed to change directory. Directory Path cannot be .\n";
                break;
            case -5:
                state->message = "550 - Failed to change directory. Directory path cannot be a full path that does not begin with the starting directory\n";
                break;
                case -6:
                state->message = "501 - Syntax error. Failed to change directory.\n";
                break;
            case 1:
                printf("Server info: This is the valid cmd arg: %s\n", cmd->arg);
                if (chdir(cmd->arg) == 0) {
                    state->message = "250 - Directory successfully changed.\n";
                }
                else {
                    state->message = "550 - Failed to change directory.\n";
                }
                break;

            default:
                // shouldn't reach here
                state->message = "550 - Failed to change directroy. Bad path\n";
            }
    }
    else {
        state->message = "503 - Login with USER.\n";
    }
    writeState(state);
}

/** 
 * Handle RETR command.
 */

void ftpRetr(Command* cmd, State* state)
{

    printf("Server info: RETR Command\n");
    if (state->logged_in) {
        if (state->mode == SERVER) {
            int res = checkPath(cmd->arg); // check the path for the file to see if valid or not
            if (res < 0) {
                state->message = "550 - Requested action Failed. File path specified is unreachable. Please specify a different file\n";
            }
            else {
                int connection;
                int fd; // file descriptor from opening the file
                struct stat stat_buf;
                int sent_total = 0;
                off_t offset = 0;
                fd = open(cmd->arg, O_RDONLY);
                // Is file readable and open the file for read-only
                if (access(cmd->arg, R_OK) == 0 && fd != -1) {
                    fstat(fd, &stat_buf); // get file stats
                    state->message = "150 - File status valid. Opening BINARY mode data connection.\n";

                    writeState(state);

                    connection = acceptConnection(state->sock_pasv);
                    if (sent_total = sendfile(connection, fd, &offset, stat_buf.st_size)) {

                        if (sent_total != stat_buf.st_size) {
                            // Did not successfully transfer all data
                            perror("Could not transfer all data");
                            exit(EXIT_SUCCESS);
                        }

                        state->message = "226 - File send OK. Closing connection.\n";
                    }
                    else {
                        state->message = "550 - Requested action failed. Failed to read file.\n";
                    }
                    close(fd);
                    close(connection);
                }
                else {
                    // Unable to retrieve file
                    state->message = "550 - Requested action failed. Failed to get file.\n";
                }
                close(state->sock_pasv);
                state->mode = NORMAL; // set mode back to normal after accepting connection
            }
        }
        else {
            state->message = "425 - Data connection not open. Please use PASV first.\n";
        }
    }
    else {
        state->message = "530 - Please login with USER.\n";
    }
    writeState(state);
}

/** 
 * Handle TYPE command.
 * BINARY and ASCII.
 */
void ftpType(Command* cmd, State* state)
{
    if (state->logged_in) {
        printf("Server called TYPE %s\n", cmd->arg);

        if (strcasecmp(cmd->arg, "I") == 0) {
            state->message = "200 - Switching to Binary mode.\n";
        }
        else if (strcasecmp(cmd->arg, "A") == 0) {
            /* Type A must be always accepted according to RFC */
            state->message = "200 - Switching to ASCII mode.\n";
        }
        else {
            if (cmd->arg != NULL && (cmd->arg[0] == '\0')) {
                state->message = "501 - Syntax Error.\n";
            }
            else {
                state->message = "504 - Type not implemented for that parameter.\n";
            }
        }
    }
    else {
        state->message = "530 - Please login with USER.\n";
    }
    writeState(state);
}
