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
    char buffer[SIZE_BLOCK];

    if (fd == -1)
    {
        return 0;
    }

    // MFS_Stat_t m;
    // MFS_Stat(inum, &m);

    int inum = MFS_Lookup(0, "foo");
    MFS_Read(inum, buffer, 0);
    printf("foo : %s\n", buffer);

    MFS_Write(inum, "Jecris une nouvelle version de foo", 0);
    inum = MFS_Lookup(0, "foo");
    MFS_Read(inum, buffer, 0);
    printf("foo after rewrite : %s\n", buffer);

    MFS_Creat(0, MFS_DIRECTORY, "myDir");

    inum = MFS_Lookup(0, "myDir");

    MFS_Stat_t m;
    MFS_Stat(inum, &m);
    printf("Stat ==============> type %d, size %d\n", m.type, m.size);

    int pinum = MFS_Lookup(inum, "..");

    // MFS_Write(0, "buffer string", 2);
    printf("myDyr inum : %d, pinum : %d\n", inum, pinum);
    MFS_Unlink(pinum, "myDir");

    MFS_Shutdown();

    // UDP_Close(fd);
    return 0;
}
