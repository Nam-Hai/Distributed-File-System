#include "mfs.h"
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include "udp.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

struct sockaddr_in sockaddr;
struct sockaddr_in *server_addr;
int cd = -1;
int sd = -1;

int MFS_function(int inum, int block)
{
    printf("CLIENT PROXY start =============\n");
    Message_t message;
    message.message_type = M_READ;
    message.inum = inum;
    message.block = block;

    struct sockaddr_in read_addr;

    // on appele le serveur qui va lui call MFS_function_SERVEUR
    UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];
    UDP_Read(cd, &read_addr, (char *)answer, SIZE_BLOCK);

    int result = atoi(answer);
    printf("CLIENT PROXY end =============\nanswer : %d\n", result);
    return result;
};

int MFS_function_SERVER(int inum, char *buffer, int block)
{
    printf("SERVER PROXY =============\n");
    // DO SOMETHING ////////////////////////////////////////

    ////////////////////////////////////////////////////////
    return 0;
};