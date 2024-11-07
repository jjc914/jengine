#ifndef SPARSE_VECTOR_HPP
#define SPARSE_VECTOR_HPP

#include <vector>
#include <set>
#include <iostream>

template <typename T>
class SparseVector {
public:
    template <typename... Args>
    size_t emplace(Args&&... args) {
        if (_free.empty()) {
            _internal.emplace_back(args...);
            return _internal.size() - 1;
        }
        auto index_it = _free.begin();
        size_t index = *index_it;
        _free.erase(index_it);
        _internal[index] = T(std::forward<Args>(args)...);
        return index;
    }

    T erase(const size_t index) {
        _free.insert(index);
        return _internal[index];
    }

    size_t size(void) {
        return _internal.size() - _free.size();
    }

    size_t next(void) {
        if (_free.empty()) {
            return _internal.size();
        }
        return *_free.begin();
    }

    T operator[](size_t i) const {
        if (i >= _internal.size() && _free.find(i) != _free.end()) {
            throw std::out_of_range("index is not allocated or out of range");
        }
        return _internal[i];
    }

    T& operator[](size_t i) {
        if (i >= _internal.size() && _free.find(i) != _free.end()) {
            throw std::out_of_range("index is not allocated or out of range");
        }
        return _internal[i];
    }

private:
    std::vector<T> _internal;
    std::set<size_t> _free;
};

#endif