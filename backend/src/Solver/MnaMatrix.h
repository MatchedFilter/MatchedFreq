#pragma once

#include "Netlist/Netlist.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace MFreq::Solver
{

class MnaMatrixManager
{
public:
  MnaMatrixManager() = default;

  // Build node indexing mapping ground ("0" or "gnd") to index -1
  auto BuildNodeMap(const std::vector<Netlist::Component> &components) -> void;

  [[nodiscard]] auto GetNodeIndex(const std::string &nodeName) const -> int32_t;
  [[nodiscard]] auto GetMatrixSize() const noexcept -> size_t
  {
    return m_SystemSize;
  }
  [[nodiscard]] auto GetNodeCount() const noexcept -> size_t
  {
    return m_NodeCount;
  }

private:
  std::unordered_map<std::string, int32_t> m_NodeMap;
  size_t m_NodeCount{0};
  size_t m_VoltageSourceCount{0};
  size_t m_SystemSize{0};
};

} // namespace MFreq::Solver
