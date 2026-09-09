#include "parallel_execution.hpp"
#include "check.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
using namespace tutorial;
int main() {
 rejects([]{parallel_map({},0,1,square);}); rejects([]{parallel_map({},257,1,square);}); rejects([]{parallel_map({},1,0,square);});
 CHECK(parallel_map({},4,1,square).per_worker.empty());
 std::vector<int> input{std::numeric_limits<int>::min(),std::numeric_limits<int>::max(),0,-1};
 std::mt19937 rng(445); for(int i=0;i<103;++i) input.push_back(static_cast<int>(rng()%2000)-1000);
 std::vector<std::int64_t> serial; for(auto x:input) serial.push_back(square(x));
 for(std::size_t p:{1,2,4,16}) for(std::size_t c:{1,3,20,1000}) {
  auto out=parallel_map(input,p,c,square); CHECK(out.values==serial);
  CHECK(std::accumulate(out.per_worker.begin(),out.per_worker.end(),std::size_t{0})==input.size());
 }
 CHECK(parallel_map({5},8,1,square).per_worker.size()==1);
 std::mutex mutex; std::condition_variable cv; std::set<std::thread::id> ids;
 auto barrier=[&,arrived=false](int value) mutable -> std::int64_t {
  if(!arrived) {
   arrived=true; std::unique_lock<std::mutex> lock(mutex); ids.insert(std::this_thread::get_id()); cv.notify_all();
   if(!cv.wait_for(lock,std::chrono::seconds(3),[&]{return ids.size()==4;})) throw std::runtime_error("thread barrier timeout");
  }
  return square(value);
 };
 auto concurrent=parallel_map(std::vector<int>(8,2),4,1,barrier);
 CHECK(ids.size()==4); CHECK(concurrent.values==std::vector<std::int64_t>(8,4));
 std::atomic<int> active{0},calls{0};
 bool propagated=false;
 try {
  parallel_map(std::vector<int>(100,1),4,1,[&](int)->std::int64_t {
   struct Guard {std::atomic<int>& n; explicit Guard(std::atomic<int>& x):n(x){++n;} ~Guard(){--n;}} guard(active);
   ++calls; throw std::domain_error("worker failed");
  });
 } catch(const std::domain_error& e) {propagated=std::string(e.what())=="worker failed";}
 CHECK(propagated && active.load()==0 && calls.load()>0);
}
