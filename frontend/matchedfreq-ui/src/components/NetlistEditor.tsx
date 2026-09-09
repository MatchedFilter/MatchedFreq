import React, { useState } from 'react';
import { WaveformPlotter } from './WaveformPlotter';
import type { SimulationType } from '../types/simulation';

interface NetlistEditorProps {
  onRunSimulation: (netlist: string, type: SimulationType) => void;
  messages: (string | object)[];
}

const DEFAULT_NETLIST = `* RC Low Pass Filter
V1 in 0 DC 5 AC 1
R1 in out 1k
C1 out 0 10n
.tran 1u 100u
.end`;

export const NetlistEditor: React.FC<NetlistEditorProps> = ({ onRunSimulation, messages }) => {
  const [netlist, setNetlist] = useState<string>(DEFAULT_NETLIST);
  const [simType, setSimType] = useState<SimulationType>('tran');

  const handleTypeChange = (newType: SimulationType) => {
    setSimType(newType);

    // Automatically update or swap the SPICE control line in the netlist
    setNetlist((prevNetlist) => {
      // Strip out existing analysis control lines (.tran, .ac, .dc)
      const cleanNetlist = prevNetlist
        .split('\n')
        .filter((line) => !/^\s*\.(tran|ac|dc)\b/i.test(line))
        .join('\n')
        .replace(/^\s*\.end/im, ''); // Remove .end temporarily to place directive before it

      let directive = '.tran 1u 100u';
      if (newType === 'ac') {
        directive = '.ac dec 10 10 100k';
      } else if (newType === 'dc') {
        directive = '.dc V1 0 5 0.1';
      }

      return `${cleanNetlist.trim()}\n${directive}\n.end`;
    });
  };

  const handleRun = () => {
    onRunSimulation(netlist, simType);
  };

  return (
    <div className="space-y-6">
      {/* Top Section: Editor Header & Action Bar */}
      <div className="bg-slate-950 p-6 rounded-lg border border-slate-800 space-y-4">
        <div className="flex items-center justify-between border-b border-slate-800 pb-4">
          <div>
            <h2 className="text-xl font-bold text-cyan-400">SPICE Netlist & Workspace</h2>
            <p className="text-xs text-slate-400 mt-0.5">
              Edit circuit netlist and execute SPICE simulations
            </p>
          </div>
          <div className="flex items-center gap-3">
            <select
              value={simType}
              onChange={(e) => handleTypeChange(e.target.value as SimulationType)}
              className="bg-slate-900 border border-slate-700 text-xs text-slate-200 px-3 py-2 rounded-md focus:outline-none focus:border-cyan-500 font-mono"
            >
              <option value="tran">Transient (.tran)</option>
              <option value="ac">AC Analysis (.ac)</option>
              <option value="dc">DC Sweep (.dc)</option>
            </select>
            <button
              onClick={handleRun}
              className="px-4 py-2 bg-cyan-600 hover:bg-cyan-500 text-slate-950 font-semibold text-xs rounded-md transition shadow-md cursor-pointer flex items-center gap-1.5"
            >
              <span>▶</span> Run Simulation
            </button>
          </div>
        </div>

        {/* SPICE Netlist Code Area */}
        <div className="space-y-2">
          <label className="text-xs font-mono font-medium text-slate-400 uppercase tracking-wider">
            Netlist Input
          </label>
          <textarea
            value={netlist}
            onChange={(e) => setNetlist(e.target.value)}
            rows={9}
            className="w-full p-4 font-mono text-xs bg-slate-900 text-cyan-300 border border-slate-800 rounded-md focus:outline-none focus:border-cyan-500/80 resize-none shadow-inner"
            placeholder="Enter SPICE netlist here..."
          />
        </div>
      </div>

      {/* Bottom Section: Embedded Waveform Plotter */}
      <WaveformPlotter messages={messages} />
    </div>
  );
};

export default NetlistEditor;
