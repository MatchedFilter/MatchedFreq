#include "Api/SimulationWebSocket.h"
#include "Netlist/NetlistParser.h"
#include "Solver/TransientSolver.h"
#include <crow/json.h>

namespace MFreq::Api
{

auto SimulationWebSocketHandler::HandleOpen(crow::websocket::connection &conn)
    -> void
{
  spdlog::info("Client connected to simulation stream");
  conn.send_text(R"({"event":"connected","message":"MatchedFreq WS Ready"})");
}

auto SimulationWebSocketHandler::HandleClose(
    crow::websocket::connection & /*conn*/, const std::string &reason,
    uint16_t statusCode) -> void
{
  spdlog::info("Simulation stream closed: {} (code: {})", reason, statusCode);
}

auto SimulationWebSocketHandler::HandleMessage(
    crow::websocket::connection &conn, const std::string &data, bool isBinary)
    -> void
{
  if (isBinary)
  {
    return;
  }

  const auto json = crow::json::load(data);
  if (!json)
  {
    conn.send_text(R"({"status":"error","message":"Invalid JSON payload"})");
    return;
  }

  if (json.has("command") && json["command"].s() == "RUN_SIMULATION")
  {
    const std::string simType =
        json.has("type") ? std::string(json["type"].s()) : "tran";
    const std::string netlistText =
        json.has("netlist") ? std::string(json["netlist"].s()) : "";

    spdlog::info("Received simulation request (Type: {})", simType);

    // Inside your Crow websocket message handler:
    const auto parseResult = Netlist::NetlistParser::Parse(netlistText);
    if (!parseResult)
    {
      crow::json::wvalue errResponse;
      errResponse["event"] = "error";
      errResponse["message"] = "Netlist parsing failed";
      conn.send_text(errResponse.dump());
      return;
    }

    for (const auto &directive : parseResult->directives)
    {
      if (std::holds_alternative<Netlist::TransientAnalysis>(directive))
      {
        const auto &tranConfig =
            std::get<Netlist::TransientAnalysis>(directive);
        auto simResult =
            Solver::TransientSolver::Solve(*parseResult, tranConfig);

        if (simResult)
        {
          spdlog::info("Simulation finished with {} time steps",
                       simResult->steps.size());

          // Construct JSON payload
          crow::json::wvalue payload;
          payload["event"] = "simulation_data";
          payload["type"] = "tran";

          // Node headers
          for (uint32_t i = 0; i < simResult->nodeNames.size(); ++i)
          {
            payload["nodes"][i] = simResult->nodeNames[i];
          }

          // Time steps and voltages
          for (uint32_t i = 0; i < simResult->steps.size(); ++i)
          {
            payload["data"][i]["t"] = simResult->steps[i].time;
            for (uint32_t j = 0; j < simResult->steps[i].nodeVoltages.size();
                 ++j)
            {
              payload["data"][i]["v"][j] = simResult->steps[i].nodeVoltages[j];
            }
          }

          // Send payload to UI
          conn.send_text(payload.dump());

          // Send status signal
          crow::json::wvalue statusMsg;
          statusMsg["event"] = "status";
          statusMsg["state"] = "simulation_complete";
          conn.send_text(statusMsg.dump());
          return;
        }
      }
    }

    conn.send_text(
        R"({"event":"status","state":"netlist_parsed_successfully"})");
    return;
  }

  conn.send_text(R"({"event":"ack","received":")" + data + R"("})");
}

} // namespace MFreq::Api
