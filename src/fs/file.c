/* File descriptors */

#include "file.h"
#include <common/defines.h>
#include <common/spinlock.h>
#include <common/sem.h>
#include <fs/inode.h>
#include <common/list.h>
#include <kernel/mem.h>
#include "fs.h"

static struct ftable ftable;

void init_ftable() {
    // TODO: initialize your ftable
}

void init_oftable(struct oftable *oftable) {
    // TODO: initialize your oftable for a new process
}

/* Allocate a file structure. */
struct file* filealloc() {
    /* TODO: Lab10 Shell */
    return 0;
}

/* Increment ref count for file f. */
struct file* filedup(struct file* f) {
    /* TODO: Lab10 Shell */
    return f;
}

/* Close file f. (Decrement ref count, close when reaches 0.) */
void fileclose(struct file* f) {
    /* TODO: Lab10 Shell */
}

/* Get metadata about file f. */
int filestat(struct file* f, struct stat* st) {
    /* TODO: Lab10 Shell */
    return -1;
}

/* Read from file f. */
isize fileread(struct file* f, char* addr, isize n) {
    /* TODO: Lab10 Shell */
    return 0;
}

/* Write to file f. */
isize filewrite(struct file* f, char* addr, isize n) {
    /* TODO: Lab10 Shell */
    return 0;
}
