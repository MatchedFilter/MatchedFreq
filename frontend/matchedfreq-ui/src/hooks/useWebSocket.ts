import { useState, useRef, useCallback } from 'react';

export function useWebSocket() {
  const [wsStatus, setWsStatus] = useState<'Disconnected' | 'Connecting' | 'Connected'>('Disconnected');
  const [messages, setMessages] = useState<string[]>([]);
  const wsRef = useRef<WebSocket | null>(null);

  const connect = useCallback(() => {
    if (wsRef.current) return;

    setWsStatus('Connecting');
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const socket = new WebSocket(`${protocol}//${window.location.host}/ws/simulation`);

    socket.onopen = () => setWsStatus('Connected');
    socket.onmessage = (event) => setMessages((prev) => [...prev, `[Server]: ${event.data}`]);
    socket.onclose = () => { setWsStatus('Disconnected'); wsRef.current = null; };
    socket.onerror = () => { setWsStatus('Disconnected'); wsRef.current = null; };

    wsRef.current = socket;
  }, []);

  const disconnect = useCallback(() => {
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
  }, []);

  const sendMessage = useCallback((msg: string) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(msg);
      setMessages((prev) => [...prev, `[Sent]: ${msg}`]);
      return true;
    }
    return false;
  }, []);

  return { wsStatus, messages, connect, disconnect, sendMessage };
}
