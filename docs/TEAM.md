# Team split

Each member owns one module end to end: C++ code, tests, route file and the matching React page.

| Module | Owner | Backend code | Algorithms | Frontend page |
|---|---|---|---|---|
| 1. Graphs and navigation | Member 1 | `src/graph/`, `src/api/map_routes.cpp` | Adjacency list, BFS, DFS, Dijkstra, connected components | Disaster Map |
| 2. Trees, hashing, linked lists, victims | Member 2 | `src/shelter/`, `src/api/shelter_routes.cpp` | Singly/doubly/circular linked list, BST, AVL, hash map (chaining and open addressing), Rabin-Karp or KMP string search | Shelters & Victims |
| 3. Heaps, queues, DP, rescue allocation | Member 3 | `src/rescue/`, `src/resource/`, `src/api/rescue_routes.cpp` | Min/max heap, heap sort, priority queue, queue/deque, merge sort, 0/1 knapsack, greedy baseline | Rescue Requests, Resource Optimizer |
| 4. Frontend, integration, Algorithm Lab | Member 4 | `src/lab/`, `src/api/lab_routes.cpp`, `src/main.cpp` | Arrays/strings, stack, circular queue, all sorts, linear/binary search, LCS, LIS, coin change, Fibonacci | Dashboard, Algorithm Laboratory, shared UI |

## Working agreements

- Work on a branch per module (`module-1-graphs`, ...); merge to `main` through pull requests.
- Do not edit another module's folder. Shared files (`CMakeLists.txt`, `api.hpp`, `App.jsx`) are changed in small, separate commits.
- A module is done when its test executable passes, its endpoints return correct JSON for good and bad input, and its page works with no console errors.
