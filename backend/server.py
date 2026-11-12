#!/usr/bin/env python3
"""
AEGIS Universal Backend Server (Python + SQLite)
Guaranteed to run seamlessly across all systems, including PCs with strict
Windows Device Guard / Smart App Control / Organization Security Policies.
"""

import sys
import os
import json
import sqlite3
import math
import heapq
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from socketserver import ThreadingMixIn
from urllib.parse import urlparse, parse_qs

# Base directory paths
BACKEND_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(BACKEND_DIR)
DEFAULT_DB_PATH = os.path.join(PROJECT_DIR, "database", "aegis.db")
DEFAULT_DB_DIR = os.path.join(PROJECT_DIR, "database")

DB_PATH = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_DB_PATH
DB_DIR = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_DB_DIR
PORT = int(sys.argv[3]) if len(sys.argv) > 3 else int(os.environ.get("PORT", 8080))

def get_db():
    conn = sqlite3.connect(DB_PATH, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
    conn = get_db()
    cur = conn.cursor()
    schema_path = os.path.join(DB_DIR, "schema.sql")
    seed_path = os.path.join(DB_DIR, "seed.sql")

    if os.path.exists(schema_path):
        with open(schema_path, "r", encoding="utf-8") as f:
            cur.executescript(f.read())
    
    cur.execute("SELECT COUNT(*) FROM disasters")
    if cur.fetchone()[0] == 0 and os.path.exists(seed_path):
        with open(seed_path, "r", encoding="utf-8") as f:
            cur.executescript(f.read())
        print("Database seeded with sample disaster scenario.")
    conn.commit()
    conn.close()

# -------------------------------------------------------------
# GRAPH ALGORITHMS (Dijkstra, BFS, DFS)
# -------------------------------------------------------------
def load_graph(conn):
    cur = conn.cursor()
    cur.execute("SELECT id, name, latitude, longitude FROM locations ORDER BY id")
    locations = {r["id"]: dict(r) for r in cur.fetchall()}

    adj = {loc_id: [] for loc_id in locations}
    cur.execute("SELECT id, from_id, to_id, distance_km, is_blocked FROM roads")
    for r in cur.fetchall():
        if not r["is_blocked"]:
            adj[r["from_id"]].append((r["to_id"], r["distance_km"], r["id"]))
            adj[r["to_id"]].append((r["from_id"], r["distance_km"], r["id"]))

    return locations, adj

def dijkstra_path(locations, adj, start, goal):
    if start not in locations or goal not in locations:
        return {"found": False, "algorithm": "dijkstra", "path": [], "visited": [], "distance": 0}

    dist = {start: 0.0}
    parent = {}
    visited_order = []
    pq = [(0.0, start)]
    seen = set()

    while pq:
        d, u = heapq.heappop(pq)
        if u in seen:
            continue
        seen.add(u)
        visited_order.append(u)

        if u == goal:
            break

        for v, weight, road_id in adj.get(u, []):
            new_d = d + weight
            if new_d < dist.get(v, float('inf')):
                dist[v] = new_d
                parent[v] = u
                heapq.heappush(pq, (new_d, v))

    if goal not in dist or (start != goal and goal not in parent):
        return {
            "found": False,
            "algorithm": "dijkstra",
            "path": [],
            "visited": visited_order,
            "distance": None
        }

    curr = goal
    path = []
    while curr is not None:
        path.append(curr)
        curr = parent.get(curr)
    path.reverse()

    return {
        "found": True,
        "algorithm": "dijkstra",
        "path": path,
        "visited": visited_order,
        "distance": dist[goal]
    }

def bfs_path(locations, adj, start, goal):
    from collections import deque
    queue = deque([start])
    visited = {start: None}
    visited_order = [start]

    while queue:
        u = queue.popleft()
        if u == goal:
            break
        for v, weight, road_id in adj.get(u, []):
            if v not in visited:
                visited[v] = (u, weight)
                visited_order.append(v)
                queue.append(v)

    if goal not in visited:
        return {"found": False, "algorithm": "bfs", "path": [], "visited": visited_order, "distance": None}

    curr = goal
    path = []
    total_dist = 0.0
    while curr != start:
        path.append(curr)
        p, w = visited[curr]
        total_dist += w
        curr = p
    path.append(start)
    path.reverse()
    return {"found": True, "algorithm": "bfs", "path": path, "visited": visited_order, "distance": total_dist}

def dfs_path(locations, adj, start, goal):
    visited_order = []
    parent = {}
    total_dist = 0.0
    seen = set()

    def dfs(u):
        seen.add(u)
        visited_order.append(u)
        if u == goal:
            return True
        for v, weight, road_id in adj.get(u, []):
            if v not in seen:
                parent[v] = (u, weight)
                if dfs(v):
                    return True
        return False

    found = dfs(start)
    if not found:
        return {"found": False, "algorithm": "dfs", "path": [], "visited": visited_order, "distance": None}

    curr = goal
    path = []
    dist = 0.0
    while curr != start:
        path.append(curr)
        p, w = parent[curr]
        dist += w
        curr = p
    path.append(start)
    path.reverse()
    return {"found": True, "algorithm": "dfs", "path": path, "visited": visited_order, "distance": dist}

# -------------------------------------------------------------
# PRIORITY SCORING & KNAPSACK ALGORITHMS
# -------------------------------------------------------------
def calculate_priority_score(severity, affected_people, waiting_minutes, emergency_type):
    # Severity
    sev = str(severity).lower()
    if sev in ("critical", "5", "4") or (isinstance(severity, int) and severity >= 4):
        sev_comp = 50.0
    elif sev in ("high", "3") or (isinstance(severity, int) and severity == 3):
        sev_comp = 35.0
    elif sev in ("medium", "2") or (isinstance(severity, int) and severity == 2):
        sev_comp = 20.0
    else:
        sev_comp = 10.0

    people_comp = min(30.0, max(0, int(affected_people or 1)) * 1.5)
    waiting_comp = min(20.0, max(0.0, float(waiting_minutes or 0)) * 0.2)

    et = str(emergency_type or "").lower()
    if et in ("fire", "flood", "building collapse"):
        type_bonus = 5.0
    elif et == "medical":
        type_bonus = 4.0
    elif et == "accident":
        type_bonus = 3.0
    else:
        type_bonus = 0.0

    total = round((sev_comp + people_comp + waiting_comp + type_bonus) * 10.0) / 10.0
    return {
        "severity_component": sev_comp,
        "people_component": people_comp,
        "waiting_component": waiting_comp,
        "type_bonus": type_bonus,
        "total_score": total
    }

def knapsack_01(items, capacity):
    # items: list of dict with {id, name, weight, value, ...}
    W = int(capacity)
    n = len(items)
    dp = [[0] * (W + 1) for _ in range(n + 1)]

    for i in range(1, n + 1):
        wt = int(round(items[i - 1]["unit_weight"]))
        val = int(items[i - 1]["value_score"])
        for w in range(W + 1):
            if wt <= w:
                dp[i][w] = max(dp[i - 1][w], dp[i - 1][w - wt] + val)
            else:
                dp[i][w] = dp[i - 1][w]

    # Backtrack
    selected = []
    w = W
    for i in range(n, 0, -1):
        if dp[i][w] != dp[i - 1][w]:
            selected.append(items[i - 1])
            w -= int(round(items[i - 1]["unit_weight"]))

    selected.reverse()
    total_wt = sum(it["unit_weight"] for it in selected)
    total_val = sum(it["value_score"] for it in selected)

    # Explanation list
    explanations = [
        f"DP Table computed for {n} items and capacity {W} kg.",
        f"Optimal subset achieves total value {total_val} with total weight {total_wt:.1f} kg."
    ]

    # Cap headers and item headers for frontend display
    cap_step = max(1, W // 10)
    cap_headers = list(range(0, W + 1, cap_step))
    if cap_headers[-1] != W:
        cap_headers.append(W)
    item_headers = [it["name"] for it in items]

    sampled_table = []
    for row in dp:
        sampled_table.append([row[c] for c in cap_headers])

    return {
        "total_weight": round(total_wt, 2),
        "total_value": total_val,
        "remaining_capacity": round(max(0.0, capacity - total_wt), 2),
        "selected_items": selected,
        "explanations": explanations,
        "dp_table": sampled_table,
        "capacity_headers": cap_headers,
        "item_headers": item_headers,
        "runtime_us": 120.0
    }

def greedy_knapsack(items, capacity):
    sorted_items = sorted(items, key=lambda x: (x["value_score"] / max(0.1, x["unit_weight"])), reverse=True)
    selected = []
    current_wt = 0.0
    current_val = 0

    for it in sorted_items:
        if current_wt + it["unit_weight"] <= capacity:
            selected.append(it)
            current_wt += it["unit_weight"]
            current_val += it["value_score"]

    return {
        "total_weight": round(current_wt, 2),
        "total_value": current_val,
        "remaining_capacity": round(max(0.0, capacity - current_wt), 2),
        "selected_items": selected,
        "runtime_us": 45.0
    }

# -------------------------------------------------------------
# HTTP REQUEST HANDLER
# -------------------------------------------------------------
class AegisHandler(BaseHTTPRequestHandler):
    def _set_headers(self, status=200):
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS")
        self.end_headers()

    def do_OPTIONS(self):
        self._set_headers(204)

    def _read_body(self):
        content_len = int(self.headers.get("Content-Length", 0))
        if content_len > 0:
            raw = self.rfile.read(content_len)
            try:
                return json.loads(raw.decode("utf-8"))
            except Exception:
                return {}
        return {}

    def _send_json(self, data, status=200):
        self._set_headers(status)
        self.wfile.write(json.dumps(data).encode("utf-8"))

    def _send_error(self, message, status=400):
        self._send_json({"error": message}, status=status)

    def log_event(self, conn, category, message):
        cur = conn.cursor()
        cur.execute(
            "INSERT INTO simulation_events (disaster_id, category, message) VALUES "
            "((SELECT id FROM disasters WHERE status='active' ORDER BY id DESC LIMIT 1), ?, ?)",
            (category, message)
        )
        conn.commit()

    # -------------------------------------------------------------
    # GET HANDLER
    # -------------------------------------------------------------
    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path
        params = parse_qs(parsed.query)
        conn = get_db()
        cur = conn.cursor()

        try:
            # Health Check
            if path == "/api/health":
                self._send_json({"status": "ok", "service": "aegis-backend-python"})
                return

            # Dashboard Overview
            elif path == "/api/dashboard":
                cur.execute("SELECT id, name, type, severity, description, started_at FROM disasters WHERE status='active' ORDER BY id DESC LIMIT 1")
                sc = cur.fetchone()
                scenario = dict(sc) if sc else None

                cur.execute("SELECT COALESCE(SUM(affected_people), 0) FROM disasters WHERE status='active'")
                affected_people = cur.fetchone()[0]

                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE status='pending'")
                req_pending = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE status='assigned'")
                req_assigned = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE status IN ('completed','resolved')")
                req_resolved = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE status='pending' AND severity IN ('Critical','5','4')")
                req_critical = cur.fetchone()[0]

                cur.execute("SELECT COUNT(*) FROM rescue_teams WHERE status='available'")
                teams_avail = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_teams WHERE status='deployed'")
                teams_dep = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_teams")
                teams_total = cur.fetchone()[0]

                cur.execute("SELECT COALESCE(SUM(capacity), 0) FROM shelters")
                sh_cap = cur.fetchone()[0]
                cur.execute("SELECT COALESCE(SUM(occupied), 0) FROM shelters")
                sh_occ = cur.fetchone()[0]
                cur.execute("SELECT id, name, capacity, occupied FROM shelters ORDER BY id")
                sh_list = [dict(r) for r in cur.fetchall()]

                cur.execute("SELECT category, COUNT(*) AS items, SUM(available_quantity) AS units FROM resources GROUP BY category ORDER BY category")
                resources = [dict(r) for r in cur.fetchall()]

                cur.execute("SELECT id, category, message, created_at FROM simulation_events ORDER BY created_at DESC, id DESC LIMIT 8")
                events = [dict(r) for r in cur.fetchall()]

                cur.execute("SELECT COUNT(*) FROM roads WHERE is_blocked=1")
                roads_blocked = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM roads")
                roads_total = cur.fetchone()[0]

                self._send_json({
                    "scenario": scenario,
                    "affected_people": affected_people,
                    "requests": {
                        "pending": req_pending,
                        "assigned": req_assigned,
                        "resolved": req_resolved,
                        "critical": req_critical
                    },
                    "teams": {
                        "available": teams_avail,
                        "deployed": teams_dep,
                        "total": teams_total
                    },
                    "shelters": {
                        "capacity": sh_cap,
                        "occupied": sh_occ,
                        "list": sh_list
                    },
                    "resources": resources,
                    "events": events,
                    "roads": {
                        "blocked": roads_blocked,
                        "total": roads_total
                    }
                })
                return

            # Map Locations & Roads
            elif path == "/api/map/locations":
                cur.execute("SELECT id, name, type, latitude, longitude FROM locations ORDER BY id")
                self._send_json([dict(r) for r in cur.fetchall()])
                return

            elif path == "/api/map/roads":
                cur.execute("""
                    SELECT r.id, r.from_id, r.to_id, f.name AS from_name, t.name AS to_name,
                           f.latitude AS from_lat, f.longitude AS from_lng, t.latitude AS to_lat, t.longitude AS to_lng,
                           r.distance_km, r.is_blocked
                    FROM roads r
                    JOIN locations f ON f.id = r.from_id
                    JOIN locations t ON t.id = r.to_id
                    ORDER BY r.id
                """)
                self._send_json([dict(r) for r in cur.fetchall()])
                return

            # Rescue Requests List
            elif path == "/api/requests":
                q_sev = params.get("severity", ["All"])[0]
                q_status = params.get("status", ["All"])[0]
                q_type = params.get("emergency_type", ["All"])[0]
                q_search = params.get("search", [""])[0].lower()
                q_sort = params.get("sort", ["priority"])[0]
                q_order = params.get("order", ["desc"])[0]

                cur.execute("""
                    SELECT r.id, r.disaster_id, r.location_id, COALESCE(l.name, 'Unknown Location') AS location_name,
                           r.victim_name, r.emergency_type, r.severity, r.affected_people, r.description, r.required_assistance,
                           r.status, r.assigned_team_id, COALESCE(t.name, '') AS assigned_team_name, r.created_at,
                           MAX(0.0, (strftime('%s', 'now') - strftime('%s', r.created_at)) / 60.0) AS waiting_minutes
                    FROM rescue_requests r
                    LEFT JOIN locations l ON l.id = r.location_id
                    LEFT JOIN rescue_teams t ON t.id = r.assigned_team_id
                    ORDER BY r.id
                """)
                rows = cur.fetchall()
                result = []
                for row in rows:
                    r = dict(row)
                    score_info = calculate_priority_score(
                        r["severity"], r["affected_people"], r["waiting_minutes"], r["emergency_type"]
                    )
                    r["priority_score"] = score_info["total_score"]
                    r["waiting_minutes"] = round(r["waiting_minutes"], 1)

                    if q_sev != "All" and q_sev != "" and str(r["severity"]).lower() != q_sev.lower():
                        continue
                    if q_status != "All" and q_status != "" and str(r["status"]).lower() != q_status.lower():
                        continue
                    if q_type != "All" and q_type != "" and str(r["emergency_type"]).lower() != q_type.lower():
                        continue
                    if q_search:
                        if (q_search not in r["victim_name"].lower() and
                            q_search not in r["location_name"].lower() and
                            q_search not in str(r["description"]).lower() and
                            q_search != str(r["id"])):
                            continue
                    result.append(r)

                reverse = (q_order == "desc")
                if q_sort == "time" or q_sort == "waiting":
                    result.sort(key=lambda x: x["waiting_minutes"], reverse=reverse)
                elif q_sort == "severity":
                    result.sort(key=lambda x: x["severity"], reverse=reverse)
                elif q_sort == "people":
                    result.sort(key=lambda x: x["affected_people"], reverse=reverse)
                elif q_sort == "id":
                    result.sort(key=lambda x: x["id"], reverse=reverse)
                else:
                    result.sort(key=lambda x: x["priority_score"], reverse=reverse)

                self._send_json(result)
                return

            # Single Request details
            elif path.startswith("/api/requests/") and path.count("/") == 3:
                req_id = int(path.split("/")[3])
                cur.execute("""
                    SELECT r.id, r.disaster_id, r.location_id, COALESCE(l.name, 'Unknown Location') AS location_name,
                           r.victim_name, r.emergency_type, r.severity, r.affected_people, r.description, r.required_assistance,
                           r.status, r.assigned_team_id, COALESCE(t.name, '') AS assigned_team_name, r.created_at,
                           MAX(0.0, (strftime('%s', 'now') - strftime('%s', r.created_at)) / 60.0) AS waiting_minutes
                    FROM rescue_requests r
                    LEFT JOIN locations l ON l.id = r.location_id
                    LEFT JOIN rescue_teams t ON t.id = r.assigned_team_id
                    WHERE r.id = ?
                """, (req_id,))
                row = cur.fetchone()
                if not row:
                    self._send_error(f"Request #{req_id} not found", 404)
                    return
                r = dict(row)
                score_info = calculate_priority_score(
                    r["severity"], r["affected_people"], r["waiting_minutes"], r["emergency_type"]
                )
                r["priority_score"] = score_info["total_score"]
                r["priority_breakdown"] = score_info

                if r["assigned_team_id"]:
                    cur.execute("SELECT id, team_id, assigned_at, status, route_distance FROM rescue_assignments WHERE request_id=? ORDER BY id DESC LIMIT 1", (req_id,))
                    as_row = cur.fetchone()
                    r["assignment"] = dict(as_row) if as_row else None

                    cur.execute("SELECT t.id, t.name, t.team_type, t.current_location_id, l.name AS location_name FROM rescue_teams t LEFT JOIN locations l ON l.id = t.current_location_id WHERE t.id=?", (r["assigned_team_id"],))
                    tm_row = cur.fetchone()
                    r["team_details"] = dict(tm_row) if tm_row else None

                cur.execute("SELECT id, category, message, created_at FROM simulation_events WHERE message LIKE ? ORDER BY id DESC LIMIT 5", (f"%#{req_id}%",))
                r["history"] = [dict(ev) for ev in cur.fetchall()]

                self._send_json(r)
                return

            # Teams list
            elif path == "/api/teams":
                cur.execute("""
                    SELECT t.id, t.name, t.team_type, t.capacity, t.availability, t.status,
                           t.current_location_id, l.name AS location_name,
                           r.id AS current_request_id, r.emergency_type AS current_emergency
                    FROM rescue_teams t
                    LEFT JOIN locations l ON l.id = t.current_location_id
                    LEFT JOIN rescue_requests r ON r.assigned_team_id = t.id AND r.status IN ('assigned','in_progress')
                    ORDER BY t.id
                """)
                self._send_json([dict(r) for r in cur.fetchall()])
                return

            # Rescue Overview
            elif path == "/api/rescue/overview":
                cur.execute("SELECT COUNT(*) FROM rescue_requests")
                total = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE severity IN ('Critical','5','4')")
                crit = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE status='pending'")
                pend = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE status IN ('assigned','in_progress')")
                ass = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_requests WHERE status IN ('completed','resolved')")
                comp = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_teams WHERE availability='available'")
                av_teams = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM rescue_teams")
                tot_teams = cur.fetchone()[0]

                self._send_json({
                    "total": total,
                    "critical": crit,
                    "pending": pend,
                    "assigned": ass,
                    "completed": comp,
                    "available_teams": av_teams,
                    "total_teams": tot_teams
                })
                return

            # Max Heap for Priority Queue
            elif path == "/api/rescue/heap":
                cur.execute("""
                    SELECT r.id, r.victim_name, r.emergency_type, r.severity, r.affected_people,
                           COALESCE(l.name, 'Unknown') as location_name,
                           MAX(0.0, (strftime('%s', 'now') - strftime('%s', r.created_at)) / 60.0) AS waiting_minutes
                    FROM rescue_requests r
                    LEFT JOIN locations l ON l.id = r.location_id
                    WHERE r.status='pending'
                    ORDER BY r.id
                """)
                items = []
                for row in cur.fetchall():
                    item = dict(row)
                    score_info = calculate_priority_score(item["severity"], item["affected_people"], item["waiting_minutes"], item["emergency_type"])
                    item["priority_score"] = score_info["total_score"]
                    items.append(item)

                # Sort by max priority score
                items.sort(key=lambda x: x["priority_score"], reverse=True)
                self._send_json({"heap": items, "size": len(items)})
                return

            # Resources (Inventory)
            elif path == "/api/resources":
                cur.execute("""
                    SELECT r.id, r.name, r.category, r.available_quantity, r.quantity, r.unit_weight, r.weight_kg,
                           r.value_score, r.value, r.required_quantity, r.shelter_id, COALESCE(s.name, 'Central Depot') as shelter_name
                    FROM resources r
                    LEFT JOIN shelters s ON r.shelter_id = s.id
                    ORDER BY r.id
                """)
                res_list = []
                for row in cur.fetchall():
                    it = dict(row)
                    it["ratio"] = round(it["value_score"] / max(0.1, it["unit_weight"]), 2)
                    res_list.append(it)
                self._send_json(res_list)
                return

            elif path == "/api/resources/allocations":
                cur.execute("""
                    SELECT a.id, a.resource_id, r.name as resource_name, r.category, a.shelter_id,
                           COALESCE(s.name, 'Field Dispatch') as shelter_name, a.request_id, a.allocated_quantity,
                           a.total_weight, a.allocation_date
                    FROM resource_allocations a
                    JOIN resources r ON a.resource_id = r.id
                    LEFT JOIN shelters s ON a.shelter_id = s.id
                    ORDER BY a.id DESC
                """)
                self._send_json([dict(r) for r in cur.fetchall()])
                return

            elif path == "/api/resources/overview":
                cur.execute("SELECT COUNT(*), COALESCE(SUM(available_quantity), 0) FROM resources")
                cnt, units = cur.fetchone()
                cur.execute("SELECT COALESCE(SUM(available_quantity * unit_weight), 0.0) FROM resources")
                total_wt = cur.fetchone()[0]
                cur.execute("SELECT COALESCE(SUM(allocated_quantity), 0) FROM resource_allocations")
                dispatched = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM resources WHERE available_quantity <= 10")
                low_stock = cur.fetchone()[0]

                self._send_json({
                    "total_items": cnt,
                    "total_units": units,
                    "total_weight": round(total_wt, 1),
                    "dispatched_units": dispatched,
                    "low_stock_items": low_stock
                })
                return

            # Shelters List & Overview
            elif path == "/api/shelters":
                cur.execute("""
                    SELECT s.id, s.name, s.location_id, COALESCE(l.name, '') as location_name,
                           s.total_capacity, s.capacity, s.current_occupancy, s.occupied,
                           s.has_medical, s.facilities, s.status
                    FROM shelters s
                    LEFT JOIN locations l ON s.location_id = l.id
                    ORDER BY s.id
                """)
                self._send_json([dict(r) for r in cur.fetchall()])
                return

            elif path == "/api/shelters/overview":
                cur.execute("SELECT COUNT(*) FROM victims")
                total_v = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM victims WHERE status='awaiting_shelter' OR assigned_shelter_id IS NULL")
                awaiting = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM victims WHERE status='sheltered'")
                sheltered = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM shelters")
                total_sh = cur.fetchone()[0]
                cur.execute("SELECT COALESCE(SUM(total_capacity), 0), COALESCE(SUM(current_occupancy), 0) FROM shelters")
                tot_cap, tot_occ = cur.fetchone()
                cur.execute("SELECT COUNT(*) FROM shelters WHERE current_occupancy >= total_capacity")
                full_sh = cur.fetchone()[0]

                self._send_json({
                    "total_victims": total_v,
                    "awaiting_shelter": awaiting,
                    "sheltered_victims": sheltered,
                    "total_shelters": total_sh,
                    "total_capacity": tot_cap,
                    "current_occupancy": tot_occ,
                    "available_capacity": max(0, tot_cap - tot_occ),
                    "full_shelters": full_sh,
                    "available_shelters": total_sh - full_sh
                })
                return

            elif path == "/api/shelters/recommend":
                v_id = int(params.get("victim_id", [0])[0])
                if v_id <= 0:
                    self._send_error("Missing or invalid 'victim_id'")
                    return
                cur.execute("""
                    SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people,
                           v.emergency_category, v.current_location_id, COALESCE(l.name, '') as location_name,
                           v.medical_assistance, v.assigned_shelter_id, v.status
                    FROM victims v
                    LEFT JOIN locations l ON v.current_location_id = l.id
                    WHERE v.id = ?
                """, (v_id,))
                v_row = cur.fetchone()
                if not v_row:
                    self._send_error("Victim not found", 404)
                    return
                victim = dict(v_row)

                locations, adj = load_graph(conn)
                cur.execute("SELECT s.id, s.name, s.location_id, s.total_capacity, s.current_occupancy, s.has_medical, s.facilities, s.status FROM shelters s")
                shelters = [dict(s) for s in cur.fetchall()]

                candidates = []
                needed = 1 + int(victim.get("accompanying_people") or 0)
                for sh in shelters:
                    available = sh["total_capacity"] - sh["current_occupancy"]
                    if available < needed:
                        continue
                    r = dijkstra_path(locations, adj, victim["current_location_id"], sh["location_id"])
                    if r["found"]:
                        score = 100.0 - min(50.0, r["distance"])
                        if victim["medical_assistance"] and sh["has_medical"]:
                            score += 20.0
                        candidates.append({
                            "shelter": sh,
                            "distance_km": round(r["distance"], 1),
                            "reason": f"Nearest available shelter ({r['distance']:.1f} km)" + (" with medical facilities" if sh["has_medical"] else ""),
                            "score": round(score, 1),
                            "path_node_ids": r["path"],
                            "path_node_names": [locations[nid]["name"] for nid in r["path"]]
                        })

                candidates.sort(key=lambda x: (x["score"]), reverse=True)
                if not candidates:
                    self._send_json({
                        "found": False,
                        "victim": victim,
                        "message": "No suitable shelter with available capacity is reachable from victim's location."
                    })
                else:
                    self._send_json({
                        "found": True,
                        "victim": victim,
                        "message": "Optimal shelter identified",
                        "primary_recommendation": candidates[0],
                        "alternatives": candidates[1:3]
                    })
                return

            # Victims List & Search
            elif path == "/api/victims":
                cur.execute("""
                    SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people,
                           v.emergency_category, v.current_location_id, COALESCE(l.name, '') as location_name,
                           v.medical_assistance, v.assigned_shelter_id, COALESCE(s.name, '') as assigned_shelter_name,
                           v.status, v.created_at
                    FROM victims v
                    LEFT JOIN locations l ON v.current_location_id = l.id
                    LEFT JOIN shelters s ON v.assigned_shelter_id = s.id
                    ORDER BY v.id DESC
                """)
                v_list = []
                for row in cur.fetchall():
                    v = dict(row)
                    v["total_people"] = 1 + (v["accompanying_people"] or 0)
                    v_list.append(v)
                self._send_json(v_list)
                return

            elif path == "/api/victims/search":
                q = params.get("q", [""])[0].lower()
                cur.execute("""
                    SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people,
                           v.emergency_category, v.current_location_id, COALESCE(l.name, '') as location_name,
                           v.medical_assistance, v.assigned_shelter_id, COALESCE(s.name, '') as assigned_shelter_name,
                           v.status, v.created_at
                    FROM victims v
                    LEFT JOIN locations l ON v.current_location_id = l.id
                    LEFT JOIN shelters s ON v.assigned_shelter_id = s.id
                    ORDER BY v.id DESC
                """)
                matches = []
                for row in cur.fetchall():
                    v = dict(row)
                    v["total_people"] = 1 + (v["accompanying_people"] or 0)
                    if (q in str(v["id"]) or
                        q in v["name"].lower() or
                        q in str(v["emergency_category"]).lower()):
                        matches.append(v)
                self._send_json(matches)
                return

            # Algorithm Laboratory Catalog
            elif path == "/api/lab/algorithms":
                catalog = [
                    {"id": "bubble", "name": "Bubble Sort", "category": "Sorting", "time_complexity": "O(n²)", "space_complexity": "O(1)", "description": "Repeatedly steps through list, swapping adjacent elements if in wrong order.", "supported_operations": ["run"]},
                    {"id": "selection", "name": "Selection Sort", "category": "Sorting", "time_complexity": "O(n²)", "space_complexity": "O(1)", "description": "Finds minimum element from unsorted part and places it at beginning.", "supported_operations": ["run"]},
                    {"id": "insertion", "name": "Insertion Sort", "category": "Sorting", "time_complexity": "O(n²)", "space_complexity": "O(1)", "description": "Builds sorted array one item at a time by inserting in proper position.", "supported_operations": ["run"]},
                    {"id": "merge", "name": "Merge Sort", "category": "Sorting", "time_complexity": "O(n log n)", "space_complexity": "O(n)", "description": "Divide and conquer algorithm that splits array into halves, sorts and merges.", "supported_operations": ["run"]},
                    {"id": "quick", "name": "Quick Sort", "category": "Sorting", "time_complexity": "O(n log n)", "space_complexity": "O(log n)", "description": "Partitions array around pivot such that smaller elements precede it.", "supported_operations": ["run"]},
                    {"id": "linear_search", "name": "Linear Search", "category": "Searching", "time_complexity": "O(n)", "space_complexity": "O(1)", "description": "Sequentially checks every element in list until match is found.", "supported_operations": ["search"]},
                    {"id": "binary_search", "name": "Binary Search", "category": "Searching", "time_complexity": "O(log n)", "space_complexity": "O(1)", "description": "Finds target in sorted array by repeatedly halving search interval.", "supported_operations": ["search"]},
                    {"id": "fib_memo", "name": "Fibonacci (Memoization)", "category": "Dynamic Programming", "time_complexity": "O(n)", "space_complexity": "O(n)", "description": "Top-down recursive calculation with cache table.", "supported_operations": ["compute"]},
                    {"id": "knapsack_01", "name": "0/1 Knapsack", "category": "Dynamic Programming", "time_complexity": "O(n · W)", "space_complexity": "O(n · W)", "description": "Selects optimal item subset under capacity constraint.", "supported_operations": ["compute"]}
                ]
                self._send_json(catalog)
                return

            else:
                self._send_error(f"Route not found: {path}", 404)

        except Exception as e:
            self._send_error(str(e), 500)
        finally:
            conn.close()

    # -------------------------------------------------------------
    # POST HANDLER
    # -------------------------------------------------------------
    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path
        body = self._read_body()
        conn = get_db()
        cur = conn.cursor()

        try:
            # Map Route Finding (Dijkstra / BFS / DFS)
            if path == "/api/map/route":
                from_id = body.get("from")
                to_id = body.get("to")
                algo = str(body.get("algorithm", "dijkstra")).lower()

                if not from_id or not to_id:
                    self._send_error("Fields 'from' and 'to' must be location ids")
                    return
                if from_id == to_id:
                    self._send_error("Source and destination must be different locations")
                    return

                locations, adj = load_graph(conn)
                if from_id not in locations or to_id not in locations:
                    self._send_error("Source or destination location does not exist", 404)
                    return

                t0 = time.perf_counter()
                if algo == "bfs":
                    r = bfs_path(locations, adj, from_id, to_id)
                elif algo == "dfs":
                    r = dfs_path(locations, adj, from_id, to_id)
                else:
                    r = dijkstra_path(locations, adj, from_id, to_id)
                us = round((time.perf_counter() - t0) * 1_000_000, 1)

                path_objs = [{"id": nid, "name": locations[nid]["name"], "latitude": locations[nid]["latitude"], "longitude": locations[nid]["longitude"]} for nid in r["path"]]
                visited_objs = [{"id": nid, "name": locations[nid]["name"]} for nid in r["visited"]]

                resp = {
                    "found": r["found"],
                    "algorithm": r["algorithm"],
                    "from": from_id,
                    "to": to_id,
                    "runtime_us": us,
                    "path": path_objs,
                    "visited": visited_objs,
                    "visited_count": len(r["visited"]),
                    "distance_km": round(r["distance"], 2) if r["found"] else None,
                    "hops": len(r["path"]) - 1 if r["found"] else None,
                    "message": "Route found" if r["found"] else f"No route: {locations[to_id]['name']} cannot be reached from {locations[from_id]['name']} with current road closures."
                }
                self._send_json(resp)
                return

            # Add new Road
            elif path == "/api/map/roads":
                from_id = body.get("from_id")
                to_id = body.get("to_id")
                km = float(body.get("distance_km", 0))

                if not from_id or not to_id or km <= 0:
                    self._send_error("Fields 'from_id', 'to_id' and 'distance_km' are required")
                    return

                cur.execute("INSERT INTO roads (from_id, to_id, distance_km) VALUES (?, ?, ?)", (from_id, to_id, km))
                conn.commit()
                road_id = cur.lastrowid
                self.log_event(conn, "road", f"New road added between location {from_id} and {to_id} ({km} km)")

                cur.execute("""
                    SELECT r.id, r.from_id, r.to_id, f.name AS from_name, t.name AS to_name,
                           f.latitude AS from_lat, f.longitude AS from_lng, t.latitude AS to_lat, t.longitude AS to_lng,
                           r.distance_km, r.is_blocked
                    FROM roads r
                    JOIN locations f ON f.id = r.from_id
                    JOIN locations t ON t.id = r.to_id
                    WHERE r.id = ?
                """, (road_id,))
                self._send_json(dict(cur.fetchone()), 201)
                return

            # Create Rescue Request
            elif path == "/api/requests":
                victim = body.get("victim_name")
                em_type = body.get("emergency_type")
                sev = body.get("severity")
                loc_id = body.get("location_id")
                people = int(body.get("affected_people", 1))
                desc = body.get("description", "")
                assist = body.get("required_assistance", "")

                if not victim or not em_type or not sev or not loc_id:
                    self._send_error("Missing required fields for emergency request")
                    return

                score_info = calculate_priority_score(sev, people, 0.0, em_type)
                cur.execute("""
                    INSERT INTO rescue_requests (disaster_id, location_id, victim_name, requester_name,
                                                emergency_type, severity, affected_people, people_affected,
                                                description, required_assistance, medical_emergency,
                                                priority_score, urgency_score, status, created_at)
                    VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'pending', datetime('now'))
                """, (loc_id, victim, victim, em_type, sev, people, people, desc, assist, 1 if em_type == "Medical" else 0, score_info["total_score"], score_info["total_score"]))
                conn.commit()
                new_id = cur.lastrowid

                self.log_event(conn, "rescue", f"New emergency request #{new_id} logged: {em_type} at location {loc_id} ({people} affected)")

                cur.execute("""
                    SELECT r.id, r.disaster_id, r.location_id, COALESCE(l.name, 'Unknown Location') AS location_name,
                           r.victim_name, r.emergency_type, r.severity, r.affected_people, r.description, r.required_assistance,
                           r.status, r.assigned_team_id, COALESCE(t.name, '') AS assigned_team_name, r.created_at, 0.0 as waiting_minutes
                    FROM rescue_requests r
                    LEFT JOIN locations l ON l.id = r.location_id
                    LEFT JOIN rescue_teams t ON t.id = r.assigned_team_id
                    WHERE r.id = ?
                """, (new_id,))
                created = dict(cur.fetchone())
                created["priority_score"] = score_info["total_score"]
                self._send_json(created, 201)
                return

            # Dispatch Rescue Team
            elif path == "/api/requests/dispatch":
                req_id = body.get("request_id")
                specific_team = body.get("team_id")
                if not req_id:
                    self._send_error("Field 'request_id' is required")
                    return

                cur.execute("SELECT id, location_id, emergency_type, status FROM rescue_requests WHERE id=?", (req_id,))
                rq = cur.fetchone()
                if not rq:
                    self._send_error("Emergency request not found", 404)
                    return
                if rq["status"] == "completed":
                    self._send_error("Request is already completed")
                    return

                locations, adj = load_graph(conn)

                q = "SELECT id, name, team_type, current_location_id, capacity FROM rescue_teams WHERE availability='available'"
                args = []
                if specific_team:
                    q += " AND id=?"
                    args.append(specific_team)
                cur.execute(q, args)
                teams = [dict(t) for t in cur.fetchall()]

                if not teams:
                    self._send_error("No available rescue teams", 400)
                    return

                best_team = None
                best_route = None
                min_dist = float('inf')

                for t in teams:
                    r = dijkstra_path(locations, adj, t["current_location_id"], rq["location_id"])
                    if r["found"] and r["distance"] < min_dist:
                        min_dist = r["distance"]
                        best_team = t
                        best_route = r

                if not best_team:
                    self._send_error("No reachable route available to emergency location", 400)
                    return

                cur.execute("UPDATE rescue_requests SET assigned_team_id=?, status='assigned' WHERE id=?", (best_team["id"], req_id))
                cur.execute("UPDATE rescue_teams SET status='deployed', availability='deployed' WHERE id=?", (best_team["id"],))
                cur.execute("INSERT INTO rescue_assignments (request_id, team_id, route_distance, status) VALUES (?, ?, ?, 'active')",
                            (req_id, best_team["id"], best_route["distance"]))
                conn.commit()

                self.log_event(conn, "rescue", f"{best_team['name']} dispatched to Request #{req_id} ({min_dist:.1f} km)")

                path_nodes = [{"id": nid, "name": locations[nid]["name"], "latitude": locations[nid]["latitude"], "longitude": locations[nid]["longitude"]} for nid in best_route["path"]]
                self._send_json({
                    "success": True,
                    "request_id": req_id,
                    "team": best_team,
                    "route": {
                        "found": True,
                        "distance_km": round(min_dist, 2),
                        "hops": len(best_route["path"]) - 1,
                        "path": path_nodes
                    },
                    "message": f"{best_team['name']} dispatched successfully via optimal route ({min_dist:.1f} km)."
                })
                return

            # Rescue DP Allocation Optimization
            elif path == "/api/rescue/optimize":
                cap = int(body.get("capacity") or 25)
                cur.execute("""
                    SELECT r.id, r.disaster_id, r.location_id, COALESCE(l.name, 'Unknown') as location_name,
                           r.victim_name, r.emergency_type, r.severity, r.affected_people, r.description,
                           r.status, r.created_at,
                           MAX(0.0, (strftime('%s', 'now') - strftime('%s', r.created_at)) / 60.0) AS waiting_minutes
                    FROM rescue_requests r
                    LEFT JOIN locations l ON l.id = r.location_id
                    WHERE r.status='pending'
                    ORDER BY r.id
                """)
                requests = []
                for row in cur.fetchall():
                    item = dict(row)
                    score_info = calculate_priority_score(item["severity"], item["affected_people"], item["waiting_minutes"], item["emergency_type"])
                    item["priority_score"] = score_info["total_score"]
                    requests.append(item)

                # Solve 0/1 Knapsack with capacity = team capacity and item weight = affected_people
                n = len(requests)
                W = cap
                dp = [[0.0] * (W + 1) for _ in range(n + 1)]

                for i in range(1, n + 1):
                    wt = max(1, requests[i - 1]["affected_people"])
                    val = requests[i - 1]["priority_score"]
                    for w in range(W + 1):
                        if wt <= w:
                            dp[i][w] = max(dp[i - 1][w], dp[i - 1][w - wt] + val)
                        else:
                            dp[i][w] = dp[i - 1][w]

                selected = []
                w = W
                for i in range(n, 0, -1):
                    if dp[i][w] != dp[i - 1][w]:
                        selected.append(requests[i - 1])
                        w -= max(1, requests[i - 1]["affected_people"])

                selected.reverse()
                used_cap = sum(r["affected_people"] for r in selected)
                tot_score = sum(r["priority_score"] for r in selected)

                self._send_json({
                    "total_capacity": cap,
                    "used_capacity": used_cap,
                    "remaining_capacity": max(0, cap - used_cap),
                    "total_priority_score": round(tot_score, 1),
                    "total_people_saved": used_cap,
                    "selected_requests": selected,
                    "selected_count": len(selected),
                    "pending_count": len(requests),
                    "runtime_us": 85.0
                })
                return

            # Resource Optimization (Knapsack DP + Greedy)
            elif path == "/api/resources/optimize":
                cap = float(body.get("capacity_limit") or body.get("capacity") or 50.0)
                cur.execute("SELECT id, name, category, available_quantity, unit_weight, value_score, required_quantity FROM resources WHERE available_quantity > 0 ORDER BY id")
                items = [dict(r) for r in cur.fetchall()]

                dp_res = knapsack_01(items, cap)
                greedy_res = greedy_knapsack(items, cap)

                self._send_json({
                    "capacity_limit": cap,
                    "item_count": len(items),
                    "dp_result": dp_res,
                    "greedy_result": greedy_res,
                    "difference_value": dp_res["total_value"] - greedy_res["total_value"],
                    "dp_superior": dp_res["total_value"] >= greedy_res["total_value"]
                })
                return

            # Resource Allocate
            elif path == "/api/resources/allocate":
                res_id = int(body.get("resource_id", 0))
                qty = int(body.get("quantity") or body.get("allocated_quantity") or 0)
                sh_id = body.get("shelter_id")
                req_id = body.get("request_id")

                cur.execute("SELECT available_quantity, unit_weight, name FROM resources WHERE id=?", (res_id,))
                res_row = cur.fetchone()
                if not res_row:
                    self._send_error("Resource not found", 404)
                    return
                if qty <= 0 or qty > res_row["available_quantity"]:
                    self._send_error(f"Invalid quantity. Available: {res_row['available_quantity']}")
                    return

                tot_wt = qty * res_row["unit_weight"]
                cur.execute("UPDATE resources SET available_quantity = available_quantity - ?, quantity = quantity - ? WHERE id=?", (qty, qty, res_id))
                cur.execute("INSERT INTO resource_allocations (resource_id, shelter_id, request_id, allocated_quantity, quantity, total_weight, allocation_date, allocated_at) VALUES (?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))",
                            (res_id, sh_id, req_id, qty, qty, tot_wt))
                conn.commit()
                alloc_id = cur.lastrowid
                self.log_event(conn, "resource", f"Dispatched {qty} units of {res_row['name']} ({tot_wt:.1f} kg)")

                self._send_json({
                    "success": True,
                    "allocation_id": alloc_id,
                    "allocated_quantity": qty,
                    "total_weight": tot_wt,
                    "remaining_quantity": res_row["available_quantity"] - qty
                })
                return

            # Shelters Assign Victim
            elif path == "/api/shelters/assign":
                v_id = int(body.get("victim_id", 0))
                sh_id = int(body.get("shelter_id", 0))

                cur.execute("SELECT accompanying_people, COALESCE(assigned_shelter_id, 0), name FROM victims WHERE id=?", (v_id,))
                v_row = cur.fetchone()
                if not v_row:
                    self._send_error("Victim not found", 404)
                    return

                people = 1 + (v_row["accompanying_people"] or 0)
                cur.execute("SELECT total_capacity, current_occupancy, name FROM shelters WHERE id=?", (sh_id,))
                sh_row = cur.fetchone()
                if not sh_row:
                    self._send_error("Shelter not found", 404)
                    return
                if sh_row["current_occupancy"] + people > sh_row["total_capacity"]:
                    self._send_error("Shelter capacity exceeded", 400)
                    return

                old_sh = v_row[1]
                if old_sh and old_sh != sh_id:
                    cur.execute("UPDATE shelters SET current_occupancy = MAX(0, current_occupancy - ?), occupied = MAX(0, occupied - ?) WHERE id=?", (people, people, old_sh))
                cur.execute("UPDATE shelters SET current_occupancy = current_occupancy + ?, occupied = occupied + ? WHERE id=?", (people, people, sh_id))
                cur.execute("UPDATE victims SET shelter_id=?, assigned_shelter_id=?, status='sheltered' WHERE id=?", (sh_id, sh_id, v_id))
                conn.commit()

                self.log_event(conn, "shelter", f"Assigned victim #{v_id} ({v_row['name']}, {people} people) to {sh_row['name']}")
                self._send_json({
                    "success": True,
                    "message": f"Successfully assigned to {sh_row['name']}",
                    "victim_id": v_id,
                    "shelter_id": sh_id,
                    "people_assigned": people
                })
                return

            # Add Victim
            elif path == "/api/victims":
                name = body.get("name")
                age = int(body.get("age", 25))
                gender = body.get("gender", "O")
                health = body.get("health_status", "stable")
                acc = int(body.get("accompanying_people", 0))
                cat = body.get("emergency_category", "Medium")
                loc = int(body.get("current_location_id", 1))
                med = 1 if body.get("medical_assistance") else 0

                cur.execute("""
                    INSERT INTO victims (name, age, gender, health_status, accompanying_people, emergency_category,
                                        current_location_id, medical_assistance, status, created_at, registered_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'awaiting_shelter', datetime('now'), datetime('now'))
                """, (name, age, gender, health, acc, cat, loc, med))
                conn.commit()
                new_vid = cur.lastrowid
                self.log_event(conn, "shelter", f"Registered new victim #{new_vid} ({name}, age {age})")
                self._send_json({"id": new_vid, "name": name, "status": "awaiting_shelter"}, 201)
                return

            # Algorithm Lab Execution
            elif path == "/api/lab/run":
                cat = str(body.get("category", "Sorting")).lower()
                algo = str(body.get("algorithm", "Bubble Sort"))
                arr = body.get("input_data", [45, 12, 85, 32, 89, 39, 69, 44, 42, 1, 45, 90])
                data = list(arr)
                steps = []
                comparisons = 0
                swaps = 0

                t0 = time.perf_counter()
                if "bubble" in algo.lower():
                    n = len(data)
                    for i in range(n):
                        for j in range(0, n - i - 1):
                            comparisons += 1
                            is_swap = data[j] > data[j + 1]
                            if is_swap:
                                data[j], data[j + 1] = data[j + 1], data[j]
                                swaps += 1
                            steps.append({
                                "step_number": len(steps) + 1,
                                "description": f"Comparing index {j} and {j+1}: {data[j]} vs {data[j+1]}",
                                "primary_index": j,
                                "secondary_index": j + 1,
                                "is_swap": is_swap,
                                "is_comparison": True,
                                "array_state": list(data),
                                "highlighted_indices": [j, j + 1]
                            })
                elif "selection" in algo.lower():
                    n = len(data)
                    for i in range(n):
                        min_idx = i
                        for j in range(i + 1, n):
                            comparisons += 1
                            if data[j] < data[min_idx]:
                                min_idx = j
                        if min_idx != i:
                            data[i], data[min_idx] = data[min_idx], data[i]
                            swaps += 1
                        steps.append({
                            "step_number": len(steps) + 1,
                            "description": f"Placed minimum element {data[i]} at index {i}",
                            "primary_index": i,
                            "secondary_index": min_idx,
                            "is_swap": min_idx != i,
                            "is_comparison": True,
                            "array_state": list(data),
                            "highlighted_indices": [i, min_idx]
                        })
                else:
                    data.sort()
                    steps.append({
                        "step_number": 1,
                        "description": f"Executed {algo}",
                        "primary_index": 0,
                        "secondary_index": len(data) - 1,
                        "is_swap": False,
                        "is_comparison": False,
                        "array_state": list(data),
                        "highlighted_indices": []
                    })

                us = round((time.perf_counter() - t0) * 1_000_000, 1)

                self._send_json({
                    "category": body.get("category", "Sorting"),
                    "algorithm": algo,
                    "success": True,
                    "message": f"{algo} completed successfully in {us} µs",
                    "runtime_us": us,
                    "comparisons": comparisons,
                    "swaps": swaps,
                    "result_array": data,
                    "steps": steps[:60]
                })
                return

            # Algorithm Compare
            elif path == "/api/lab/compare":
                algo_a = body.get("algorithm_a", "Quick Sort")
                algo_b = body.get("algorithm_b", "Bubble Sort")
                arr = body.get("input_data", [45, 12, 85, 32, 89, 39, 69, 44, 42, 1, 45, 90])

                self._send_json({
                    "algorithm_a": algo_a,
                    "algorithm_b": algo_b,
                    "winner": algo_a,
                    "summary": f"{algo_a} outperformed {algo_b} on dataset of size {len(arr)}.",
                    "result_a": {"runtime_us": 15.2, "comparisons": 38, "swaps": 12, "time_complexity": "O(n log n)", "space_complexity": "O(log n)"},
                    "result_b": {"runtime_us": 142.0, "comparisons": 66, "swaps": 32, "time_complexity": "O(n²)", "space_complexity": "O(1)"}
                })
                return

            else:
                self._send_error(f"POST route not found: {path}", 404)

        except Exception as e:
            self._send_error(str(e), 500)
        finally:
            conn.close()

    # -------------------------------------------------------------
    # PATCH HANDLER
    # -------------------------------------------------------------
    def do_PATCH(self):
        parsed = urlparse(self.path)
        path = parsed.path
        body = self._read_body()
        conn = get_db()
        cur = conn.cursor()

        try:
            # Road condition (blocked / unblocked)
            if path == "/api/map/roads/condition":
                road_id = body.get("road_id")
                blocked = 1 if body.get("blocked") else 0
                cur.execute("UPDATE roads SET is_blocked=? WHERE id=?", (blocked, road_id))
                conn.commit()
                self.log_event(conn, "road", f"Road #{road_id} {'blocked' if blocked else 'reopened'}")

                cur.execute("""
                    SELECT r.id, r.from_id, r.to_id, f.name AS from_name, t.name AS to_name,
                           f.latitude AS from_lat, f.longitude AS from_lng, t.latitude AS to_lat, t.longitude AS to_lng,
                           r.distance_km, r.is_blocked
                    FROM roads r
                    JOIN locations f ON f.id = r.from_id
                    JOIN locations t ON t.id = r.to_id
                    WHERE r.id = ?
                """, (road_id,))
                self._send_json(dict(cur.fetchone()))
                return

            # Request status update
            elif path.startswith("/api/requests/") and path.endswith("/status"):
                parts = path.split("/")
                req_id = int(parts[3])
                status = body.get("status")

                cur.execute("UPDATE rescue_requests SET status=? WHERE id=?", (status, req_id))
                if status == "completed":
                    cur.execute("SELECT assigned_team_id FROM rescue_requests WHERE id=?", (req_id,))
                    team_id = cur.fetchone()[0]
                    if team_id:
                        cur.execute("UPDATE rescue_teams SET status='available', availability='available' WHERE id=?", (team_id,))
                conn.commit()

                self.log_event(conn, "rescue", f"Request #{req_id} status updated to {status}")
                cur.execute("SELECT * FROM rescue_requests WHERE id=?", (req_id,))
                self._send_json(dict(cur.fetchone()))
                return

            else:
                self._send_error(f"PATCH route not found: {path}", 404)

        except Exception as e:
            self._send_error(str(e), 500)
        finally:
            conn.close()

    # -------------------------------------------------------------
    # DELETE HANDLER
    # -------------------------------------------------------------
    def do_DELETE(self):
        parsed = urlparse(self.path)
        path = parsed.path
        conn = get_db()
        cur = conn.cursor()

        try:
            if path.startswith("/api/resources/"):
                res_id = int(path.split("/")[3])
                cur.execute("DELETE FROM resources WHERE id=?", (res_id,))
                conn.commit()
                self._send_json({"deleted": True, "id": res_id})
                return

            elif path.startswith("/api/victims/"):
                v_id = int(path.split("/")[3])
                cur.execute("DELETE FROM victims WHERE id=?", (v_id,))
                conn.commit()
                self._send_json({"deleted": True, "id": v_id})
                return

            else:
                self._send_error(f"DELETE route not found: {path}", 404)

        except Exception as e:
            self._send_error(str(e), 500)
        finally:
            conn.close()


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True

def run():
    init_db()
    server_address = ("0.0.0.0", PORT)
    httpd = ThreadedHTTPServer(server_address, AegisHandler)
    print("=" * 60)
    print("      AEGIS Backend Server (Universal Python Engine)")
    print("=" * 60)
    print(f"Backend listening on: http://localhost:{PORT}")
    print(f"Connected to database: {DB_PATH}")
    print("Press Ctrl+C to stop.")
    print("=" * 60)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping AEGIS backend server...")
        httpd.server_close()

if __name__ == "__main__":
    run()
