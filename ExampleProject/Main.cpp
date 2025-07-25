#include "Foo.h"
#include <limits>
#include <iostream>

int main()
{
    Foo();

    int x = std::numeric_limits<int>::max();
    x++;

    std::cout << x << std::endl;

    return 0;
}