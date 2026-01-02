#include <iostream>
#include "src/vector.hpp"

int main() {
    Vector a(1.0, 2.0, 3.0);
    Vector b(4.0, 5.0, 6.0);

    Vector c = a + b;
    std::cout << "a + b = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    c = a - b;
    std::cout << "a - b = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    c = a * 2.0;
    std::cout << "a * 2.0 = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    double d = a.dot(b);
    std::cout << "a . b = " << d << "\n";
    c = a.cross(b);
    std::cout << "a x b = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    d = a.length();
    std::cout << "|a| = " << d << "\n";

    c = a.normalize();
    std::cout << "normalize(a) = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    return 0;
}
