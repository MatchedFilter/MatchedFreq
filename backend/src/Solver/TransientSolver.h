#pragma once

#include "Netlist/Netlist.h"
#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

namespace MFreq::Solver
{

struct TimeStepData
{
  double time{0.0};
  std::vector<double> nodeVoltages; // Index corresponds to nodeIndexMap
};

struct SimulationResult
{
  std::vector<std::string>
      nodeNames; // Mapped node names in order (e.g. ["in", "out"])
  std::vector<TimeStepData> steps;
};

class TransientSolver
{
public:
  TransientSolver() = default;

  [[nodiscard]] static auto Solve(const Netlist::CircuitNetlist &netlist,
                                  const Netlist::TransientAnalysis &config)
      -> std::expected<SimulationResult, std::string>;
};

} // namespace MFreq::Solver
