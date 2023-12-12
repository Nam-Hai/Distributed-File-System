#include "mfs.h"
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include "udp.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

struct sockaddr_in sockaddr;

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

    UDP_FillSockAddr(&sockaddr, hostname, server_port);

    Message_t message;
    message.message_type = M_INIT;

    UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];
    UDP_Read(cd, &sockaddr, answer, SERVER_BUFFER_SIZE);

    int result = atoi(answer);
    printf("CLIENT PROXY end ============= INIT\nanswer : %d\n", result);
    return result;
};

int fd;
Checkpoint_t checkpoint;
off_t imap_cache[SIZE_BLOCK / SIZE_ADDR];
void *image;

off_t imap_addr;
off_t curr_addr;
int curr_block;

int block_to_addr(int b)
{
    return b * MFS_BLOCK_SIZE;
}
int addr_to_block(int addr)
{
    int b = addr / MFS_BLOCK_SIZE;
    return b;
}

// wrapper for lseek
off_t seek_block(int n)
{
    curr_block = n;
    return lseek(fd, block_to_addr(n), SEEK_SET);
}

off_t seek_next_block()
{
    return seek_block(curr_block + 1);
}

// seek a la fin, donc a l'imap
off_t seek_imap()
{
    return seek_block(addr_to_block(imap_addr));
}

// wrapper for write
ssize_t log_write(const void *buffer, enum SIZE_ENUM size)
{
    ssize_t s = write(fd, buffer, size);
    if (s == -1)
    {
        perror("Error writing file");
        return -1;
    }
    curr_addr = lseek(fd, 0, SEEK_CUR);
    curr_block = addr_to_block(curr_addr);
    return s;
}

// wrapper for read
ssize_t log_read(void *buffer, enum SIZE_ENUM size)
{
    ssize_t s = read(fd, buffer, size);
    if (s == -1)
    {
        perror("Error reading file");
        return -1;
    }

    curr_addr = lseek(fd, 0, SEEK_CUR);
    curr_block = addr_to_block(curr_addr);
    return s;
}

void update_checkpoint()
{
    // checkpoint region
    seek_block(0);
    write(fd, &checkpoint, sizeof(Checkpoint_t));
}

struct sockaddr_in *server_addr;
int MFS_Init_SERVER(int _sd, struct sockaddr_in *addr)
{
    server_addr = addr;
    sd = _sd;
    printf("SERVER PROXY ============= INIT\n");

    fd = open("filesystem.img", O_RDWR | O_CREAT, S_IRWXU | S_IRUSR);
    if (fd == -1)
    {
        perror("open");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return 1;
    }

    char answer[SERVER_BUFFER_SIZE] = "0";

    // Si je veux utiliser directement image_disk
    // DEBUG
    if (0)
    {
        printf("IMAGE_DISK_OPEN");
        seek_block(0);
        log_read(&checkpoint, sizeof(Checkpoint_t));

        imap_addr = seek_block(addr_to_block(checkpoint.imaps_addr));
        seek_imap();
        log_read(imap_cache, SIZE_BLOCK);

        for (int i = 0; i < SIZE_BLOCK / SIZE_ADDR; i++)
        {
            if (imap_cache[i] == 0)
            {
                break;
            }
            printf("imap_cache[%d]: %ld\n", i, (long)imap_cache[i]);
        }

        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return 0;
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
    seek_block(0);
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

    // block 1
    // Creation d'un directory root
    off_t root_block_offset = seek_next_block();
    MFS_DirEnt_t root_dir;
    strcpy(root_dir.name, ".");

    root_dir.inum = 0;
    log_write(&root_dir, SIZE_DIR);

    // .. et . doivent pointer sur root ie le inode de lui meme
    strcpy(root_dir.name, "..");
    log_write(&root_dir, SIZE_DIR);

    strcpy(root_dir.name, "foo");
    root_dir.inum = 1; // foo sera le 2nd inode de la imap
    log_write(&root_dir, SIZE_DIR);

    // block 2
    // Creation du INode Dir Root
    off_t inode_addr = seek_next_block();

    // Header
    MFS_Stat_t inode_root;
    inode_root.type = MFS_DIRECTORY;
    inode_root.size = 1;
    checkpoint.inode_number++;
    log_write(&inode_root, SIZE_INODE_H);

    // Inode point vers le dir
    log_write(&root_block_offset, SIZE_ADDR);

    // block 3
    // Creation d'un file foo
    off_t foo_addr = seek_next_block();
    char buffer[MFS_BLOCK_SIZE] = "Ceci est un file";
    log_write(&buffer, SIZE_BLOCK);

    off_t foo_inode_addr = lseek(fd, 0, SEEK_CUR);
    // block 4
    MFS_Stat_t inode_file;
    inode_file.type = MFS_REGULAR_FILE;
    inode_file.size = 1; // le file ne fait que 1 seul block data, c.a.d l'inode ne va pointer que vers 1 block

    checkpoint.inode_number++;
    log_write(&inode_file, SIZE_INODE_H);
    log_write(&foo_addr, SIZE_ADDR);

    // block 5
    // Init l'imap
    imap_addr = seek_next_block();
    checkpoint.imaps_addr = imap_addr;

    // root inode est bien l'inode avec inum == 0, c.a.d la premiere addresse de imap
    // inode_addr = block_to_addr(2);

    log_write(&inode_addr, SIZE_ADDR);

    log_write(&foo_inode_addr, SIZE_ADDR);

    seek_imap();
    log_read(imap_cache, SIZE_BLOCK);

    for (int i = 0; i < SIZE_BLOCK / SIZE_ADDR; i++)
    {
        if (imap_cache[i] == 0)
        {
            break;
        }
        printf("imap_cache[%d]: %ld\n", i, (long)imap_cache[i]);
    }

    update_checkpoint();

    UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
    return 0;
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
    UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];
    UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);
    int result = atoi(answer);
    printf("CLIENT PROXY end ============= LOOKUP\nanswer : %d\n", result);
    return result;
};

int MFS_Lookup_SERVER(int pinum, char name[FILE_NAME_SIZE])
{
    printf("SERVER PROXY ============= LOOKUP\n");

    // aller au dir pinum
    // c'est ici qu'on check si pinum exist
    off_t parent_inode_addr = imap_cache[pinum];

    if (pinum < 0 || parent_inode_addr == 0)
    {
        perror("MFS_Lookup pinum is not in imap");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }
    seek_block(addr_to_block(parent_inode_addr));

    // on dans le inode du dir parent
    MFS_Stat_t stat_buffer;
    log_read(&stat_buffer, SIZE_INODE_H);

    if (stat_buffer.type != MFS_DIRECTORY)
    {
        perror("MFS_Lookup pinum not a directory");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }
    if (stat_buffer.size < 1)
    {
        // il y a forcement .. et .
        perror("MFS_Lookup directory miss initialiazed, size = 0");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }

    // on assume size == 1, un seul block pour dir
    off_t dir_addr;
    log_read(&dir_addr, SIZE_ADDR);

    seek_block(addr_to_block(dir_addr));

    MFS_DirEnt_t dir_buffer;
    for (int i = 0; i < SIZE_BLOCK / SIZE_DIR; i++)
    {
        log_read(&dir_buffer, SIZE_DIR);

        // DEBUG
        // printf("dir name %s\n", dir_buffer.name);
        if (strcmp(dir_buffer.name, name) == 0)
        {
            // file exist
            char answer[SERVER_BUFFER_SIZE];
            snprintf(answer, sizeof(answer), "%d", dir_buffer.inum);

            UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
            return 0;
        }
        if (strcmp(dir_buffer.name, "") == 0)
        {
            // on a atteint la fin du block
            // file dont exist
            char answer[SERVER_BUFFER_SIZE] = "-1";
            UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
            return -1;
            break;
        }
    }
    // on a atteint la fin du block
    // file dont exist
    char answer[SERVER_BUFFER_SIZE] = "-1";
    UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
    return -1;
};

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    printf("CLIENT PROXY start ============= STAT\n");

    Message_t message;
    message.message_type = M_STAT;
    message.inum = inum;

    struct sockaddr_in read_addr;

    UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    // char answer[SERVER_BUFFER_SIZE];
    MFS_Stat_t m_stat_proxy;

    // UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);
    UDP_Read(cd, &read_addr, (char *)&m_stat_proxy, sizeof(MFS_Stat_t));

    // int result = atoi(answer);
    // printf("CLIENT PROXY end 1 ============= LOOKUP\nanswer : %d\n", result);
    printf("CLIENT PROXY end 2 ============= STAT\nm_stat : type %d size %d\n", m_stat_proxy.type, m_stat_proxy.size);

    m->size = m_stat_proxy.size;
    m->type = m_stat_proxy.type;

    if (m_stat_proxy.type == -1)
        return -1;
    return 0;
};
int MFS_Stat_SERVER(int inum)
{
    printf("SERVER PROXY ============= STAT\n");

    MFS_Stat_t stat_buffer;
    // Explicitement pour lookup -> inum = -1
    if (inum == -1)
    {
        stat_buffer.type = -1;
        stat_buffer.size = 0;
        UDP_Write(sd, server_addr, (char *)&stat_buffer, sizeof(MFS_Stat_t));
        return -1;
    }

    // DO SOMETHING //////////////////////////////

    off_t inode_addr = imap_cache[inum];
    if (inode_addr == 0)
    {
        stat_buffer.type = -1;
        stat_buffer.size = 0;
        UDP_Write(sd, server_addr, (char *)&stat_buffer, sizeof(MFS_Stat_t));
        return -1;
    }

    seek_block(addr_to_block(inode_addr));

    log_read(&stat_buffer, SIZE_INODE_H);

    //////////////////////////////////////////////

    UDP_Write(sd, server_addr, (char *)&stat_buffer, sizeof(MFS_Stat_t));
    return 0;
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
    UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);
    int result = atoi(answer);
    printf("CLIENT PROXY end ============= WRITE\nanswer : %d\n", result);
    return result;
};

int MFS_Write_SERVER(int inum, char buffer[SIZE_BLOCK], int block)
{
    printf("SERVER PROXY ============= WRITE\n");

    // DO SOMETHING //////////////////////////////////////////

    off_t inode_addr = imap_cache[inum];

    if (inum < 0 || inode_addr == 0)
    {
        perror("MFS_WRITE inode overflow");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }
    seek_block(addr_to_block(inode_addr));

    MFS_Stat_t inode_file;
    log_read(&inode_file, SIZE_INODE_H);

    if (inode_file.type != MFS_REGULAR_FILE)
    {
        perror("MFS_WRITE tried to write a Dir");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }

    // je vais pas prendre en compte pour l'instant le cas ou le inode est full
    // c'est a dire que le file est tellement long qu'il faudrait plusieur inode cad inode->inode->data_block
    int size = inode_file.size;

    if (block > size + 1)
    {
        perror("MFS_WRITE block write overflow");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }

    // dans la doc, il y a ecrit qu'on prend que 14 pointer par inode
    off_t pointers[14];
    log_read(&pointers, SIZE_ADDR * 14);

    // end of file
    seek_imap();

    if (block == size && 1 + size > 14)
    {
        perror("MFS_WRITE writing buffer too big");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }

    // on creer un nouveau block
    off_t block_addr = seek_next_block();
    char block_buffer[SIZE_BLOCK];
    strcpy(block_buffer, buffer);

    log_write(block_buffer, SIZE_BLOCK);

    pointers[block] = block_addr;

    // c'est pas une update
    if (block == size)
    {
        size++;
    }

    // nouvelle inode
    inode_addr = seek_next_block();
    inode_file.type = MFS_REGULAR_FILE;
    inode_file.size = size;
    log_write(&inode_file, SIZE_INODE_H);
    log_write(&pointers, sizeof(pointers));

    // nouvelle imap + checkpoint
    checkpoint.inode_number++;

    // j'update inode_cache
    imap_cache[inum] = inode_addr;

    // j'ajoute la nouvelle inode sur disk, imap_cache
    imap_addr = seek_next_block();
    checkpoint.imaps_addr = imap_addr;
    log_write(&imap_cache, SIZE_BLOCK);

    ////////////////////////////////////////////////////////
    char answer[SERVER_BUFFER_SIZE] = "0";
    UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

    return 0;
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

    char answer[SIZE_BLOCK];
    UDP_Read(cd, &read_addr, (char *)answer, SIZE_BLOCK);

    strcpy(buffer, answer);

    int result = 0;
    printf("CLIENT PROXY end ============= READ\nanswer : %d\n", result);
    return result;
};
int MFS_Read_SERVER(int inum, char *buffer, int block)
{
    printf("SERVER PROXY ============= READ\n");
    // DO SOMETHING ////////////////////////////////////////

    off_t inode_addr = imap_cache[inum];

    if (inum < 0 || inode_addr == 0)
    {
        perror("MFS_READ inode overflow");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }
    seek_block(addr_to_block(inode_addr));

    MFS_Stat_t inode_file;
    log_read(&inode_file, SIZE_INODE_H);

    if (block > inode_file.size)
    {
        perror("MFS_READ block overflow");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }

    lseek(fd, block * SIZE_ADDR, SEEK_CUR);
    off_t block_addr;
    log_read(&block_addr, SIZE_ADDR);

    seek_block(addr_to_block(block_addr));

    log_read(buffer, SIZE_BLOCK);

    ////////////////////////////////////////////////////////
    // char answer[SERVER_BUFFER_SIZE] = "0";
    // UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
    UDP_Write(sd, server_addr, buffer, SIZE_BLOCK);

    return 0;
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

    UDP_Write(cd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];

    UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);

    int result = atoi(answer);
    printf("CLIENT PROXY end ============= Creat\nanswer : %d\n", result);
    return result;
};

int MFS_Creat_SERVER(int pinum, int type, char *name)
{
    printf("SERVER PROXY ============= CREAT\n");
    // DO SOMETHING ////////////////////////////////////////////

    if (sizeof(name) > FILE_NAME_SIZE)
    {
        perror("name is too long");
        return -1;
    }

    // aller au dir pinum
    // c'est ici qu'on check si pinum exist
    off_t parent_inode_addr = imap_cache[pinum];

    if (pinum < 0 || parent_inode_addr == 0)
    {
        perror("MFS_CREAT inode overflow");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }

    seek_block(addr_to_block(parent_inode_addr));
    MFS_Stat_t stat_buffer;
    log_read(&stat_buffer, SIZE_INODE_H);

    if (stat_buffer.type != MFS_DIRECTORY)
    {
        perror("MFS_CREAT pinum not a directory");

        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }
    if (stat_buffer.size < 1)
    {
        // il y a forcement .. et .
        perror("MFS_CREAT directory miss initialiazed, size = 0");
        char answer[SERVER_BUFFER_SIZE] = "-1";
        UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
        return -1;
    }

    // Je pars pour l'instand du principe que le block du dir est jamais remplis au max, ce n'est evidement pas toujours le cas
    // Il faut alors revenir dans le inode et alors creer un nouveau block du Dir et faire pointer le inode vers ce block
    // lseek(fd, (stat_buffer.size - 1) * SIZE_ADDR, SEEK_CUR);
    off_t dir_addr;
    log_read(&dir_addr, SIZE_ADDR);

    seek_block(addr_to_block(dir_addr));
    // lseek(fd, dir_addr, SEEK_SET);

    MFS_DirEnt_t dir_buffer;
    for (int i = 0; i < SIZE_BLOCK / SIZE_DIR; i++)
    {
        log_read(&dir_buffer, SIZE_DIR);

        // DEBUG
        // printf("dir name %s\n", dir_buffer.name);
        if (strcmp(dir_buffer.name, name) == 0)
        {
            // FILE ALREADY EXIST OMG
            char answer[SERVER_BUFFER_SIZE] = "0";
            UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
            return 0;
        }
        if (strcmp(dir_buffer.name, "") == 0)
        {
            // on remonte l'aiguille
            lseek(fd, -SIZE_DIR, SEEK_CUR);

            MFS_DirEnt_t dir_content;
            checkpoint.inode_number++;
            strcpy(dir_content.name, name);
            dir_content.inum = checkpoint.inode_number;
            log_write(&dir_content, SIZE_DIR);
            break;
        }
    }

    seek_block(addr_to_block(dir_addr));

    // FILE
    // Creer une inode d'un file nomme name, pointe par le directory d'inum pinum
    seek_imap(); // seek end of file

    off_t inode_addr = seek_next_block();
    if (type == MFS_REGULAR_FILE)
    {
        // Header
        MFS_Stat_t inode;
        inode.type = 1; // file
        inode.size = 0; // il y a rien dedans pour l'instant
        log_write(&inode, SIZE_INODE_H);
        imap_cache[checkpoint.inode_number] = inode_addr;
    }
    else if (type == MFS_DIRECTORY)
    {
        // Creer un dir qui pointe vers root et parent
        MFS_DirEnt_t root_dir;
        strcpy(root_dir.name, ".");
        root_dir.inum = 0;

        log_write(&root_dir, SIZE_DIR);

        strcpy(root_dir.name, "..");
        root_dir.inum = pinum;
        log_write(&root_dir, SIZE_DIR);

        seek_next_block();

        // Header
        MFS_Stat_t inode;
        inode.type = 0; // dir
        inode.size = 0; // il y a rien dedans pour l'instant
        log_write(&inode, SIZE_INODE_H);
        imap_cache[checkpoint.inode_number] = inode_addr;
    }
    else
    {
        perror("MFS Creat type error");
    }

    // Creer une nouvelle imap
    imap_addr = seek_next_block();
    log_write(&imap_cache, SIZE_BLOCK);
    checkpoint.imaps_addr = imap_addr;

    // update checkpoint, on devrait update checkpoint periodiquement mais bon c'est pas pour tout de suite
    update_checkpoint();

    //////////////////////////////////////////////////////////

    char answer[SERVER_BUFFER_SIZE] = "0";
    UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);
    return 0;
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

    UDP_Write(fd, &sockaddr, (char *)&message, sizeof(Message_t));

    char answer[SERVER_BUFFER_SIZE];
    UDP_Read(cd, &read_addr, answer, SERVER_BUFFER_SIZE);
    int result = atoi(answer);
    printf("CLIENT PROXY end ============= SHUTDOWN\nanswer : %d\n", result);
    UDP_Close(sd);
    exit(0);
    return result;
};
int MFS_Shutdown_SERVER()
{
    printf("SERVER PROXY ============= SHUTDOWN\n");
    char answer[SERVER_BUFFER_SIZE] = "0";
    UDP_Write(sd, server_addr, answer, SERVER_BUFFER_SIZE);

    UDP_Close(sd);
    exit(0);
    return 0;
};