#include "bplus_tree.h"
#include <condition_variable>
#include <iostream>
#include <thread>
#include <chrono>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
class Barrier {
    std::mutex mutex_; std::condition_variable cv_; int remaining_;
public:
    explicit Barrier(int count):remaining_(count) {}
    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (--remaining_==0) cv_.notify_all();
        else if (!cv_.wait_for(lock,std::chrono::seconds(10),[&]{return remaining_==0;}))
            throw std::runtime_error("barrier timeout: leaf writes not concurrent");
    }
};
int main() {
    ConcurrentBPlusTree tree; CHECK(!tree.get(1)); CHECK(!tree.erase(1));
    for (int i=0;i<1000;++i) tree.put(i,i);
    tree.validate();
    // These updates must simultaneously hold DISTINCT leaf write latches.
    Barrier overlap(2);
    std::thread a([&]{tree.put(100,100,[&]{overlap.wait();});});
    std::thread b([&]{tree.put(900,900,[&]{overlap.wait();});});
    a.join(); b.join(); CHECK(tree.fast_writes()>=2);
    Barrier start(5); std::vector<std::thread> threads;
    std::atomic<bool> ok{true};
    for (int worker=0;worker<4;++worker) threads.emplace_back([&,worker]{
        start.wait();
        for (int i=0;i<800;++i) {
            int key=worker*10000+i;
            tree.put(key,key*2);
            if (tree.get(key)!=key*2) ok=false;
            if (i%2==0 && !tree.erase(key)) ok=false;
        }
    });
    threads.emplace_back([&]{start.wait();for(int i=0;i<12000;++i) (void)tree.get(i%1000);});
    for(auto& thread:threads) thread.join();
    CHECK(ok); tree.validate();
    for(int worker=0;worker<4;++worker) for(int i=0;i<800;++i) {
        int key=worker*10000+i;
        CHECK(tree.get(key)==(i%2?std::optional<int>{key*2}:std::optional<int>{}));
    }
    Barrier contested(4); threads.clear();
    for(int t=0;t<4;++t) threads.emplace_back([&,t]{
        contested.wait();
        for(int i=0;i<2000;++i) {
            int key=i%100;
            if ((i+t)%3==0) tree.erase(key); else tree.put(key,t);
            (void)tree.get(key);
        }
    });
    for(auto& thread:threads) thread.join(); tree.validate();
    for(int i=0;i<31000;++i) tree.erase(i);
    tree.validate(); CHECK(tree.height()==1);
    std::cout << "coupled readers, overlapping leaf writers, mixed updates passed\n";
}
