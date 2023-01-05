//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include <fcntl.h>

#include <aarch64/mmu.h>
#include <common/defines.h>
#include <common/spinlock.h>
#include <kernel/printk.h>
#include <kernel/proc.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <kernel/mem.h>
#include <kernel/paging.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <sys/syscall.h>
#include <kernel/mem.h>
#include "syscall.h"
#include <fs/pipe.h>
#include <common/string.h>

struct iovec {
    void* iov_base; /* Starting address. */
    usize iov_len; /* Number of bytes to transfer. */
};

/*
 * Fetch the nth word-sized system call argument as a file descriptor
 * and return both the descriptor and the corresponding struct file.
 */
static int argfd(int n, i64* pfd, struct file** pf) {
    i32 fd;
    struct file* f;
    oFilenode* op;
    if (argint(n, &fd) < 0)
        return -1;

    if (fd < 0 || fd >= thisproc()->ofilecnt)
        return -1;
    ListNode* p = &thisproc()->ofilehead.node;
    ListNode* pp = p->next;
    while (1) {
        if (pp == p)
            return -1;
        op = container_of(pp, oFilenode, node);
        if (op->fd == fd)
            break;
        pp = pp->next;
    }
    f = op->f;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

/*
 * Allocate a file descriptor for the given file.
 * Takes over file reference from caller on success.
 */
int fdalloc(struct file* f) {
    /* TODO: Lab10 Shell */
    return -1;
}
/*
 *	map addr to a file
 */
define_syscall(mmap) {
    u64 addr;
    int length, prot, flags, fd, offset;
    if (argu64(0, &addr) < 0 || argint(1, &length) < 0 || argint(2, &prot) < 0
        || argint(3, &flags) < 0 || argint(4, &fd) < 0 || argint(5, &offset) < 0) {
        return -1;
    }
    if (addr != 0)
        PANIC();
    if (offset != 0)
        PANIC();

    struct proc* p = thisproc();
    int i = p->ofilecnt++;
    oFilenode* op = kalloc(sizeof(oFilenode));
    init_list_node(&op->node);
    op->fd = i;
    op->f = f;
    _merge_list(&op->node, &p->ofilehead.node);
    return i;
    // for (i = 0; i < NOFILE; i++) {
    //     if (!p->ofile[i]) {
    //         p->ofile[i] = f;
    //         return i;
    //     }
    // }
    // return -1;
}

/*
 * Get the parameters and call filedup.
 */
define_syscall(dup) {
    /* TODO: Lab10 Shell. */
    return 0;
}

/*
 * Get the parameters and call fileread.
 */
define_syscall(read) {
    /* TODO: Lab10 Shell */
    return -1;
}

/*
 * Get the parameters and call filewrite.
 */
define_syscall(write) {
    /* TODO: Lab10 Shell */
    return -1;
}

define_syscall(writev) {
    struct file* f;
    i64 fd;
    int iovcnt;
    struct iovec *iov, *p;
    if (argfd(0, &fd, &f) < 0 || argint(2, &iovcnt) < 0
        || argptr(1, (char**)&iov, iovcnt * sizeof(struct iovec)) < 0) {
        return -1;
    }

    usize tot = 0;
    for (p = iov; p < iov + iovcnt; p++) {
        // in_user(p, n) checks if va [p, p+n) lies in user address space.
        if (!in_user(p->iov_base, p->iov_len))
            return -1;
        tot += filewrite(f, p->iov_base, p->iov_len);
    }
    return tot;
}

/*
 * Get the parameters and call fileclose.
 * Clear this fd of this process.
 */
define_syscall(close) {
    /* TODO: Lab10 Shell */
    return 0;
}

/*
 * Get the parameters and call filestat.
 */
define_syscall(fstat) {
    /* TODO: Lab10 Shell */
    return 0;
}

define_syscall(newfstatat) {
    i32 dirfd, flags;
    char* path;
    struct stat* st;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argptr(2, (void*)&st, sizeof(*st)) < 0
        || argint(3, &flags) < 0)
        return -1;

    if (dirfd != AT_FDCWD) {
        printk("sys_fstatat: dirfd unimplemented\n");
        return -1;
    }
    if (flags != 0) {
        printk("sys_fstatat: flags unimplemented\n");
        return -1;
    }

    Inode* ip;
    OpContext ctx;
    bcache.begin_op(&ctx);
    if ((ip = namei(path, &ctx)) == 0) {
        bcache.end_op(&ctx);
        return -1;
    }
    inodes.lock(ip);
    stati(ip, st);
    inodes.unlock(ip);
    inodes.put(&ctx, ip);
    bcache.end_op(&ctx);

    return 0;
}

/*
 * Create an inode.
 *
 * Example:
 * Path is "/foo/bar/bar1", type is normal file.
 * You should get the inode of "/foo/bar", and
 * create an inode named "bar1" in this directory.
 *
 * If type is directory, you should additionally handle "." and "..".
 */
Inode* create(char* path, short type, short major, short minor, OpContext* ctx) {
    /* TODO: Lab10 Shell */
    return 0;
}

define_syscall(openat) {
    // printk("enter openat\n");
    char* path;
    int dirfd, fd, omode;
    struct file* f;
    Inode* ip;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &omode) < 0)
        return -1;

    // printk("%d, %s, %lld\n", dirfd, path, omode);
    if (dirfd != AT_FDCWD) {
        printk("sys_openat: dirfd unimplemented\n");
        return -1;
    }
    // if ((omode & O_LARGEFILE) == 0) {
    //     printk("sys_openat: expect O_LARGEFILE in open flags\n");
    //     return -1;
    // }

    OpContext ctx;
    bcache.begin_op(&ctx);
    if (omode & O_CREAT) {
        // FIXME: Support acl mode.
        ip = create(path, INODE_REGULAR, 0, 0, &ctx);
        if (ip == 0) {
            bcache.end_op(&ctx);
            return -1;
        }
    } else {
        if ((ip = namei(path, &ctx)) == 0) {
            bcache.end_op(&ctx);
            return -1;
        }
        inodes.lock(ip);
        // if (ip->entry.type == INODE_DIRECTORY && omode != (O_RDONLY |
        // O_LARGEFILE)) {
        //     inodes.unlock(ip);
        //     inodes.put(&ctx, ip);
        //     bcache.end_op(&ctx);
        //     return -1;
        // }
    }

    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        inodes.unlock(ip);
        inodes.put(&ctx, ip);
        bcache.end_op(&ctx);
        return -1;
    }
    inodes.unlock(ip);
    bcache.end_op(&ctx);

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    return fd;
}

define_syscall(mkdirat) {
    // printk("enter mkdir\n");
    i32 dirfd, mode;
    char* path;
    Inode* ip;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &mode) < 0)
        return -1;
    if (dirfd != AT_FDCWD) {
        printk("sys_mkdirat: dirfd unimplemented\n");
        return -1;
    }
    if (mode != 0) {
        printk("sys_mkdirat: mode unimplemented\n");
        return -1;
    }
    OpContext ctx;
    bcache.begin_op(&ctx);
    if ((ip = create(path, INODE_DIRECTORY, 0, 0, &ctx)) == 0) {
        bcache.end_op(&ctx);
        return -1;
    }
    inodes.unlock(ip);
    inodes.put(&ctx, ip);
    bcache.end_op(&ctx);
    return 0;
}

define_syscall(mknodat) {
    // printk("enter mknodat\n");
    Inode* ip;
    char* path;
    i32 dirfd, major, minor;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &major) < 0
        || argint(3, &minor))
        return -1;

    if (dirfd != AT_FDCWD) {
        printk("sys_mknodat: dirfd unimplemented\n");
        return -1;
    }
    printk("mknodat: path '%s', major:minor %d:%d\n", path, major, minor);

    OpContext ctx;
    bcache.begin_op(&ctx);
    if ((ip = create(path, INODE_DEVICE, major, minor, &ctx)) == 0) {
        bcache.end_op(&ctx);
        return -1;
    }
    inodes.unlock(ip);
    inodes.put(&ctx, ip);
    bcache.end_op(&ctx);
    return 0;
}

define_syscall(chdir) {
    // printk("enter chdir\n");
    char* path;
    Inode* ip;
    struct proc* curproc = thisproc();

    OpContext ctx;
    bcache.begin_op(&ctx);
    if (argstr(0, &path) < 0 || (ip = namei(path, &ctx)) == 0) {
        bcache.end_op(&ctx);
        return -1;
    }
    inodes.lock(ip);
    if (ip->entry.type != INODE_DIRECTORY) {
        inodes.unlock(ip);
        inodes.put(&ctx, ip);
        bcache.end_op(&ctx);
        return -1;
    }
    inodes.unlock(ip);
    inodes.put(&ctx, curproc->cwd);
    bcache.end_op(&ctx);
    curproc->cwd = ip;
    return 0;
}

int execve(const char* path, char* const argv[], char* const envp[]);
define_syscall(execve) {
    // TODO: Lab10 Shell
    char* p;
    void *argv, *envp;
    if (argstr(0, &p) < 0 || argu64(1, (u64*)&argv) < 0 || argu64(2, (u64*)&envp) < 0)
        return -1;
    return execve(p, argv, envp);
}
