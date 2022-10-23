#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "../exception.hpp"

template <typename Key, typename Value>
class Map {
public:
    template <typename... Args>
    void add(const Key &key, Args &&...args) {
        std::unique_lock lock(mutex);
        if (!map.try_emplace(key, std::forward<Args>(args)...).second)
            throw Internal("key already exists");
    }

    template <typename... Args>
    void try_add(const Key &key, Args &&...args) {
        std::unique_lock lock(mutex);
        map.try_emplace(key, std::forward<Args>(args)...);
    }

    bool contain(const Key &key) {
        std::shared_lock lock(mutex);
        return map.find(key) != map.end();
    }

    auto operator[](const Key &key) -> Value & {
        std::shared_lock lock(mutex);
        auto it = map.find(key);
        if (it == map.end()) {
            printf("\n%d\n", key);
            printf("\n%p\n", __builtin_return_address(0));
            printf("\n%p\n", __builtin_return_address(1));
            printf("\n%p\n", __builtin_return_address(2));
            printf("\n%p\n", __builtin_return_address(3));
            printf("\n%p\n", __builtin_return_address(4));
            printf("\n%p\n", __builtin_return_address(5));
            throw Internal("key not found");
        }
        return it->second;
    }

    auto safe_get(const Key &key) -> Value & {
        std::shared_lock lock(mutex);
        auto it = map.find(key);
        if (it == map.end()) {
            lock.unlock();
            try_add(key);
            lock.lock();
        }
        return map[key];
    }

private:
    std::shared_mutex mutex;
    std::unordered_map<Key, Value> map;
};
