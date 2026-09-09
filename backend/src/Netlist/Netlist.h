#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace MFreq::Netlist
{

enum class ComponentType : std::uint8_t
{
  Resistor,
  Capacitor,
  Inductor,
  VoltageSource
};

struct Component
{
  std::string name;
  ComponentType type;
  std::string node1;
  std::string node2;
  double value{0.0};
  double acValue{0.0}; // Optional AC magnitude for frequency domain analysis
};

struct TransientAnalysis
{
  double step{0.0};
  double stopTime{0.0};
};

struct AcAnalysis
{
  std::string variation; // e.g., "dec", "oct", "lin"
  int numPoints{0};
  double startFreq{0.0};
  double stopFreq{0.0};
};

struct DcOpAnalysis
{
  // DC Operating point metadata
};

using AnalysisDirective =
    std::variant<TransientAnalysis, AcAnalysis, DcOpAnalysis>;

struct CircuitNetlist
{
  std::string title;
  std::vector<Component> components;
  std::vector<AnalysisDirective> directives;

  [[nodiscard]] auto IsEmpty() const noexcept -> bool
  {
    return components.empty();
  }
};

} // namespace MFreq::Netlist
