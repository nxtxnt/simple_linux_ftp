#include "main.h"

int error(char *message, errno err_type) {
  printf("Error : %s, error code 0x%xx\n", message, err_type);
}
