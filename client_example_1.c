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

    MFS_Creat(0, MFS_DIRECTORY, "myDir");
    int inum = MFS_Lookup(0, "myDir");
    MFS_Creat(inum, MFS_REGULAR_FILE, "myFile");
    int fileInum = MFS_Lookup(inum, "myFile");
    printf("myFile inum : %d\n", fileInum);
    MFS_Write(fileInum, "myFile content block 0", 0);
    MFS_Write(fileInum, "myFile content block 1", 1);
    MFS_Write(fileInum, "myFile content block 4", 4);

    MFS_Read(fileInum, buffer, 0);
    printf("myFile content block 0 : %s\n", buffer);
    MFS_Read(fileInum, buffer, 1);
    printf("myFile content block 1 : %s\n", buffer);
    MFS_Read(fileInum, buffer, 4);
    printf("myFile content block 4 : %s\n", buffer);

    inum = MFS_Lookup(0, "myDir");
    int pinum = MFS_Lookup(inum, "..");

    // MFS_Write(0, "buffer string", 2);
    printf("myDyr inum : %d, pinum : %d\n", inum, pinum);
    MFS_Unlink(pinum, "myDir");
    fileInum = MFS_Lookup(inum, "myFile");
    printf("myFile inum : %d\n", fileInum);

    MFS_Shutdown();

    // UDP_Close(fd);
    return 0;
}
