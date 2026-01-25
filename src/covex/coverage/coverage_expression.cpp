#include "covex/coverage/coverage_expression.hpp"

#include <algorithm>
#include <cctype>
#include <stack>

namespace binja::covex::coverage {

namespace {

bool is_identifier_start(char ch) {
  return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool is_identifier_body(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

int precedence(ExprTokenType type) {
  switch (type) {
  case ExprTokenType::IntersectOp:
    return 2;
  case ExprTokenType::UnionOp:
  case ExprTokenType::SubtractOp:
    return 1;
  default:
    return 0;
  }
}

bool is_operator(ExprTokenType type) {
  return type == ExprTokenType::UnionOp || type == ExprTokenType::IntersectOp ||
         type == ExprTokenType::SubtractOp;
}

std::string upper_copy(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return value;
}

} // namespace

std::variant<ComposePlan, ComposeError>
parse_expression(std::string_view expression) {
  ComposePlan plan;
  std::vector<ExprToken> output;
  std::vector<ExprToken> ops;

  size_t i = 0;
  while (i < expression.size()) {
    char ch = expression[i];
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
      ++i;
      continue;
    }
    if (is_identifier_start(ch)) {
      size_t start = i;
      ++i;
      while (i < expression.size() && is_identifier_body(expression[i])) {
        ++i;
      }
      std::string ident(expression.substr(start, i - start));
      ident = upper_copy(ident);
      output.push_back({ExprTokenType::Identifier, ident, start});
      if (std::find(plan.aliases.begin(), plan.aliases.end(), ident) ==
          plan.aliases.end()) {
        plan.aliases.push_back(ident);
      }
      continue;
    }

    ExprToken token;
    token.position = i;
    switch (ch) {
    case '|':
      token.type = ExprTokenType::UnionOp;
      break;
    case '&':
      token.type = ExprTokenType::IntersectOp;
      break;
    case '-':
      token.type = ExprTokenType::SubtractOp;
      break;
    case '(':
      token.type = ExprTokenType::LParen;
      break;
    case ')':
      token.type = ExprTokenType::RParen;
      break;
    default:
      return ComposeError{"Unexpected character in expression", i};
    }

    if (token.type == ExprTokenType::LParen) {
      ops.push_back(token);
    } else if (token.type == ExprTokenType::RParen) {
      bool matched = false;
      while (!ops.empty()) {
        auto op = ops.back();
        ops.pop_back();
        if (op.type == ExprTokenType::LParen) {
          matched = true;
          break;
        }
        output.push_back(op);
      }
      if (!matched) {
        return ComposeError{"Unmatched closing parenthesis", i};
      }
    } else {
      while (!ops.empty()) {
        auto top = ops.back();
        if (!is_operator(top.type)) {
          break;
        }
        if (precedence(top.type) < precedence(token.type)) {
          break;
        }
        ops.pop_back();
        output.push_back(top);
      }
      ops.push_back(token);
    }
    ++i;
  }

  while (!ops.empty()) {
    auto op = ops.back();
    ops.pop_back();
    if (op.type == ExprTokenType::LParen || op.type == ExprTokenType::RParen) {
      return ComposeError{"Unmatched parenthesis in expression", op.position};
    }
    output.push_back(op);
  }

  if (output.empty()) {
    return ComposeError{"Empty composition expression", 0};
  }

  plan.rpn = std::move(output);
  return plan;
}

std::variant<CoverageDataset, ComposeError> evaluate_expression(
    const ComposePlan &plan,
    const std::unordered_map<std::string, CoverageDataset> &datasets,
    HitMergePolicy union_policy, HitMergePolicy intersect_policy,
    HitMergePolicy subtract_policy) {
  std::vector<CoverageDataset> stack;
  stack.reserve(plan.rpn.size());

  for (const auto &token : plan.rpn) {
    if (token.type == ExprTokenType::Identifier) {
      auto it = datasets.find(token.text);
      if (it == datasets.end()) {
        return ComposeError{"Unknown alias: " + token.text, token.position};
      }
      stack.push_back(it->second);
      continue;
    }

    if (!is_operator(token.type)) {
      return ComposeError{"Invalid token in expression", token.position};
    }

    if (stack.size() < 2) {
      return ComposeError{"Malformed expression: missing operand",
                          token.position};
    }

    CoverageDataset right = std::move(stack.back());
    stack.pop_back();
    CoverageDataset left = std::move(stack.back());
    stack.pop_back();

    CompositionOp op = CompositionOp::Union;
    HitMergePolicy policy = union_policy;
    if (token.type == ExprTokenType::IntersectOp) {
      op = CompositionOp::Intersection;
      policy = intersect_policy;
    } else if (token.type == ExprTokenType::SubtractOp) {
      op = CompositionOp::Subtract;
      policy = subtract_policy;
    }

    stack.push_back(compose(left, right, op, policy));
  }

  if (stack.size() != 1) {
    return ComposeError{"Malformed expression: extra operands", 0};
  }

  return stack.front();
}

} // namespace binja::covex::coverage
