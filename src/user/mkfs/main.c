#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t uint;

// this file should be compiled with normal gcc...

#define stat  xv6_stat  // avoid clash with host struct stat
#define sleep xv6_sleep
// #include "../../../inc/fs.h"
#include "../../fs/defines.h"
// #include "../../fs/inode.h"

#ifndef static_assert
#define static_assert(a, b)                                                                        \
    do {                                                                                           \
        switch (0)                                                                                 \
        case 0:                                                                                    \
        case (a):;                                                                                 \
    } while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]
#define BSIZE         BLOCK_SIZE
#define LOGSIZE       LOG_MAX_SIZE
#define NDIRECT       INODE_NUM_DIRECT
#define NINDIRECT     INODE_NUM_INDIRECT
#define DIRSIZ        FILE_NAME_MAX_LENGTH
#define IPB           (BSIZE / sizeof(InodeEntry))
#define IBLOCK(i, sb) ((i) / IPB + sb.inode_start)

int nbitmap = FSSIZE / (BSIZE * 8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int num_log_blocks = LOGSIZE;
int nmeta;            // Number of meta blocks (boot, sb, num_log_blocks, inode, bitmap)
int num_data_blocks;  // Number of data blocks

int fsfd;
SuperBlock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;

void balloc(int);
void wsect(uint, void *);
void winode(uint, struct dinode *);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);

// convert to little-endian byte order
ushort xshort(ushort x) {
    ushort y;
    uchar *a = (uchar *)&y;
    a[0] = x;
    a[1] = x >> 8;
    return y;
}

uint xint(uint x) {
    uint y;
    uchar *a = (uchar *)&y;
    a[0] = x;
    a[1] = x >> 8;
    a[2] = x >> 16;
    a[3] = x >> 24;
    return y;
}

int main(int argc, char *argv[]) {
    int i, cc, fd;
    uint rootino, inum, off;
    struct dirent de;
    char buf[BSIZE];
    InodeEntry din;

    static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

    if (argc < 2) {
        fprintf(stderr, "Usage: mkfs fs.img files...\n");
        exit(1);
    }

    assert((BSIZE % sizeof(struct dinode)) == 0);
    assert((BSIZE % sizeof(struct dirent)) == 0);

    fsfd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fsfd < 0) {
        perror(argv[1]);
        exit(1);
    }

    // 1 fs block = 1 disk sector
    nmeta = 2 + num_log_blocks + ninodeblocks + nbitmap;
    num_data_blocks = FSSIZE - nmeta;

    sb.num_blocks = xint(FSSIZE);
    sb.num_data_blocks = xint(num_data_blocks);
    sb.num_inodes = xint(NINODES);
    sb.num_log_blocks = xint(num_log_blocks);
    sb.log_start = xint(2);
    sb.inode_start = xint(2 + num_log_blocks);
    sb.bitmap_start = xint(2 + num_log_blocks + ninodeblocks);

    printf("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d "
           "total %d\n",
           nmeta,
           num_log_blocks,
           ninodeblocks,
           nbitmap,
           num_data_blocks,
           FSSIZE);

    freeblock = nmeta;  // the first free block that we can allocate

    for (i = 0; i < FSSIZE; i++)
        wsect(i, zeroes);

    memset(buf, 0, sizeof(buf));
    memmove(buf, &sb, sizeof(sb));
    wsect(1, buf);

    rootino = ialloc(INODE_DIRECTORY);
    assert(rootino == ROOT_INODE_NO);

    bzero(&de, sizeof(de));
    de.inode_no = xshort(rootino);
    strcpy(de.name, ".");
    iappend(rootino, &de, sizeof(de));

    bzero(&de, sizeof(de));
    de.inode_no = xshort(rootino);
    strcpy(de.name, "..");
    iappend(rootino, &de, sizeof(de));

    for (i = 2; i < argc; i++) {
        char *path = argv[i];
        int j = 0;
        for (; *argv[i]; argv[i]++) {
            if (*argv[i] == '/')
                j = -1;
            j++;
        }
        argv[i] -= j;
        printf("input: '%s' -> '%s'\n", path, argv[i]);

        assert(index(argv[i], '/') == 0);

        if ((fd = open(path, 0)) < 0) {
            perror(argv[i]);
            exit(1);
        }

        // Skip leading _ in name when writing to file system.
        // The binaries are named _rm, _cat, etc. to keep the
        // build operating system from trying to execute them
        // in place of system binaries like rm and cat.
        if (argv[i][0] == '_')
            ++argv[i];

        inum = ialloc(INODE_REGULAR);

        bzero(&de, sizeof(de));
        de.inode_no = xshort(inum);
        strncpy(de.name, argv[i], DIRSIZ);
        iappend(rootino, &de, sizeof(de));

        while ((cc = read(fd, buf, sizeof(buf))) > 0)
            iappend(inum, buf, cc);

        close(fd);
    }

    // fix size of root inode dir
    rinode(rootino, &din);
    off = xint(din.num_bytes);
    off = ((off / BSIZE) + 1) * BSIZE;
    din.num_bytes = xint(off);
    winode(rootino, &din);

    balloc(freeblock);

    exit(0);
}

void wsect(uint sec, void *buf) {
    if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
        perror("lseek");
        exit(1);
    }
    if (write(fsfd, buf, BSIZE) != BSIZE) {
        perror("write");
        exit(1);
    }
}

void winode(uint inum, struct dinode *ip) {
    char buf[BSIZE];
    uint bn;
    struct dinode *dip;

    bn = IBLOCK(inum, sb);
    rsect(bn, buf);
    dip = ((struct dinode *)buf) + (inum % IPB);
    *dip = *ip;
    wsect(bn, buf);
}

void rinode(uint inum, struct dinode *ip) {
    char buf[BSIZE];
    uint bn;
    struct dinode *dip;

    bn = IBLOCK(inum, sb);
    rsect(bn, buf);
    dip = ((struct dinode *)buf) + (inum % IPB);
    *ip = *dip;
}

void rsect(uint sec, void *buf) {
    if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
        perror("lseek");
        exit(1);
    }
    if (read(fsfd, buf, BSIZE) != BSIZE) {
        perror("read");
        exit(1);
    }
}

uint ialloc(ushort type) {
    uint inum = freeinode++;
    struct dinode din;

    bzero(&din, sizeof(din));
    din.type = xshort(type);
    din.num_links = xshort(1);
    din.num_bytes = xint(0);
    winode(inum, &din);
    return inum;
}

void balloc(int used) {
    uchar buf[BSIZE];
    int i;

    printf("balloc: first %d blocks have been allocated\n", used);
    assert(used < BSIZE * 8);
    bzero(buf, BSIZE);
    for (i = 0; i < used; i++) {
        buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
    }
    printf("balloc: write bitmap block at sector %d\n", sb.bitmap_start);
    wsect(sb.bitmap_start, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void iappend(uint inum, void *xp, int n) {
    char *p = (char *)xp;
    uint fbn, off, n1;
    struct dinode din;
    char buf[BSIZE];
    uint indirect[NINDIRECT];
    uint x;

    rinode(inum, &din);
    off = xint(din.num_bytes);
    // printf("append inum %d at off %d sz %d\n", inum, off, n);
    while (n > 0) {
        fbn = off / BSIZE;
        assert(fbn < INODE_MAX_BLOCKS);
        if (fbn < NDIRECT) {
            if (xint(din.addrs[fbn]) == 0) {
                din.addrs[fbn] = xint(freeblock++);
            }
            x = xint(din.addrs[fbn]);
        } else {
            if (xint(din.indirect) == 0) {
                din.indirect = xint(freeblock++);
            }
            rsect(xint(din.indirect), (char *)indirect);
            if (indirect[fbn - NDIRECT] == 0) {
                indirect[fbn - NDIRECT] = xint(freeblock++);
                wsect(xint(din.indirect), (char *)indirect);
            }
            x = xint(indirect[fbn - NDIRECT]);
        }
        n1 = min(n, (fbn + 1) * BSIZE - off);
        rsect(x, buf);
        bcopy(p, buf + off - (fbn * BSIZE), n1);
        wsect(x, buf);
        n -= n1;
        off += n1;
        p += n1;
    }
    din.num_bytes = xint(off);
    winode(inum, &din);
}
