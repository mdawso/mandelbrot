#include "mandelbrot.hpp"

bool diverges(const Complex& c, int max_iterations)
{
    Complex z(0.0, 0.0);
    for (int i = 0; i < max_iterations; ++i) {
        z = z * z + c;
        if (std::norm(z) > 4.0) {
            return true;
        }
    }
    return false;
}


