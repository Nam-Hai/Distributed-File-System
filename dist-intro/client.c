#include <stdio.h>
#include "udp.h"
#include "mfs.h"

// client code
int main(int argc, char *argv[])
{
    UDP_Close(client_port);

    char *hostname = "localhost";
    // int sd = UDP_Open(20);
    int port = client_port;
    int fd = MFS_Init(hostname, port);

    MFS_Lookup(0, "test");

    UDP_Close(fd);
    return 0;
}
