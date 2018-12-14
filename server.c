#include "main.h"
#include "net.h"

int set_server(address _addr, port _port)
{
    byte *addr = (byte*) &_addr;

    int server_sock_desc, client_sock_desc, errno;
    struct sockaddr_in server, client;
    socklen_t cli_len;

    server_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock_desc < 0) {
      error("creating socket", errno = server_sock_desc);
      close(server_sock_desc);
      return errno;
    }

    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = _addr;
    server.sin_port        = htons(_port);

    errno = bind(server_sock_desc, (struct sockaddr*) &server,
                                   (socklen_t) sizeof(struct sockaddr_in));

    if(errno < 0) {
      error("binding socket", errno);
      close(server_sock_desc);
      return errno;
    }

    errno = listen(server_sock_desc, 1);

    if(errno < 0) {
      error("listening", errno);
      close(server_sock_desc);
      return errno;
    }

    printf("listening for incomming connections on %u.%u.%u.%u on port %u\n",
                                                          (uint32_t) addr[0],
                                                          (uint32_t) addr[1],
                                                          (uint32_t) addr[2],
                                                          (uint32_t) addr[3],
                                                          (uint16_t) _port);

    cli_len = sizeof(struct sockaddr_in);

    client_sock_desc = accept(server_sock_desc, (struct sockaddr*) &client,
                                                                   &cli_len);

    if(client_sock_desc < 0) {
      error("accept", errno = client_sock_desc);
      close(server_sock_desc);
      close(client_sock_desc);
      return errno;
    }

    printf("a client has connected\n");
    server_connection_handler(client_sock_desc);

    close(client_sock_desc);
    close(server_sock_desc);
}

int server_connection_handler(int client_sock_desc)
{                                                                               //handles the connection between the server and the client
  struct data_layer client_data;                                                //the data layer is described in the net.h header
  char *char_data = (char*) &client_data.packet_data;                           //pointer to packet_data that serves to read either filename or message
  int errno, file_desc, nbytes = 0;
  struct stat file_stat;

  while(1) {

    nbytes = read(client_sock_desc, &client_data.header, 2);
    if(nbytes != sizeof(struct data_header))                                    //if the size of the data read doesn't match that of the header
    {                                                                           //or if the client has abrubtly disconnected
      error("client sent illegal data format, disconnecting", 0);
      close(client_sock_desc);
      return 0;
    }

    switch(client_data.header.instruction) {                                    //reading the packet's instruction

      case REQUEST_FILE_INSTRUCT :                                              //see net.h

        nbytes = read(client_sock_desc, &client_data.packet_data,
                                        client_data.header.data_size);
        if(nbytes != client_data.header.data_size)
        {                                                                       //if the number of bytes read does not match the size specified in the header
          error("client sent illegal data format, disconnecting", 0);
          close(client_sock_desc);
          return -1;
        }

        char_data[client_data.header.data_size] = 0;                            //sets up a null character at the end of the string

        file_desc = open_file((char*) &client_data.packet_data, &file_stat);
        if(file_desc < 0)
            return -1;

        send_file(client_sock_desc, &file_stat, file_desc);
        return 0;
    }
  }
}

int open_file(char *filename, struct stat *file_stat)
{
  int file_desc, errno;
  char *buff_ptr;

  stat(filename, file_stat);

  file_desc = open(filename, O_RDONLY);
  if(file_desc < 0)
  {
    error("could not read file", errno = file_desc);
    return -1;
  }

  return file_desc;
}

int send_file(int client_sock_desc, struct stat *file_stat, int file_desc)
{
  int nchunk, nremainder, errno, block_count = 0;;
  struct data_layer client_file_info;
  struct chunk_info *client_chunk_data =
                    (struct chunk_info*) &client_file_info.packet_data;

  nchunk                               = file_stat->st_size / CHUNK_MAX_SIZE;
  nremainder                           = file_stat->st_size % CHUNK_MAX_SIZE;

  client_file_info.header.instruction  = SIZE_INSTRUCTION;
  client_file_info.header.data_size    = CHUNK_MAX_SIZE;
  client_chunk_data->nchunk            = nchunk;                                //number of chunks the client has to read
  client_chunk_data->nremain           = nremainder;                            //size of the remaining data

  errno = send(client_sock_desc,
               &client_file_info,
               sizeof(struct data_header) + sizeof(struct chunk_info),
               0);
  if(errno < 0)
  {
    error("send() syscall", errno);
    return errno;
  }

  printf("sending %d blocks, %ld bytes\n", nchunk, file_stat->st_size);

  while(nchunk--)
  {
    errno = sendfile(client_sock_desc, file_desc, 0, CHUNK_MAX_SIZE);
    if(errno != CHUNK_MAX_SIZE)
    {
      error("sendfile() syscall", errno);
      printf("on block %d\n", block_count);
    }

    block_count++;
  }

  errno = sendfile(client_sock_desc, file_desc, 0, nremainder);
  if(errno != nremainder)
  {
    error("sendfile() syscall on last block", errno);
    return errno;
  }

  return 0;
}
