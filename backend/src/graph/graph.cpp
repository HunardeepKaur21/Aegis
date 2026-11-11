#include "graphs/graph.hpp"
#include "heaps/min_heap.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace aegis {

void Graph::addNode(int id) {
    if (index_.count(id)) return;
    index_[id] = static_cast<int>(ids_.size());
    ids_.push_back(id);
    adj_.emplace_back();
}

bool Graph::hasNode(int id) const { return index_.count(id) > 0; }

int Graph::indexOf(int id) const {
    auto it = index_.find(id);
    if (it == index_.end()) throw std::invalid_argument("Unknown location id " + std::to_string(id));
    return it->second;
}

void Graph::addEdge(int a, int b, double weight, int roadId, bool blocked) {
    if (weight <= 0) throw std::invalid_argument("Road distance must be positive");
    int ia = indexOf(a), ib = indexOf(b);
    adj_[ia].push_back({ib, weight, roadId, blocked});
    adj_[ib].push_back({ia, weight, roadId, blocked});
    ++edgeCount_;
}

bool Graph::setBlocked(int roadId, bool blocked) {
    bool found = false;
    for (auto& list : adj_)
        for (auto& e : list)
            if (e.roadId == roadId) { e.blocked = blocked; found = true; }
    return found;
}

PathResult Graph::buildResult(const char* name, int src, int dst, bool found,
                              const std::vector<int>& parent, const std::vector<double>& parentWeight,
                              const std::vector<int>& visitedIdx) const {
    PathResult r;
    r.algorithm = name;
    r.found = found;
    for (int v : visitedIdx) r.visited.push_back(ids_[v]);
    if (!found) return r;
    int s = index_.at(src), cur = index_.at(dst);
    while (cur != s) {
        r.path.push_back(ids_[cur]);
        r.distance += parentWeight[cur];
        cur = parent[cur];
    }
    r.path.push_back(ids_[s]);
    std::reverse(r.path.begin(), r.path.end());
    return r;
}

// Breadth-first search: explores level by level, so the first time it reaches the
// destination it has used the fewest roads. O(V + E).
PathResult Graph::bfs(int src, int dst) const {
    int s = indexOf(src), t = indexOf(dst);
    const int n = static_cast<int>(ids_.size());
    std::vector<int> parent(n, -1), visited;
    std::vector<double> pw(n, 0);
    std::vector<bool> seen(n, false);
    std::vector<int> queue;           // simple array queue with a head index
    size_t head = 0;
    queue.push_back(s);
    seen[s] = true;
    bool found = false;
    while (head < queue.size()) {
        int u = queue[head++];
        visited.push_back(u);
        if (u == t) { found = true; break; }
        for (const Edge& e : adj_[u]) {
            if (e.blocked || seen[e.to]) continue;
            seen[e.to] = true;
            parent[e.to] = u;
            pw[e.to] = e.weight;
            queue.push_back(e.to);
        }
    }
    return buildResult("bfs", src, dst, found, parent, pw, visited);
}

// Depth-first search with an explicit stack: follows one road as far as it can before
// backtracking. Finds a route if one exists, but not necessarily a short one. O(V + E).
PathResult Graph::dfs(int src, int dst) const {
    int s = indexOf(src), t = indexOf(dst);
    const int n = static_cast<int>(ids_.size());
    struct Frame { int node, from; double w; };
    std::vector<int> parent(n, -1), visited;
    std::vector<double> pw(n, 0);
    std::vector<bool> seen(n, false);
    std::vector<Frame> stack;
    stack.push_back({s, -1, 0});
    bool found = false;
    while (!stack.empty()) {
        Frame f = stack.back();
        stack.pop_back();
        if (seen[f.node]) continue;
        seen[f.node] = true;
        parent[f.node] = f.from;
        pw[f.node] = f.w;
        visited.push_back(f.node);
        if (f.node == t) { found = true; break; }
        // push in reverse so neighbours are explored in insertion order
        for (auto it = adj_[f.node].rbegin(); it != adj_[f.node].rend(); ++it)
            if (!it->blocked && !seen[it->to]) stack.push_back({it->to, f.node, it->weight});
    }
    return buildResult("dfs", src, dst, found, parent, pw, visited);
}

// Dijkstra: always settles the closest unsettled node next, using the custom min-heap.
// Correct because all road distances are positive. O((V + E) log V).
PathResult Graph::dijkstra(int src, int dst) const {
    int s = indexOf(src), t = indexOf(dst);
    const int n = static_cast<int>(ids_.size());
    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dist(n, INF), pw(n, 0);
    std::vector<int> parent(n, -1), visited;
    std::vector<bool> done(n, false);
    using Item = std::pair<double, int>;  // (distance, node)
    MinHeap<Item> heap;
    dist[s] = 0;
    heap.push({0, s});
    bool found = false;
    while (!heap.empty()) {
        Item cur = heap.pop();
        int u = cur.second;
        if (done[u]) continue;  // stale heap entry
        done[u] = true;
        visited.push_back(u);
        if (u == t) { found = true; break; }
        for (const Edge& e : adj_[u]) {
            if (e.blocked || done[e.to]) continue;
            double nd = dist[u] + e.weight;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                parent[e.to] = u;
                pw[e.to] = e.weight;
                heap.push({nd, e.to});
            }
        }
    }
    return buildResult("dijkstra", src, dst, found, parent, pw, visited);
}

std::vector<std::vector<int>> Graph::connectedComponents() const {
    const int n = static_cast<int>(ids_.size());
    std::vector<bool> seen(n, false);
    std::vector<std::vector<int>> comps;
    for (int start = 0; start < n; ++start) {
        if (seen[start]) continue;
        std::vector<int> comp, stack{start};
        seen[start] = true;
        while (!stack.empty()) {
            int u = stack.back();
            stack.pop_back();
            comp.push_back(ids_[u]);
            for (const Edge& e : adj_[u])
                if (!e.blocked && !seen[e.to]) { seen[e.to] = true; stack.push_back(e.to); }
        }
        std::sort(comp.begin(), comp.end());
        comps.push_back(comp);
    }
    return comps;
}

}  // namespace aegis
