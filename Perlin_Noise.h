
#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <ctime>
#include <cmath>

class PerlinNoise {
    std::vector<int> p;
public:
    PerlinNoise();
    double noise(double x, double y) const;
private:
    static double fade(double t);
    static double lerp(double t, double a, double b);
    static double grad(int hash, double x, double y);
};

#endif // PERLIN_NOISE_H