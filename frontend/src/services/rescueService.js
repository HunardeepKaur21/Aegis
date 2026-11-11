import { api } from '../api.js';

export async function getRequests(params = {}) {
  const query = new URLSearchParams();
  if (params.sort) query.set('sort', params.sort);
  if (params.order) query.set('order', params.order);
  if (params.severity && params.severity !== 'All') query.set('severity', params.severity);
  if (params.status && params.status !== 'All') query.set('status', params.status);
  if (params.emergency_type && params.emergency_type !== 'All') query.set('emergency_type', params.emergency_type);
  if (params.search) query.set('search', params.search);

  const qs = query.toString();
  return api(`/requests${qs ? `?${qs}` : ''}`);
}

export async function getRequest(id) {
  return api(`/requests/${id}`);
}

export async function createRequest(payload) {
  return api('/requests', {
    method: 'POST',
    body: payload,
  });
}

export async function updateRequestStatus(id, status) {
  return api(`/requests/${id}/status`, {
    method: 'PATCH',
    body: { status },
  });
}

export async function dispatchTeam(requestId, teamId = null) {
  const body = { request_id: Number(requestId) };
  if (teamId) body.team_id = Number(teamId);
  return api('/requests/dispatch', {
    method: 'POST',
    body,
  });
}

export async function getTeams() {
  return api('/teams');
}

export async function getRescueOverview() {
  return api('/rescue/overview');
}

export async function optimizeAllocation(capacity = null) {
  const body = capacity ? { capacity: Number(capacity) } : {};
  return api('/rescue/optimize', {
    method: 'POST',
    body,
  });
}

export async function getRescueHeap() {
  return api('/rescue/heap');
}
