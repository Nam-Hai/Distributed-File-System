#include "mfs.h"
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include "udp.h"

struct sockaddr_in sockaddr;
char buffer[70];

static int fd;

int MFS_Init(char *hostname, int port)
{
    fd = UDP_Open(port);
    if (fd == -1)
    {
        printf("client open failed\n");
        fd = UDP_Open(port);
    }

    fd_set r;
    FD_ZERO(&r);
    FD_SET(fd, &r);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int rc = UDP_FillSockAddr(&sockaddr, hostname, server_port);

    Message_t message;
    message.message_type = M_INIT;

    struct sockaddr_in read_addr;

    printf("client proxy\nMFS_INIT message payload type %d\n", message.message_type);

    rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    int result = select(fd + 1, &r, NULL, NULL, &timeout);
    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &sockaddr, answer, SERVER_BUFFER_SIZE);
    printf("client proxy\nINIT, answer : %s , rc : %d\n", answer, rc);

    return rc;
};

int MFS_Init_SERVER(int fd, struct sockaddr_in *addr)
{
    printf("server proxy\nINIT\n");
    char sb[SERVER_BUFFER_SIZE] = "server init answer message";

    printf("%d\n", addr->sin_port);
    int rc = UDP_Write(fd, addr, sb, SERVER_BUFFER_SIZE);
    return rc;
};

int MFS_Lookup(int pinum, char *name)
{
    printf("client proxy\nLookup client\n");

    Message_t message;
    message.message_type = M_LOOKUP;

    struct sockaddr_in read_addr;
    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);
    printf("client proxy\nlookup, answer : %s , rc : %d", answer, rc);

    return rc;
};

int MFS_Lookup_SERVER(int fd, struct sockaddr_in *addr)
{
    printf("server proxy\nLOOKUP\n");
    char sb[SERVER_BUFFER_SIZE] = "SERVER LOOKUP";

    int rc = UDP_Write(fd, addr, sb, SERVER_BUFFER_SIZE);
    return rc;
};