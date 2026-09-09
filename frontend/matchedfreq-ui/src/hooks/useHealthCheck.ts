import { useState, useEffect } from 'react';
import type { HealthData } from '../types/simulation';

export function useHealthCheck(endpoint: string = '/api/v1/health') {
  const [health, setHealth] = useState<HealthData | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    fetch(endpoint)
      .then((res) => {
        if (!res.ok) throw new Error(`HTTP Error: ${res.status}`);
        return res.json();
      })
      .then((data: HealthData) => setHealth(data))
      .catch((err: Error) => setError(err.message));
  }, [endpoint]);

  return { health, error };
}
