import { useEffect, useState, useRef } from 'react';

interface HealthData {
  status: string;
  engine: string;
  version: string;
  asio_version: number;
}

export default function App() {
  const [health, setHealth] = useState<HealthData | null>(null);
  const [healthError, setHealthError] = useState<string | null>(null);

  const [wsStatus, setWsStatus] = useState<'Disconnected' | 'Connecting' | 'Connected'>('Disconnected');
  const [messages, setMessages] = useState<string[]>([]);
  const [inputMsg, setInputMsg] = useState('');

  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    fetch('/api/v1/health')
      .then((res) => {
        if (!res.ok) throw new Error(`HTTP Error: ${res.status}`);
        return res.json();
      })
      .then((data: HealthData) => setHealth(data))
      .catch((err: Error) => setHealthError(err.message));
  }, []);

  const connectWebSocket = () => {
    if (wsRef.current) return;

    setWsStatus('Connecting');
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const socket = new WebSocket(`${protocol}//${window.location.host}/ws/simulation`);

    socket.onopen = () => {
      setWsStatus('Connected');
    };

    socket.onmessage = (event) => {
      setMessages((prev) => [...prev, `Received: ${event.data}`]);
    };

    socket.onclose = () => {
      setWsStatus('Disconnected');
      wsRef.current = null;
    };

    socket.onerror = () => {
      setWsStatus('Disconnected');
      wsRef.current = null;
    };

    wsRef.current = socket;
  };

  const disconnectWebSocket = () => {
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
  };

  const sendMessage = () => {
    if (wsRef.current && wsStatus === 'Connected' && inputMsg.trim()) {
      wsRef.current.send(inputMsg);
      setMessages((prev) => [...prev, `Sent: ${inputMsg}`]);
      setInputMsg('');
    }
  };

  return (
    <div className="min-h-screen p-8 max-w-4xl mx-auto space-y-6">
      <header className="border-b border-slate-700 pb-4">
        <h1 className="text-3xl font-bold tracking-tight text-cyan-400">MatchedFreq Dashboard</h1>
        <p className="text-slate-400 text-sm mt-1">
          Circuit Simulation Control Panel
        </p>
      </header>

      {/* REST API Panel */}
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

      {/* WebSocket Stream Panel */}
      <section className="bg-slate-800 rounded-lg p-5 border border-slate-700 space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="text-xl font-semibold text-slate-200">Simulation Stream</h2>
          <span className="text-xs px-2.5 py-1 rounded-full bg-slate-900 text-slate-300">
            {wsStatus}
          </span>
        </div>

        <div>
          {wsStatus === 'Disconnected' ? (
            <button
              onClick={connectWebSocket}
              className="px-4 py-2 bg-cyan-600 hover:bg-cyan-500 font-medium text-sm rounded transition cursor-pointer"
            >
              Connect WebSocket
            </button>
          ) : (
            <button
              onClick={disconnectWebSocket}
              className="px-4 py-2 bg-rose-600 hover:bg-rose-500 font-medium text-sm rounded transition cursor-pointer"
            >
              Disconnect
            </button>
          )}
        </div>

        {/* Console Log */}
        <div className="h-40 bg-slate-950 rounded p-3 font-mono text-xs overflow-y-auto space-y-1">
          {messages.length === 0 ? (
            <span className="text-slate-600">No stream data received.</span>
          ) : (
            messages.map((msg, i) => <div key={i} className="text-slate-300">{msg}</div>)
          )}
        </div>

        {/* Input Controls */}
        <div className="flex gap-2">
          <input
            type="text"
            value={inputMsg}
            onChange={(e) => setInputMsg(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && sendMessage()}
            disabled={wsStatus !== 'Connected'}
            placeholder="Type message to C++ backend..."
            className="flex-1 bg-slate-900 border border-slate-700 rounded px-3 py-2 text-sm focus:outline-none disabled:opacity-50"
          />
          <button
            onClick={sendMessage}
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
