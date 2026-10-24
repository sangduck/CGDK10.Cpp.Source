#include <stdio.h>
#include <iostream>
#include <chrono>

int func_10(int _param) { return _param * 2; }
int func_9(int _param) { return func_10(_param); }
int func_8(int _param) { return func_9(_param); }
int func_7(int _param) { return func_8(_param); }
int func_6(int _param) { return func_7(_param); }
int func_5(int _param) { return func_6(_param); }
int func_4(int _param) { return func_5(_param); }
int func_3(int _param) { return func_4(_param); }
int func_2(int _param) { return func_3(_param); }
int func_1(int _param) { return func_2(_param); }
int func_test(int _param){ return func_1(_param);}
int main() {
	std::chrono::high_resolution_clock::time_point p1 = std::chrono::high_resolution_clock::now();

	int total = 0;
	for (int j = 0; j < 100000; ++j)
		for (int i = 0; i < 100000; ++i)
			total += func_test(i);

	std::chrono::high_resolution_clock::time_point p2 = std::chrono::high_resolution_clock::now();
	std::cout << "result= " << total 
			  << "   elapsed time:" << std::chrono::duration_cast<std::chrono::milliseconds>(p2 - p1).count() << "ms"
			  << std::endl;
	return 0;
}

