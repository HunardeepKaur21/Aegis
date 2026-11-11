import { useEffect } from 'react';
import { CircleMarker, MapContainer, Polyline, TileLayer, Tooltip, useMap } from 'react-leaflet';

export const TYPE_COLORS = {
  hospital: '#EF4444',
  shelter: '#22C55E',
  rescue_center: '#38BDF8',
  base: '#38BDF8',
  building: '#A78BFA',
  depot: '#F59E0B',
  village: '#E2E8F0',
  junction: '#64748B',
  bridge: '#64748B',
};
export const TYPE_LABELS = {
  hospital: 'Hospital', shelter: 'Shelter', rescue_center: 'Rescue center', base: 'Rescue center',
  building: 'Building', depot: 'Supply depot', village: 'Village', junction: 'Junction', bridge: 'Bridge',
};

function FitToLocations({ locations }) {
  const map = useMap();
  useEffect(() => {
    if (locations.length) map.fitBounds(locations.map((l) => [l.latitude, l.longitude]), { padding: [40, 40] });
  }, [locations.length]); // eslint-disable-line react-hooks/exhaustive-deps
  return null;
}

export default function MapView({ locations, roads, route, from, to, onPick }) {
  const visited = new Set(route?.found ? route.visited.map((v) => v.id) : []);
  const routeLine = route?.found ? route.path.map((p) => [p.latitude, p.longitude]) : null;

  return (
    <MapContainer center={[28.6139, 77.209]} zoom={13} className="map" scrollWheelZoom zoomSnap={0.25}>
      <TileLayer
        attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
        url="https://tile.openstreetmap.org/{z}/{x}/{y}.png"
      />
      <FitToLocations locations={locations} />

      {roads.map((r) => (
        <Polyline
          key={r.id}
          positions={[[r.from_lat, r.from_lng], [r.to_lat, r.to_lng]]}
          pathOptions={r.is_blocked ? { color: '#EF4444', weight: 4, dashArray: '8 8', opacity: 0.9 } : { color: '#38BDF8', weight: 3, opacity: 0.7 }}
        >
          <Tooltip permanent direction="center" className={'road-label' + (r.is_blocked ? ' blocked' : '')}>
            {r.is_blocked ? 'Blocked · ' : ''}{r.distance_km} km
          </Tooltip>
        </Polyline>
      ))}

      {routeLine && <Polyline positions={routeLine} pathOptions={{ color: '#22C55E', weight: 8, opacity: 0.95 }} interactive={false} />}

      {locations.map((l) => {
        const selected = l.id === from || l.id === to;
        return (
          <CircleMarker
            key={l.id}
            center={[l.latitude, l.longitude]}
            radius={selected ? 13 : 9}
            pathOptions={{
              color: selected ? '#F1F5F9' : visited.has(l.id) ? '#F59E0B' : '#080D18',
              weight: selected ? 3 : visited.has(l.id) ? 3 : 2,
              fillColor: TYPE_COLORS[l.type] || '#64748B',
              fillOpacity: 1,
            }}
            eventHandlers={{ click: () => onPick(l.id) }}
          >
            <Tooltip direction="top" offset={[0, -8]} permanent={selected} className="loc-label">
              {l.id === from ? 'Start: ' : l.id === to ? 'End: ' : ''}{l.name}
              {!selected && <span className="loc-type"> ({TYPE_LABELS[l.type]})</span>}
            </Tooltip>
          </CircleMarker>
        );
      })}
    </MapContainer>
  );
}
