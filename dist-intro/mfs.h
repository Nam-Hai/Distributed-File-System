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
    MFS_REGULAR_FILE
};

#define MFS_BLOCK_SIZE (4096)

typedef struct __MFS_Stat_t
{
    enum BLOCK_ENUM type; // MFS_DIRECTORY or MFS_REGULAR
    int size;             // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t
{
    char name[28]; // up to 28 bytes of name in directory (including \0)
    int inum;      // inode number of entry (-1 means entry not used)
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
    M_SHUTDOWN
};

typedef struct __Message_t
{
    enum MESSAGE_ENUM message_type;
} Message_t;

#define SERVER_BUFFER_SIZE (1000)

int MFS_Init(char *hostname, int port);
int MFS_Init_SERVER(int fd, struct sockaddr_in *addr);
int MFS_Lookup(int pinum, char *name);
int MFS_Lookup_SERVER(int fd, struct sockaddr_in *addr);

#endif // __MFS_h__
