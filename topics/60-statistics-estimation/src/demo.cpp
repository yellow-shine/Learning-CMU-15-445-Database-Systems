#include "statistics.hpp"
#include <iostream>
int main(){using namespace db;
 std::vector<std::optional<int>> values{0,1,2,3,4,5,6,7,8,9};
 auto s=analyze(values,5);
 std::size_t actual=0;for(auto x:values)if(x&&*x<4)++actual; // y=x, so both predicates match together
 std::cout<<"rows="<<s.rows<<" ndv="<<s.distinct<<" P(x<4)="<<s.less(4)<<" estimate="<<s.less(4)*s.rows<<'\n';
 std::cout<<"correlated x=y: independent estimate="<<conjunction(s.less(4),s.less(4))*s.rows<<" actual="<<actual<<'\n';
 require(close(s.less(4),.4L));
}
