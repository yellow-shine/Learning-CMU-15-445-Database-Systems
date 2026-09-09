#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <exception>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
namespace tutorial {
class JoinThreads {
 std::vector<std::thread>& threads_;
 public:
 explicit JoinThreads(std::vector<std::thread>& threads):threads_(threads) {}
 ~JoinThreads() {for(auto& thread:threads_) if(thread.joinable()) thread.join();}
 JoinThreads(const JoinThreads&)=delete;
 JoinThreads& operator=(const JoinThreads&)=delete;
};
struct ParallelResult {std::vector<std::int64_t> values; std::vector<std::size_t> per_worker;};
template<class Function>
ParallelResult parallel_map(const std::vector<int>& input,std::size_t workers,std::size_t chunk,Function function) {
 if(workers==0 || workers>256 || chunk==0) throw std::invalid_argument("workers must be 1..256, chunk positive");
 const auto tasks=input.size()/chunk+(input.size()%chunk!=0);
 workers=std::min(workers,tasks);
 ParallelResult out{std::vector<std::int64_t>(input.size()),std::vector<std::size_t>(workers)};
 std::atomic<std::size_t> next{0}; std::atomic<bool> stop{false};
 std::vector<std::exception_ptr> errors(workers);
 {
  std::vector<std::thread> threads; threads.reserve(workers);
  JoinThreads join(threads); // Declared after every object used by worker callbacks.
  for(std::size_t worker=0;worker<workers;++worker) {
   threads.emplace_back([&,worker,function]() mutable {
    try {
     while(!stop.load(std::memory_order_relaxed)) {
      auto task=next.fetch_add(1,std::memory_order_relaxed);
      if(task>=tasks) break;
      auto begin=task*chunk, end=begin+std::min(chunk,input.size()-begin);
      for(auto i=begin;i<end;++i) {out.values[i]=function(input[i]); ++out.per_worker[worker];}
     }
    } catch(...) {errors[worker]=std::current_exception(); stop.store(true,std::memory_order_relaxed);}
   });
  }
 }
 for(const auto& error:errors) if(error) std::rethrow_exception(error);
 return out;
}
inline std::int64_t square(int value) {
 static_assert(std::numeric_limits<int>::digits<=31,"square requires at most 32-bit int");
 auto wide=static_cast<std::int64_t>(value); return wide*wide;
}
}
