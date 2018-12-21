#include "net.h"

int main(int argc, char *argv[]) {
  set_server(inet_addr("127.0.0.1"), 3000);
}
