// Module 1 tests: custom min-heap and graph algorithms. No external framework.
#include "graphs/graph.hpp"
#include "heaps/min_heap.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)
using aegis::Graph;

static void testHeap() {
    aegis::MinHeap<int> h;
    std::srand(7);
    for (int i = 0; i < 500; ++i) h.push(std::rand() % 1000);
    CHECK(h.size() == 500);
    int prev = -1;
    bool sorted = true;
    while (!h.empty()) { int v = h.pop(); if (v < prev) sorted = false; prev = v; }
    CHECK(sorted);
    bool threw = false;
    try { h.pop(); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
}

// 1-2 (1), 2-3 (1), 1-3 (5), 3-4 (1), node 5 isolated
static Graph sample() {
    Graph g;
    for (int i = 1; i <= 5; ++i) g.addNode(i);
    g.addEdge(1, 2, 1, 100);
    g.addEdge(2, 3, 1, 101);
    g.addEdge(1, 3, 5, 102);
    g.addEdge(3, 4, 1, 103);
    return g;
}

static void testShortestPaths() {
    Graph g = sample();
    auto d = g.dijkstra(1, 4);
    CHECK(d.found);
    CHECK((d.path == std::vector<int>{1, 2, 3, 4}));
    CHECK(std::fabs(d.distance - 3.0) < 1e-9);
    CHECK(d.visited.front() == 1 && d.visited.back() == 4);

    auto b = g.bfs(1, 3);   // fewest roads uses the long direct road
    CHECK((b.path == std::vector<int>{1, 3}));
    CHECK(std::fabs(b.distance - 5.0) < 1e-9);

    auto f = g.dfs(1, 4);
    CHECK(f.found && f.path.front() == 1 && f.path.back() == 4);
}

static void testBlockedAndUnreachable() {
    Graph g = sample();
    CHECK(g.setBlocked(101, true));            // close 2-3
    auto d = g.dijkstra(1, 3);
    CHECK((d.path == std::vector<int>{1, 3}) && std::fabs(d.distance - 5.0) < 1e-9);
    CHECK(g.setBlocked(102, true));            // close 1-3 as well: 3 and 4 are cut off
    CHECK(!g.dijkstra(1, 4).found);
    CHECK(!g.bfs(1, 4).found);
    CHECK(!g.dfs(1, 4).found);
    CHECK(g.dijkstra(1, 4).path.empty());
    CHECK(g.setBlocked(101, false));           // reopening 2-3 restores 1-2-3-4
    CHECK(g.dijkstra(1, 4).found);
    CHECK((g.dijkstra(1, 4).path == std::vector<int>{1, 2, 3, 4}));
    CHECK(!g.setBlocked(999, true));
    CHECK(!g.dijkstra(1, 5).found);            // isolated node
}

static void testErrors() {
    Graph g = sample();
    bool threw = false;
    try { g.dijkstra(1, 42); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { g.addEdge(1, 2, -3, 1); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    auto same = g.dijkstra(2, 2);
    CHECK(same.found && same.path.size() == 1 && same.distance == 0);
}

static void testComponents() {
    Graph g = sample();
    auto c = g.connectedComponents();
    CHECK(c.size() == 2);
    g.setBlocked(103, true);
    CHECK(g.connectedComponents().size() == 3);
}

// Compare Dijkstra with a simple Bellman-Ford written in the test, on random graphs.
static void testDijkstraMatchesBellmanFord() {
    std::srand(42);
    for (int round = 0; round < 50; ++round) {
        const int n = 9;
        Graph g;
        for (int i = 0; i < n; ++i) g.addNode(i);
        struct E { int a, b; double w; };
        std::vector<E> es;
        for (int i = 0; i < 14; ++i) {
            int a = std::rand() % n, b = std::rand() % n;
            if (a == b) continue;
            double w = 1 + std::rand() % 20;
            g.addEdge(a, b, w, i);
            es.push_back({a, b, w});
        }
        const double INF = std::numeric_limits<double>::infinity();
        std::vector<double> dist(n, INF);
        dist[0] = 0;
        for (int k = 0; k < n; ++k)
            for (auto& e : es) {
                if (dist[e.a] + e.w < dist[e.b]) dist[e.b] = dist[e.a] + e.w;
                if (dist[e.b] + e.w < dist[e.a]) dist[e.a] = dist[e.b] + e.w;
            }
        for (int t = 1; t < n; ++t) {
            auto r = g.dijkstra(0, t);
            CHECK(r.found == (dist[t] < INF));
            if (r.found) CHECK(std::fabs(r.distance - dist[t]) < 1e-9);
        }
    }
}

int main() {
    testHeap();
    testShortestPaths();
    testBlockedAndUnreachable();
    testErrors();
    testComponents();
    testDijkstraMatchesBellmanFord();
    if (failures == 0) std::cout << "test_graph: all tests passed\n";
    return failures == 0 ? 0 : 1;
}
