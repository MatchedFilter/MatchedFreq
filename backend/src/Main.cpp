#include "Server.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace
{

auto InitLogging() -> void
{
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

  constexpr bool IS_DEBUG_BUILD =
#if defined(NDEBUG)
      false;
#else
      true;
#endif

  if constexpr (IS_DEBUG_BUILD)
  {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Logging initialized in DEBUG mode");
  }
  else
  {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Logging initialized in RELEASE mode");
  }

  spdlog::info("MatchedFreq Engine initializing...");
}

} // namespace

auto main() -> int
{
  constexpr uint16_t DEFAULT_PORT = 18080;

  InitLogging();

  auto server = std::make_unique<MFreq::SimulatorServer>(DEFAULT_PORT);
  server->Run();

  return 0;
}
