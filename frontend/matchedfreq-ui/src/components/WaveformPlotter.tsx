import React, { useMemo, useState } from 'react';
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from 'recharts';

interface SimulationStep {
  t: number;
  v: number[];
}

interface SimulationDataMessage {
  event: 'simulation_data';
  type: string;
  nodes: string[];
  data: SimulationStep[];
}

interface WaveformPlotterProps {
  messages: (string | object)[];
}

const NODE_COLORS = ['#38bdf8', '#34d399', '#fbbf24', '#f87171', '#c084fc', '#f472b6'];

const formatTime = (seconds: number): string => {
  if (seconds === undefined || seconds === null) return '0 s';
  const absVal = Math.abs(seconds);
  if (absVal === 0) return '0 s';
  if (absVal < 1e-6) return `${(seconds * 1e9).toFixed(1)} ns`;
  if (absVal < 1e-3) return `${(seconds * 1e6).toFixed(1)} µs`;
  if (absVal < 1) return `${(seconds * 1e3).toFixed(1)} ms`;
  return `${seconds.toFixed(2)} s`;
};

const formatVoltage = (volts: number): string => {
  if (volts === undefined || volts === null) return '0 V';
  const absVal = Math.abs(volts);
  if (absVal < 1e-3) return `${(volts * 1e6).toFixed(1)} µV`;
  if (absVal < 1) return `${(volts * 1e3).toFixed(1)} mV`;
  return `${volts.toFixed(2)} V`;
};

export const WaveformPlotter: React.FC<WaveformPlotterProps> = ({ messages }) => {
  const [hiddenNodes, setHiddenNodes] = useState<Set<string>>(new Set());

  // Extract the latest simulation result from raw strings OR objects in messages
  const latestData = useMemo(() => {
    for (let i = messages.length - 1; i >= 0; i--) {
      const msg = messages[i];
      let parsed: any = null;

      if (typeof msg === 'string') {
        try {
          // Clean potential server message prefixes (e.g. "[Server]: ")
          const cleanStr = msg.replace(/^\[Server\]:\s*/, '').trim();
          parsed = JSON.parse(cleanStr);
        } catch {
          continue;
        }
      } else if (typeof msg === 'object' && msg !== null) {
        parsed = msg;
      }

      if (parsed && parsed.event === 'simulation_data' && Array.isArray(parsed.data)) {
        return parsed as SimulationDataMessage;
      }
    }
    return null;
  }, [messages]);

  const nodes = latestData?.nodes || [];
  const rawSteps = latestData?.data || [];

  const chartData = useMemo(() => {
    return rawSteps.map((step) => {
      const point: Record<string, number> = { t: step.t };
      nodes.forEach((nodeName, index) => {
        point[nodeName] = step.v[index] ?? 0;
      });
      return point;
    });
  }, [rawSteps, nodes]);

  const toggleNodeVisibility = (nodeName: string) => {
    setHiddenNodes((prev) => {
      const next = new Set(prev);
      if (next.has(nodeName)) next.delete(nodeName);
      else next.add(nodeName);
      return next;
    });
  };

  if (!latestData || rawSteps.length === 0) {
    return (
      <div className="p-6 bg-slate-950 border border-slate-800 rounded-lg text-center text-slate-500 text-sm mt-6">
        No waveform data available yet. Run a simulation to view time-series plots.
      </div>
    );
  }

  return (
    <div className="flex flex-col gap-4 bg-slate-950 p-5 rounded-lg border border-slate-800 mt-6 font-sans">
      <div className="flex items-center justify-between">
        <span className="text-xs font-semibold uppercase tracking-wider text-cyan-400">
          Transient Voltage Waveforms (V vs t)
        </span>
        <div className="flex items-center gap-2">
          {nodes.map((node, idx) => (
            <button
              key={node}
              onClick={() => toggleNodeVisibility(node)}
              className={`text-xs px-2.5 py-1 rounded-md font-mono border transition-all cursor-pointer ${hiddenNodes.has(node)
                  ? 'bg-slate-900 border-slate-800 text-slate-500 line-through'
                  : 'border-slate-700 text-slate-200'
                }`}
              style={{
                borderColor: hiddenNodes.has(node) ? undefined : NODE_COLORS[idx % NODE_COLORS.length],
              }}
            >
              v({node})
            </button>
          ))}
        </div>
      </div>

      <div className="h-80 w-full">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={chartData} margin={{ top: 10, right: 30, left: 10, bottom: 20 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="#334155" opacity={0.5} />
            <XAxis
              dataKey="t"
              stroke="#94a3b8"
              tickFormatter={formatTime}
              fontSize={11}
              tickLine={false}
              label={{ value: 'Time', position: 'insideBottom', offset: -10, fill: '#64748b', fontSize: 12 }}
            />
            <YAxis
              stroke="#94a3b8"
              tickFormatter={formatVoltage}
              fontSize={11}
              tickLine={false}
              label={{ value: 'Voltage', angle: -90, position: 'insideLeft', fill: '#64748b', fontSize: 12 }}
            />
            <Tooltip
              contentStyle={{ backgroundColor: '#0f172a', borderColor: '#334155', borderRadius: '0.5rem' }}
              labelStyle={{ color: '#94a3b8', fontSize: '12px' }}
              labelFormatter={(label: React.ReactNode) => `Time: ${formatTime(Number(label))}`}
              formatter={(value: any, name: any) => [formatVoltage(Number(value)), `v(${name})`]}
            />
            <Legend verticalAlign="top" height={36} wrapperStyle={{ fontSize: '12px' }} />

            {nodes.map((nodeName, idx) => (
              <Line
                key={nodeName}
                type="monotone"
                dataKey={nodeName}
                name={`v(${nodeName})`}
                stroke={NODE_COLORS[idx % NODE_COLORS.length]}
                strokeWidth={2}
                dot={false}
                activeDot={{ r: 4 }}
                hide={hiddenNodes.has(nodeName)}
              />
            ))}
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
};

export default WaveformPlotter;
