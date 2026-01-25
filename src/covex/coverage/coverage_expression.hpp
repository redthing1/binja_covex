#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "covex/coverage/coverage_dataset.hpp"
#include "covex/coverage/coverage_operations.hpp"

namespace binja::covex::coverage {

enum class ExprTokenType {
  Identifier,
  UnionOp,
  IntersectOp,
  SubtractOp,
  LParen,
  RParen
};

struct ExprToken {
  ExprTokenType type = ExprTokenType::Identifier;
  std::string text;
  size_t position = 0;
};

struct ComposePlan {
  std::vector<ExprToken> rpn;
  std::vector<std::string> aliases;
};

struct ComposeError {
  std::string message;
  size_t position = 0;
};

std::variant<ComposePlan, ComposeError>
parse_expression(std::string_view expression);

std::variant<CoverageDataset, ComposeError> evaluate_expression(
    const ComposePlan &plan,
    const std::unordered_map<std::string, CoverageDataset> &datasets,
    HitMergePolicy union_policy = HitMergePolicy::Sum,
    HitMergePolicy intersect_policy = HitMergePolicy::Min,
    HitMergePolicy subtract_policy = HitMergePolicy::Left);

} // namespace binja::covex::coverage
