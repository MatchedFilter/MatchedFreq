#pragma once

#include "Netlist/Netlist.h"
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace MFreq::Netlist
{

enum class ParseErrorCode : std::uint8_t
{
  EmptyNetlist,
  InvalidSyntax,
  UnknownComponent,
  InvalidNumericValue,
  MissingEndCard
};

struct ParseError
{
  ParseErrorCode code;
  size_t lineNumber{0};
  std::string message;
};

class NetlistParser
{
public:
  NetlistParser() = default;

  // Parses raw text netlist into CircuitNetlist using std::expected for clean
  // error handling
  [[nodiscard]] static auto Parse(std::string_view netlistText)
      -> std::expected<CircuitNetlist, ParseError>;

private:
  [[nodiscard]] static auto ParseLine(std::string_view line, size_t lineNum,
                                      CircuitNetlist &netlist)
      -> std::expected<void, ParseError>;

  [[nodiscard]] static auto ParseComponent(std::string_view line,
                                           size_t lineNum)
      -> std::expected<Component, ParseError>;

  [[nodiscard]] static auto ParseDirective(std::string_view line,
                                           size_t lineNum)
      -> std::expected<AnalysisDirective, ParseError>;

  [[nodiscard]] static auto ParseValue(std::string_view token)
      -> std::expected<double, bool>;
};

} // namespace MFreq::Netlist
