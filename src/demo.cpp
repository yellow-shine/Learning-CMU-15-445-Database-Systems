#include "parallel_execution.hpp"
#include <iostream>
#include <numeric>
int main() {
 std::vector<int> input{1,2,3,4,5,6,7,8}; std::vector<std::int64_t> serial;
 for(auto v:input) serial.push_back(tutorial::square(v));
 auto result=tutorial::parallel_map(input,4,2,tutorial::square);
 std::cout<<"threads="<<result.per_worker.size()<<" rows="<<result.values.size()<<" sum="
          <<std::accumulate(result.values.begin(),result.values.end(),std::int64_t{0})<<" serial_equal="<<(serial==result.values)<<'\n';
}
