#include "cost.hpp"
int main(){using namespace db;
 for(std::size_t n=0;n<8;++n)for(std::size_t m=0;m<8;++m)for(std::size_t cap:{1,2,5}){
 std::vector<int>a(n),b(m);for(std::size_t i=0;i<n;++i)a[i]=static_cast<int>(i%3);for(std::size_t j=0;j<m;++j)b[j]=static_cast<int>(j%3);
 long double actual=0;for(int x:a)for(int y:b)if(x==y)++actual;
 std::vector<std::pair<int,int>> oracle;
 for(auto kind:{Algorithm::NestedLoop,Algorithm::HashJoin}){auto e=estimate(kind,n,m,cap,actual);auto r=execute({kind,cap,e},a,b);require(r.measured.pages==e.pages&&r.measured.operations==e.operations&&r.measured.outputs==actual&&cost(e)==cost(r.measured));
 std::sort(r.rows.begin(),r.rows.end());if(kind==Algorithm::NestedLoop)oracle=r.rows;else require(r.rows==oracle);}
 auto chosen=choose(n,m,cap,actual);require(cost(chosen.estimated)==std::min(cost(estimate(Algorithm::HashJoin,n,m,cap,actual)),cost(estimate(Algorithm::NestedLoop,n,m,cap,actual))));}
 require(choose(1,1,2,1).algorithm==Algorithm::NestedLoop);require(choose(100,100,10,100).algorithm==Algorithm::HashJoin);
 auto wrong=estimate(Algorithm::HashJoin,3,3,2,3);auto skew=execute({Algorithm::HashJoin,2,wrong},{1,1,1},{1,1,1});require(skew.measured.outputs==9&&cost(wrong)!=cost(skew.measured));
 bool bad=false;try{pages(1,0);}catch(const std::invalid_argument&){bad=true;}require(bad);
 bad=false;try{cost({}, {-1,1});}catch(const std::invalid_argument&){bad=true;}require(bad);
 bad=false;try{estimate(Algorithm::HashJoin,1,1,2,2);}catch(const std::invalid_argument&){bad=true;}require(bad);
}
