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

    while (1)
    {
        struct sockaddr_in addr;

        printf("server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, (char *)&message, sizeof(Message_t));
        printf("server:: read message [size:%d contents:(%d)]\n", rc, message.message_type);
        if (rc > 0)
        {

            printf("Switch server\n");
            switch (message.message_type)
            {
            case 0:
                // INIT
                printf("Init :\n");

                MFS_Init_SERVER(sd, &addr);
                break;
            case 1:
                // LOOKUP
                printf("Lookup :\n");
                MFS_Lookup_SERVER(sd, &addr);
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                break;
            }

        }
    }

    return 0;
}
