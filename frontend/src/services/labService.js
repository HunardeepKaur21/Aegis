import { api } from '../api.js';

export async function getLabAlgorithms() {
  return api('/lab/algorithms');
}

export async function runLabAlgorithm(payload) {
  return api('/lab/run', {
    method: 'POST',
    body: payload,
  });
}

export async function compareLabAlgorithms(payload) {
  return api('/lab/compare', {
    method: 'POST',
    body: payload,
  });
}
