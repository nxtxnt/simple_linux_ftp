/**
    GNU General Public License V3
    COPYRIGHT (c) 2018 Laurent Engelbrecht

    this file contains the declaration of functions and data-structures related
    to the the file transfer's protocol

    These include :

    1 - header files
    2 - macro definitions relative to instructions, sizes, messages and errnos
    3 - client and server related data structures
    4 - client and server functions
**/


#ifndef NETWORK_HEADER_FILE
#define NETWORK_HEADER_FILE


#ifndef NET_INCLUDES
#define NET_INCLUDES

#define _GNU_SOURCE
#include <sys/sendfile.h>                                                       // header needed for the sendfile() syscall
#include <sys/syscall.h>                                                        // getdents doesn't have a proper function wrapper
#include <sys/socket.h>                                                         // headers needed for TCP protocol
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>                                                             // malloc()
#include <stdint.h>                                                             // types by size definitions
#include <string.h>                                                             // strcpy()
#include <unistd.h>
#include <dirent.h>                                                             // getdents system call
#include <signal.h>                                                             // SIGCHLD
#include <fcntl.h>                                                              // open() syscall
#include <sched.h>                                                              // clone syscall

#include <stdio.h>

#endif

/** Definitions of macros related to client and server network operations.
    These macros define the type of instructions used by client and server.
    The instructions are referenced within instruct's instruction bit-field **/

#ifndef INSTRUCTIONS
#define INSTRUCTIONS

#ifndef CLIENT_INSTRUCTIONS
#define CLIENT_INSTRUCTIONS

#define LS_REQUEST                0x1                                           // asks to list the files in current directory
#define CD_REQUEST                0x2                                           // asks to change directory
#define FILE_REQUEST              0x3                                           // the data contained in the data filed is a filename
#define CLOSE_INSTRUCT            0x4                                           // informs the peer the connection is closing
#define ABORT_INSTRUCT            0x5                                           // asks to abort the send

#endif


#ifndef SERVER_INSTRUCTIONS
#define SERVER_INSTRUCTIONS

#define SIZE_INSTRUCT             0x1                                           // the data contained in the data field is the size of the file to be sent
#define MSG_INSTRUCT              0x2                                           // the data contained in the data filed is a message
#define SEND_INSTRUCT             0x3                                           // the data contained in the data field is file data
#define ACCPT_RESPONSE            0x4                                           // the instruction can be executed
#define ERROR_RESPONSE            0x5                                           // the data contained in the data field is an error number

#endif

#ifndef SUB_ROUTINES
#define SUB_ROUTINES

#define FORCE_SBRK_ALLOC(ptr, size) \
                do {                          \
                        ptr = sbrk(size);     \
                } while(!ptr);

#define FORCE_SBRK_FREE(ptr, size) \
                void *comp = ptr;            \
                do {                         \
                        ptr = sbrk(-size);   \
                } while(ptr != comp - size);

#endif

#endif


#ifndef SIZES
#define SIZES

/** The chunks are meant to fit in a 1500 bytes ethernet layer, the protocol's
    header thus has the same size **/

#define CHUNK_MAX_SIZE    1400
#define HEADER_MAX_SIZE   1400
#define MSG_MAX_SIZE      131071                                                // 2^17 - 1
#define PEER_NUMBER       100                                                   // increase on need
#define GET_DIR_BUFF_SIZE 1024*1024                                             // we assume 1 mega byte is enough to store the current directory's data

#endif


/** messages to be issued as error inside the application or to the client **/

#ifndef MESSAGES
#define MESSAGES

#ifndef ERROR_MESSAGES
#define ERROR_MESSAGES

#define SET_SERVER_ERR  "ERROR : SETTING SERVER, errno = "
#define SET_CLIENT_ERR  "Error : SETTING CLIENT, errno = "
#define CREATING_SOCKET "Error : COULD NOT CREATE SOCKET, errno = "
#define BINDING_SOCKET  "Error : COULD NOT BIND SOCKET, errno = "
#define LISTEN_SOCKET   "Error : COULD NOT LISTEN ON SOCKET, errno = "
#define ACCEPT_CONNECT  "Error : COULD NOT ACCEPT CONNECTION, errno = "
#define CONNECT_TO_SERV "Error : COULD NOT CONNECT TO SERVER, errno = "
#define SERV_ILLEG_MSG  "Error : CLIENT MADE ILLEGAL USE OF PROTOCOL"
#define OPEN_DIR_ERR    "Error : COULD NOT OPEN DIRECTORY"
#define OPEN_FILE_ERR   "Error : FILE BELONGS TO ANOTHER USER"
#define SEND_ERROR      "Error : COULD NOT SEND DATA, errno = "
#define CLOSING         "Closing connection"

#endif

#endif


#ifndef ERRNOS
#define ERRNOS

#define OPEN_DIR  0x1
#define OPEN_FILE 0x2

#endif


/** useful type definitions **/

typedef uint8_t  byte;
typedef uint8_t  type;
typedef uint16_t port;
typedef uint32_t address;
typedef char*    file_list;


/** parameter's instruction's data structures are defined here, these data
    structures are typically used as pointers to header data structure
    as the instruct layer **/

#ifndef INSTRUCTION_PARAMS
#define INSTRUCTION_PARAMS

/** used to reference filenames and paths **/

struct file {
        char path_or_filename[256];
};

/** gives the number of chunks and the remaining data **/

struct chunk_data {
        uint32_t nchunk;                                                              // number of chunks
        uint16_t nremain;                                                             // number of bytes remaining at the end of the chunks
};

/** message to the client from the server **/

struct message {
        char msg[MSG_MAX_SIZE];
};

/** error response from the server **/

struct error {
        type error;
        int  errno;
};

#endif


/** used to transfer instructions and requests between the client and the
    server, is used as pointer to the header data structure **/

struct instruct {
        type        instruction   : 3;
        uint32_t    data_size     : 21;                                               // gives the size  or number of the following instruction parameters if any, becomes
};                                                                              // a parameter when the message instruction is used

/** the data transfer layer which may contains file data, or instruction header
    and parameters if any.

                        HEADER
        ________________________________________
       | INSTRUCTION |        DATA_SIZE        |  <-- INSTRUCT data structure
       |---------------------------------------|
       |  DATA,  ERROR STRUCTURE,  FILE/PATH,  |  <-- DATA, ERROR STRUCT or
       |_______________________________________|      FILE/PATH data structure
**/


struct header {

        union {
                struct instruct instruct;
                byte full_packet_data[0];
        };

        union {
                struct error      error;
                struct chunk_data chunk_data;
                struct message    message;
                struct file       file;
                file_list         client_list;
                byte packet_data[CHUNK_MAX_SIZE - sizeof(struct instruct)];
        };
};


#ifndef SERVER_FUNCTIONS
#define SERVER_FUNCTIONS

#ifndef LINUX_DIRENT_STRUCT
#define LINUX_DIRENT_STRUCT

/** data structure that will be used as a pointer to travel through the
    directory buffer **/

struct linux_dirent {
        long           d_ino;
        off_t          d_off;
        unsigned short d_reclen;
        char           d_name[];
};

#endif

/** sets up the server **/

int set_server(address _addr, port _port);

/** ensures connection for one or multiple clients **/

int serv_connect_handler(struct sockaddr_in *server, int server_sock_desc);

/** reads and treats client instructions **/

int serv_instruct_handler(int client_sock_desc);

/** lists the files into a char array referencend within the file_list
    structure (which is itself a pointer to the header data structure)
    and returns the size of the array **/

int make_list(file_list *list, int *curr_dir);

/** changes directory **/

int change_dir(char dir_name[]);

/** checks if the requested file exists and if the user has the right to
    access it. If successful, check_file fills up the stat structure and returns
    the file's file descriptor. Otherwise returns an error. **/

int check_file(char *name, struct stat *file_stat);

/** sends the requested file to client **/

int send_file(int client_sock_desc, struct stat *file_stat, int file_desc);

#endif


#ifndef CLIENT_FUNCTIONS
#define CLIENT_FUNCTIONS

/** sets up the client's connection with the server **/

int set_client(address _addr, port _port);

/** handles the sending of instructions to the server and the reception of
    server responses **/

int client_connection_handler(struct sockaddr_in *server, int server_sock_desc);

/** handles the reception of the filenames **/

int recieve_file_list(int server_sock_desc, file_list *list, int size);

/** handles the reception of files **/

int recieve_file(int server_sock_desc, int nchunk, int remainder);

#endif


#endif
