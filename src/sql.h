#pragma once
#include "relation.h"
#include <memory>
#include <optional>

namespace tutorial {
inline bool letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
inline bool digit(char c) { return c >= '0' && c <= '9'; }
inline std::string lowercase(std::string s) {
  for (auto &c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  return s;
}
inline bool keyword(const std::string &s) {
  return s == "select" || s == "from" || s == "where" || s == "and" || s == "null";
}
inline bool identifier(const std::string &s) {
  if (s.empty() || !letter(s[0]) || keyword(lowercase(s))) return false;
  for (char c : s) if (!letter(c) && !digit(c)) return false;
  return true;
}
enum class TokenKind { Name, Keyword, Integer, Text, Symbol, End };
struct Token { TokenKind kind; std::string text; std::size_t position; };
inline std::vector<Token> lex(const std::string &sql) {
  std::vector<Token> tokens;
  std::size_t i = 0;
  while (i < sql.size()) {
    const auto start = i;
    char c = sql[i];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { ++i; continue; }
    if (letter(c)) {
      while (i < sql.size() && (letter(sql[i]) || digit(sql[i]))) ++i;
      auto text = lowercase(sql.substr(start, i - start));
      tokens.push_back({keyword(text) ? TokenKind::Keyword : TokenKind::Name, text, start});
    } else if (digit(c) || c == '-') {
      if (c == '-') ++i;
      if (i == sql.size() || !digit(sql[i])) throw std::invalid_argument("expected digits at " + std::to_string(start));
      while (i < sql.size() && digit(sql[i])) ++i;
      if (i < sql.size() && letter(sql[i])) throw std::invalid_argument("invalid numeric token at " + std::to_string(start));
      tokens.push_back({TokenKind::Integer, sql.substr(start, i - start), start});
    } else if (c == '\'') {
      ++i;
      std::string text;
      bool closed = false;
      while (i < sql.size()) {
        c = sql[i++];
        if (c != '\'') text += c;
        else if (i < sql.size() && sql[i] == '\'') { text += '\''; ++i; }
        else { closed = true; break; }
      }
      if (!closed) throw std::invalid_argument("unterminated string at " + std::to_string(start));
      tokens.push_back({TokenKind::Text, std::move(text), start});
    } else if (std::string(",.*=;").find(c) != std::string::npos) {
      tokens.push_back({TokenKind::Symbol, std::string(1, c), i++});
    } else throw std::invalid_argument("unsupported character at " + std::to_string(i));
  }
  tokens.push_back({TokenKind::End, "", sql.size()});
  return tokens;
}
struct ColumnName { std::string table, column; };
struct Operand { std::optional<ColumnName> column; Value literal; };
struct Equality { ColumnName left; Operand right; };
struct Query {
  bool star = false;
  std::vector<ColumnName> columns;
  std::vector<std::string> tables;
  std::vector<Equality> predicates;
};
class Parser {
 public:
  explicit Parser(const std::string &sql) : tokens_(lex(sql)) {}
  Query parse() {
    Query query;
    expect("select");
    if (take("*")) query.star = true;
    else { do { query.columns.push_back(column()); } while (take(",")); }
    expect("from");
    do { query.tables.push_back(name()); } while (take(","));
    if (take("where")) {
      do {
        auto left = column(); expect("=");
        query.predicates.push_back({std::move(left), operand()});
      } while (take("and"));
    }
    take(";");
    if (current().kind != TokenKind::End) fail("trailing input");
    return query;
  }
 private:
  std::vector<Token> tokens_;
  std::size_t position_ = 0;
  const Token &current() const { return tokens_.at(position_); }
  [[noreturn]] void fail(const std::string &message) const {
    throw std::invalid_argument(message + " at " + std::to_string(current().position));
  }
  bool take(const std::string &text) {
    if ((current().kind == TokenKind::Keyword || current().kind == TokenKind::Symbol) && current().text == text) {
      ++position_; return true;
    }
    return false;
  }
  void expect(const std::string &text) { if (!take(text)) fail("expected " + text); }
  std::string name() {
    if (current().kind != TokenKind::Name) fail("expected identifier");
    return tokens_[position_++].text;
  }
  ColumnName column() {
    auto first = name();
    if (take(".")) return {first, name()};
    return {"", first};
  }
  Operand operand() {
    if (current().kind == TokenKind::Name) return {column(), {}};
    if (current().kind == TokenKind::Text) return {std::nullopt, tokens_[position_++].text};
    if (take("null")) return {std::nullopt, std::monostate{}};
    if (current().kind == TokenKind::Integer) {
      std::int64_t value;
      try { value = std::stoll(current().text); }
      catch (const std::out_of_range &) { fail("integer outside int64 range"); }
      ++position_; return {std::nullopt, value};
    }
    fail("expected column or literal");
  }
};

struct BagTable { std::string name; Schema schema; std::vector<Tuple> rows; };
class Catalog {
 public:
  void create(std::string name, Schema schema, std::vector<Tuple> rows) {
    name = lowercase(std::move(name));
    if (!identifier(name)) throw std::invalid_argument("invalid table name");
    for (const auto &t : tables_) if (t.name == name) throw std::invalid_argument("duplicate table");
    auto attributes = schema.attributes();
    for (auto &a : attributes) {
      a.name = lowercase(a.name);
      if (!identifier(a.name)) throw std::invalid_argument("invalid column name");
    }
    Schema normalized(std::move(attributes));  // Reject case-folded duplicate names.
    for (const auto &row : rows) normalized.validate(row);
    tables_.push_back({std::move(name), std::move(normalized), std::move(rows)});
  }
  const BagTable &table(const std::string &name) const {
    for (const auto &t : tables_) if (t.name == name) return t;
    throw std::invalid_argument("unknown table: " + name);
  }
 private:
  std::vector<BagTable> tables_;
};
struct BoundEquality {
  std::size_t left;
  std::optional<std::size_t> right_column;
  Value literal;
};
// Logical meaning: Project(columns, Select(predicates, Product(sources))).
// Owning source snapshots avoid dangling catalog references and stale bindings.
struct LogicalPlan {
  std::vector<BagTable> sources;
  std::vector<BoundEquality> predicates;
  std::vector<std::size_t> columns;
  std::vector<Attribute> output;  // SQL permits repeated output names.
};
inline LogicalPlan bind(const Query &query, const Catalog &catalog) {
  LogicalPlan plan;
  std::set<std::string> seen;
  std::vector<Attribute> attributes;
  std::vector<std::string> owners;
  if (query.tables.empty() || (!query.star && query.columns.empty())) throw std::invalid_argument("empty query");
  for (const auto &name : query.tables) {
    if (!seen.insert(name).second) throw std::invalid_argument("repeated FROM table requires aliases (unsupported)");
    plan.sources.push_back(catalog.table(name));
    for (const auto &a : plan.sources.back().schema.attributes()) { attributes.push_back(a); owners.push_back(name); }
  }
  auto resolve = [&](const ColumnName &name) {
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i < attributes.size(); ++i)
      if (attributes[i].name == name.column && (name.table.empty() || owners[i] == name.table)) {
        if (found) throw std::invalid_argument("ambiguous column: " + name.column);
        found = i;
      }
    if (!found) throw std::invalid_argument("unknown column: " + name.column);
    return *found;
  };
  for (const auto &predicate : query.predicates) {
    const auto left = resolve(predicate.left);
    BoundEquality bound{left, std::nullopt, predicate.right.literal};
    if (predicate.right.column) {
      bound.right_column = resolve(*predicate.right.column);
      if (attributes[left].type != attributes[*bound.right_column].type) throw std::invalid_argument("comparison type mismatch");
    } else {
      auto a = attributes[left]; a.nullable = true;
      Schema({a}).validate({bound.literal});  // NULL is allowed in any equality, but evaluates UNKNOWN.
    }
    plan.predicates.push_back(std::move(bound));
  }
  if (query.star) for (std::size_t i = 0; i < attributes.size(); ++i) plan.columns.push_back(i);
  else for (const auto &column : query.columns) plan.columns.push_back(resolve(column));
  for (auto i : plan.columns) { auto a = attributes[i]; a.name = owners[i] + "." + a.name; plan.output.push_back(a); }
  return plan;
}
inline std::string explain(const LogicalPlan &plan) {
  std::string text = "Product[";
  for (std::size_t i = 0; i < plan.sources.size(); ++i) {
    if (i != 0) text += ",";
    text += plan.sources[i].name;
  }
  text += "]";
  if (!plan.predicates.empty()) text = "Select[" + std::to_string(plan.predicates.size()) + "](" + text + ")";
  return "Project[" + std::to_string(plan.columns.size()) + "](" + text + ")";
}
enum class PhysicalOp { SeqScan, NestedLoopProduct, Filter, Gather };
struct PhysicalPlan {
  PhysicalOp operation;
  std::unique_ptr<PhysicalPlan> left, right;
  std::vector<Tuple> rows;
  std::vector<BoundEquality> predicates;
  std::vector<std::size_t> columns;
  explicit PhysicalPlan(PhysicalOp op) : operation(op) {}
};
inline std::unique_ptr<PhysicalPlan> lower(const LogicalPlan &logical) {
  std::unique_ptr<PhysicalPlan> root;
  for (const auto &table : logical.sources) {
    auto scan = std::make_unique<PhysicalPlan>(PhysicalOp::SeqScan);
    scan->rows = table.rows;
    if (!root) root = std::move(scan);
    else {
      auto product = std::make_unique<PhysicalPlan>(PhysicalOp::NestedLoopProduct);
      product->left = std::move(root); product->right = std::move(scan); root = std::move(product);
    }
  }
  if (!root) throw std::invalid_argument("logical plan needs a source");
  if (!logical.predicates.empty()) {
    auto filter = std::make_unique<PhysicalPlan>(PhysicalOp::Filter);
    filter->predicates = logical.predicates; filter->left = std::move(root); root = std::move(filter);
  }
  auto project = std::make_unique<PhysicalPlan>(PhysicalOp::Gather);
  project->columns = logical.columns; project->left = std::move(root);
  return project;
}
inline std::string explain(const PhysicalPlan &plan) {
  switch (plan.operation) {
    case PhysicalOp::SeqScan: return "SeqScan";
    case PhysicalOp::NestedLoopProduct: return "NestedLoopProduct(" + explain(*plan.left) + "," + explain(*plan.right) + ")";
    case PhysicalOp::Filter: return "Filter(" + explain(*plan.left) + ")";
    case PhysicalOp::Gather: return "Gather(" + explain(*plan.left) + ")";
  }
  throw std::invalid_argument("unknown physical operator");
}
inline std::vector<Tuple> execute(const PhysicalPlan &plan) {
  if (plan.operation == PhysicalOp::SeqScan) return plan.rows;
  auto input = execute(*plan.left);
  std::vector<Tuple> output;
  if (plan.operation == PhysicalOp::NestedLoopProduct) {
    auto right = execute(*plan.right);
    // ponytail: materialize the product; add streaming/hash join for large inputs.
    for (const auto &l : input) for (const auto &r : right) {
      auto row = l; row.insert(row.end(), r.begin(), r.end()); output.push_back(std::move(row));
    }
  } else if (plan.operation == PhysicalOp::Filter) {
    for (auto &row : input) {
      bool matches = true;
      for (const auto &predicate : plan.predicates) {
        const auto &left = row.at(predicate.left);
        const auto &right = predicate.right_column ? row.at(*predicate.right_column) : predicate.literal;
        // WHERE keeps only TRUE. Any equality with NULL is UNKNOWN, hence discarded.
        if (std::holds_alternative<std::monostate>(left) || std::holds_alternative<std::monostate>(right) || left != right) {
          matches = false; break;
        }
      }
      if (matches) output.push_back(std::move(row));
    }
  } else if (plan.operation == PhysicalOp::Gather) {
    for (const auto &row : input) {
      Tuple projected;
      for (auto column : plan.columns) projected.push_back(row.at(column));
      output.push_back(std::move(projected));  // Bag semantics: never deduplicate.
    }
  } else throw std::invalid_argument("unknown physical operator");
  return output;
}
}  // namespace tutorial
