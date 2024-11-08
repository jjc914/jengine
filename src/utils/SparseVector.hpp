#ifndef SPARSE_VECTOR_HPP
#define SPARSE_VECTOR_HPP

#include <vector>
#include <set>
#include <iostream>
#include <typeindex>

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
        _free.emplace(index);
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

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator(SparseVector<T>* vec, size_t index) 
            : _vector(vec), _index(index) {
            advance_to_next_valid();
        }

        reference operator*() { return (*_vector)[_index]; }
        pointer operator->() { return &(*_vector)[_index]; }

        Iterator& operator++() {
            ++_index;
            advance_to_next_valid();
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            return _index == other._index && _vector == other._vector;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        void advance_to_next_valid() {
            while (_index < _vector->_internal.size() && _vector->_free.find(_index) != _vector->_free.end()) {
                ++_index;
            }
        }

        SparseVector<T>* _vector;
        size_t _index;
    };

    Iterator begin() {
        return Iterator(this, 0);
    }

    Iterator end() {
        return Iterator(this, _internal.size());
    }

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const SparseVector<U>& vec);
private:
    std::vector<T> _internal;
    std::set<size_t> _free;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const SparseVector<T>& vec) {
    os << "{ ";
    for (size_t i = 0; i < vec._internal.size(); ++i) {
        if (vec._free.find(i) == vec._free.end()) { // Only print non-free elements
            os << vec._internal[i] << " ";
        } else {
            os << "null ";
        }
    }
    os << "}";
    return os;
}

#endif