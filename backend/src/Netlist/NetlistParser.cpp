#include "Netlist/NetlistParser.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <sstream>

namespace MFreq::Netlist
{

namespace
{

// Utility to trim trailing \r, \n, and spaces
auto Trim(std::string_view str) -> std::string_view
{
  const size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string_view::npos)
  {
    return "";
  }
  const size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

// Utility to normalize tokens to lower case
auto ToLower(std::string_view str) -> std::string
{
  std::string result(str);
  std::ranges::transform(result, result.begin(), [](unsigned char c) -> char
                         { return static_cast<char>(std::tolower(c)); });
  return result;
}

} // namespace

auto NetlistParser::ParseValue(std::string_view token)
    -> std::expected<double, bool>
{
  const std::string_view trimmed = Trim(token);
  if (trimmed.empty())
  {
    return std::unexpected(false);
  }

  const std::string lowerToken = ToLower(trimmed);
  double multiplier = 1.0;
  size_t valEnd = lowerToken.length();

  if (lowerToken.ends_with("meg"))
  {
    constexpr double MEGA_CONST = 1e6;
    multiplier = MEGA_CONST;
    valEnd -= 3;
  }
  else if (lowerToken.back() == 'k')
  {
    constexpr double KILO_CONST = 1e3;
    multiplier = KILO_CONST;
    valEnd -= 1;
  }
  else if (lowerToken.back() == 'm')
  {
    constexpr double MILLI_CONST = 1e-3;
    multiplier = MILLI_CONST;
    valEnd -= 1;
  }
  else if (lowerToken.back() == 'u')
  {
    constexpr double MICRO_CONST = 1e-6;
    multiplier = MICRO_CONST;
    valEnd -= 1;
  }
  else if (lowerToken.back() == 'n')
  {
    constexpr double NANO_CONST = 1e-9;
    multiplier = NANO_CONST;
    valEnd -= 1;
  }
  else if (lowerToken.back() == 'p')
  {
    constexpr double PICO_CONST = 1e-12;
    multiplier = PICO_CONST;
    valEnd -= 1;
  }

  double val = 0.0;
  const auto [ptr, ec] =
      std::from_chars(lowerToken.data(), lowerToken.data() + valEnd, val);

  if (ec == std::errc{})
  {
    return val * multiplier;
  }

  return std::unexpected(false);
}

auto NetlistParser::ParseComponent(std::string_view line, size_t lineNum)
    -> std::expected<Component, ParseError>
{
  std::stringstream ss{std::string(line)};
  std::string name;
  std::string node1;
  std::string node2;
  std::string valToken;

  if (!(ss >> name >> node1 >> node2 >> valToken))
  {
    return std::unexpected(ParseError{
        .code = ParseErrorCode::InvalidSyntax,
        .lineNumber = lineNum,
        .message = std::format("Invalid component line syntax: '{}'", line)});
  }

  const char typeChar = static_cast<char>(std::toupper(name.front()));
  ComponentType type{};

  switch (typeChar)
  {
  case 'R':
    type = ComponentType::Resistor;
    break;
  case 'C':
    type = ComponentType::Capacitor;
    break;
  case 'L':
    type = ComponentType::Inductor;
    break;
  case 'V':
    type = ComponentType::VoltageSource;
    break;
  default:
    return std::unexpected(ParseError{
        .code = ParseErrorCode::UnknownComponent,
        .lineNumber = lineNum,
        .message = std::format("Unsupported component prefix '{}'", typeChar)});
  }

  double primaryValue = 0.0;
  double acVal = 0.0;

  // Handle explicit key specifiers like "V1 in 0 DC 5 AC 1" or raw "V1 in 0 5"
  if (ToLower(valToken) == "dc")
  {
    std::string dcValStr;
    if (ss >> dcValStr)
    {
      if (auto parsedDc = ParseValue(dcValStr))
      {
        primaryValue = *parsedDc;
      }
    }
  }
  else
  {
    if (auto parsedVal = ParseValue(valToken))
    {
      primaryValue = *parsedVal;
    }
    else
    {
      return std::unexpected(ParseError{
          .code = ParseErrorCode::InvalidNumericValue,
          .lineNumber = lineNum,
          .message = std::format("Invalid numeric value '{}'", valToken)});
    }
  }

  // Parse remaining optional parameters on the line (e.g. AC <val>)
  std::string paramKey;
  while (ss >> paramKey)
  {
    if (ToLower(paramKey) == "ac")
    {
      std::string acValStr;
      if (ss >> acValStr)
      {
        if (auto parsedAc = ParseValue(acValStr))
        {
          acVal = *parsedAc;
        }
      }
    }
  }

  return Component{.name = name,
                   .type = type,
                   .node1 = node1,
                   .node2 = node2,
                   .value = primaryValue,
                   .acValue = acVal};
}

auto NetlistParser::ParseDirective(std::string_view line, size_t lineNum)
    -> std::expected<AnalysisDirective, ParseError>
{
  std::stringstream ss{std::string(line)};
  std::string directive;
  ss >> directive;

  const std::string lowerDir = ToLower(directive);

  if (lowerDir == ".tran")
  {
    std::string stepStr;
    std::string stopStr;
    if (ss >> stepStr >> stopStr)
    {
      auto stepVal = ParseValue(stepStr);
      auto stopVal = ParseValue(stopStr);
      if (stepVal && stopVal)
      {
        return TransientAnalysis{.step = *stepVal, .stopTime = *stopVal};
      }
    }
  }
  else if (lowerDir == ".op")
  {
    return DcOpAnalysis{};
  }

  return std::unexpected(ParseError{
      .code = ParseErrorCode::InvalidSyntax,
      .lineNumber = lineNum,
      .message = std::format("Unsupported directive line: '{}'", line)});
}

auto NetlistParser::ParseLine(std::string_view rawLine, size_t lineNum,
                              CircuitNetlist &netlist)
    -> std::expected<void, ParseError>
{
  const std::string_view line = Trim(rawLine);
  if (line.empty())
  {
    return {}; // Blank line
  }

  // Skip comments (* or ;)
  if (line.starts_with('*') || line.starts_with(';'))
  {
    if (lineNum == 1 && netlist.title.empty())
    {
      netlist.title = std::string(line.substr(1));
    }
    return {};
  }

  if (line.starts_with('.'))
  {
    if (ToLower(line).starts_with(".end"))
    {
      return {};
    }

    auto directive = ParseDirective(line, lineNum);
    if (!directive)
    {
      return std::unexpected(directive.error());
    }
    netlist.directives.push_back(*directive);
    return {};
  }

  auto comp = ParseComponent(line, lineNum);
  if (!comp)
  {
    return std::unexpected(comp.error());
  }

  netlist.components.push_back(*comp);
  return {};
}

auto NetlistParser::Parse(std::string_view netlistText)
    -> std::expected<CircuitNetlist, ParseError>
{
  if (netlistText.empty())
  {
    return std::unexpected(ParseError{.code = ParseErrorCode::EmptyNetlist,
                                      .lineNumber = 0,
                                      .message = "Netlist text is empty"});
  }

  CircuitNetlist netlist;
  std::stringstream ss{std::string(netlistText)};
  std::string line;
  size_t lineNumber = 0;

  while (std::getline(ss, line))
  {
    ++lineNumber;
    auto result = ParseLine(line, lineNumber, netlist);
    if (!result)
    {
      return std::unexpected(result.error());
    }
  }

  return netlist;
}

} // namespace MFreq::Netlist
