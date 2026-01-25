#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "covex/coverage/coverage_operations.hpp"
#include "covex/coverage/coverage_types.hpp"

namespace binja::covex::coverage {

struct TraceRecord {
  uint64_t id = 0;
  CoverageTrace trace;
};

struct DatasetRecord {
  uint64_t id = 0;
  std::string name;
  CompositionOp op = CompositionOp::Union;
  std::vector<uint64_t> inputs;
  HitMergePolicy policy = HitMergePolicy::Sum;
};

class CoverageStore {
public:
  uint64_t add_trace(CoverageTrace trace);
  bool remove_trace(uint64_t id);
  std::vector<TraceRecord> traces() const { return traces_; }

  uint64_t add_dataset(const DatasetRecord &dataset);
  std::vector<DatasetRecord> datasets() const { return datasets_; }

private:
  uint64_t next_id_ = 1;
  std::vector<TraceRecord> traces_;
  std::vector<DatasetRecord> datasets_;
};

} // namespace binja::covex::coverage
