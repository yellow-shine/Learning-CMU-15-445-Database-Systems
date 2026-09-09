#include "statistics.hpp"
#include <iostream>
int main(){using namespace db;
 auto s=analyze({0,1,2,3,4,5,6,7,8,9},5);
 std::cout<<"rows="<<s.rows<<" ndv="<<s.distinct<<" P(x<4)="<<s.less(4)<<" estimate="<<s.less(4)*s.rows<<'\n';
 std::cout<<"correlated x=y: independent estimate="<<conjunction(s.less(4),s.less(4))*s.rows<<" actual=4\n";
 require(close(s.less(4),.4L));
}
