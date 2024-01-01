#include <stdio.h>
#include "udp.h"
#include "mfs.h"

// client code
int main(int argc, char *argv[])
{
    char *hostname = "localhost";

    int port = client_port;
    int fd = MFS_Init(hostname, port);

    if (fd == -1)
    {
        return 0;
    }

    char buffer[SIZE_BLOCK];

    int inum = MFS_Lookup(0, "foo");
    MFS_Read(inum, buffer, 0);
    printf("foo before rewrite: %s\n", buffer);

    MFS_Write(inum, "Jecris une nouvelle version de foo", 0);
    inum = MFS_Lookup(0, "foo");
    MFS_Read(inum, buffer, 0);
    printf("foo after rewrite : %s\n", buffer);

    MFS_Creat(0, MFS_DIRECTORY, "myDir");

    inum = MFS_Lookup(0, "myDir");

    MFS_Stat_t m;
    MFS_Stat(inum, &m);

    int pinum = MFS_Lookup(inum, "..");

    printf("myDyr inum : %d, pinum : %d\n", inum, pinum);
    MFS_Unlink(pinum, "myDir");
    inum = MFS_Lookup(0, "myDir");
    printf("after Unlink myDyr inum : %d\n", inum);

    MFS_Shutdown();

    return 0;
}
