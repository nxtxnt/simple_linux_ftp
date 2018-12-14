#ifndef NETWORK_HEADER_FILE
#define NETWORK_HEADER_FILE

#include <sys/sendfile.h>                                                       //header needed for the sendfile() syscall
#include <sys/socket.h>                                                         //headers needed for TCP protocol
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>                                                             //types by size definitions
#include <string.h>                                                             //strcpy()
#include <stdio.h>

#define REQUEST_FILE_INSTRUCT     0x0                                           //the data contained in the data filed is a filename
#define ABORT_SEND_INSTRUCT       0x1                                           //abort the sending
#define CLOSE_INSTRUCTION         0x2                                           //close the connection
#define MESSAGE_INSTRUCTION       0x3                                           //the data contained in the data filed is a message
#define DATA_SEND_INSTRUCT        0x4                                           //the data contained in the data field is file data
#define SIZE_INSTRUCTION          0x5                                           //the data contained in the data field is the size of the file


#define CHUNK_MAX_SIZE 1300                                                     //1500 - 100 - sizeof(data_header)

typedef uint8_t  byte;                                                          //readable definitions for usefull types
typedef byte     type;
typedef uint32_t address;
typedef uint16_t port;

struct data_header {
  type        instruction   : 4;
  uint16_t    data_size     : 12;
};

struct chunk_info {
  uint32_t nchunk;                                                              //number of chunks
  uint16_t nremain;                                                             //number of bytes remaining at the end of the chunks
};

struct data_layer {
  struct data_header header;
  byte               packet_data[CHUNK_MAX_SIZE];
};

int set_server(address _addr, port _port);
int set_client(void *arg[]);

int server_connection_handler(int client_sock_desc);
int client_connection_handler(int server_sock_desc);
int open_file(char *name, struct stat *file_stat);
int send_file(int client_sock_desc, struct stat *file_stat, int file_desc);
int recieve_file(int client_sock_desc, int nchunk, int remainder);

#endif
