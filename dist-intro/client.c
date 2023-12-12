#include <stdio.h>
#include "udp.h"
#include "mfs.h"

// client code
int main(int argc, char *argv[])
{
    char *hostname = "localhost";
    // int sd = UDP_Open(20);

    // int *pointer;
    // printf("pointer size %lu\n", sizeof(pointer));

    // printf("pointer size %lu\n", sizeof(MFS_Stat_t));

    int port = client_port;
    int fd = MFS_Init(hostname, port);
    if (fd == -1)
    {
        return 0;
    }
    MFS_Creat(0, MFS_DIRECTORY, "myDir");

    int inum = MFS_Lookup(0, "myDir");
    printf("lookup res %d\n", inum);

    MFS_Stat_t m;
    MFS_Stat(inum, &m);

    inum = MFS_Lookup(0, "fakeDir");
    printf("lookup res %d\n", inum);

    MFS_Stat(inum, &m);

    // MFS_Write(0, "buffer string", 2);

    // MFS_Shutdown();

    // UDP_Close(fd);
    return 0;
}
