import { api } from '../api.js';

export const getLocations = () => api('/map/locations');
export const getRoads = () => api('/map/roads');
export const findRoute = (from, to, algorithm) => api('/map/route', { method: 'POST', body: { from, to, algorithm } });
export const setRoadBlocked = (road_id, blocked) => api('/map/roads/condition', { method: 'PATCH', body: { road_id, blocked } });
export const addRoad = (from_id, to_id, distance_km) => api('/map/roads', { method: 'POST', body: { from_id, to_id, distance_km } });
