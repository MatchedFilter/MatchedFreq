#include "Version.h"
#include "spdlog/spdlog.h"
#include <asio.hpp>
#include <crow.h>
#include <crow/http_response.h>
#include <cstdint>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>

namespace
{
auto InitLogging() -> void;

auto InitLogging() -> void
{
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

  // Determine build mode at compile time
  constexpr bool IS_DEBUG_BUILD =
#if defined(NDEBUG)
      false;
#else
      true;
#endif

  if constexpr (IS_DEBUG_BUILD)
  {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Logging initialized in DEBUG mode (Level: DEBUG)");
  }
  else
  {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Logging initialized in RELEASE mode (Level: INFO)");
  }

  spdlog::info("MatchedFreq Engine initializing...");
}

}; // namespace

namespace MFreq
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
              spdlog::info("Client connected to simulation stream");
              conn.send_text(
                  R"({"event":"connected","message":"MatchedFreq WS Ready"})");
            })
        .onclose(
            // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
            [](crow::websocket::connection &, const std::string &reason,
               uint16_t status_code) -> void
            {
              spdlog::info("Simulation stream closed: {} (code: {})", reason,
                           std::to_string(status_code));
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

} // namespace MFreq

auto main() -> int
{
  constexpr uint16_t DEFAULT_PORT = 18080;
  InitLogging();
  auto server = std::make_unique<MFreq::SimulatorServer>(DEFAULT_PORT);
  server->Run();

  return 0;
}
