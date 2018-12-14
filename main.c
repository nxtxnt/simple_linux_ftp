#include "main.h"
#include "net.h"

int main(int argc, char *argv[]) {
  int client_pid;
  uint32_t arg_addr;                                                            //address argument
  uint16_t arg_port;                                                            //port argument
  void *arg[2] = {&arg_addr, &arg_port};                                        //pointer array to the arguments

  if(argc != 4)
  {
    write(STDIN_FILENO, MAIN_ERR_MSG, sizeof(MAIN_ERR_MSG));
    return -1;
  }

  if(argv[1][1] == 'c')
  {
    arg_addr = inet_addr(argv[2]);
    arg_port = atoi(argv[3]);
/**
    client_pid = clone((void*) set_client, sbrk(1024*10) + 1024*10,
                                          SIGCHLD | CLONE_VM, arg);**/
    set_client(arg);
  }

  else if(argv[1][1] == 's')
    set_server(inet_addr(argv[2]), atoi(argv[3]));

  //kill(client_pid, SIGTERM);
}
