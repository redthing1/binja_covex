#include "covex/coverage/coverage_store.hpp"

#include <algorithm>

namespace binja::covex::coverage {

uint64_t CoverageStore::add_trace(CoverageTrace trace) {
  TraceRecord record;
  record.id = next_id_++;
  record.trace = std::move(trace);
  traces_.push_back(std::move(record));
  return traces_.back().id;
}

bool CoverageStore::remove_trace(uint64_t id) {
  auto it =
      std::find_if(traces_.begin(), traces_.end(),
                   [id](const TraceRecord &record) { return record.id == id; });
  if (it == traces_.end()) {
    return false;
  }
  traces_.erase(it);
  return true;
}

uint64_t CoverageStore::add_dataset(const DatasetRecord &dataset) {
  DatasetRecord record = dataset;
  record.id = next_id_++;
  datasets_.push_back(std::move(record));
  return datasets_.back().id;
}

} // namespace binja::covex::coverage
