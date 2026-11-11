import { api } from '../api.js';

export async function getVictims(params = {}) {
  const query = new URLSearchParams();
  if (params.category && params.category !== 'All') query.set('category', params.category);
  if (params.status && params.status !== 'All') query.set('status', params.status);
  if (params.search) query.set('search', params.search);

  const qs = query.toString();
  return api(`/victims${qs ? `?${qs}` : ''}`);
}

export async function getVictim(id) {
  return api(`/victims/${id}`);
}

export async function createVictim(payload) {
  return api('/victims', {
    method: 'POST',
    body: payload,
  });
}

export async function updateVictim(id, payload) {
  return api(`/victims/${id}`, {
    method: 'PUT',
    body: payload,
  });
}

export async function deleteVictim(id) {
  return api(`/victims/${id}`, {
    method: 'DELETE',
  });
}

export async function searchVictims(q) {
  return api(`/victims/search?q=${encodeURIComponent(q)}`);
}

export async function getShelters() {
  return api('/shelters');
}

export async function createShelter(payload) {
  return api('/shelters', {
    method: 'POST',
    body: payload,
  });
}

export async function updateShelter(id, payload) {
  return api(`/shelters/${id}`, {
    method: 'PUT',
    body: payload,
  });
}

export async function deleteShelter(id) {
  return api(`/shelters/${id}`, {
    method: 'DELETE',
  });
}

export async function recommendShelter(victimId) {
  return api(`/shelters/recommend?victim_id=${victimId}`);
}

export async function assignShelter(victimId, shelterId) {
  return api('/shelters/assign', {
    method: 'POST',
    body: {
      victim_id: Number(victimId),
      shelter_id: Number(shelterId),
    },
  });
}

export async function getShelterOverview() {
  return api('/shelters/overview');
}
