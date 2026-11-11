import { useCallback, useEffect, useState } from 'react';
import MapView, { TYPE_COLORS, TYPE_LABELS } from '../components/MapView.jsx';
import { addRoad, findRoute, getLocations, getRoads, setRoadBlocked } from '../services/mapService.js';

const ALGORITHMS = {
  dijkstra: { label: 'Dijkstra', note: 'Shortest total distance. Best choice for rescue routes.' },
  bfs: { label: 'BFS', note: 'Fewest roads, ignoring distance.' },
  dfs: { label: 'DFS', note: 'Finds any route by going deep first. Often long.' },
};
const LEGEND = ['hospital', 'shelter', 'rescue_center', 'building', 'depot', 'village', 'junction'];

export default function DisasterMap() {
  const [locations, setLocations] = useState([]);
  const [roads, setRoads] = useState([]);
  const [from, setFrom] = useState('');
  const [to, setTo] = useState('');
  const [algorithm, setAlgorithm] = useState('dijkstra');
  const [route, setRoute] = useState(null);
  const [error, setError] = useState('');
  const [busy, setBusy] = useState(false);
  const [newRoad, setNewRoad] = useState({ from: '', to: '', km: '' });
  const [roadMsg, setRoadMsg] = useState({ text: '', ok: true });

  const loadMap = useCallback(async () => {
    try {
      const [l, r] = await Promise.all([getLocations(), getRoads()]);
      setLocations(l);
      setRoads(r);
      setError('');
    } catch (e) { setError(e.message); }
  }, []);
  useEffect(() => { loadMap(); }, [loadMap]);

  const runRoute = useCallback(async (f, t, algo) => {
    if (!f || !t) return;
    setBusy(true);
    try {
      setRoute(await findRoute(Number(f), Number(t), algo));
      setError('');
    } catch (e) { setRoute(null); setError(e.message); }
    setBusy(false);
  }, []);

  const onSubmit = (e) => {
    e.preventDefault();
    if (!from || !to) return setError('Choose both a source and a destination.');
    if (from === to) return setError('Source and destination must be different locations.');
    runRoute(from, to, algorithm);
  };

  // Clicking markers: first click sets the start, second the end, third starts over.
  const onPick = (id) => {
    const s = String(id);
    setRoute(null);
    if (!from || (from && to)) { setFrom(s); setTo(''); }
    else if (s !== from) setTo(s);
  };

  const toggleRoad = async (road) => {
    try {
      await setRoadBlocked(road.id, !road.is_blocked);
      await loadMap();
      if (route && from && to) await runRoute(from, to, algorithm); // refresh the route with the new closure
    } catch (e) { setError(e.message); }
  };

  const submitRoad = async (e) => {
    e.preventDefault();
    try {
      const r = await addRoad(Number(newRoad.from), Number(newRoad.to), Number(newRoad.km));
      setRoadMsg({ text: `Added road ${r.from_name} to ${r.to_name}.`, ok: true });
      setNewRoad({ from: '', to: '', km: '' });
      await loadMap();
    } catch (err) { setRoadMsg({ text: err.message, ok: false }); }
  };

  const nameOf = (id) => locations.find((l) => l.id === Number(id))?.name;
  const options = locations.map((l) => <option key={l.id} value={l.id}>{l.name}</option>);

  return (
    <>
      <header className="page-head">
        <h1>Disaster Map</h1>
        <p>Click two locations on the map or use the form to plan a rescue route. Blocked roads are skipped by every algorithm.</p>
      </header>
      {error && <div className="notice critical inline" role="alert">{error}</div>}

      <div className="map-layout">
        <div className="map-col">
          <MapView locations={locations} roads={roads} route={route} from={Number(from)} to={Number(to)} onPick={onPick} />
          <ul className="legend">
            {LEGEND.map((t) => (
              <li key={t}><i style={{ background: TYPE_COLORS[t] }} />{TYPE_LABELS[t]}</li>
            ))}
            <li><i className="line open" />Open road</li>
            <li><i className="line closed" />Blocked road</li>
          </ul>
        </div>

        <div className="side-col">
          <form className="card" onSubmit={onSubmit}>
            <h3>Find a route</h3>
            <label>From
              <select value={from} onChange={(e) => { setFrom(e.target.value); setRoute(null); }}>
                <option value="">Choose a location</option>{options}
              </select>
            </label>
            <label>To
              <select value={to} onChange={(e) => { setTo(e.target.value); setRoute(null); }}>
                <option value="">Choose a location</option>{options}
              </select>
            </label>
            <fieldset>
              <legend>Algorithm</legend>
              <div className="segmented">
                {Object.entries(ALGORITHMS).map(([key, a]) => (
                  <label key={key} className={algorithm === key ? 'on' : ''}>
                    <input type="radio" name="algo" checked={algorithm === key} onChange={() => { setAlgorithm(key); if (route) runRoute(from, to, key); }} />
                    {a.label}
                  </label>
                ))}
              </div>
              <p className="muted">{ALGORITHMS[algorithm].note}</p>
            </fieldset>
            <button className="btn" disabled={busy}>{busy ? 'Searching…' : 'Find route'}</button>
          </form>

          {route && (
            <section className={'card result' + (route.found ? '' : ' none')} aria-live="polite">
              {route.found ? (
                <>
                  <h3>Route found</h3>
                  <div className="result-nums">
                    <div><strong>{route.distance_km} km</strong><span>Total distance</span></div>
                    <div><strong>{route.hops}</strong><span>Roads used</span></div>
                    <div><strong>{route.visited_count}</strong><span>Nodes visited</span></div>
                  </div>
                  <p className="muted">{ALGORITHMS[route.algorithm].label} finished in {route.runtime_us} µs.</p>
                  <ol className="path">{route.path.map((p) => <li key={p.id}>{p.name}</li>)}</ol>
                  <details>
                    <summary>Visit order ({route.visited_count})</summary>
                    <p className="muted">{route.visited.map((v) => v.name).join(' → ')}</p>
                  </details>
                </>
              ) : (
                <>
                  <h3>No route available</h3>
                  <p>{route.message}</p>
                  <p className="muted">{route.algorithm.toUpperCase()} explored {route.visited_count} locations before running out of open roads. Reopen a road below to try again.</p>
                </>
              )}
            </section>
          )}

          <section className="card">
            <h3>Roads ({roads.filter((r) => r.is_blocked).length} blocked)</h3>
            <ul className="road-list">
              {roads.map((r) => (
                <li key={r.id} className={r.is_blocked ? 'blocked' : ''}>
                  <span>{r.from_name} to {r.to_name}<small>{r.distance_km} km</small></span>
                  <button className={'btn-small ' + (r.is_blocked ? 'reopen' : 'block')} onClick={() => toggleRoad(r)}
                          aria-label={`${r.is_blocked ? 'Reopen' : 'Block'} road ${r.from_name} to ${r.to_name}`}>
                    {r.is_blocked ? 'Reopen' : 'Block'}
                  </button>
                </li>
              ))}
            </ul>
          </section>

          <form className="card" onSubmit={submitRoad}>
            <h3>Add a road</h3>
            <label>Between
              <select required value={newRoad.from} onChange={(e) => setNewRoad({ ...newRoad, from: e.target.value })}>
                <option value="">Choose a location</option>{options}
              </select>
            </label>
            <label>And
              <select required value={newRoad.to} onChange={(e) => setNewRoad({ ...newRoad, to: e.target.value })}>
                <option value="">Choose a location</option>{options}
              </select>
            </label>
            <label>Distance (km)
              <input required type="number" step="0.1" min="0.1" max="500" value={newRoad.km} onChange={(e) => setNewRoad({ ...newRoad, km: e.target.value })} />
            </label>
            <button className="btn">Add road</button>
            {roadMsg.text && <p className={roadMsg.ok ? 'form-ok' : 'form-err'} role="status">{roadMsg.text}</p>}
          </form>
        </div>
      </div>
    </>
  );
}
