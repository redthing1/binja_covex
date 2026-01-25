#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace binja::covex::core {

struct BlockFilterContext {
  uint64_t address = 0;
  uint32_t size = 0;
  uint64_t hits = 0;
  std::string function;
};

enum class BlockFilterField { Address, Size, Hits, Function };

enum class BlockFilterOp {
  Equal,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Contains
};

struct BlockFilterCondition {
  BlockFilterField field = BlockFilterField::Address;
  BlockFilterOp op = BlockFilterOp::Equal;
  uint64_t value = 0;
  std::string text;
};

struct BlockFilter {
  std::vector<BlockFilterCondition> conditions;
  bool empty() const { return conditions.empty(); }
  bool matches(const BlockFilterContext &ctx) const;
};

struct BlockFilterError {
  std::string message;
  size_t position = 0;
};

std::variant<BlockFilter, BlockFilterError>
parse_block_filter(std::string_view expression);

} // namespace binja::covex::core
