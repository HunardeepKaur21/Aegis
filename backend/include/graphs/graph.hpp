#pragma once
// Undirected weighted graph stored as an adjacency list. Blocked roads stay in the
// structure but every traversal skips them.
#include <string>
#include <unordered_map>
#include <vector>

namespace aegis {

struct Edge {
    int to;          // index of the neighbour node (not its id)
    double weight;   // distance in km
    int roadId;
    bool blocked;
};

struct PathResult {
    bool found = false;
    std::string algorithm;
    std::vector<int> path;     // node ids from source to destination
    double distance = 0;       // sum of edge weights along path
    std::vector<int> visited;  // node ids in the order the algorithm visited them
};

class Graph {
public:
    void addNode(int id);                                    // ignored if it already exists
    bool hasNode(int id) const;
    void addEdge(int a, int b, double weight, int roadId, bool blocked = false);
    bool setBlocked(int roadId, bool blocked);               // false if no such road
    size_t nodeCount() const { return ids_.size(); }
    size_t edgeCount() const { return edgeCount_; }

    PathResult bfs(int src, int dst) const;       // fewest roads, ignores distance
    PathResult dfs(int src, int dst) const;       // any route, not necessarily short
    PathResult dijkstra(int src, int dst) const;  // shortest distance
    std::vector<std::vector<int>> connectedComponents() const;  // node ids, open roads only

private:
    int indexOf(int id) const;  // throws std::invalid_argument for unknown ids
    PathResult buildResult(const char* name, int src, int dst, bool found,
                           const std::vector<int>& parent, const std::vector<double>& parentWeight,
                           const std::vector<int>& visitedIdx) const;

    std::vector<int> ids_;
    std::unordered_map<int, int> index_;
    std::vector<std::vector<Edge>> adj_;
    size_t edgeCount_ = 0;
};

}  // namespace aegis
