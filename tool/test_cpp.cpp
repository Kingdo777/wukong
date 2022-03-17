//
// Created by kingdo on 2022/3/16.
//

#include <mutex>
#include <shared_mutex>
#include <thread>

using namespace std;
typedef std::unique_lock<std::mutex> UniqueLock;
typedef std::unique_lock<std::recursive_mutex> WriteLock;
typedef std::shared_lock<std::recursive_mutex> ReadLock;

class Test {
public:
    std::shared_mutex mx;

    void f2() {
        ReadLock lock1(mx);
    }
    void f1() {
        WriteLock lock(mx);
        f2();
    }
};
int main() {
    Test t;
    std::thread thread1([&]() {
        t.f1();
    });
    thread1.join();
}