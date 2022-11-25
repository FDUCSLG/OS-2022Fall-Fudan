#include <driver/sd.h>
#include <fs/block_device.h>
#include <kernel/printk.h>

#define BLOCKNO_OFFSET 0x20800

static void sd_read(usize block_no, u8* buffer) {
    struct buf b;
    b.blockno = (u32)block_no+BLOCKNO_OFFSET;
    b.flags = 0;
    sdrw(&b);
    memcpy(buffer, b.data, BLOCK_SIZE);
}

static void sd_write(usize block_no, u8* buffer) {
    struct buf b;
    b.blockno = (u32)block_no+BLOCKNO_OFFSET;
    b.flags = B_DIRTY | B_VALID;
    memcpy(b.data, buffer, BLOCK_SIZE);
    sdrw(&b);
}

static u8 sblock_data[BLOCK_SIZE];
BlockDevice block_device;

void init_block_device() {
    // FIXME
    // sd_init();
    sd_read(1, sblock_data);
    block_device.read = sd_read;
    block_device.write = sd_write;
	const SuperBlock* sb = get_super_block();
	printk("num_blocks: %d\n",sb->num_blocks);
	printk("num_data_blocks: %d\n", sb->num_data_blocks);
	printk("num_inodes: %d\n", sb->num_inodes);
	printk("num_log_blocks: %d\n", sb->num_log_blocks);
	printk("log_start: %d\n", sb->log_start);
	printk("inode_start: %d\n", sb->inode_start);
	printk("bitmap_start: %d\n", sb->bitmap_start);
}

const SuperBlock* get_super_block() {
    return (const SuperBlock*)sblock_data;
}
