#include "mfs.h"
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include "udp.h"

struct sockaddr_in sockaddr;

static int fd;

int MFS_Init(char *hostname, int port)
{
    printf("CLIENT PROXY start ============= INIT\n");
    fd = UDP_Open(port);
    if (fd == -1)
    {
        fd = UDP_Open(port);
    }

    int rc = UDP_FillSockAddr(&sockaddr, hostname, server_port);

    Message_t message;
    message.message_type = M_INIT;

    rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &sockaddr, answer, SERVER_BUFFER_SIZE);
    printf("CLIENT PROXY end ============= INIT\nanswer : %s\n", answer);

    return rc;
};

static int server_fd = -1;
struct sockaddr_in *server_addr;
int MFS_Init_SERVER(int fd, struct sockaddr_in *addr)
{
    printf("SERVER PROXY ============= INIT\n");
    server_fd = fd;
    server_addr = addr;

    char answer[SERVER_BUFFER_SIZE] = "ok";

    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);

    return rc;
};

int MFS_Lookup(int pinum, char name[FILE_NAME_SIZE])
{
    printf("CLIENT PROXY start ============= LOOKUP\n");

    Message_t message;
    message.message_type = M_LOOKUP;
    message.pinum = pinum;
    // message.name = name;
    strcpy(message.name, name);

    struct sockaddr_in read_addr;
    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];
    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= LOOKUP\nanswer : %s\n", answer);
    return rc;
};

int MFS_Lookup_SERVER(int pinum, char name[FILE_NAME_SIZE])
{
    printf("SERVER PROXY ============= LOOKUP\n");

    // is good
    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("lookup args : %d, %s\n", pinum, name);
    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);
    return rc;
};

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    printf("CLIENT PROXY start ============= STAT\n");

    Message_t message;
    message.message_type = M_STAT;
    message.inum = inum;

    struct sockaddr_in read_addr;

    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);

    rc = UDP_Read(fd, &read_addr, (char *)m, sizeof(MFS_Stat_t));

    printf("CLIENT PROXY end ============= STAT\nanswer : %s\n", answer);
    printf("CLIENT PROXY end ============= STAT\nm_stat : %d\n", m->type);
    return rc;
};

int MFS_Stat_SERVER(int inum)
{
    printf("SERVER PROXY ============= STAT\n");

    MFS_Stat_t m_stat;
    m_stat.type = MFS_DIRECTORY;
    m_stat.size = 20;

    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("Stat args : %d\n", inum);
    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);
    rc = UDP_Write(server_fd, server_addr, (char *)&m_stat, sizeof(MFS_Stat_t));

    return rc;
};

int MFS_Write(int inum, char buffer[MFS_BLOCK_SIZE], int block)
{
    printf("CLIENT PROXY start ============= WRITE\n");
    Message_t message;
    message.message_type = M_WRITE;
    message.inum = inum;
    message.block = block;
    strcpy(message.buffer, buffer);

    struct sockaddr_in read_addr;

    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= WRITE\n");
    return rc;
};
int MFS_Write_SERVER(int inum, char *buffer, int block)
{
    printf("SERVER PROXY ============= WRITE\n");

    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("write args : inum %d buffer %s block %d\n", inum, buffer, block);

    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);

    return rc;
};
int MFS_Read(int inum, char buffer[MFS_BLOCK_SIZE], int block)
{
    printf("CLIENT PROXY start ============= READ\n");
    Message_t message;
    message.message_type = M_READ;
    message.inum = inum;
    message.block = block;
    strcpy(message.buffer, buffer);

    struct sockaddr_in read_addr;

    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= READ\n");
    return rc;
};
int MFS_Read_SERVER(int inum, char *buffer, int block)
{
    printf("SERVER PROXY ============= READ\n");
    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("write args : inum %d buffer %s block %d\n", inum, buffer, block);

    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);

    return rc;
};

int MFS_Creat(int pinum, int type, char *name)
{
    printf("CLIENT PROXY start ============= CREAT\n");
    Message_t message;
    message.message_type = M_CREAT;
    message.pinum = pinum;
    message.type = type;
    strcpy(message.name, name);

    struct sockaddr_in read_addr;

    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= CREAT\n");
    return rc;
};
int MFS_Creat_SERVER(int pinum, int type, char *name)
{
    printf("SERVER PROXY ============= CREAT\n");
    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("write args : pinum %d, type %d, name %s", pinum, type, name);

    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);

    return rc;
};

int MFS_Unlink(int pinum, char *name)
{
    printf("CLIENT PROXY start ============= UNLINK\n");
    Message_t message;
    message.message_type = M_UNLINK;
    message.pinum = pinum;
    strcpy(message.name, name);

    struct sockaddr_in read_addr;

    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= UNLINK\n");
    return rc;
};
int MFS_Unlink_SERVER(int pinum, char *name)
{
    printf("SERVER PROXY ============= UNLINK\n");
    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("write args : pinum %d, name %s", pinum, name);

    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);

    return rc;
};

int MFS_Shutdown()
{
    printf("CLIENT PROXY start ============= SHUTDOWN\n");
    Message_t message;
    message.message_type = M_SHUTDOWN;

    struct sockaddr_in read_addr;

    int rc = UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(fd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= SHUTDOWN\n");
    return rc;
};
int MFS_Shutdown_SERVER()
{
    printf("SERVER PROXY ============= SHUTDOWN\n");
    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    int rc = UDP_Write(server_fd, server_addr, answer, SERVER_BUFFER_SIZE);

    return rc;
};