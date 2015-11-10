#include "fmacros.h"
#include "version.h"

#define BUFFER (1024*16) //16k
#define FIFO "/var/tmp/Redis-Thread.fifo"
int redis_transfer_thread();
