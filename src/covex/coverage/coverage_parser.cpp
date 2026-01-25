#include "covex/coverage/coverage_parser.hpp"

namespace binja::covex::coverage {

void CoverageParserRegistry::register_parser(
    std::unique_ptr<CoverageParser> parser) {
  if (!parser) {
    return;
  }
  parsers_.push_back(std::move(parser));
}

std::optional<CoverageTrace>
CoverageParserRegistry::parse_first_match(const std::string &path) const {
  for (const auto &parser : parsers_) {
    if (!parser) {
      continue;
    }
    if (!parser->can_parse(path)) {
      continue;
    }
    return parser->parse(path);
  }
  return std::nullopt;
}

} // namespace binja::covex::coverage
