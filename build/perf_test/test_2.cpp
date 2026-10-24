#include <string.h>
#include <iostream>
#include <chrono>

int main() {
    auto p1 = std::chrono::high_resolution_clock::now();

    size_t total = 0;
    for (int j = 0; j < 100000; j++)
        for (int i = 0; i < 100000; ++i) {
            std::string x{ "x" };
            total += x.length();
        }

    auto p2 = std::chrono::high_resolution_clock::now();
    std::cout << "result= " << total
              << "   elapsed time:" << std::chrono::duration_cast<std::chrono::nanoseconds>(p2 - p1).count() << "ns"
              << std::endl;
    return 0;
}
