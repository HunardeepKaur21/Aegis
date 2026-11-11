# AEGIS: Advanced Emergency & Geospatial Intelligence System

A disaster-response simulator built for a university DSA project. It finds rescue routes, ranks emergency requests, picks which supplies to load, and manages shelters and victim records. Every algorithm runs in C++; the React app only displays results.

**Stack:** React + Recharts + Leaflet (frontend), C++17 + Crow (REST backend), SQLite (storage).

## Run it

You need CMake 3.16+, a C++17 compiler, SQLite dev headers, standalone Asio headers, and Node 18+.

```bash
# Ubuntu / WSL
sudo apt install build-essential cmake libsqlite3-dev libasio-dev

# 1. Backend (port 8080). First run creates and seeds database/aegis.db
cd backend && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j4
./bin/aegis_server ../../database/aegis.db ../../database 8080

# 2. Frontend (port 5173), in a second terminal
cd frontend && npm install && npm run dev
```

Open http://localhost:5173. To reset the data, stop the backend and delete `database/aegis.db`. (Delete it after pulling schema changes too, so the new schema is applied.)

## Tests

```bash
cd backend/build && ctest --output-on-failure
```

Each module has one test executable in `backend/tests/`. You can also test the API with Postman using the endpoints in `docs/ARCHITECTURE.md`.

## Repository layout

| Folder | Contents |
|---|---|
| `backend/` | C++ engine (`src/<module>/`), REST routes (`src/api/`), tests (`tests/`) |
| `frontend/` | React app: one page per module in `src/pages/` |
| `database/` | `schema.sql`, `seed.sql` (invented sample flood scenario) |
| `docs/` | `ARCHITECTURE.md`, `TEAM.md` |

## Build status

| Step | Status |
|---|---|
| 1. Architecture, schema, UI shell, dashboard API | Done |
| 2. Module 1: Disaster Map and Route Finder (min-heap, graph, BFS, DFS, Dijkstra, API, Leaflet page) | Done |
| 3. Module 2: Smart Rescue Allocation (heaps, queues, hash map, sorting) | Next |
| 4. Module 3: Resource Optimizer (knapsack DP, greedy comparison) | Planned |
| 5. Module 4: Shelters and Victims (linked lists, BST, AVL, hashing, string search) | Planned |
| 6. Module 5: Algorithm Laboratory and final integration | Planned |
