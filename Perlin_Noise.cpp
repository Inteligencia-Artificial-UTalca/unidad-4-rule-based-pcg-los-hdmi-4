#include "perlin_noise.h"

PerlinNoise::PerlinNoise() {
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    std::shuffle(p.begin(), p.end(), std::default_random_engine(time(nullptr)));
    p.insert(p.end(), p.begin(), p.end());
}

double PerlinNoise::noise(double x, double y) const {
    int X = (int)floor(x) & 255, Y = (int)floor(y) & 255;
    x -= floor(x); y -= floor(y);
    double u = fade(x), v = fade(y);
    int A = p[X  ]+Y, AA = p[A], AB = p[A+1], B = p[X+1]+Y, BA = p[B], BB = p[B+1];
    return lerp(v, lerp(u, grad(p[AA], x, y), grad(p[BA], x-1, y)),
                   lerp(u, grad(p[AB], x, y-1), grad(p[BB], x-1, y-1)));
}

double PerlinNoise::fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
double PerlinNoise::lerp(double t, double a, double b) { return a + t * (b - a); }
double PerlinNoise::grad(int hash, double x, double y) {
    int h = hash & 3;
    double u = h<2 ? x : y, v = h<2 ? y : x;
    return ((h&1) ? -u : u) + ((h&2) ? -2.0*v : 2.0*v);
}