import { api } from '../api.js';

export async function getResources() {
  return api('/resources');
}

export async function createResource(payload) {
  return api('/resources', {
    method: 'POST',
    body: payload,
  });
}

export async function updateResource(id, payload) {
  return api(`/resources/${id}`, {
    method: 'PUT',
    body: payload,
  });
}

export async function deleteResource(id) {
  return api(`/resources/${id}`, {
    method: 'DELETE',
  });
}

export async function optimizeResources(payload) {
  return api('/resources/optimize', {
    method: 'POST',
    body: payload,
  });
}

export async function allocateResource(payload) {
  return api('/resources/allocate', {
    method: 'POST',
    body: payload,
  });
}

export async function getAllocations() {
  return api('/resources/allocations');
}

export async function getResourceOverview() {
  return api('/resources/overview');
}

export async function getShelters() {
  return api('/shelters');
}
