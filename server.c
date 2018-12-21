#include "net.h"

/** simple tcp server **/

int set_server(address _addr, port _port)
{
  int server_sock_desc,
      errno;

  struct sockaddr_in server;

  server_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
  if(server_sock_desc < 0)
  {
          printf("%s %d\n", CREATING_SOCKET, errno = server_sock_desc);
          return -1;
  }

  server.sin_family        = AF_INET;
  server.sin_addr.s_addr   = _addr;
  server.sin_port          = htons(_port);

  errno = bind(server_sock_desc, (struct sockaddr*) &server,
                                  sizeof(struct sockaddr_in));
  if(errno < 0)
  {
          printf("%s %d\n", BINDING_SOCKET, errno);
          return -1;
  }

  errno = listen(server_sock_desc, PEER_NUMBER);
  if(errno < 0)
  {
          printf("%s %d\n", LISTEN_SOCKET, errno);
          return -1;
  }

  errno = serv_connect_handler(&server, server_sock_desc);
  if(errno < 0)
  {
          return -1;
  }

  return 0;
}

/** The connection handler forks into a child process on each new connection to
    ensure clients can be dealt with in parallel **/

int serv_connect_handler(struct sockaddr_in *server, int server_sock_desc)
{
  int client_sock_desc,
      errno,
      pid = 0;

  void *ptr = &&next;

  socklen_t cli_len = sizeof(struct sockaddr_in);

  for(;;) {
          client_sock_desc = accept(server_sock_desc,
                 (struct sockaddr*) server,
                                   &cli_len);
          if(client_sock_desc < 0)
          {
                  printf("%s %d", ACCEPT_CONNECT, errno = client_sock_desc);
                  continue;
          }

          pid = fork();

          next :
          printf("address : %llx", (long long) ptr);
          if(pid < 0)
                  printf("error on fork");

          if(pid == 0)                                                          // pid should be 0 for the child process
          {
                  printf("thread %ld : a client has connected\n",
                          syscall(SYS_gettid));
                  close(server_sock_desc);
                  serv_instruct_handler(client_sock_desc);
                  close(client_sock_desc);
                  exit(0);                                                      // when the child finishes it simply exits
          }

          else if(pid > 0)
                  close(client_sock_desc);                                      // parent process closes the descriptor for it doesn't need it
  }

}


int serv_instruct_handler(int client_sock_desc)
{
  struct header server_response,
                client_response;

  struct stat file_stat;

  int curr_dir = open(".", O_RDONLY | O_DIRECTORY),
      nbytes,
      fd,
      err;

  for(;;) {
          nbytes = read(client_sock_desc,
                       &client_response.instruct,
                        sizeof(struct instruct));
          if(nbytes != sizeof(struct instruct))
          {
                  printf("%s %s\n", SERV_ILLEG_MSG, CLOSING);
                  close(client_sock_desc);
                  return -1;
          }

          switch(client_response.instruct.instruction) {

          case LS_REQUEST :
                  nbytes = make_list(&server_response.client_list,
                                    &curr_dir);
          if(nbytes < 0)
          {
                  server_response.instruct.instruction = ERROR_RESPONSE;

                  server_response.error.error          = OPEN_DIR;
                  server_response.error.errno          = err = nbytes;          // remember make_list() returns the error in case of failure

                  err = send(client_sock_desc,
                            &server_response,
                            sizeof(struct instruct) +
                            sizeof(struct error),
                            0);
                 if(err < 0)
                 {
                        printf("%s %d\n %s\n",
                                SEND_ERROR,
                                err,
                                CLOSING);
                        close(client_sock_desc);
                        return -1;
                 }
          } else {
                  server_response.instruct.instruction = ACCPT_RESPONSE;
                  server_response.instruct.data_size   = nbytes;

                  err = send(client_sock_desc,
                            &server_response,
                             sizeof(struct instruct) + nbytes,
                             0);
                  if(err < 0)
                  {
                          printf("%s %d\n %s\n",
                                  SEND_ERROR,
                                  err,
                                  CLOSING);
                          close(client_sock_desc);
                          return -1;
                  }
          }

          case CD_REQUEST :
                nbytes = read(client_sock_desc,
                              client_response.file.path_or_filename,
                              client_response.instruct.data_size);
                if(nbytes != client_response.instruct.data_size)
                {
                        printf("%s\n", SERV_ILLEG_MSG);
                        close(client_sock_desc);
                        return -1;
                }

                curr_dir = change_dir(client_response.file.path_or_filename);
                if(curr_dir < 0)
                {
                        server_response.instruct.instruction = ERROR_RESPONSE;

                        server_response.error.error          = OPEN_DIR;
                        server_response.error.errno          = err = curr_dir;

                        err = send(client_sock_desc,
                                  &server_response,
                                   sizeof(struct instruct) +
                                   sizeof(struct error),
                                   0);
                        if(err < 0)
                        {
                                printf("%s %d\n %s\n",
                                        SEND_ERROR,
                                        err,
                                        CLOSING);
                               close(client_sock_desc);
                               return -1;
                        }
                } else {
                        server_response.instruct.instruction = ACCPT_RESPONSE;
                        server_response.instruct.data_size   = 0;

                        err = send(client_sock_desc,
                                  &server_response,
                                   sizeof(struct instruct),
                                   0);
                        if(err < 0)
                        {
                                printf("%s %d\n %s\n",
                                        SEND_ERROR,
                                        err,
                                        CLOSING);
                                close(client_sock_desc);
                                return -1;
                        }
                }

        case FILE_REQUEST :
                nbytes = read(client_sock_desc,
                             &client_response.file,
                              client_response.instruct.data_size);
                if(nbytes != client_response.instruct.data_size)
                {
                        printf("%s\n", SERV_ILLEG_MSG);
                        close(client_sock_desc);
                        return -1;
                }

                fd = check_file(client_response.file.path_or_filename,
                               &file_stat);
                if(fd < 0)
                {
                        server_response.instruct.instruction = ERROR_RESPONSE;

                        server_response.error.error          = OPEN_FILE;
                        server_response.error.errno          = err = fd;

                        err = send(client_sock_desc,
                                  &server_response,
                                   sizeof(struct instruct) +
                                   sizeof(struct error),
                                   0);
                        if(err < 0)
                        {
                                printf("%s %d\n %s\n",
                                        SEND_ERROR,
                                        err,
                                        CLOSING);
                                close(client_sock_desc);
                                return -1;
                        }
                } else {
                        send_file(client_sock_desc, &file_stat, fd);
                }

                default :
                        printf("%s\n", SERV_ILLEG_MSG);
                }
        }
}

int make_list(file_list *list, int *curr_dir)
{
  int errno,
      nbytes,
      byte_pos = 0;

  struct linux_dirent *dir_ptr;

  char *dir_raw_data;

  FORCE_SBRK_ALLOC(dir_raw_data, GET_DIR_BUFF_SIZE);                                // we make sure we got those 1 mega bytes
  nbytes = syscall(SYS_getdents, dir_raw_data, list, GET_DIR_BUFF_SIZE);

  dir_ptr = (struct linux_dirent*) dir_raw_data;

  while(dir_ptr < (struct linux_dirent*) ((unsigned long long) dir_raw_data +
                                                               nbytes)) {
          memcpy(&curr_dir[byte_pos], &dir_ptr->d_name, dir_ptr->d_reclen);
          byte_pos += dir_ptr->d_reclen;
          dir_ptr += dir_ptr->d_reclen;
  }

  return nbytes;
}

int change_dir(char dir_name[])
{
  int fd,
      errno;

  fd = open(dir_name, O_RDONLY | O_DIRECTORY);
  if(fd < 0)
  {
          printf("%s\n", OPEN_DIR_ERR);
          return errno = fd;
  }

  return fd;
}

int check_file(char *name, struct stat *file_stat)
{
  int id,
      fd;

  stat(name, file_stat);

  id = syscall(SYS_gettid);

  fd = open(name, O_RDONLY);
  if(fd < 0)
  {
          printf("%s\n", OPEN_FILE_ERR);
          return -1;
  }
  else
          return fd;
}

int send_file(int client_sock_desc, struct stat *file_stat, int file_desc)
{
  int err;
  struct header server_response;

  server_response.instruct.instruction = SEND_INSTRUCT;

  server_response.chunk_data.nchunk    = file_stat->st_size / CHUNK_MAX_SIZE;
  server_response.chunk_data.nremain   = file_stat->st_size % CHUNK_MAX_SIZE;

  err = send(client_sock_desc,
            &server_response,
            sizeof(struct instruct) + sizeof(struct chunk_data),
            0);
  if(err < 0)
  {
          printf("%s %d\n %s\n", SEND_ERROR, err, CLOSING);
          close(client_sock_desc);
          return -1;
  }

  do {
          err = sendfile(client_sock_desc, file_desc, 0, CHUNK_MAX_SIZE);
          if(err < 0)
          {
                printf("%s %d\n %s\n", SEND_ERROR, err, CLOSING);
                close(client_sock_desc);
                return -1;
          }
  } while(server_response.chunk_data.nchunk--);

  err = sendfile(client_sock_desc,
                 file_desc,
                 0,
                 server_response.chunk_data.nremain);
  if(err < 0)
  {
          printf("%s %d\n %s\n", SEND_ERROR, err, CLOSING);
          close(client_sock_desc);
          return -1;
  }

  return 0;
}
