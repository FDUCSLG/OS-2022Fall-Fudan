#pragma once

extern "C" {
#include <fs/cache.h>
}

#include <atomic>
#include <fstream>
#include <functional>
#include <iomanip>
#include <mutex>
#include <random>
#include <vector>

#include "../exception.hpp"

struct MockBlockDevice {
    struct Block {
        std::mutex mutex;
        u8 data[BLOCK_SIZE];

        void fill_junk() {
            static std::mt19937 gen(0x19260817);

            for (usize i = 0; i < BLOCK_SIZE; i++) {
                data[i] = gen() & 0xff;
            }
        }

        void fill_zero() {
            std::fill(std::begin(data), std::end(data), 0);
        }
    };

    const SuperBlock *sblock;

    std::atomic<bool> offline;
    std::atomic<usize> read_count;
    std::atomic<usize> write_count;
    std::vector<Block> disk;

    using Hook = std::function<void(usize block_no, u8 *buffer)>;

    Hook on_read;
    Hook on_write;

    void initialize(const SuperBlock &_sblock) {
        sblock = &_sblock;

        offline = false;
        read_count = 0;
        write_count = 0;
        {
            std::vector<Block> new_disk(sblock->num_blocks);
            std::swap(disk, new_disk);
        }

        for (auto &block : disk) {
            block.fill_junk();
        }

        if (sblock->num_log_blocks < 2)
            throw Internal("logging area is too small");
        disk[sblock->log_start].fill_zero();

        usize num_bitmap_blocks = (sblock->num_data_blocks + BIT_PER_BLOCK - 1) / BIT_PER_BLOCK;
        for (usize i = 0; i < num_bitmap_blocks; i++) {
            disk[sblock->bitmap_start + i].fill_zero();
        }

        usize num_preallocated = 1 + 1 + sblock->num_log_blocks +
                                 ((sblock->num_inodes + INODE_PER_BLOCK - 1) / INODE_PER_BLOCK) +
                                 num_bitmap_blocks;
        if (num_preallocated + sblock->num_data_blocks > sblock->num_blocks)
            throw Internal("invalid super block");
        for (usize i = 0; i < num_preallocated; i++) {
            usize j = i / BIT_PER_BLOCK, k = i % BIT_PER_BLOCK;
            disk[sblock->bitmap_start + j].data[k / 8] |= (1 << (k % 8));
        }
    }

    auto inspect(usize block_no) -> u8 * {
        if (block_no >= disk.size())
            throw Internal("block number is out of range");
        return disk[block_no].data;
    }

    auto inspect_log(usize index) -> u8 * {
        return inspect(sblock->log_start + 1 + index);
    }

    auto inspect_log_header() -> LogHeader * {
        return reinterpret_cast<LogHeader *>(inspect(sblock->log_start));
    }

    void dump(std::ostream &stream) {
        for (auto &block : disk) {
            std::scoped_lock lock(block.mutex);
            for (usize i = 0; i < BLOCK_SIZE; i++) {
                stream << std::setfill('0') << std::setw(2) << std::hex
                       << static_cast<u64>(block.data[i]) << " ";
            }
            stream << "\n";
        }
    }

    void load(std::istream &stream) {
        for (auto &block : disk) {
            for (usize i = 0; i < BLOCK_SIZE; i++) {
                u64 value;
                stream >> std::hex >> value;
                block.data[i] = value & 0xff;
            }
        }
    }

    void dump(const std::string &path) {
        std::ofstream file(path);
        dump(file);
    }

    void load(const std::string &path) {
        std::ifstream file(path);
        load(file);
    }

    void check_offline() {
        if (offline)
            throw Offline("disk power failure");
    }

    void read(usize block_no, u8 *buffer) {
        if (block_no >= disk.size())
            throw AssertionFailure("block number is out of range");

        check_offline();

        auto &block = disk[block_no];
        std::scoped_lock lock(block.mutex);

        if (on_read)
            on_read(block_no, buffer);

        check_offline();

        for (usize i = 0; i < BLOCK_SIZE; i++) {
            buffer[i] = block.data[i];
        }

        read_count++;

        check_offline();
    }

    void write(usize block_no, u8 *buffer) {
        if (block_no >= disk.size())
            throw AssertionFailure("block number is out of range");

        check_offline();

        auto &block = disk[block_no];
        std::scoped_lock lock(block.mutex);

        if (on_write)
            on_write(block_no, buffer);

        check_offline();

        for (usize i = 0; i < BLOCK_SIZE; i++) {
            block.data[i] = buffer[i];
        }

        write_count++;

        check_offline();
    }
};

namespace {
#include "block_device.ipp"
}  // namespace
