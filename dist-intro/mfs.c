#include "mfs.h"
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include "udp.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

struct sockaddr_in sockaddr;

int *pointer;
static int pointer_size = sizeof(pointer);
int cd = -1;
int sd = -1;

int MFS_Init(char *hostname, int port)
{
    printf("CLIENT PROXY start ============= INIT\n");
    cd = UDP_Open(port);
    if (cd == -1)
    {
        cd = UDP_Open(port);
    }

    int rc = UDP_FillSockAddr(&sockaddr, hostname, server_port);

    Message_t message;
    message.message_type = M_INIT;

    rc = UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    UDP_Read(cd, &sockaddr, answer, SERVER_BUFFER_SIZE);
    printf("CLIENT PROXY end ============= INIT\nanswer : %s\n", answer);

    return strcmp(answer, "ok") == 0 ? cd : -1;
};

int fd;
Checkpoint_t *checkpoint;
void *image;

struct sockaddr_in *server_addr;
int MFS_Init_SERVER(int _sd, struct sockaddr_in *addr)
{
    sd = _sd;
    printf("SERVER PROXY ============= INIT\n");

    fd = open("filesystem.img", O_RDWR | O_CREAT, S_IRWXU | S_IRUSR);
    if (fd == -1)
    {
        perror("open");
        char answer[SERVER_BUFFER_SIZE] = "crash";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return 1;
    }

    off_t new_size = 1024 * 1024; // 1 MB
    // Set the size of the file
    if (ftruncate(fd, new_size) == -1)
    {
        perror("Error truncating file");
        close(fd);
        return 1;
    }

    unsigned char *empty_buffer;
    empty_buffer = calloc(MFS_BLOCK_SIZE, 1);

    int total_blocks = new_size / MFS_BLOCK_SIZE;
    // Make sure each block on the image is empty
    int index = lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < total_blocks; i++)
    {
        int w = write(fd, empty_buffer, MFS_BLOCK_SIZE);

        if (w != MFS_BLOCK_SIZE)
        {
            perror("write");
            exit(1);
        }
    }

    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    assert(rc > -1);
    int size = (int)stat_buf.st_size;
    printf("size image : %d\n", size);

    off_t root_block_offset = lseek(fd, MFS_BLOCK_SIZE, SEEK_SET);

    MFS_DirEnt_t root_dir;
    root_dir.name[0] = '.';
    root_dir.name[1] = '\0';
    root_dir.inum = 0;
    write(fd, &root_dir, sizeof(MFS_DirEnt_t));

    off_t root_block_offset2 = lseek(fd, MFS_BLOCK_SIZE - sizeof(MFS_DirEnt_t), SEEK_CUR);
    // for root ../ and ./ is the same
    root_dir.name[1] = '.';
    root_dir.name[2] = '\0';
    write(fd, &root_dir, sizeof(MFS_DirEnt_t));

    off_t inode_offset = lseek(fd, 0, SEEK_CUR);
    MFS_Stat_t inode_root;
    inode_root.type = MFS_DIRECTORY;
    inode_root.size = MFS_BLOCK_SIZE;
    write(fd, &inode_root, sizeof(MFS_Stat_t));

    write(fd, &root_block_offset, sizeof(off_t));
    write(fd, &root_block_offset2, sizeof(off_t));

    // rest of the inode point to nothing
    for (int i = 0; i < (MFS_BLOCK_SIZE - sizeof(MFS_Stat_t) - 2 * sizeof(off_t)); i++)
    {
        write(fd, (const void *)(-1), sizeof(off_t));
    }

    // imap
    off_t imap_offset = lseek(fd, 0, SEEK_CUR);
    write(fd, &inode_offset, sizeof(off_t));

    server_addr = addr;

    char answer[SERVER_BUFFER_SIZE] = "ok";
    UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

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
    int rc = UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];
    rc = UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= LOOKUP\nanswer : %s\n", answer);
    return rc;
};

int MFS_Lookup_SERVER(int pinum, char name[FILE_NAME_SIZE])
{
    printf("SERVER PROXY ============= LOOKUP\n");

    // is good
    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("lookup args : %d, %s\n", pinum, name);
    int rc = UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
    return rc;
};

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    printf("CLIENT PROXY start ============= STAT\n");

    Message_t message;
    message.message_type = M_STAT;
    message.inum = inum;

    struct sockaddr_in read_addr;

    int rc = UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);

    rc = UDP_Read(cd, &read_addr, (char *)m, sizeof(MFS_Stat_t));

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
    int rc = UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
    rc = UDP_Write(sd, server_addr, (char *)&m_stat, sizeof(MFS_Stat_t));

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

    int rc = UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= WRITE\n");
    return rc;
};
int MFS_Write_SERVER(int inum, char *buffer, int block)
{
    printf("SERVER PROXY ============= WRITE\n");

    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("write args : inum %d buffer %s block %d\n", inum, buffer, block);

    int rc = UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

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

    int rc = UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= READ\n");
    return rc;
};
int MFS_Read_SERVER(int inum, char *buffer, int block)
{
    printf("SERVER PROXY ============= READ\n");
    // DO SOMETHING

    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("write args : inum %d buffer %s block %d\n", inum, buffer, block);

    int rc = UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

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

    int rc = UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    rc = UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);

    printf("CLIENT PROXY end ============= CREAT\n");
    return rc;
};

unsigned int get_bit(unsigned int *bitmap, int position)
{
    int index = position / 32;
    int offset = 31 - (position % 32);
    return (bitmap[index] >> offset) & 0x1;
}

int MFS_Creat_SERVER(int pinum, int type, char *name)
{
    printf("SERVER PROXY ============= CREAT\n");
    // DO SOMETHING ////////////////////////////////////////////

    //////////////////////////////////////////////////////////
    char answer[SERVER_BUFFER_SIZE] = "ok";

    printf("write args : pinum %d, type %d, name %s", pinum, type, name);

    int rc = UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

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

    int rc = UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

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

    int rc = UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

    return rc;
};