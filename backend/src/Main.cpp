#include "Version.h"
#include <asio.hpp>
#include <crow.h>
#include <crow/http_response.h>
#include <cstdint>
#include <string>

namespace MatchedFreq
{

class SimulatorServer
{
public:
  explicit SimulatorServer(uint16_t port) : m_Port(port)
  {
  }

  auto Run() const
  {
    crow::SimpleApp app;

    // Health check endpoint returning engine version and status
    CROW_ROUTE(app, "/api/v1/health")
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

    // WebSocket route for streaming circuit simulation time-step data
    CROW_ROUTE(app, "/ws/simulation")
        .websocket(&app)
        .onopen(
            [](crow::websocket::connection &conn) -> void
            {
              CROW_LOG_INFO << "Client connected to simulation stream";
              conn.send_text(
                  R"({"event":"connected","message":"MatchedFreq WS Ready"})");
            })
        .onclose(
            // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
            [](crow::websocket::connection &, const std::string &reason,
               uint16_t status_code) -> void
            {
              CROW_LOG_INFO << "Simulation stream closed: " << reason
                            << " (code: " << std::to_string(status_code) << ")";
            })
        .onmessage(
            [](crow::websocket::connection &conn, const std::string &data,
               bool isBinary) -> void
            {
              if (!isBinary)
              {
                conn.send_text(R"({"event":"ack","received":")" + data +
                               R"("})");
              }
            });

    app.port(m_Port).multithreaded().run();
  }

private:
  uint16_t m_Port;
  static inline uint32_t s_ActiveSessions = 0;
};

} // namespace MatchedFreq

auto main() -> int
{
  constexpr uint16_t DEFAULT_PORT = 18080;
  MatchedFreq::SimulatorServer server(DEFAULT_PORT);
  server.Run();

  return 0;
}
