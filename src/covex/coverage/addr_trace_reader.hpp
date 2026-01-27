#pragma once

#include <string>

#include "covex/coverage/coverage_parser.hpp"

namespace binja::covex::coverage {

class AddrTraceReader {
public:
  static CoverageTrace read(const std::string &path);
};

class AddrTraceParser final : public CoverageParser {
public:
  bool can_parse(const std::string &path) const override;
  CoverageTrace parse(const std::string &path) const override;
};

} // namespace binja::covex::coverage
