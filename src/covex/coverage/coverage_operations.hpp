#pragma once

#include "covex/coverage/coverage_dataset.hpp"

namespace binja::covex::coverage {

enum class CompositionOp { Union, Intersection, Subtract };
enum class HitMergePolicy { Sum, Min, Max, Left };

CoverageDataset compose(const CoverageDataset &a, const CoverageDataset &b,
                        CompositionOp op, HitMergePolicy policy);

} // namespace binja::covex::coverage
