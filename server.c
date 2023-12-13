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

    MFS_Stat_t stat_buffer;

    while (1)
    {
        printf("Server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, (char *)&message, sizeof(Message_t));
        printf("Server:: read message\n");
        int res = 0;
        char answer[SERVER_BUFFER_SIZE];
        if (rc > 0)
        {
            printf("Server handle client request of type : %d\n", message.message_type);
            switch (message.message_type)
            {
            case 0:
                // INIT
                res = MFS_Init_SERVER(sd, &addr);
                sprintf(answer, "%d", res);

                UDP_Write(sd, &addr, answer, SERVER_BUFFER_SIZE);
                break;
            case 1:
                // LOOKUP
                res = MFS_Lookup_SERVER(message.pinum, message.name);
                sprintf(answer, "%d", res);
                UDP_Write(sd, &addr, answer, SERVER_BUFFER_SIZE);
                break;
            case 2:

                MFS_Stat_SERVER(message.inum, &stat_buffer);

                UDP_Write(sd, &addr, (char *)&stat_buffer, sizeof(MFS_Stat_t));
                break;
            case 3:
                res = MFS_Write_SERVER(message.inum, message.buffer, message.block);
                sprintf(answer, "%d", res);

                UDP_Write(sd, &addr, answer, SERVER_BUFFER_SIZE);
                break;
            case 4:
                res = MFS_Read_SERVER(message.inum, message.buffer, message.block);

                if (res == -1)
                {
                    sprintf(answer, "%d", res);
                    UDP_Write(sd, &addr, answer, SERVER_BUFFER_SIZE);
                }
                else
                {
                    UDP_Write(sd, &addr, message.buffer, SIZE_BLOCK);
                }

                break;
            case 5:
                res = MFS_Creat_SERVER(message.pinum, message.type, message.name);

                sprintf(answer, "%d", res);
                UDP_Write(sd, &addr, answer, SERVER_BUFFER_SIZE);
                break;
            case 6:
                res = MFS_Unlink_SERVER(message.pinum, message.name);
                sprintf(answer, "%d", res);
                UDP_Write(sd, &addr, answer, SERVER_BUFFER_SIZE);
                break;
            case 7:
                MFS_Shutdown_SERVER();

                sprintf(answer, "%d", res);
                UDP_Write(sd, &addr, answer, SERVER_BUFFER_SIZE);
                UDP_Close(sd);
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
