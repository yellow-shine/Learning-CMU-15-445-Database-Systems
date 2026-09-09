#include "tutorial.hpp"
#include <iostream>
#include <type_traits>
using namespace tutorial;
static_assert(!std::is_copy_constructible_v<Bank>);
static_assert(!std::is_copy_assignable_v<Bank>);
static_assert(!std::is_move_constructible_v<Bank>);
static_assert(!std::is_move_assignable_v<Bank>);
void check(bool ok) { if (!ok) throw std::runtime_error("check failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() { try {

Bank b({100,50});
{ Transaction t(b); t.transfer(0,1,30); check(b.balance(0)==100); t.commit();
  check(t.state()==Transaction::State::Committed); rejects([&]{t.abort();}); }
check(b.balance(0)==70 && b.balance(1)==80);
{ Transaction t(b); t.transfer(0,1,20); rejects([&]{t.transfer(0,1,1000);});
  check(t.state()==Transaction::State::Aborted); rejects([&]{t.commit();}); }
check(b.balance(0)==70 && b.balance(1)==80);
{ Transaction t(b); t.transfer(1,0,5); rejects([&]{Transaction other(b);}); }
check(b.balance(0)==70);
{ Transaction t(b); t.transfer(0,0,70); t.transfer(0,1,0); t.commit(); }
{ Transaction t(b); rejects([&]{t.transfer(3,0,1);}); }
{ Transaction t(b); rejects([&]{t.transfer(0,1,-1);}); }
Bank big({1,std::numeric_limits<std::int64_t>::max()});
{ Transaction t(big); rejects([&]{t.transfer(0,1,1);}); }
check(big.balance(0)==1);
Bank empty({}); { Transaction t(empty); t.commit(); }
rejects([]{Bank bad({-1});});

std::cout << "all checks passed\n"; return 0;
} catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }
