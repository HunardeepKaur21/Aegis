// Single place where the frontend talks to the C++ backend.
export class ApiError extends Error {}

export async function api(path, options = {}) {
  let res;
  const baseUrl = import.meta.env.VITE_API_URL || '';
  try {
    res = await fetch(`${baseUrl}/api${path}`, {
      headers: { 'Content-Type': 'application/json' },
      ...options,
      body: options.body ? JSON.stringify(options.body) : undefined,
    });
  } catch {
    throw new ApiError('Cannot reach the AEGIS backend. Start it with ./aegis_server on port 8080.');
  }
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw new ApiError(data.error || `Request failed (${res.status})`);
  return data;
}
