import { useCallback, useEffect, useState } from 'react';
import { Bar, BarChart, CartesianGrid, Cell, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts';
import { api } from '../api.js';

const COLORS = { primary: '#38BDF8', warning: '#F59E0B', critical: '#EF4444', success: '#22C55E' };
const SEVERITY_LABEL = ['', 'Minor', 'Moderate', 'Serious', 'Severe', 'Catastrophic'];

// SQLite stores UTC as "YYYY-MM-DD HH:MM:SS"
function ago(ts) {
  const mins = Math.max(0, Math.round((Date.now() - new Date(ts.replace(' ', 'T') + 'Z')) / 60000));
  if (mins < 60) return `${mins} min ago`;
  const h = Math.floor(mins / 60);
  return `${h} h ${mins % 60} min ago`;
}

function fillColor(pct) {
  return pct >= 95 ? COLORS.critical : pct >= 75 ? COLORS.warning : COLORS.success;
}

function Stat({ label, value, note, tone }) {
  return (
    <div className="card stat">
      <span className="stat-label">{label}</span>
      <strong className={tone ? `tone-${tone}` : ''}>{value}</strong>
      <span className="stat-note">{note}</span>
    </div>
  );
}

export default function Dashboard() {
  const [data, setData] = useState(null);
  const [error, setError] = useState('');

  const load = useCallback(() => {
    api('/dashboard').then((d) => { setData(d); setError(''); }).catch((e) => setError(e.message));
  }, []);

  useEffect(() => {
    load();
    const t = setInterval(load, 10000);
    return () => clearInterval(t);
  }, [load]);

  if (error && !data) {
    return (
      <>
        <header className="page-head"><h1>Dashboard</h1></header>
        <section className="card notice critical">
          <h2>Backend not reachable</h2>
          <p>{error}</p>
          <button className="btn" onClick={load}>Try again</button>
        </section>
      </>
    );
  }
  if (!data) return <p className="muted">Loading dashboard…</p>;

  const s = data.scenario;
  const free = data.shelters.capacity - data.shelters.occupied;
  const shelterRows = data.shelters.list.map((x) => ({
    name: x.name.replace(' Shelter', ''),
    pct: Math.round((x.occupied / x.capacity) * 100),
    occupied: x.occupied,
    capacity: x.capacity,
  }));

  return (
    <>
      <header className="page-head">
        <h1>Dashboard</h1>
        <p>Live status of the active scenario. Refreshes every 10 seconds.</p>
      </header>

      {error && <div className="notice critical inline">Lost connection to the backend. Showing the last data received.</div>}

      {s ? (
        <section className="card scenario">
          <div className="scenario-main">
            <span className={`pill ${s.severity >= 4 ? 'critical' : 'warning'}`}>{SEVERITY_LABEL[s.severity]}</span>
            <h2>{s.name}</h2>
            <p>{s.description}</p>
          </div>
          <div className="scenario-side">
            <div className="meter" aria-label={`Severity ${s.severity} of 5`}>
              {[1, 2, 3, 4, 5].map((n) => (
                <i key={n} className={n <= s.severity ? (s.severity >= 4 ? 'on critical' : 'on warning') : ''} />
              ))}
            </div>
            <span className="muted">Started {ago(s.started_at)}</span>
            <span className="muted">{data.roads.blocked} of {data.roads.total} roads blocked</span>
          </div>
        </section>
      ) : (
        <section className="card notice"><h2>No active scenario</h2><p>Start a scenario to see live figures.</p></section>
      )}

      <section className="stats">
        <Stat label="People affected" value={data.affected_people.toLocaleString()} note="Estimated across the scenario" />
        <Stat label="Pending requests" value={data.requests.pending} note={`${data.requests.critical} are severe or worse`} tone={data.requests.critical ? 'critical' : ''} />
        <Stat label="Teams available" value={`${data.teams.available} / ${data.teams.total}`} note={`${data.teams.deployed} deployed`} tone="primary" />
        <Stat label="Shelter beds free" value={free.toLocaleString()} note={`${data.shelters.occupied.toLocaleString()} of ${data.shelters.capacity.toLocaleString()} in use`} tone={free < 150 ? 'warning' : 'success'} />
      </section>

      <section className="grid-2">
        <div className="card">
          <h3>Shelter occupancy</h3>
          <ResponsiveContainer width="100%" height={200}>
            <BarChart data={shelterRows} layout="vertical" margin={{ left: 10, right: 20 }}>
              <CartesianGrid stroke="#22304a" horizontal={false} />
              <XAxis type="number" domain={[0, 100]} tickFormatter={(v) => `${v}%`} stroke="#8392ab" />
              <YAxis type="category" dataKey="name" width={130} stroke="#8392ab" />
              <Tooltip
                cursor={{ fill: 'rgba(56,189,248,0.06)' }}
                contentStyle={{ background: '#101827', border: '1px solid #22304a' }}
                formatter={(v, _n, p) => [`${p.payload.occupied} of ${p.payload.capacity} beds (${v}%)`, 'Occupied']}
              />
              <Bar dataKey="pct" radius={[0, 4, 4, 0]}>
                {shelterRows.map((r) => <Cell key={r.name} fill={fillColor(r.pct)} />)}
              </Bar>
            </BarChart>
          </ResponsiveContainer>
        </div>

        <div className="card">
          <h3>Resource stock</h3>
          <ResponsiveContainer width="100%" height={200}>
            <BarChart data={data.resources} margin={{ right: 10 }}>
              <CartesianGrid stroke="#22304a" vertical={false} />
              <XAxis dataKey="category" stroke="#8392ab" />
              <YAxis stroke="#8392ab" />
              <Tooltip
                cursor={{ fill: 'rgba(56,189,248,0.06)' }}
                contentStyle={{ background: '#101827', border: '1px solid #22304a' }}
                formatter={(v, _n, p) => [`${v} units across ${p.payload.items} items`, 'In stock']}
              />
              <Bar dataKey="units" fill={COLORS.primary} radius={[4, 4, 0, 0]} />
            </BarChart>
          </ResponsiveContainer>
        </div>
      </section>

      <section className="card">
        <h3>Recent events</h3>
        <ul className="events">
          {data.events.map((e) => (
            <li key={e.id}>
              <span className={`dot ${e.category}`} />
              <span className="ev-msg">{e.message}</span>
              <span className="muted">{ago(e.created_at)}</span>
            </li>
          ))}
        </ul>
      </section>
    </>
  );
}
