#include <iostream>
#include <thread>
#include "ThreadPool.hpp"
#include <atomic>


using namespace std;


int main() {
    ThreadPool threadPool(10);
    mutex m;
    std::atomic<int> at = 0;
    for (int i = 0; i < 100000; ++i) {
        threadPool.AddTask([&a = at]() {
            for (int j = 0; j < 10000; ++j) {
                a++;
            }
        });

    }
    for (int i = 0; i < 20; ++i) {
        cout << at << endl;
        std::this_thread::sleep_for(std::chrono::microseconds (100));
    }
    std::this_thread::sleep_for(std::chrono::minutes(100));
    return 0;
}
