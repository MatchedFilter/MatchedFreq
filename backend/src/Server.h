#pragma once

#include <crow.h>
#include <cstdint>

namespace MFreq
{

class SimulatorServer
{
public:
  explicit SimulatorServer(uint16_t port);
  auto Run() -> void;

private:
  uint16_t m_Port;
  crow::SimpleApp m_App;
};

} // namespace MFreq
