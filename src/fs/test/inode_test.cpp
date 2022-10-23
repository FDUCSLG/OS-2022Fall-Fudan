extern "C" {
#include <fs/inode.h>
}

#include "assert.hpp"
#include "pause.hpp"
#include "runner.hpp"

#include "mock/cache.hpp"

void test_init() {
    init_inodes(&sblock, &cache);
    assert_eq(mock.count_inodes(), 1);
    assert_eq(mock.count_blocks(), 0);
}

namespace adhoc {

static OpContext _ctx, *ctx = &_ctx;

void test_alloc() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);

    assert_eq(mock.count_inodes(), 1);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 2);

    auto* p = inodes.get(ino);

    inodes.lock(p);
    // printf("hello\n");
    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);

    assert_eq(mock.count_inodes(), 2);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 1);
}

void test_sync() {
    auto* p = inodes.get(1);

    inodes.lock(p);
    assert_eq(p->entry.type, INODE_DIRECTORY);
    p->entry.major = 0x19;
    p->entry.minor = 0x26;
    p->entry.indirect = 0xa817;
    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.lock(p);
    inodes.sync(ctx, p, true);
    inodes.unlock(p);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    auto* q = mock.inspect(1);
    assert_eq(q->type, INODE_DIRECTORY);
    assert_eq(q->major, 0x19);
    assert_eq(q->minor, 0x26);
    assert_eq(q->indirect, 0xa817);
}

void test_touch() {
    auto* p = inodes.get(1);
    inodes.lock(p);

    for (usize i = 2; i < mock.num_inodes; i++) {
        mock.begin_op(ctx);
        usize ino = inodes.alloc(ctx, INODE_REGULAR);
        inodes.insert(ctx, p, std::to_string(i).data(), ino);

        auto* q = inodes.get(ino);
        inodes.lock(q);
        assert_eq(q->entry.type, INODE_REGULAR);
        assert_eq(q->entry.major, 0);
        assert_eq(q->entry.minor, 0);
        assert_eq(q->entry.num_links, 0);
        assert_eq(q->entry.num_bytes, 0);
        assert_eq(q->entry.indirect, 0);
        for (usize j = 0; j < INODE_NUM_DIRECT; j++) {
            assert_eq(q->entry.addrs[j], 0);
        }

        q->entry.num_links++;

        inodes.sync(ctx, q, true);

        inodes.unlock(q);
        inodes.put(ctx, q);

        assert_eq(mock.count_inodes(), i - 1);
        mock.end_op(ctx);
        assert_eq(mock.count_inodes(), i);
    }

    usize n = mock.num_inodes - 1;
    for (usize i = 2; i < mock.num_inodes; i += 2, n--) {
        mock.begin_op(ctx);
        usize index = 10086;
        assert_ne(inodes.lookup(p, std::to_string(i).data(), &index), 0);
        assert_ne(index, 10086);
        inodes.remove(ctx, p, index);

        auto* q = inodes.get(i);
        inodes.lock(q);
        q->entry.num_links = 0;
        inodes.sync(ctx, q, true);
        inodes.unlock(q);
        inodes.put(ctx, q);

        assert_eq(mock.count_inodes(), n);
        mock.end_op(ctx);
        assert_eq(mock.count_inodes(), n - 1);
    }

    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_DIRECTORY);
    auto* q = inodes.get(ino);
    inodes.lock(q);
    assert_eq(q->entry.type, INODE_DIRECTORY);
    inodes.unlock(q);
    assert_eq(mock.count_inodes(), n);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), n + 1);

    mock.begin_op(ctx);
    inodes.put(ctx, q);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), n);

    for (usize i = 3; i < mock.num_inodes; i += 2, n--) {
        mock.begin_op(ctx);
        q = inodes.get(i);
        inodes.lock(q);
        q->entry.num_links = 0;
        inodes.sync(ctx, q, true);
        inodes.unlock(q);
        inodes.put(ctx, q);
        assert_eq(mock.count_inodes(), n);
        mock.end_op(ctx);
        assert_eq(mock.count_inodes(), n - 1);
    }

    inodes.unlock(p);
}

void test_share() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 2);

    auto* p = inodes.get(ino);
    auto* q = inodes.share(p);
    auto* r = inodes.get(ino);

    assert_eq(r->rc.count, 3);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    assert_eq(q->rc.count, 2);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 2);

    mock.begin_op(ctx);
    inodes.put(ctx, q);
    assert_eq(r->rc.count, 1);
    assert_eq(mock.count_inodes(), 2);
    inodes.put(ctx, r);
    assert_eq(mock.count_inodes(), 2);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 1);
}

void test_small_file() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    u8 buf[1];
    auto* p = inodes.get(ino);
    inodes.lock(p);

    buf[0] = 0xcc;
    inodes.read(p, buf, 0, 0);
    assert_eq(buf[0], 0xcc);

    mock.begin_op(ctx);
    inodes.write(ctx, p, buf, 0, 1);
    assert_eq(mock.count_blocks(), 0);
    mock.end_op(ctx);

    auto* q = mock.inspect(ino);
    assert_eq(q->indirect, 0);
    assert_ne(q->addrs[0], 0);
    assert_eq(q->addrs[1], 0);
    assert_eq(q->num_bytes, 1);
    assert_eq(mock.count_blocks(), 1);

    mock.fill_junk();
    buf[0] = 0;
    inodes.read(p, buf, 0, 1);
    assert_eq(buf[0], 0xcc);

    inodes.unlock(p);

    inodes.lock(p);

    mock.begin_op(ctx);
    inodes.clear(ctx, p);
    mock.end_op(ctx);

    q = mock.inspect(ino);
    assert_eq(q->indirect, 0);
    assert_eq(q->addrs[0], 0);
    assert_eq(q->num_bytes, 0);
    assert_eq(mock.count_blocks(), 0);

    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 1);
}

void test_large_file() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    constexpr usize max_size = 65535;
    u8 buf[max_size], copy[max_size];
    std::mt19937 gen(0x12345678);
    for (usize i = 0; i < max_size; i++) {
        copy[i] = buf[i] = gen() & 0xff;
    }

    auto* p = inodes.get(ino);

    inodes.lock(p);
    for (usize i = 0, n = 0; i < max_size; i += n) {
        n = std::min(static_cast<usize>(gen() % 10000), max_size - i);

        mock.begin_op(ctx);
        inodes.write(ctx, p, buf + i, i, n);
        auto* q = mock.inspect(ino);
        assert_eq(q->num_bytes, i);
        mock.end_op(ctx);
        assert_eq(q->num_bytes, i + n);
    }
    inodes.unlock(p);

    for (usize i = 0; i < max_size; i++) {
        buf[i] = 0;
    }

    inodes.lock(p);
    inodes.read(p, buf, 0, max_size);
    inodes.unlock(p);

    for (usize i = 0; i < max_size; i++) {
        assert_eq(buf[i], copy[i]);
    }

    inodes.lock(p);
    mock.begin_op(ctx);
    inodes.clear(ctx, p);
    inodes.unlock(p);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 2);
    assert_eq(mock.count_blocks(), 0);

    for (usize i = 0; i < max_size; i++) {
        copy[i] = buf[i] = gen() & 0xff;
    }

    inodes.lock(p);
    mock.begin_op(ctx);
    inodes.write(ctx, p, buf, 0, max_size);
    mock.end_op(ctx);
    inodes.unlock(p);

    auto* q = mock.inspect(ino);
    assert_eq(q->num_bytes, max_size);

    for (usize i = 0; i < max_size; i++) {
        buf[i] = 0;
    }

    inodes.lock(p);
    for (usize i = 0, n = 0; i < max_size; i += n) {
        n = std::min(static_cast<usize>(gen() % 10000), max_size - i);
        inodes.read(p, buf + i, i, n);
        for (usize j = 0; j < i + n; j++) {
            assert_eq(buf[j], copy[j]);
        }
    }
    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    assert_eq(mock.count_inodes(), 1);
    assert_eq(mock.count_blocks(), 0);
}

void test_dir() {
    usize ino[5] = {1};

    mock.begin_op(ctx);
    ino[1] = inodes.alloc(ctx, INODE_DIRECTORY);
    ino[2] = inodes.alloc(ctx, INODE_REGULAR);
    ino[3] = inodes.alloc(ctx, INODE_REGULAR);
    ino[4] = inodes.alloc(ctx, INODE_REGULAR);
    assert_eq(mock.count_inodes(), 1);
    mock.end_op(ctx);
    assert_eq(mock.count_inodes(), 5);

    Inode* p[5];
    for (usize i = 0; i < 5; i++) {
        p[i] = inodes.get(ino[i]);
        inodes.lock(p[i]);
    }

    mock.begin_op(ctx);
    inodes.insert(ctx, p[0], "fudan", ino[1]);
    p[1]->entry.num_links++;
    inodes.sync(ctx, p[1], true);

    auto* q = mock.inspect(ino[0]);
    assert_eq(q->addrs[0], 0);
    assert_eq(inodes.lookup(p[0], "fudan", NULL), ino[1]);
    mock.end_op(ctx);

    assert_eq(inodes.lookup(p[0], "fudan", NULL), ino[1]);
    assert_eq(inodes.lookup(p[0], "sjtu", NULL), 0);
    assert_eq(inodes.lookup(p[0], "pku", NULL), 0);
    assert_eq(inodes.lookup(p[0], "tsinghua", NULL), 0);

    mock.begin_op(ctx);
    inodes.insert(ctx, p[0], ".vimrc", ino[2]);
    inodes.insert(ctx, p[1], "alice", ino[3]);
    inodes.insert(ctx, p[1], "bob", ino[4]);
    p[2]->entry.num_links++;
    p[3]->entry.num_links++;
    p[4]->entry.num_links++;
    inodes.sync(ctx, p[2], true);
    inodes.sync(ctx, p[3], true);
    inodes.sync(ctx, p[4], true);
    mock.end_op(ctx);

    for (usize i = 1; i < 5; i++) {
        q = mock.inspect(ino[i]);
        assert_eq(q->num_links, 1);
    }

    usize index = 233;
    assert_eq(inodes.lookup(p[0], "vimrc", &index), 0);
    assert_eq(index, 233);
    assert_eq(inodes.lookup(p[0], ".vimrc", &index), ino[2]);
    assert_ne(index, 233);
    index = 244;
    assert_eq(inodes.lookup(p[1], "nano", &index), 0);
    assert_eq(index, 244);
    assert_eq(inodes.lookup(p[1], "alice", &index), ino[3]);
    usize index2 = 255;
    assert_eq(inodes.lookup(p[1], "bob", &index2), ino[4]);
    assert_ne(index, 244);
    assert_ne(index2, 255);
    assert_ne(index, index2);

    mock.begin_op(ctx);
    inodes.clear(ctx, p[1]);
    p[2]->entry.num_links = 0;
    inodes.sync(ctx, p[2], true);

    q = mock.inspect(ino[1]);
    assert_ne(q->addrs[0], 0);
    assert_eq(inodes.lookup(p[1], "alice", NULL), 0);
    assert_eq(inodes.lookup(p[1], "bob", NULL), 0);
    mock.end_op(ctx);

    assert_eq(q->addrs[0], 0);
    assert_eq(mock.count_inodes(), 5);
    assert_ne(mock.count_blocks(), 0);

    for (usize i = 0; i < 5; i++) {
        mock.begin_op(ctx);
        inodes.unlock(p[i]);
        inodes.put(ctx, p[i]);
        mock.end_op(ctx);
        assert_eq(mock.count_inodes(), (i < 2 ? 5 : 4));
    }
}

}  // namespace adhoc

int main() {
    if (Runner::run({"init", test_init}))
        init_inodes(&sblock, &cache);
    else
        return -1;

    std::vector<Testcase> tests = {
        {"alloc", adhoc::test_alloc},
        {"sync", adhoc::test_sync},
        {"touch", adhoc::test_touch},
        {"share", adhoc::test_share},
        {"small_file", adhoc::test_small_file},
        {"large_file", adhoc::test_large_file},
        {"dir", adhoc::test_dir},
    };
    Runner(tests).run();

    return 0;
}
