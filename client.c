#include "main.h"
#include "net.h"

int set_client(void *arg[]) {
  address *_addr = arg[0];
  port    *_port = arg[1];
  byte *addr = (uint8_t*) _addr;

  int server_sock_desc, errno;
  struct sockaddr_in server;

  server_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
  if(server_sock_desc < 0)
  {
    error("could not create socket", errno = server_sock_desc);
    return errno;
  }

  server.sin_family      = AF_INET;
  server.sin_addr.s_addr = *_addr;
  server.sin_port        = htons(*_port);

  printf("attempting connection with %u.%u.%u.%u on port %u\n",
                                            (uint32_t) addr[0],
                                            (uint32_t) addr[1],
                                            (uint32_t) addr[2],
                                            (uint32_t) addr[3],
                                            (uint16_t) *_port);

  errno = connect(server_sock_desc, (struct sockaddr*) &server,
                                    (socklen_t) sizeof(struct sockaddr_in));
  if(errno < 0)
    error("could not connect", errno);

  client_connection_handler(server_sock_desc);
  close(server_sock_desc);
}

int client_connection_handler(int server_sock_desc)
{
  uint32_t nchunk, remainder;
  int nbytes, errno;
  char filename[256];
  struct data_layer to_server;
  struct chunk_info *chunk_data = (struct chunk_info*) &to_server.packet_data;

  write(STDIN_FILENO, "filename > ", 11);
  scanf("%s", filename);
  strcpy((char*) &to_server.packet_data, filename);

  to_server.header.instruction = REQUEST_FILE_INSTRUCT;
  to_server.header.data_size = strlen(filename);

  errno = send(server_sock_desc,
               &to_server,
               sizeof(struct data_header) + strlen(filename),
               0);
  if(errno < 0)
  {
    error("send() syscall", errno);
    return errno;
  }

  nbytes = read(server_sock_desc, &to_server, sizeof(struct data_header));
  if(nbytes != sizeof(struct data_header))
  {
    error("server made illegal use of protocol", nbytes);
    return -1;
  } else if(nbytes < 0)
  {
    error("read() syscall", errno = nbytes);
    return errno;
  }

  switch(to_server.header.instruction) {

    case SIZE_INSTRUCTION :
      read(server_sock_desc, chunk_data, sizeof(struct chunk_info));
      nchunk    = chunk_data->nchunk;
      remainder = chunk_data->nremain;
      recieve_file(server_sock_desc, nchunk, remainder);
      break;

    case MESSAGE_INSTRUCTION :

    case CLOSE_INSTRUCTION :

    default :
      error("server made illegal use of protocol", to_server.header.instruction);
      return -1;
   }


}

int recieve_file(int server_sock_desc, int nchunk, int remainder) {
  char buff[CHUNK_MAX_SIZE];
  int errno, i = 0;

  while(nchunk--)
  {
    errno = read(server_sock_desc, &buff, CHUNK_MAX_SIZE);
    if(errno != CHUNK_MAX_SIZE)
    {
      if(errno > 0)
      {
        int bytes_left = CHUNK_MAX_SIZE-errno;
        while(bytes_left)
          bytes_left -= read(server_sock_desc, &buff, bytes_left);
      } else {
        error("read returned error", errno);
        return -1;
      }
    }
    i++;
  }

  printf("%d blocks read\n", i);

  errno = read(server_sock_desc, &buff, remainder);
  if(errno != remainder)
  {
    if(errno > 0)
    {
      int bytes_left = remainder-errno;
      while(bytes_left)
        bytes_left -= read(server_sock_desc, &buff, bytes_left);
    } else {
      error("read returned error on the last block", errno);
      return -1;
    }
  }
  printf("file recieved\n");
}
