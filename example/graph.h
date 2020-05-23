#pragma once

#include <algorithm>
#include <cassert>
#include <iterator>
#include <stack>
#include <stddef.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace example
{
template<typename NodeType>
class Graph
{
public:
    Graph()
        : current_id_(0), nodes_(), adjacencies_(), node_ids_(), edges_(),
          edge_ids_()
    {
    }

    // Element access

    const NodeType& node(int node_id) const;
    NodeType& node(int node_id);
    size_t num_adjacencies(int node_id) const;

    // Modifiers

    int insert_node(NodeType node);
    void erase_node(int node_id);

    int insert_edge(int from_node, int to_node);
    void erase_edge(int edge_id);

private:
    struct Edge
    {
        int from, to;

        Edge(const int f, const int t) : from(f), to(t) {}

        int opposite(int n) const
        {
            assert(n == from || n == to);
            return n == from ? to : from;
        }
    };

    template<typename T, typename Visitor>
    void dfs_traverse(const Graph<T>& graph, int start_node, Visitor visitor);

    int current_id_;
    std::vector<NodeType> nodes_;
    std::vector<std::vector<int>> adjacencies_;
    std::vector<int> node_ids_;
    std::vector<Edge> edges_;
    std::vector<int> edge_ids_;
};

template<typename NodeType>
const NodeType& Graph<NodeType>::node(const int node_id) const
{
    auto lower_bound =
        std::lower_bound(node_ids_.begin(), node_ids_.end(), node_id);

    assert(lower_bound != node_ids_.end());
    assert(node_id == *lower_bound);

    const auto distance = std::distance(node_ids_.begin(), lower_bound);
    assert(distance >= 0);
    return *std::next(nodes_.begin(), distance);
}

template<typename NodeType>
NodeType& Graph<NodeType>::node(const int node_id)
{
    return const_cast<NodeType&>(
        static_cast<const Graph<NodeType>*>(this)->node(node_id));
}

template<typename NodeType>
size_t Graph<NodeType>::num_adjacencies(const int node_id) const
{
    auto lower_bound =
        std::lower_bound(node_ids_.begin(), node_ids_.end(), node_id);
    assert(lower_bound != node_ids_.end());
    assert(node_id == *lower_bound);
    auto distance = std::distance(node_ids_.begin(), lower_bound);
    return std::next(adjacencies_.begin(), distance)->size();
}

template<typename NodeType>
int Graph<NodeType>::insert_node(NodeType node)
{
    const int id = ++current_id_;

    // Insert the node id by first doing a sorted insert of the id into the node
    // id vector.

    auto lower_bound = std::lower_bound(node_ids_.begin(), node_ids_.end(), id);
    assert(lower_bound == node_ids_.end() || id < *lower_bound);

    // The relative position of the id in the node id vector is used to insert
    // the node into the node vector.

    const auto distance = std::distance(node_ids_.begin(), lower_bound);
    assert(distance >= 0);
    auto insert_node_at = std::next(nodes_.begin(), distance);
    auto insert_adjacencies_at = std::next(adjacencies_.begin(), distance);
    node_ids_.insert(lower_bound, id);
    nodes_.insert(insert_node_at, node);
    adjacencies_.insert(insert_adjacencies_at, std::vector<int>());

    return id;
}

template<typename NodeType>
void Graph<NodeType>::erase_node(const int node_id)
{
    auto lower_bound =
        std::lower_bound(node_ids_.begin(), node_ids_.end(), node_id);

    assert(lower_bound != node_ids_.end());
    assert(node_id == *lower_bound);

    const auto distance = std::distance(node_ids_.begin(), lower_bound);
    assert(distance >= 0);
    auto erase_node_at = std::next(nodes_.begin(), distance);
    auto erase_adjacencies_at = std::next(adjacencies_.begin(), distance);
    node_ids_.erase(lower_bound);
    nodes_.erase(erase_node_at);
    adjacencies_.erase(erase_adjacencies_at);
}

template<typename NodeType>
int Graph<NodeType>::insert_edge(const int from, const int to)
{
    const int id = ++current_id_;

    {
        // Insert the edge again via sorted insert into id vector

        auto lower_bound =
            std::lower_bound(edge_ids_.begin(), edge_ids_.end(), id);
        assert(lower_bound == edge_ids_.end() || id < *lower_bound);

        // Use relative position to insert into the edge vector

        auto insert_at = std::next(
            edges_.begin(), std::distance(edge_ids_.begin(), lower_bound));
        edge_ids_.insert(lower_bound, id);
        edges_.insert(insert_at, Edge(from, to));
    }

    // Update node adjacency list

    {
        auto lower_bound =
            std::lower_bound(node_ids_.begin(), node_ids_.end(), from);
        assert(lower_bound != node_ids_.end());
        assert(from == *lower_bound);

        const auto distance = std::distance(node_ids_.begin(), lower_bound);
        auto adjacencies = std::next(adjacencies_.begin(), distance);

        // The adjacency should not yet exist.
        assert(
            std::find(adjacencies->cbegin(), adjacencies->cend(), to) ==
            adjacencies->cend());
        adjacencies->push_back(to);
    }

    return id;
}

template<typename NodeType>
void Graph<NodeType>::erase_edge(const int edge_id)
{
    {
        auto lower_bound =
            std::lower_bound(edge_ids_.begin(), edge_ids_.end(), edge_id);

        assert(lower_bound != edge_ids_.end());
        assert(edge_id == *lower_bound);

        auto erase_at = std::next(
            edges_.begin(), std::distance(edge_ids_.begin(), lower_bound));
        edge_ids_.erase(lower_bound);
        edges_.erase(erase_at);
    }
}

template<typename NodeType, typename Visitor>
void dfs_traverse(
    const Graph<NodeType>& /*graph*/,
    const int /*start_node*/,
    Visitor /*visitor*/)
{
    // TODO
}
} // namespace example
