#include <stdio.h>
#include "udp.h"
#include "mfs.h"

// server code
int main(int argc, char *argv[])
{
    int sd = UDP_Open(server_port);

    if (sd == -1)
    {
        sd = UDP_Open(server_port);
    }
    assert(sd != -1);

    Message_t message;
    message.message_type = -1;

    struct sockaddr_in addr;

    while (1)
    {
        printf("Server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, (char *)&message, sizeof(Message_t));
        printf("Server:: read message\n");
        if (rc > 0)
        {
            printf("Server handle client request of type : %d\n", message.message_type);
            switch (message.message_type)
            {
            case 0:
                // INIT
                MFS_Init_SERVER(sd, &addr);
                break;
            case 1:
                // LOOKUP
                MFS_Lookup_SERVER(message.pinum, message.name);
                break;
            case 2:

                MFS_Stat_SERVER(message.inum);
                break;
            case 3:
                MFS_Write_SERVER(message.inum, message.buffer, message.block);
                break;
            case 4:
                MFS_Read_SERVER(message.inum, message.buffer, message.block);
                break;
            case 5:
                MFS_Creat_SERVER(message.pinum, message.type, message.name);
                break;
            case 6:
                MFS_Unlink_SERVER(message.pinum, message.name);
                break;
            case 7:
                MFS_Shutdown_SERVER();
                return 0;
                break;
            default:
                printf("default");
                break;
            }
        }
    }

    return 0;
}
