export interface HealthData {
  status: string;
  engine: string;
  version: string;
  asio_version: number;
}

export type SimulationType = 'tran' | 'ac' | 'op';
