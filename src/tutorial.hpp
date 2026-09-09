#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tutorial {

class Bank {
 public:
  explicit Bank(std::vector<std::int64_t> balances) : balances_(std::move(balances)) {
    for (auto b : balances_) if (b < 0) throw std::invalid_argument("negative balance");
  }
  // Transactions retain a reference: the bank must keep a stable identity.
  Bank(const Bank&) = delete;
  Bank& operator=(const Bank&) = delete;
  Bank(Bank&&) = delete;
  Bank& operator=(Bank&&) = delete;
  std::int64_t balance(std::size_t id) const { return balances_.at(id); }
 private:
  std::vector<std::int64_t> balances_;
  bool busy_ = false;
  friend class Transaction;
};
class Transaction {
 public:
  enum class State { Active, Committed, Aborted };
  explicit Transaction(Bank& bank) : bank_(bank), staged_(bank.balances_) {
    if (bank_.busy_) throw std::logic_error("single transaction only");
    bank_.busy_ = true;
  }
  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;
  ~Transaction() { if (state_ == State::Active) abort(); }
  State state() const { return state_; }
  void transfer(std::size_t from, std::size_t to, std::int64_t amount) {
    active();
    try {
      const auto a = staged_.at(from), b = staged_.at(to);
      if (amount < 0 || amount > a) throw std::invalid_argument("invalid amount");
      if (from == to) return;
      if (b > std::numeric_limits<std::int64_t>::max() - amount)
        throw std::overflow_error("balance overflow");
      staged_[from] -= amount;
      staged_[to] += amount;
    } catch (...) { abort(); throw; }
  }
  void commit() {
    active();
    bank_.balances_.swap(staged_); // no throwing operation after publication
    state_ = State::Committed; bank_.busy_ = false;
  }
  void abort() { active(); state_ = State::Aborted; bank_.busy_ = false; }
 private:
  void active() const { if (state_ != State::Active) throw std::logic_error("finished transaction"); }
  Bank& bank_;
  std::vector<std::int64_t> staged_;
  State state_ = State::Active;
};

}
