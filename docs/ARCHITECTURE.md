# Architecture

```
React (Vite, :5173)  --/api (JSON)-->  Crow server (C++, :8080)  -->  SQLite file
  pages, Leaflet map,                    routes in src/api/             database/aegis.db
  Recharts                               DSA engine in src/<module>/
```

## Rules that keep it simple

1. **Algorithms live in the engine, never in routes.** Each module is plain C++ classes under `backend/src/<module>/` with headers in `backend/include/aegis/`. They do not include Crow or SQLite headers, so they can be unit-tested alone.
2. **Routes are thin.** A route reads SQLite, builds the engine's data structure, calls the algorithm, and returns JSON. Routes for one module go in one file in `src/api/`.
3. **SQLite stores records; the engine computes.** For example, roads are saved in the `roads` table, loaded into an adjacency list for each routing request, and Dijkstra runs on that list.
4. **Errors are JSON.** Validation failures return `{"error": "..."}` with status 400, missing records 404. The frontend shows the message.
5. **No fake results.** The frontend never contains algorithm output. If an endpoint is missing, the page says so.

## Database

Tables: `scenarios`, `locations`, `roads`, `rescue_teams`, `rescue_requests`, `resources`, `settings`, `shelters`, `victims`, `events`, `algorithm_runs`. See `database/schema.sql`; every table has CHECK constraints so bad data is rejected even if validation is skipped. Roads are two-way and weighted by distance in km; a blocked road keeps its row with `is_blocked = 1`.

## REST contract

| Module | Endpoints | Status |
|---|---|---|
| Core | `GET /api/health`, `GET /api/dashboard` | Done |
| 1 Map | `GET /api/map/locations`, `GET /api/map/roads`, `POST /api/map/route`, `POST /api/map/roads`, `PATCH /api/map/roads/condition` | Done |
| 2 Rescue | `GET/POST /api/rescue/requests`, `POST /api/rescue/assign`, `PATCH /api/rescue/status` | Planned |
| 3 Resources | `GET/POST /api/resources`, `POST /api/resources/optimize` | Planned |
| 4 Shelters and victims | `GET/POST /api/shelters`, `GET/POST /api/victims`, `GET /api/victims/search` | Planned |
| 5 Lab | `POST /api/algorithms/sort`, `/search`, `/execute` | Planned |

### Module 1 request and response examples

`POST /api/map/route` with `{"from": 1, "to": 3, "algorithm": "dijkstra"}` (algorithm is `dijkstra`, `bfs` or `dfs`) returns `found`, `algorithm`, `distance_km`, `hops`, `path` (ordered locations with coordinates), `visited` (in visit order), `visited_count`, `runtime_us` and `message`. An unreachable destination returns `found: false` with an explanatory `message`, not an error.

`PATCH /api/map/roads/condition` with `{"road_id": 4, "blocked": true}` closes or reopens a road and records a `simulation_events` row. Errors use 400 (bad input), 404 (unknown id) and 409 (duplicate road), always as `{"error": "..."}`.

## Testing approach

Each module gets `backend/tests/test_<module>.cpp`, a small executable linked against `aegis_core` and registered in `CMakeLists.txt` with `add_test`. Run all with `cd backend/build && ctest`.
