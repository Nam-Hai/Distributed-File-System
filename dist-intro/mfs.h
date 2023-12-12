#ifndef __MFS_h__
#define __MFS_h__

#include "udp.h"

// #define MFS_DIRECTORY (0)
// #define MFS_REGULAR_FILE (1)

static int server_port = 10000;
static int client_port = 20000;

enum BLOCK_ENUM
{
    MFS_DIRECTORY,
    MFS_REGULAR_FILE,
    MFS_ERROR = -1
};

#define MFS_BLOCK_SIZE (4096)
#define FILE_NAME_SIZE (28)

typedef struct __MFS_Stat_t
{
    enum BLOCK_ENUM type; // MFS_DIRECTORY or MFS_REGULAR
    int size;             // bytes
} MFS_Stat_t;

// 32 bit
typedef struct __MFS_DirEnt_t
{
    char name[FILE_NAME_SIZE]; // up to 28 bytes of name in directory (including \0)
    int inum;                  // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

enum MESSAGE_ENUM
{
    M_INIT,
    M_LOOKUP,
    M_STAT,
    M_WRITE,
    M_READ,
    M_CREAT,
    M_UNLINK,
    M_SHUTDOWN,
    M_ERROR = -1
};

typedef struct __Message_t
{
    enum MESSAGE_ENUM message_type;
    int type;
    int port;
    int pinum;
    int inum;
    int block;
    MFS_Stat_t stat;
    char buffer[MFS_BLOCK_SIZE];
    char name[FILE_NAME_SIZE];
} Message_t;

typedef struct __Checkpoint_t
{
    off_t imaps_addr;
    int inode_number;
} Checkpoint_t;

// used for write, read, seek wrapper in mfs.c
enum SIZE_ENUM
{
    SIZE_DIR = sizeof(MFS_DirEnt_t),
    SIZE_INODE_H = sizeof(MFS_Stat_t),
    SIZE_ADDR = sizeof(off_t),
    SIZE_BLOCK = MFS_BLOCK_SIZE
};

#define SERVER_BUFFER_SIZE (1000)

int MFS_Init(char *hostname, int port);
int MFS_Init_SERVER(int fd, struct sockaddr_in *addr);

int MFS_Lookup(int pinum, char name[FILE_NAME_SIZE]);
int MFS_Lookup_SERVER(int pinum, char name[FILE_NAME_SIZE]);

int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Stat_SERVER(int inum);

int MFS_Write(int inum, char *buffer, int block);
int MFS_Write_SERVER(int inum, char *buffer, int block);

int MFS_Read(int inum, char *buffer, int block);
int MFS_Read_SERVER(int inum, char *buffer, int block);

int MFS_Creat(int pinum, int type, char *name);
int MFS_Creat_SERVER(int pinum, int type, char *name);

int MFS_Unlink(int pinum, char *name);
int MFS_Unlink_SERVER(int pinum, char *name);

int MFS_Shutdown();
int MFS_Shutdown_SERVER();

#endif // __MFS_h__
