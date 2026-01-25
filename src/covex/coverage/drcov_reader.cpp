#include "covex/coverage/drcov_reader.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

#include "drcov.hpp"

namespace binja::covex::coverage {

namespace {

constexpr size_t kHeaderProbeSize = 16;

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

CoverageTrace build_trace(const std::string &path,
                          const drcov::coverage_data &data) {
  CoverageTrace trace;
  trace.format = TraceFormat::DrcovBlocks;
  trace.source_path = path;
  trace.name = std::filesystem::path(path).filename().string();
  trace.has_hitcounts = data.has_hitcounts();

  for (const auto &module : data.modules) {
    ModuleInfo info;
    info.id = module.id;
    info.base = module.base;
    info.end = module.end;
    info.path = module.path;
    trace.modules.emplace(info.id, std::move(info));
  }

  trace.spans.reserve(data.basic_blocks.size());
  for (size_t i = 0; i < data.basic_blocks.size(); ++i) {
    const auto &bb = data.basic_blocks[i];
    auto module_it = trace.modules.find(bb.module_id);
    if (module_it == trace.modules.end()) {
      continue;
    }
    CoverageSpan span;
    span.address = module_it->second.base + bb.start;
    span.size = bb.size;
    span.hits = trace.has_hitcounts ? data.hitcounts[i] : 1;
    span.module_id = bb.module_id;
    trace.spans.push_back(span);
  }

  return trace;
}

} // namespace

CoverageTrace DrcovReader::read(const std::string &path) {
  try {
    const auto data = drcov::read(path);
    return build_trace(path, data);
  } catch (const drcov::parse_error &err) {
    throw std::runtime_error(err.what());
  }
}

bool DrcovParser::can_parse(const std::string &path) const {
  return has_drcov_header(path);
}

CoverageTrace DrcovParser::parse(const std::string &path) const {
  return DrcovReader::read(path);
}

} // namespace binja::covex::coverage
