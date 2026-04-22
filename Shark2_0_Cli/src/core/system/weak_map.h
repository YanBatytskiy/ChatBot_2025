// weak_map.h

#pragma once
#include <memory>
#include <unordered_map>

template <typename T>
struct WeakPtrHash {
    size_t operator()(const std::weak_ptr<T>& w) const {
        auto s = w.lock();
        return std::hash<T*>()(s.get());
    }
};

template <typename T>
struct WeakPtrEqual {
    bool operator()(const std::weak_ptr<T>& a, const std::weak_ptr<T>& b) const {
        return !a.owner_before(b) && !b.owner_before(a);
    }
};

template <typename T, typename V>
using weak_map = std::unordered_map<std::weak_ptr<T>, V, WeakPtrHash<T>, WeakPtrEqual<T>>;
