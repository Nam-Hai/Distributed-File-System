#include <stdio.h>
#include "udp.h"
#include "mfs.h"

// client code
int main(int argc, char *argv[])
{
    char *hostname = "localhost";
    // int sd = UDP_Open(20);
    int port = client_port;
    int fd = MFS_Init(hostname, port);

    MFS_Lookup(0, "test");
    MFS_Stat_t m;
    MFS_Stat(0, &m);
    MFS_Write(0, "buffer string", 2);

    MFS_Shutdown();
    UDP_Close(fd);
    return 0;
}
