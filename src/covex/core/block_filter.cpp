#include "covex/core/block_filter.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace binja::covex::core {

namespace {

std::string lower_copy(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

bool parse_number(std::string_view text, uint64_t &out) {
  std::string value(text);
  if (value.empty()) {
    return false;
  }
  int base = 10;
  if (value.size() > 2 && value[0] == '0' &&
      (value[1] == 'x' || value[1] == 'X')) {
    base = 16;
  }
  char *end = nullptr;
  out = std::strtoull(value.c_str(), &end, base);
  return end != nullptr && *end == '\0';
}

bool parse_condition(std::string_view token, BlockFilterCondition &out,
                     BlockFilterError &err) {
  size_t pos = token.find_first_of("=<>:");
  if (pos == std::string_view::npos) {
    err = {"Expected filter condition (eg hits>=10)", 0};
    return false;
  }

  std::string field_text(token.substr(0, pos));
  std::string op_text;
  std::string value_text;

  if (token.substr(pos, 2) == ">=" || token.substr(pos, 2) == "<=" ||
      token.substr(pos, 2) == "!=" || token.substr(pos, 2) == "==") {
    op_text = std::string(token.substr(pos, 2));
    value_text = std::string(token.substr(pos + 2));
  } else {
    op_text = std::string(token.substr(pos, 1));
    value_text = std::string(token.substr(pos + 1));
  }

  field_text = lower_copy(field_text);
  value_text = lower_copy(value_text);

  if (field_text == "addr" || field_text == "address") {
    out.field = BlockFilterField::Address;
  } else if (field_text == "hits" || field_text == "count") {
    out.field = BlockFilterField::Hits;
  } else if (field_text == "size") {
    out.field = BlockFilterField::Size;
  } else if (field_text == "func" || field_text == "function") {
    out.field = BlockFilterField::Function;
  } else {
    err = {"Unknown filter field: " + field_text, 0};
    return false;
  }

  if (op_text == ":" || op_text == "=" || op_text == "==") {
    out.op = BlockFilterOp::Equal;
  } else if (op_text == "!=") {
    out.op = BlockFilterOp::NotEqual;
  } else if (op_text == "<") {
    out.op = BlockFilterOp::Less;
  } else if (op_text == "<=") {
    out.op = BlockFilterOp::LessEqual;
  } else if (op_text == ">") {
    out.op = BlockFilterOp::Greater;
  } else if (op_text == ">=") {
    out.op = BlockFilterOp::GreaterEqual;
  } else {
    err = {"Unknown filter operator: " + op_text, 0};
    return false;
  }

  out.text = value_text;
  out.value = 0;
  if (out.field == BlockFilterField::Function) {
    out.op = BlockFilterOp::Contains;
    return true;
  }

  if (!parse_number(value_text, out.value)) {
    err = {"Invalid numeric filter value: " + value_text, 0};
    return false;
  }

  return true;
}

} // namespace

std::variant<BlockFilter, BlockFilterError>
parse_block_filter(std::string_view expression) {
  BlockFilter filter;
  BlockFilterError error;

  size_t start = 0;
  while (start < expression.size()) {
    while (start < expression.size() &&
           std::isspace(static_cast<unsigned char>(expression[start])) != 0) {
      ++start;
    }
    if (start >= expression.size()) {
      break;
    }
    size_t end = start;
    while (end < expression.size() &&
           std::isspace(static_cast<unsigned char>(expression[end])) == 0) {
      ++end;
    }
    std::string_view token = expression.substr(start, end - start);
    BlockFilterCondition condition;
    if (!parse_condition(token, condition, error)) {
      error.position = start;
      return error;
    }
    filter.conditions.push_back(std::move(condition));
    start = end;
  }

  return filter;
}

bool BlockFilter::matches(const BlockFilterContext &ctx) const {
  for (const auto &cond : conditions) {
    switch (cond.field) {
    case BlockFilterField::Address: {
      uint64_t val = ctx.address;
      bool ok = false;
      switch (cond.op) {
      case BlockFilterOp::Equal:
        ok = (val == cond.value);
        break;
      case BlockFilterOp::NotEqual:
        ok = (val != cond.value);
        break;
      case BlockFilterOp::Less:
        ok = (val < cond.value);
        break;
      case BlockFilterOp::LessEqual:
        ok = (val <= cond.value);
        break;
      case BlockFilterOp::Greater:
        ok = (val > cond.value);
        break;
      case BlockFilterOp::GreaterEqual:
        ok = (val >= cond.value);
        break;
      default:
        ok = false;
        break;
      }
      if (!ok) {
        return false;
      }
      break;
    }
    case BlockFilterField::Size: {
      uint64_t val = ctx.size;
      bool ok = false;
      switch (cond.op) {
      case BlockFilterOp::Equal:
        ok = (val == cond.value);
        break;
      case BlockFilterOp::NotEqual:
        ok = (val != cond.value);
        break;
      case BlockFilterOp::Less:
        ok = (val < cond.value);
        break;
      case BlockFilterOp::LessEqual:
        ok = (val <= cond.value);
        break;
      case BlockFilterOp::Greater:
        ok = (val > cond.value);
        break;
      case BlockFilterOp::GreaterEqual:
        ok = (val >= cond.value);
        break;
      default:
        ok = false;
        break;
      }
      if (!ok) {
        return false;
      }
      break;
    }
    case BlockFilterField::Hits: {
      uint64_t val = ctx.hits;
      bool ok = false;
      switch (cond.op) {
      case BlockFilterOp::Equal:
        ok = (val == cond.value);
        break;
      case BlockFilterOp::NotEqual:
        ok = (val != cond.value);
        break;
      case BlockFilterOp::Less:
        ok = (val < cond.value);
        break;
      case BlockFilterOp::LessEqual:
        ok = (val <= cond.value);
        break;
      case BlockFilterOp::Greater:
        ok = (val > cond.value);
        break;
      case BlockFilterOp::GreaterEqual:
        ok = (val >= cond.value);
        break;
      default:
        ok = false;
        break;
      }
      if (!ok) {
        return false;
      }
      break;
    }
    case BlockFilterField::Function: {
      std::string needle = cond.text;
      if (needle.empty()) {
        continue;
      }
      std::string hay = lower_copy(ctx.function);
      if (hay.find(needle) == std::string::npos) {
        return false;
      }
      break;
    }
    }
  }
  return true;
}

} // namespace binja::covex::core
