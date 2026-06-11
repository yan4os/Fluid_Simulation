//
// Created by MacBook on 13/04/2026.
//

#ifndef FLUIDSIMULATION_COMMON_H
#define FLUIDSIMULATION_COMMON_H

#include <vector>

struct Vec2 {
    float x, y;
};

struct Particle
{
    float x, y;
    float vx, vy;
    float density;
    float nearDensity;
    float radius;
};

struct Entry {
    int particleIndex;
    unsigned int cellKey;

    bool operator<(const Entry& other) const {
        return cellKey < other.cellKey;
    }
};

const float PI = 3.1415926535f;

#endif //FLUIDSIMULATION_COMMON_H
