#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "covex/coverage/coverage_types.hpp"

namespace binja::covex::coverage {

class CoverageParser {
public:
  virtual ~CoverageParser() = default;
  virtual bool can_parse(const std::string &path) const = 0;
  virtual CoverageTrace parse(const std::string &path) const = 0;
};

class CoverageParserRegistry {
public:
  void register_parser(std::unique_ptr<CoverageParser> parser);
  std::optional<CoverageTrace> parse_first_match(const std::string &path) const;

private:
  std::vector<std::unique_ptr<CoverageParser>> parsers_;
};

} // namespace binja::covex::coverage
