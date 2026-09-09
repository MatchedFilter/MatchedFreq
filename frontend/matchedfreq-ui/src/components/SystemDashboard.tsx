import { useState } from 'react';
import type { HealthData } from '../types/simulation';

interface SystemDashboardProps {
  health: HealthData | null;
  healthError: string | null;
  wsStatus: 'Disconnected' | 'Connecting' | 'Connected';
  messages: string[];
  onConnectWs: () => void;
  onDisconnectWs: () => void;
  onSendMessage: (msg: string) => boolean;
}

export function SystemDashboard({
  health,
  healthError,
  wsStatus,
  messages,
  onConnectWs,
  onDisconnectWs,
  onSendMessage
}: SystemDashboardProps) {
  const [inputMsg, setInputMsg] = useState('');

  const handleSend = () => {
    if (inputMsg.trim()) {
      const sent = onSendMessage(inputMsg);
      if (sent) setInputMsg('');
    }
  };

  return (
    <div className="space-y-6">
      {/* REST API Health Card */}
      <section className="bg-slate-800 rounded-lg p-5 border border-slate-700">
        <h2 className="text-xl font-semibold mb-3 text-slate-200">Backend Status</h2>
        {healthError ? (
          <div className="text-red-400 text-sm">Offline: {healthError}</div>
        ) : health ? (
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4 text-sm">
            <div className="bg-slate-900 p-3 rounded">
              <span className="text-slate-500 block text-xs">Engine</span>
              <span className="font-mono text-cyan-300">{health.engine}</span>
            </div>
            <div className="bg-slate-900 p-3 rounded">
              <span className="text-slate-500 block text-xs">Status</span>
              <span className="text-emerald-400 font-medium">{health.status}</span>
            </div>
            <div className="bg-slate-900 p-3 rounded">
              <span className="text-slate-500 block text-xs">Version</span>
              <span className="font-mono">{health.version}</span>
            </div>
            <div className="bg-slate-900 p-3 rounded">
              <span className="text-slate-500 block text-xs">Asio</span>
              <span className="font-mono">{health.asio_version}</span>
            </div>
          </div>
        ) : (
          <div className="text-slate-400 text-sm">Connecting to server...</div>
        )}
      </section>

      {/* WebSocket Stream Monitor */}
      <section className="bg-slate-800 rounded-lg p-5 border border-slate-700 space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="text-xl font-semibold text-slate-200">Simulation Stream Log</h2>
          <div>
            {wsStatus === 'Disconnected' ? (
              <button
                onClick={onConnectWs}
                className="px-3 py-1.5 bg-cyan-600 hover:bg-cyan-500 font-medium text-xs rounded transition cursor-pointer"
              >
                Connect
              </button>
            ) : (
              <button
                onClick={onDisconnectWs}
                className="px-3 py-1.5 bg-rose-600 hover:bg-rose-500 font-medium text-xs rounded transition cursor-pointer"
              >
                Disconnect
              </button>
            )}
          </div>
        </div>

        <div className="h-48 bg-slate-950 rounded p-3 font-mono text-xs overflow-y-auto space-y-1">
          {messages.length === 0 ? (
            <span className="text-slate-600">No stream data received.</span>
          ) : (
            messages.map((msg, i) => <div key={i} className="text-slate-300">{msg}</div>)
          )}
        </div>

        <div className="flex gap-2">
          <input
            type="text"
            value={inputMsg}
            onChange={(e) => setInputMsg(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && handleSend()}
            disabled={wsStatus !== 'Connected'}
            placeholder="Type raw JSON or command..."
            className="flex-1 bg-slate-900 border border-slate-700 rounded px-3 py-2 text-sm focus:outline-none disabled:opacity-50"
          />
          <button
            onClick={handleSend}
            disabled={wsStatus !== 'Connected'}
            className="px-4 py-2 bg-slate-700 hover:bg-slate-600 text-sm rounded font-medium disabled:opacity-50 transition cursor-pointer"
          >
            Send
          </button>
        </div>
      </section>
    </div>
  );
}
