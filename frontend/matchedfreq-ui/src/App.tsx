import { useState } from 'react';
import { useHealthCheck } from './hooks/useHealthCheck';
import { useWebSocket } from './hooks/useWebSocket';
import { NetlistEditor } from './components/NetlistEditor';
import { SystemDashboard } from './components/SystemDashboard';
import type { SimulationType } from './types/simulation';

export default function App() {
  const [activeTab, setActiveTab] = useState<'dashboard' | 'schematics'>('dashboard');

  const { health, error: healthError } = useHealthCheck('/api/v1/health');
  const { wsStatus, messages, connect, disconnect, sendMessage } = useWebSocket();

  const handleRunSimulation = (netlist: string, type: SimulationType) => {
    const payload = JSON.stringify({
      command: 'RUN_SIMULATION',
      type,
      netlist
    });

    if (wsStatus === 'Connected') {
      sendMessage(payload);
    } else {
      alert('WebSocket is disconnected. Connecting now...');
      connect();
    }
  };

  return (
    <div className="min-h-screen bg-slate-900 text-slate-100 flex">
      {/* Left Vertical Navigation Sidebar */}
      <aside className="w-64 bg-slate-950 border-r border-slate-800 p-6 flex flex-col justify-between shrink-0">
        <div className="space-y-6">
          <div>
            <h1 className="text-2xl font-bold tracking-tight text-cyan-400">MatchedFreq</h1>
            <p className="text-slate-400 text-xs mt-1">Analog Circuit Simulator</p>
          </div>

          <nav className="space-y-1">
            <button
              onClick={() => setActiveTab('dashboard')}
              className={`w-full text-left px-3 py-2.5 rounded-md font-medium text-sm transition cursor-pointer flex items-center justify-between ${activeTab === 'dashboard'
                  ? 'bg-slate-800 text-cyan-400 border-l-2 border-cyan-400'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
                }`}
            >
              System Dashboard
            </button>
            <button
              onClick={() => setActiveTab('schematics')}
              className={`w-full text-left px-3 py-2.5 rounded-md font-medium text-sm transition cursor-pointer flex items-center justify-between ${activeTab === 'schematics'
                  ? 'bg-slate-800 text-cyan-400 border-l-2 border-cyan-400'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
                }`}
            >
              Schematics & Netlist
            </button>
          </nav>
        </div>

        <div className="pt-4 border-t border-slate-800 space-y-2">
          <div className="flex items-center justify-between">
            <span className="text-xs text-slate-400">WebSocket:</span>
            <span
              className={`text-xs px-2.5 py-0.5 rounded-full font-medium ${wsStatus === 'Connected'
                  ? 'bg-emerald-950 text-emerald-400 border border-emerald-800'
                  : wsStatus === 'Connecting'
                    ? 'bg-amber-950 text-amber-400 border border-amber-800'
                    : 'bg-slate-800 text-slate-400 border border-slate-700'
                }`}
            >
              {wsStatus}
            </span>
          </div>
          {wsStatus === 'Disconnected' && (
            <button
              onClick={connect}
              className="w-full py-1.5 bg-cyan-600 hover:bg-cyan-500 font-medium text-xs rounded transition cursor-pointer"
            >
              Connect WS
            </button>
          )}
        </div>
      </aside>

      {/* Main Content Area */}
      <main className="flex-1 p-8 max-w-6xl mx-auto overflow-y-auto">
        {activeTab === 'dashboard' ? (
          <SystemDashboard
            health={health}
            healthError={healthError}
            wsStatus={wsStatus}
            messages={messages}
            onConnectWs={connect}
            onDisconnectWs={disconnect}
            onSendMessage={sendMessage}
          />
        ) : (
          <NetlistEditor onRunSimulation={handleRunSimulation} messages={messages} />
        )}
      </main>
    </div>
  );
}
