-- AEGIS database schema (SQLite). Applied automatically by the backend on first run.
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS disasters (
  id              INTEGER PRIMARY KEY,
  name            TEXT NOT NULL,
  type            TEXT NOT NULL CHECK (type IN ('flood','earthquake','cyclone','fire')),
  severity        INTEGER NOT NULL CHECK (severity BETWEEN 1 AND 5),
  description     TEXT,
  affected_people INTEGER NOT NULL DEFAULT 0 CHECK (affected_people >= 0),
  status          TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active','closed')),
  started_at      TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Graph nodes (Module 1)
CREATE TABLE IF NOT EXISTS locations (
  id          INTEGER PRIMARY KEY,
  disaster_id INTEGER NOT NULL REFERENCES disasters(id) ON DELETE CASCADE,
  name        TEXT NOT NULL,
  type        TEXT NOT NULL CHECK (type IN ('base','rescue_center','hospital','shelter','building','depot','village','junction','bridge')),
  latitude    REAL NOT NULL CHECK (latitude BETWEEN -90 AND 90),
  longitude   REAL NOT NULL CHECK (longitude BETWEEN -180 AND 180)
);

-- Graph edges (Module 1). Roads are two-way; weight is distance in km.
CREATE TABLE IF NOT EXISTS roads (
  id          INTEGER PRIMARY KEY,
  from_id     INTEGER NOT NULL REFERENCES locations(id) ON DELETE CASCADE,
  to_id       INTEGER NOT NULL REFERENCES locations(id) ON DELETE CASCADE,
  distance_km REAL NOT NULL CHECK (distance_km > 0),
  is_blocked  INTEGER NOT NULL DEFAULT 0 CHECK (is_blocked IN (0,1)),
  CHECK (from_id <> to_id),
  UNIQUE (from_id, to_id)
);

CREATE TABLE IF NOT EXISTS rescue_teams (
  id                  INTEGER PRIMARY KEY,
  name                TEXT NOT NULL UNIQUE,
  team_type           TEXT NOT NULL CHECK (team_type IN ('medical','fire','flood','general','search','boat')),
  type                TEXT NOT NULL DEFAULT 'general' CHECK (type IN ('medical','boat','fire','search','flood','general')),
  members             INTEGER NOT NULL DEFAULT 6 CHECK (members > 0),
  status              TEXT NOT NULL DEFAULT 'available' CHECK (status IN ('available','deployed','offline')),
  availability        TEXT NOT NULL DEFAULT 'available' CHECK (availability IN ('available','deployed','offline')),
  capacity            INTEGER NOT NULL DEFAULT 10 CHECK (capacity > 0),
  current_location_id INTEGER NOT NULL REFERENCES locations(id),
  base_location_id    INTEGER REFERENCES locations(id)
);

-- Priority queue input (Module 2). priority_score is computed by the C++ engine.
CREATE TABLE IF NOT EXISTS rescue_requests (
  id                  INTEGER PRIMARY KEY,
  disaster_id         INTEGER NOT NULL DEFAULT 1 REFERENCES disasters(id) ON DELETE CASCADE,
  location_id         INTEGER NOT NULL REFERENCES locations(id),
  victim_name         TEXT NOT NULL,
  requester_name      TEXT,
  emergency_type      TEXT NOT NULL DEFAULT 'Other' CHECK (emergency_type IN ('Fire','Flood','Medical','Accident','Building Collapse','Other','fire','flood','medical','accident','building collapse','other')),
  severity            TEXT NOT NULL DEFAULT 'Medium' CHECK (severity IN ('Critical','High','Medium','Low','critical','high','medium','low','1','2','3','4','5')),
  affected_people     INTEGER NOT NULL DEFAULT 1 CHECK (affected_people > 0),
  people_affected     INTEGER DEFAULT 1,
  description         TEXT,
  required_assistance TEXT,
  medical_emergency   INTEGER NOT NULL DEFAULT 0 CHECK (medical_emergency IN (0,1)),
  priority_score      REAL NOT NULL DEFAULT 0,
  urgency_score       REAL NOT NULL DEFAULT 0,
  status              TEXT NOT NULL DEFAULT 'pending' CHECK (status IN ('pending','assigned','in_progress','completed','resolved')),
  assigned_team_id    INTEGER REFERENCES rescue_teams(id) ON DELETE SET NULL,
  created_at          TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS rescue_assignments (
  id             INTEGER PRIMARY KEY,
  request_id     INTEGER NOT NULL REFERENCES rescue_requests(id) ON DELETE CASCADE,
  team_id        INTEGER NOT NULL REFERENCES rescue_teams(id) ON DELETE CASCADE,
  assigned_at    TEXT NOT NULL DEFAULT (datetime('now')),
  status         TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active','in_progress','completed','cancelled')),
  route_distance REAL NOT NULL DEFAULT 0 CHECK (route_distance >= 0)
);

-- Shelters table (Module 4)
CREATE TABLE IF NOT EXISTS shelters (
  id                 INTEGER PRIMARY KEY,
  name               TEXT NOT NULL UNIQUE,
  location_id        INTEGER NOT NULL REFERENCES locations(id),
  capacity           INTEGER NOT NULL CHECK (capacity > 0),
  total_capacity     INTEGER NOT NULL CHECK (total_capacity > 0),
  occupied           INTEGER NOT NULL DEFAULT 0 CHECK (occupied >= 0),
  current_occupancy  INTEGER NOT NULL DEFAULT 0 CHECK (current_occupancy >= 0),
  has_medical        INTEGER NOT NULL DEFAULT 0 CHECK (has_medical IN (0,1)),
  facilities         TEXT NOT NULL DEFAULT 'Medical Bay, Sanitation, Food Stock, Emergency Power',
  status             TEXT NOT NULL DEFAULT 'Available' CHECK (status IN ('Available','Limited Capacity','Full','available','limited','full')),
  CHECK (occupied <= capacity)
);

-- Emergency relief inventory items (Module 3). 
CREATE TABLE IF NOT EXISTS resources (
  id                 INTEGER PRIMARY KEY,
  name               TEXT NOT NULL UNIQUE,
  category           TEXT NOT NULL CHECK (category IN ('Food','Drinking Water','Medicines','First Aid','Rescue Equipment','Blankets','Other','food','water','medicine','equipment')),
  available_quantity INTEGER NOT NULL DEFAULT 0 CHECK (available_quantity >= 0),
  quantity           INTEGER NOT NULL DEFAULT 0 CHECK (quantity >= 0),
  unit_weight        REAL NOT NULL CHECK (unit_weight > 0),
  weight_kg          REAL NOT NULL DEFAULT 1 CHECK (weight_kg > 0),
  value_score        INTEGER NOT NULL CHECK (value_score > 0),
  value              INTEGER NOT NULL DEFAULT 1 CHECK (value > 0),
  required_quantity  INTEGER NOT NULL DEFAULT 0 CHECK (required_quantity >= 0),
  shelter_id         INTEGER REFERENCES shelters(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS settings (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

-- Victims table (Module 4)
CREATE TABLE IF NOT EXISTS victims (
  id                   INTEGER PRIMARY KEY,
  name                 TEXT NOT NULL,
  age                  INTEGER NOT NULL CHECK (age BETWEEN 0 AND 120),
  gender               TEXT NOT NULL DEFAULT 'O' CHECK (gender IN ('F','M','O','f','m','o','Other','Male','Female')),
  health_status        TEXT NOT NULL DEFAULT 'stable' CHECK (health_status IN ('stable','injured','critical')),
  accompanying_people  INTEGER NOT NULL DEFAULT 0 CHECK (accompanying_people >= 0),
  emergency_category   TEXT NOT NULL DEFAULT 'Medium' CHECK (emergency_category IN ('Critical','High','Medium','Low','critical','high','medium','low')),
  current_location_id  INTEGER REFERENCES locations(id),
  medical_assistance   INTEGER NOT NULL DEFAULT 0 CHECK (medical_assistance IN (0,1)),
  shelter_id           INTEGER REFERENCES shelters(id) ON DELETE SET NULL,
  assigned_shelter_id  INTEGER REFERENCES shelters(id) ON DELETE SET NULL,
  status               TEXT NOT NULL DEFAULT 'awaiting_shelter' CHECK (status IN ('awaiting_shelter','sheltered','evacuated','pending','assigned')),
  created_at           TEXT NOT NULL DEFAULT (datetime('now')),
  registered_at        TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS simulation_events (
  id          INTEGER PRIMARY KEY,
  disaster_id INTEGER REFERENCES disasters(id) ON DELETE CASCADE,
  category    TEXT NOT NULL CHECK (category IN ('road','rescue','resource','shelter','system')),
  message     TEXT NOT NULL,
  created_at  TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Supplies allocated to shelters or rescue requests (Module 3)
CREATE TABLE IF NOT EXISTS resource_allocations (
  id                 INTEGER PRIMARY KEY,
  resource_id        INTEGER NOT NULL REFERENCES resources(id) ON DELETE CASCADE,
  shelter_id         INTEGER REFERENCES shelters(id) ON DELETE SET NULL,
  request_id         INTEGER REFERENCES rescue_requests(id) ON DELETE SET NULL,
  allocated_quantity INTEGER NOT NULL CHECK (allocated_quantity > 0),
  quantity           INTEGER NOT NULL DEFAULT 1 CHECK (quantity > 0),
  total_weight       REAL NOT NULL CHECK (total_weight > 0),
  allocation_date    TEXT NOT NULL DEFAULT (datetime('now')),
  allocated_at       TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Measured runtimes from the Algorithm Laboratory (Module 4)
CREATE TABLE IF NOT EXISTS algorithm_runs (
  id         INTEGER PRIMARY KEY,
  algorithm  TEXT NOT NULL,
  input_size INTEGER NOT NULL,
  runtime_us REAL NOT NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX IF NOT EXISTS idx_roads_from     ON roads(from_id);
CREATE INDEX IF NOT EXISTS idx_roads_to       ON roads(to_id);
CREATE INDEX IF NOT EXISTS idx_requests_state ON rescue_requests(status);
CREATE INDEX IF NOT EXISTS idx_victims_name   ON victims(name);
