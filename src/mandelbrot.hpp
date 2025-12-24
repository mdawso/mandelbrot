#pragma once

#include <complex>

using Complex = std::complex<long double>;

bool diverges(const Complex& c, int max_iterations);
