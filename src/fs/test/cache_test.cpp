extern "C" {
#include <fs/cache.h>
}

#include "assert.hpp"
#include "pause.hpp"
#include "runner.hpp"

#include "mock/block_device.hpp"

#include <chrono>
#include <condition_variable>
#include <random>
#include <thread>

namespace {

constexpr int IN_CHILD = 0;

static void wait_process(int pid) {
    int wstatus;
    waitpid(pid, &wstatus, 0);
    if (!WIFEXITED(wstatus)) {
        std::stringstream buf;
        buf << "process [" << pid << "] exited abnormally with code " << wstatus;
        throw Internal(buf.str());
    }
}

}  // namespace

namespace basic {

void test_init() {
    initialize(1, 1);
}

// targets: `acquire`, `release`, `sync(NULL, ...)`.

void test_read_write() {
    initialize(1, 1);

    auto* b = bcache.acquire(1);
    auto* d = mock.inspect(1);
    assert_eq(b->block_no, 1);
    assert_eq(b->valid, true);

    for (usize i = 0; i < BLOCK_SIZE; i++) {
        assert_eq(b->data[i], d[i]);
    }

    u8 value = b->data[128];
    b->data[128] = ~value;
    bcache.sync(NULL, b);
    assert_eq(d[128], ~value);

    bcache.release(b);
    b = bcache.acquire(1);
}

void test_loop_read() {
    initialize(1, 128);
    constexpr usize num_rounds = 10;
    for (usize round = 0; round < num_rounds; round++) {
        std::vector<Block*> p;
        p.resize(sblock.num_blocks);

        for (usize i = 0; i < sblock.num_blocks; i++) {
            // PAUSE
            p[i] = bcache.acquire(i);

            assert_eq(p[i]->block_no, i);

            auto* d = mock.inspect(i);
            for (usize j = 0; j < BLOCK_SIZE; j++) {
                assert_eq(p[i]->data[j], d[j]);
            }
        }

        for (usize i = 0; i < sblock.num_blocks; i++) {
            assert_eq(p[i]->valid, true);
            bcache.release(p[i]);
        }
    }
}

void test_reuse() {
    initialize(1, 500);

    constexpr usize num_rounds = 200;
    constexpr usize blocks[] = {1, 123, 233, 399, 415};

    auto matched = [&](usize bno) {
        for (usize b : blocks) {
            if (bno == b)
                return true;
        }
        return false;
    };

    usize rcnt = 0, wcnt = 0;
    mock.on_read = [&](usize bno, auto) {
        if (matched(bno))
            rcnt++;
    };
    mock.on_write = [&](usize bno, auto) {
        if (matched(bno))
            wcnt++;
    };

    for (usize round = 0; round < num_rounds; round++) {
        std::vector<Block*> p;
        for (usize block_no : blocks) {
            p.push_back(bcache.acquire(block_no));
        }
        for (auto* b : p) {
            assert_eq(b->valid, true);
            bcache.release(b);
        }
    }

    assert_true(rcnt < 10);
    assert_eq(wcnt, 0);
}

void test_lru() {
    std::mt19937 gen(0xdeadbeef);

    usize cold_size = 1000;
    usize hot_size = EVICTION_THRESHOLD * 0.8;
    initialize(1, cold_size + hot_size);
    for (int i = 0; i < 1000; i++) {
        bool hot = (gen() % 100) <= 90;
        usize bno = hot ? (gen() % hot_size) : (hot_size + gen() % cold_size);

        auto* b = bcache.acquire(bno);
        auto* d = mock.inspect(bno);
        assert_eq(b->data[123], d[123]);
        bcache.release(b);
    }

    printf("(debug) #cached = %zu, #read = %zu\n", bcache.get_num_cached_blocks(),
           mock.read_count.load());
    assert_true(bcache.get_num_cached_blocks() <= EVICTION_THRESHOLD);
    assert_true(mock.read_count < 233);
    assert_true(mock.write_count < 5);
}

// targets: `begin_op`, `end_op`, `sync`.

void test_atomic_op() {
    initialize(32, 64);

    OpContext ctx;
    bcache.begin_op(&ctx);
    bcache.end_op(&ctx);

    bcache.begin_op(&ctx);

    usize t = sblock.num_blocks - 1;
    auto* b = bcache.acquire(t);
    assert_eq(b->block_no, t);
    assert_eq(b->valid, true);
    auto* d = mock.inspect(t);
    u8 v = d[128];
    assert_eq(b->data[128], v);

    b->data[128] = ~v;
    bcache.sync(&ctx, b);
    bcache.release(b);

    assert_eq(d[128], v);
    bcache.end_op(&ctx);
    assert_eq(d[128], ~v);

    bcache.begin_op(&ctx);

    auto* b1 = bcache.acquire(t - 1);
    auto* b2 = bcache.acquire(t - 2);
    assert_eq(b1->block_no, t - 1);
    assert_eq(b2->block_no, t - 2);

    auto* d1 = mock.inspect(t - 1);
    auto* d2 = mock.inspect(t - 2);
    u8 v1 = d1[500];
    u8 v2 = d2[10];
    assert_eq(b1->data[500], v1);
    assert_eq(b2->data[10], v2);

    b1->data[500] = ~v1;
    b2->data[10] = ~v2;
    bcache.sync(&ctx, b1);
    bcache.release(b1);
    bcache.sync(&ctx, b2);
    bcache.release(b2);

    assert_eq(d1[500], v1);
    assert_eq(d2[10], v2);
    bcache.end_op(&ctx);
    assert_eq(d1[500], ~v1);
    assert_eq(d2[10], ~v2);
}

void test_overflow() {
    initialize(100, 100);

    OpContext ctx;
    bcache.begin_op(&ctx);

    usize t = sblock.num_blocks - 1;
    for (usize i = 0; i < OP_MAX_NUM_BLOCKS; i++) {
        auto* b = bcache.acquire(t - i);
        b->data[0] = 0xaa;
        bcache.sync(&ctx, b);
        bcache.release(b);
    }

    bool panicked = false;
    auto* b = bcache.acquire(t - OP_MAX_NUM_BLOCKS);
    b->data[128] = 0x88;
    try {
        bcache.sync(&ctx, b);
    } catch (const Panic&) {
        panicked = true;
    }

    assert_eq(panicked, true);
}

void test_resident() {
    // NOTE: this test may be a little controversial.
    // the main ideas are:
    // 1. dirty blocks should be pinned in block cache before `end_op`.
    // 2. logging should not pollute block cache in most of time.

    initialize(OP_MAX_NUM_BLOCKS, 500);

    constexpr usize num_rounds = 200;
    constexpr usize blocks[] = {1, 123, 233, 399, 415};

    auto matched = [&](usize bno) {
        for (usize b : blocks) {
            if (bno == b)
                return true;
        }
        return false;
    };

    usize rcnt = 0;
    mock.on_read = [&](usize bno, auto) {
        if (matched(bno))
            rcnt++;
    };

    for (usize round = 0; round < num_rounds; round++) {
        OpContext ctx;
        bcache.begin_op(&ctx);

        for (usize block_no : blocks) {
            auto* b = bcache.acquire(block_no);
            assert_eq(b->valid, true);
            b->data[0] = 0;
            bcache.sync(&ctx, b);
            bcache.release(b);
        }

        bcache.end_op(&ctx);
    }

    assert_true(rcnt < 10);
}

void test_local_absorption() {
    constexpr usize num_rounds = 1000;

    initialize(100, 100);

    OpContext ctx;
    bcache.begin_op(&ctx);
    usize t = sblock.num_blocks - 1;
    for (usize i = 0; i < num_rounds; i++) {
        for (usize j = 0; j < OP_MAX_NUM_BLOCKS; j++) {
            auto* b = bcache.acquire(t - j);
            b->data[0] = 0xcd;
            bcache.sync(&ctx, b);
            bcache.release(b);
        }
    }
    bcache.end_op(&ctx);

    assert_true(mock.read_count < OP_MAX_NUM_BLOCKS * 5);
    assert_true(mock.write_count < OP_MAX_NUM_BLOCKS * 5);
    for (usize j = 0; j < OP_MAX_NUM_BLOCKS; j++) {
        auto* b = mock.inspect(t - j);
        assert_eq(b[0], 0xcd);
    }
}

void test_global_absorption() {
    constexpr usize op_size = 3;
    constexpr usize num_workers = 100;

    initialize(2 * OP_MAX_NUM_BLOCKS + op_size, 100);
    usize t = sblock.num_blocks - 1;

    OpContext out;
    bcache.begin_op(&out);

    for (usize i = 0; i < OP_MAX_NUM_BLOCKS; i++) {
        auto* b = bcache.acquire(t - i);
        b->data[0] = 0xcc;
        bcache.sync(&out, b);
        bcache.release(b);
    }

    std::vector<OpContext> ctx;
    std::vector<std::thread> workers;
    ctx.resize(num_workers);
    workers.reserve(num_workers);

    for (usize i = 0; i < num_workers; i++) {
        bcache.begin_op(&ctx[i]);
        for (usize j = 0; j < op_size; j++) {
            auto* b = bcache.acquire(t - j);
            b->data[0] = 0xdd;
            bcache.sync(&ctx[i], b);
            bcache.release(b);
        }
        workers.emplace_back([&, i] { bcache.end_op(&ctx[i]); });
    }

    workers.emplace_back([&] { bcache.end_op(&out); });
    for (auto& worker : workers) {
        worker.join();
    }
    for (usize i = 0; i < op_size; i++) {
        auto* b = mock.inspect(t - i);
        assert_eq(b[0], 0xdd);
    }

    for (usize i = op_size; i < OP_MAX_NUM_BLOCKS; i++) {
        auto* b = mock.inspect(t - i);
        assert_eq(b[0], 0xcc);
    }
}

// target: replay at initialization.

void test_replay() {
    initialize_mock(50, 1000);

    auto* header = mock.inspect_log_header();
    header->num_blocks = 5;
    for (usize i = 0; i < 5; i++) {
        usize v = 500 + i;
        header->block_no[i] = v;
        auto* b = mock.inspect_log(i);
        for (usize j = 0; j < BLOCK_SIZE; j++) {
            b[j] = v & 0xff;
        }
    }

    init_bcache(&sblock, &device);

    assert_eq(header->num_blocks, 0);
    for (usize i = 0; i < 5; i++) {
        usize v = 500 + i;
        auto* b = mock.inspect(v);
        for (usize j = 0; j < BLOCK_SIZE; j++) {
            assert_eq(b[j], v & 0xff);
        }
    }
}

// targets: `alloc`, `free`.

void test_alloc() {
    initialize(100, 100);

    std::vector<usize> bno;
    bno.reserve(100);
    for (int i = 0; i < 100; i++) {
        OpContext ctx;
        bcache.begin_op(&ctx);

        bno.push_back(bcache.alloc(&ctx));
        assert_ne(bno[i], 0);
        assert_true(bno[i] < sblock.num_blocks);

        auto* b = bcache.acquire(bno[i]);
        for (usize j = 0; j < BLOCK_SIZE; j++) {
            assert_eq(b->data[j], 0);
        }
        bcache.release(b);

        bcache.end_op(&ctx);
        auto* d = mock.inspect(bno[i]);
        for (usize j = 0; j < BLOCK_SIZE; j++) {
            assert_eq(d[j], 0);
        }
    }

    std::sort(bno.begin(), bno.end());
    usize count = std::unique(bno.begin(), bno.end()) - bno.begin();
    assert_eq(count, bno.size());

    OpContext ctx;
    bcache.begin_op(&ctx);

    bool panicked = false;
    try {
        usize b = bcache.alloc(&ctx);
        assert_ne(b, 0);
    } catch (const Panic&) {
        panicked = true;
    }

    assert_eq(panicked, true);
}

void test_alloc_free() {
    constexpr usize num_rounds = 5;
    constexpr usize num_data_blocks = 1000;

    initialize(100, num_data_blocks);

    for (usize round = 0; round < num_rounds; round++) {
        std::vector<usize> bno;
        for (usize i = 0; i < num_data_blocks; i++) {
            OpContext ctx;
            bcache.begin_op(&ctx);
            bno.push_back(bcache.alloc(&ctx));
            bcache.end_op(&ctx);
        }

        for (usize b : bno) {
            assert_true(b >= sblock.num_blocks - num_data_blocks);
        }

        for (usize i = 0; i < num_data_blocks; i += 2) {
            usize no = bno[i];
            assert_ne(no, 0);

            OpContext ctx;
            bcache.begin_op(&ctx);
            bcache.free(&ctx, no);
            bcache.end_op(&ctx);
        }

        OpContext ctx;
        bcache.begin_op(&ctx);
        usize no = bcache.alloc(&ctx);
        assert_ne(no, 0);
        for (usize i = 1; i < num_data_blocks; i += 2) {
            assert_ne(bno[i], no);
        }
        bcache.free(&ctx, no);
        bcache.end_op(&ctx);

        for (usize i = 1; i < num_data_blocks; i += 2) {
            bcache.begin_op(&ctx);
            bcache.free(&ctx, bno[i]);
            bcache.end_op(&ctx);
        }
    }
}

}  // namespace basic

namespace concurrent {

void test_acquire() {
    constexpr usize num_rounds = 100;
    constexpr usize num_workers = 64;

    for (usize round = 0; round < num_rounds; round++) {
        int child;
        if ((child = fork()) == IN_CHILD) {
            initialize(1, num_workers);

            std::atomic<bool> flag = false;
            std::vector<std::thread> workers;
            for (usize i = 0; i < num_workers; i++) {
                workers.emplace_back([&, i] {
                    while (!flag) {
                        std::this_thread::yield();
                    }

                    usize t = sblock.num_blocks - 1 - i;
                    auto* b = bcache.acquire(t);
                    assert_eq(b->block_no, t);
                    assert_eq(b->valid, true);
                    bcache.release(b);
                });
            }

            flag = true;
            for (auto& worker : workers) {
                worker.join();
            }

            exit(0);
        } else {
            wait_process(child);
        }
    }
}

void test_sync() {
    constexpr int num_rounds = 100;

    initialize(OP_MAX_NUM_BLOCKS * OP_MAX_NUM_BLOCKS, OP_MAX_NUM_BLOCKS);

    std::mutex mtx;
    std::condition_variable cv;
    OpContext ctx;
    int count = -1, round = -1;

    auto cookie = [](int i, int j) { return (i + 1) * 1926 + j + 817; };

    std::vector<std::thread> workers;
    for (int i = 0; i < OP_MAX_NUM_BLOCKS; i++) {
        workers.emplace_back([&, i] {
            usize t = sblock.num_blocks - 1 - i;
            for (int j = 0; j < num_rounds; j++) {
                {
                    std::unique_lock lock(mtx);
                    cv.wait(lock, [&] { return j <= round; });
                }

                auto* b = bcache.acquire(t);
                int* p = reinterpret_cast<int*>(b->data);
                *p = cookie(i, j);
                bcache.sync(&ctx, b);
                bcache.release(b);

                {
                    std::unique_lock lock(mtx);
                    count++;
                }

                cv.notify_all();
            }
        });
    }

    auto check = [&](int j) {
        for (int i = 0; i < OP_MAX_NUM_BLOCKS; i++) {
            int* b = reinterpret_cast<int*>(mock.inspect(sblock.num_blocks - 1 - i));
            assert_eq(*b, cookie(i, j));
        }
    };

    {
        std::unique_lock lock(mtx);
        for (int j = 0; j < num_rounds; j++) {
            bcache.begin_op(&ctx);
            round = j;
            count = 0;
            cv.notify_all();

            cv.wait(lock, [&] { return count >= OP_MAX_NUM_BLOCKS; });

            if (j > 0)
                check(j - 1);
            bcache.end_op(&ctx);
            check(j);
        }
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

void test_alloc() {
    initialize(100, 1000);

    std::vector<usize> bno(1000);
    std::vector<std::thread> workers;
    for (usize i = 0; i < 4; i++) {
        workers.emplace_back([&, i] {
            usize t = 250 * i;
            for (usize j = 0; j < 250; j++) {
                OpContext ctx;
                bcache.begin_op(&ctx);
                bno[t + j] = bcache.alloc(&ctx);
                bcache.end_op(&ctx);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
    std::sort(bno.begin(), bno.end());
    usize count = std::unique(bno.begin(), bno.end()) - bno.begin();
    assert_eq(count, 1000);
    assert_true(bno.front() >= sblock.num_blocks - 1000);
    assert_true(bno.back() < sblock.num_blocks);
}

}  // namespace concurrent

namespace crash {

void test_simple_crash() {
    int child;
    if ((child = fork()) == IN_CHILD) {
        initialize(100, 100);

        OpContext ctx;
        bcache.begin_op(&ctx);
        auto* b = bcache.acquire(150);
        b->data[200] = 0x19;
        b->data[201] = 0x26;
        b->data[202] = 0x08;
        b->data[203] = 0x17;
        bcache.sync(&ctx, b);
        bcache.release(b);
        bcache.end_op(&ctx);

        bcache.begin_op(&ctx);
        b = bcache.acquire(150);
        b->data[200] = 0xcc;
        b->data[201] = 0xcc;
        b->data[202] = 0xcc;
        b->data[203] = 0xcc;
        bcache.sync(&ctx, b);
        bcache.release(b);

        mock.offline = true;

        try {
            bcache.end_op(&ctx);
        } catch (const Offline&) {
        }

        mock.dump("sd.img");

        exit(0);
    } else {
        wait_process(child);
        initialize_mock(100, 100, "sd.img");

        auto* b = mock.inspect(150);
        assert_eq(b[200], 0x19);
        assert_eq(b[201], 0x26);
        assert_eq(b[202], 0x08);
        assert_eq(b[203], 0x17);

        init_bcache(&sblock, &device);
        assert_eq(b[200], 0x19);
        assert_eq(b[201], 0x26);
        assert_eq(b[202], 0x08);
        assert_eq(b[203], 0x17);
    }
}

void test_parallel(usize num_rounds, usize num_workers, usize delay_ms, usize log_cut) {
    usize log_size = num_workers * OP_MAX_NUM_BLOCKS - log_cut;
    usize num_data_blocks = 200 + num_workers * OP_MAX_NUM_BLOCKS;

    printf("(trace) running: 0/%zu", num_rounds);
    fflush(stdout);

    usize replay_count = 0;
    for (usize round = 0; round < num_rounds; round++) {
        int child;
        if ((child = fork()) == IN_CHILD) {
            initialize_mock(log_size, num_data_blocks);
            for (usize i = 0; i < num_workers * OP_MAX_NUM_BLOCKS; i++) {
                auto* b = mock.inspect(200 + i);
                std::fill(b, b + BLOCK_SIZE, 0);
            }

            init_bcache(&sblock, &device);

            std::atomic<bool> started = false;
            for (usize i = 0; i < num_workers; i++) {
                std::thread([&, i] {
                    started = true;
                    usize t = 200 + i * OP_MAX_NUM_BLOCKS;
                    try {
                        u64 v = 0;
                        while (true) {
                            OpContext ctx;
                            bcache.begin_op(&ctx);
                            for (usize j = 0; j < OP_MAX_NUM_BLOCKS; j++) {
                                auto* b = bcache.acquire(t + j);
                                for (usize k = 0; k < BLOCK_SIZE; k += sizeof(u64)) {
                                    u64* p = reinterpret_cast<u64*>(b->data + k);
                                    *p = v;
                                }
                                bcache.sync(&ctx, b);
                                bcache.release(b);
                            }
                            bcache.end_op(&ctx);

                            v++;
                        }
                    } catch (const Offline&) {
                    }
                }).detach();
            }

            // disk will power off after `delay_ms` ms.
            std::thread aha([&] {
                while (!started) {
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                mock.offline = true;
            });

            aha.join();
            mock.dump("sd.img");
            _exit(0);
        } else {
            wait_process(child);
            initialize_mock(log_size, num_data_blocks, "sd.img");
            auto* header = mock.inspect_log_header();
            if (header->num_blocks > 0)
                replay_count++;

            if ((child = fork()) == IN_CHILD) {
                init_bcache(&sblock, &device);
                assert_eq(header->num_blocks, 0);

                for (usize i = 0; i < num_workers; i++) {
                    usize t = 200 + i * OP_MAX_NUM_BLOCKS;
                    u64 v = *reinterpret_cast<u64*>(mock.inspect(t));

                    for (usize j = 0; j < OP_MAX_NUM_BLOCKS; j++) {
                        auto* b = mock.inspect(t + j);
                        for (usize k = 0; k < BLOCK_SIZE; k += sizeof(u64)) {
                            u64 u = *reinterpret_cast<u64*>(b + k);
                            assert_eq(u, v);
                        }
                    }
                }

                exit(0);
            } else
                wait_process(child);
        }

        printf("\r(trace) running: %zu/%zu (%zu replayed)", round + 1, num_rounds, replay_count);
        fflush(stdout);
    }

    puts("");
}

void test_banker() {
    using namespace std::chrono_literals;

    constexpr i64 initial = 1000;
    constexpr i64 bill = 200;
    constexpr usize num_accounts = 10;
    constexpr usize num_workers = 8;
    constexpr usize num_rounds = 30;

    constexpr usize log_size = 3 * num_workers + OP_MAX_NUM_BLOCKS;

    printf("(trace) running: 0/%zu", num_rounds);
    fflush(stdout);

    usize replay_count = 0;
    for (usize round = 0; round < num_rounds; round++) {
        int child;
        if ((child = fork()) == IN_CHILD) {
            initialize(log_size, num_accounts);

            auto begin_ts = std::chrono::steady_clock::now();

            std::vector<usize> bno;
            bno.reserve(num_accounts);
            for (usize i = 0; i < num_accounts; i++) {
                OpContext ctx;
                bcache.begin_op(&ctx);
                bno.push_back(bcache.alloc(&ctx));
                auto* b = bcache.acquire(bno.back());
                i64* p = reinterpret_cast<i64*>(b->data);
                *p = initial;
                bcache.sync(&ctx, b);
                bcache.release(b);
                bcache.end_op(&ctx);
            }

            std::random_device rd;
            std::atomic<usize> count = 0;
            std::atomic<bool> started = false;
            for (usize i = 0; i < num_workers; i++) {
                std::thread([&] {
                    std::mt19937 gen(rd());

                    started = true;
                    try {
                        while (true) {
                            usize j = gen() % num_accounts, k = gen() % num_accounts;
                            if (j == k)
                                k = (k + 1) % num_accounts;

                            OpContext ctx;
                            bcache.begin_op(&ctx);

                            Block *bj, *bk;
                            if (j < k) {
                                bj = bcache.acquire(bno[j]);
                                bk = bcache.acquire(bno[k]);
                            } else {
                                bk = bcache.acquire(bno[k]);
                                bj = bcache.acquire(bno[j]);
                            }

                            i64* vj = reinterpret_cast<i64*>(bj->data);
                            i64* vk = reinterpret_cast<i64*>(bk->data);
                            i64 transfer = std::min(*vj, (i64)(gen() % bill));

                            *vj -= transfer;
                            bcache.sync(&ctx, bj);
                            bcache.release(bj);

                            *vk += transfer;
                            bcache.sync(&ctx, bk);
                            bcache.release(bk);

                            bcache.end_op(&ctx);
                            count++;
                        }
                    } catch (const Offline&) {
                    }
                }).detach();
            }

            while (!started) {
            }
            std::this_thread::sleep_for(2s);
            mock.offline = true;

            auto end_ts = std::chrono::steady_clock::now();
            auto duration
                = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - begin_ts).count();
            printf("\r\033[K(trace) throughput = %.2f txn/s\n",
                   static_cast<double>(count) * 1000 / duration);
            fflush(stdout);

            mock.dump("sd.img");
            _exit(0);
        } else {
            wait_process(child);
            initialize_mock(log_size, num_accounts, "sd.img");
            auto* header = mock.inspect_log_header();
            if (header->num_blocks > 0)
                replay_count++;

            if ((child = fork()) == IN_CHILD) {
                init_bcache(&sblock, &device);

                i64 sum = 0;
                usize t = sblock.num_blocks - num_accounts;
                for (usize i = 0; i < num_accounts; i++) {
                    i64 value = *reinterpret_cast<i64*>(mock.inspect(t + i));
                    assert_true(value >= 0);
                    sum += value;
                }

                assert_eq(sum, num_accounts * initial);
                exit(0);
            } else
                wait_process(child);
        }

        printf("\r(trace) running: %zu/%zu (%zu replayed)", round + 1, num_rounds, replay_count);
        fflush(stdout);
    }

    puts("");
}

}  // namespace crash

int main() {
    std::vector<Testcase> tests = {
        {"init", basic::test_init},
        {"read_write", basic::test_read_write},
        {"loop_read", basic::test_loop_read},
        {"reuse", basic::test_reuse},
        {"lru", basic::test_lru},
        {"atomic_op", basic::test_atomic_op},
        {"overflow", basic::test_overflow},
        {"resident", basic::test_resident},
        {"local_absorption", basic::test_local_absorption},
        {"global_absorption", basic::test_global_absorption},
        {"replay", basic::test_replay},
        {"alloc", basic::test_alloc},
        {"alloc_free", basic::test_alloc_free},

        {"concurrent_acquire", concurrent::test_acquire},
        {"concurrent_sync", concurrent::test_sync},
        {"concurrent_alloc", concurrent::test_alloc},

        {"simple_crash", crash::test_simple_crash},
        {"single", [] { crash::test_parallel(1000, 1, 5, 0); }},
        {"parallel_1", [] { crash::test_parallel(1000, 2, 5, 0); }},
        {"parallel_2", [] { crash::test_parallel(1000, 4, 5, 0); }},
        {"parallel_3", [] { crash::test_parallel(500, 4, 10, 1); }},
        {"parallel_4", [] { crash::test_parallel(500, 4, 10, 2 * OP_MAX_NUM_BLOCKS); }},
        {"banker", crash::test_banker},
    };
    Runner(tests).run();

    printf("(info) OK: %zu tests passed.\n", tests.size());

    return 0;
}
