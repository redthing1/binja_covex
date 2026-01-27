#include "covex/coverage/addr_trace_reader.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace binja::covex::coverage {

namespace {

constexpr size_t kHeaderProbeSize = 16;
constexpr size_t kSampleLineLimit = 32;

std::string_view trim_left(std::string_view text) {
  size_t pos = 0;
  while (pos < text.size() &&
         std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
    ++pos;
  }
  return text.substr(pos);
}

std::string_view trim_right(std::string_view text) {
  size_t pos = text.size();
  while (pos > 0 &&
         std::isspace(static_cast<unsigned char>(text[pos - 1])) != 0) {
    --pos;
  }
  return text.substr(0, pos);
}

std::string_view trim(std::string_view text) {
  return trim_right(trim_left(text));
}

bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

std::string_view strip_comment(std::string_view line) {
  const std::string_view trimmed = trim_left(line);
  if (starts_with(trimmed, "#") || starts_with(trimmed, ";") ||
      starts_with(trimmed, "//")) {
    return std::string_view{};
  }

  const auto hash = line.find('#');
  const auto semi = line.find(';');
  const auto slash = line.find("//");
  size_t cut = std::string_view::npos;
  if (hash != std::string_view::npos) {
    cut = hash;
  }
  if (semi != std::string_view::npos) {
    cut = std::min(cut, semi);
  }
  if (slash != std::string_view::npos) {
    cut = std::min(cut, slash);
  }
  if (cut != std::string_view::npos) {
    return trim_right(line.substr(0, cut));
  }
  return trim_right(line);
}

bool parse_number(std::string_view text, uint64_t &out) {
  std::string value(text);
  if (value.empty()) {
    return false;
  }
  if (value.size() > 2 && value[0] == '0' &&
      (value[1] == 'x' || value[1] == 'X')) {
    value.erase(0, 2);
  }
  if (value.empty()) {
    return false;
  }
  for (const auto ch : value) {
    if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
        (ch >= 'A' && ch <= 'F')) {
      continue;
    }
    return false;
  }
  char *end = nullptr;
  errno = 0;
  out = std::strtoull(value.c_str(), &end, 16);
  if (errno != 0) {
    return false;
  }
  return end != nullptr && *end == '\0';
}

bool parse_addr_hit_tokens(const std::vector<std::string_view> &tokens,
                           uint64_t &addr, uint64_t &hits, bool &explicit_hit) {
  if (tokens.empty() || tokens.size() > 2) {
    return false;
  }
  if (!parse_number(tokens[0], addr)) {
    return false;
  }
  if (tokens.size() == 1) {
    hits = 1;
    explicit_hit = false;
    return true;
  }
  if (!parse_number(tokens[1], hits)) {
    return false;
  }
  explicit_hit = true;
  return true;
}

bool parse_addr_hit_line(std::string_view line, uint64_t &addr, uint64_t &hits,
                         bool &explicit_hit) {
  auto cleaned = strip_comment(line);
  cleaned = trim(cleaned);
  if (cleaned.empty()) {
    return false;
  }

  std::vector<std::string_view> tokens;
  std::string buffer(cleaned);
  for (auto &ch : buffer) {
    if (ch == ',' || ch == ':') {
      ch = ' ';
    }
  }
  size_t start = 0;
  while (start < buffer.size()) {
    while (start < buffer.size() &&
           std::isspace(static_cast<unsigned char>(buffer[start])) != 0) {
      ++start;
    }
    if (start >= buffer.size()) {
      break;
    }
    size_t end = start;
    while (end < buffer.size() &&
           std::isspace(static_cast<unsigned char>(buffer[end])) == 0) {
      ++end;
    }
    tokens.emplace_back(buffer.data() + start, end - start);
    start = end;
  }

  return parse_addr_hit_tokens(tokens, addr, hits, explicit_hit);
}

bool has_drcov_header(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }
  char header[kHeaderProbeSize] = {};
  file.read(header, sizeof(header));
  if (!file) {
    return false;
  }
  return std::string_view(header, 5) == "DRCOV";
}

bool sample_is_addr_trace(const std::string &path) {
  std::ifstream file(path);
  if (!file) {
    return false;
  }
  std::string line;
  size_t lines_checked = 0;
  size_t valid = 0;
  while (std::getline(file, line) && lines_checked < kSampleLineLimit) {
    ++lines_checked;
    uint64_t addr = 0;
    uint64_t hits = 0;
    bool explicit_hit = false;
    auto cleaned = trim(strip_comment(line));
    if (cleaned.empty()) {
      continue;
    }
    if (!parse_addr_hit_line(cleaned, addr, hits, explicit_hit)) {
      return false;
    }
    ++valid;
  }
  return valid > 0;
}

CoverageTrace build_trace(const std::string &path,
                          const std::unordered_map<uint64_t, uint64_t> &hits,
                          bool has_explicit_hitcounts) {
  CoverageTrace trace;
  trace.format = has_explicit_hitcounts ? TraceFormat::AddrHitTrace
                                        : TraceFormat::AddrTrace;
  trace.source_path = path;
  trace.name = std::filesystem::path(path).filename().string();
  trace.has_hitcounts = has_explicit_hitcounts;

  trace.spans.reserve(hits.size());
  for (const auto &[addr, count] : hits) {
    CoverageSpan span;
    span.address = addr;
    span.size = 1;
    span.hits = count;
    trace.spans.push_back(span);
  }

  return trace;
}

} // namespace

CoverageTrace AddrTraceReader::read(const std::string &path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open address trace file");
  }

  std::unordered_map<uint64_t, uint64_t> hits;
  bool has_explicit_hitcounts = false;
  std::string line;
  while (std::getline(file, line)) {
    uint64_t addr = 0;
    uint64_t count = 0;
    bool explicit_hit = false;
    auto cleaned = trim(strip_comment(line));
    if (cleaned.empty()) {
      continue;
    }
    if (!parse_addr_hit_line(cleaned, addr, count, explicit_hit)) {
      throw std::runtime_error("Invalid address trace line: " + line);
    }
    has_explicit_hitcounts = has_explicit_hitcounts || explicit_hit;
    hits[addr] += count;
  }

  return build_trace(path, hits, has_explicit_hitcounts);
}

bool AddrTraceParser::can_parse(const std::string &path) const {
  if (has_drcov_header(path)) {
    return false;
  }
  return sample_is_addr_trace(path);
}

CoverageTrace AddrTraceParser::parse(const std::string &path) const {
  return AddrTraceReader::read(path);
}

} // namespace binja::covex::coverage
