#include "engine.h"
#include <iostream>
int main() {

ModificationTable table; table.Insert({{1,10},{2,20}});
table.Update({{0,{3,30}}}); table.Delete({1});
try { table.Insert({{4,40},{3,99}}); } catch(const std::invalid_argument& e) { std::cout << "rollback: " << e.what() << '\n'; }
for(std::size_t rid=0;rid<table.rows().size();++rid) if(table.rows()[rid])
    std::cout << "RID=" << rid << " key=" << table.rows()[rid]->key << " value=" << table.rows()[rid]->value << '\n';
std::cout << "consistent=" << table.Consistent() << " key4_exists=" << table.Find(4).has_value() << '\n';

}
