#pragma once

#include <crow/websocket.h>
#include <spdlog/spdlog.h>
#include <string>

namespace MFreq::Api
{

class SimulationWebSocketHandler
{
public:
  static auto HandleOpen(crow::websocket::connection &conn) -> void;
  static auto HandleClose(crow::websocket::connection &conn,
                          const std::string &reason, uint16_t statusCode)
      -> void;
  static auto HandleMessage(crow::websocket::connection &conn,
                            const std::string &data, bool isBinary) -> void;
};

} // namespace MFreq::Api
