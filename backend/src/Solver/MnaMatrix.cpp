#include "Solver/MnaMatrix.h"
#include <algorithm>
#include <ranges>

namespace MFreq::Solver
{

auto MnaMatrixManager::BuildNodeMap(
    const std::vector<Netlist::Component> &components) -> void
{
  m_NodeMap.clear();
  m_NodeCount = 0;
  m_VoltageSourceCount = 0;

  // Ground is mapped to -1
  m_NodeMap["0"] = -1;
  m_NodeMap["gnd"] = -1;

  for (const auto &comp : components)
  {
    for (const auto &node : {comp.node1, comp.node2})
    {
      if (!m_NodeMap.contains(node))
      {
        m_NodeMap[node] = static_cast<int32_t>(m_NodeCount++);
      }
    }

    if (comp.type == Netlist::ComponentType::VoltageSource)
    {
      ++m_VoltageSourceCount;
    }
  }

  // System size N = (Number of non-ground nodes) + (Number of voltage source
  // branch currents)
  m_SystemSize = m_NodeCount + m_VoltageSourceCount;
}

auto MnaMatrixManager::GetNodeIndex(const std::string &nodeName) const
    -> int32_t
{
  if (auto it = m_NodeMap.find(nodeName); it != m_NodeMap.end())
  {
    return it->second;
  }
  return -1;
}

} // namespace MFreq::Solver
