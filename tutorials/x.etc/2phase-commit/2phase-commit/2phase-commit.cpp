#include <iostream>


class test
{
    int m_x{ 0 };
    int m)y{ 0 };

    void do_something(int _x, int _y);
};

void test::do_something(int _a, int _b)
{
    if (_a > 0)
    {
        this->m_x = _a / 10;
    }

    if (_y > 0)
    {
        this->m_y = _a * _b;
    }
}







int main()
{
    test foo;


    foo.do_something();













    std::cout << "Hello World!\n";
}
