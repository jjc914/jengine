#ifndef ULGRAPH_HPP
#define ULGRAPH_HPP

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <unordered_map>

typedef uint32_t vertexid_t;

template <typename T>
struct vertex {
    vertexid_t id;
    T data;
};

template <typename T>
struct edge {
    T label;
    vertexid_t oid;
    vertexid_t tid;
};

template <typename eT, typename vT>
class ulgraph {
public:
    ulgraph() { }
    
    vertexid_t add_vertex(vT data) {
        _vertices.emplace_back(data);
        std::vector<edge<eT>> vec;
        _adjacency_lists.emplace_back(vec.cbegin(), vec.cend());
        return _vertices.size()-1;
    }
    
    void add_edge(vertexid_t ida, vertexid_t idb, eT label) {
        _adjacency_lists[ida].push_back({label, ida, idb});
        _adjacency_lists[idb].push_back({label, idb, ida});
    }
    
    void remove_vertex(vertexid_t id) {
        // TODO:
    }
    
    void remove_edge(vertexid_t a, vertexid_t b) {
        // TODO:
    }
    
    vT& get_vertex(vertexid_t id) {
        return _vertices[id];
    }
    
    const std::vector<edge<eT>>& get_neighbors(vertexid_t id) const {
        return _adjacency_lists[id];
    }
    
    const std::vector<vT>& get_vertices() const {
        return _vertices;
    }
    
    const std::vector<edge<eT>>& get_edges(vertexid_t id) const {
        return _adjacency_lists[id];
    }
    
    const int vertexs_count() const {
        return _vertices.size();
    }
private:
    std::vector<vT> _vertices;
    std::vector<std::vector<edge<eT>>> _adjacency_lists;
};

#endif
