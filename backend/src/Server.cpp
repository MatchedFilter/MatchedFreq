#include "Server.h"
#include "Api/SimulationWebSocket.h"
#include "Version.h"
#include <asio.hpp>

namespace MFreq
{

SimulatorServer::SimulatorServer(uint16_t port) : m_Port(port)
{
  // REST Health check endpoint
  CROW_ROUTE(m_App, "/api/v1/health")
  (
      []() -> crow::response
      {
        crow::json::wvalue response;
        response["status"] = "active";
        response["engine"] = "MatchedFreq";
        response["version"] = MATCHEDFREQ_VERSION.data();
        response["asio_version"] = ASIO_VERSION;
        return crow::response{crow::status::OK, response};
      });

  // WebSocket Route targeting Api namespace handler
  CROW_ROUTE(m_App, "/ws/simulation")
      .websocket(&m_App)
      .onopen(Api::SimulationWebSocketHandler::HandleOpen)
      .onclose(Api::SimulationWebSocketHandler::HandleClose)
      .onmessage(Api::SimulationWebSocketHandler::HandleMessage);
}

auto SimulatorServer::Run() -> void
{
  m_App.port(m_Port).multithreaded().run();
}

} // namespace MFreq
