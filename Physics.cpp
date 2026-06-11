//
// Created by MacBook on 13/04/2026.
//
#include "Physics.h"
#include <cmath>
#include <algorithm>

const int cellOffsets[9][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0}, {0,  0}, {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
};

float gravity = -9.8f;
float targetDensity = 1.5f;
float pressureMultiplier = 15.0f;
float nearPressureMultiplier = 18.0f;
float smoothingRadius = 0.14f;
float viscosityStrength = 0.02f;
float collisionDamping = 0.5f;
float interactionRadius = 0.6f;
float interactionStrength = -70.0f;
const float mass = 1.0f;


std::vector<Entry> spatialLookup;
std::vector<int> startIndices;

unsigned int HashCell(int cellX, int cellY) {
    return (unsigned int)(cellX * 15823) ^ (unsigned int)(cellY * 9737333);
}

// Ядро плотности для ближнего давления
//генерирует отталкивание когда ч находятся слишком близко друг к другу
float NearSmoothingKernel(float dst, float radius) {
    if (dst >= radius) return 0;
    float v = radius - dst;
    return (v * v * v) / ( (PI * std::pow(radius, 5.0f)) / 10.0f );
}

// Производная ядра для ближнего давления предотвращает проникновение
float NearSmoothingKernelDerivative(float dst, float radius) {
    if (dst >= radius) return 0;
    float v = dst - radius;
    float scale = 30.0f / (std::pow(radius, 5.0f) * PI);
    return v * v * scale;
}

void ResizePhysicsData(int numParticles) {
    spatialLookup.resize(numParticles);
    startIndices.assign(numParticles, -1);
}

//Преобразование позиции в пространстве в индекс ячейки сетки
void PositionToCellCoord(float x, float y, float radius, int& cellX, int& cellY) {
    cellX = (int)std::floor(x / radius);
    cellY = (int)std::floor(y / radius);
}

//строит сетку и определяет кто где находится
void UpdateSpatialLookup(const std::vector<Vec2>& positions, float radius) {
    int n = positions.size();
    spatialLookup.resize(n);
    startIndices.assign(n, -1);

#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        int cellX, cellY;
        PositionToCellCoord(positions[i].x, positions[i].y, radius, cellX, cellY);
        unsigned int key = GetKeyFromHash(HashCell(cellX, cellY), n);
        spatialLookup[i] = { i, key };
    }

    std::sort(spatialLookup.begin(), spatialLookup.end());

#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        unsigned int key = spatialLookup[i].cellKey;
        unsigned int keyPrev = (i == 0) ? -1 : spatialLookup[i - 1].cellKey;
        if (key != keyPrev) startIndices[key] = i;
    }
}

unsigned int GetKeyFromHash(unsigned int hash, int numParticles) {
    return hash % (unsigned int)numParticles;
}

//Сглаживание скорости
float ViscositySmoothingKernel(float dst, float radius) {
    if (dst >= radius) return 0;
    float volume = (PI * std::pow(radius, 4.0f)) / 6.0f;
    return (radius - dst) / volume;
}

//как быстро растет влияние с приближением
float SmoothingKernelDerivative(float dst, float radius) {
    if (dst >= radius) return 0;
    float scale = 12.0f / (std::pow(radius, 4.0f) * PI);
    return (dst - radius) * scale;
}

float CalculateDensity(int particleIndex, const std::vector<Vec2>& positions) {
    float density = 0;
    Vec2 pos = positions[particleIndex];

    int centreX, centreY;
    PositionToCellCoord(pos.x, pos.y, smoothingRadius, centreX, centreY);
//сумируем вклад соседей
    for (int j = 0; j < 9; j++) {
        unsigned int key = GetKeyFromHash(
                HashCell(centreX + cellOffsets[j][0], centreY + cellOffsets[j][1]),
                positions.size()
        );

        int cellStartIndex = startIndices[key];
        if (cellStartIndex == -1) continue;

        for (int i = cellStartIndex; i < (int)positions.size(); i++) {
            if (spatialLookup[i].cellKey != key) break;

            int neighborIndex = spatialLookup[i].particleIndex;

            float dx = positions[neighborIndex].x - pos.x;
            float dy = positions[neighborIndex].y - pos.y;
            float dst = std::sqrt(dx * dx + dy * dy);

            density += mass * SmoothingKernel(dst, smoothingRadius);
        }
    }

    return density;
}

void CalculateAllDensities(const std::vector<Vec2>& predictedPositions, std::vector<Particle>& particles) {
    int n = predictedPositions.size();

#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        float density = 0;
        float nearDensity = 0;
        Vec2 pos = predictedPositions[i];

        // Определяем координаты ячейки
        int centreX, centreY;
        PositionToCellCoord(pos.x, pos.y, smoothingRadius, centreX, centreY);

        // Обход 9 соседних ячеек (включая текущую)
        for (int j = 0; j < 9; j++) {
            unsigned int key = GetKeyFromHash(
                    HashCell(centreX + cellOffsets[j][0], centreY + cellOffsets[j][1]),
                    n
            );

            int cellStartIndex = startIndices[key];
            if (cellStartIndex == -1) continue;

            // Проход по частицам в данной ячейке
            for (int k = cellStartIndex; k < n; k++) {
                if (spatialLookup[k].cellKey != key) break;

                int neighborIndex = spatialLookup[k].particleIndex;

                float dx = predictedPositions[neighborIndex].x - pos.x;
                float dy = predictedPositions[neighborIndex].y - pos.y;
                float sqrDst = dx * dx + dy * dy;

                // Проверка радиуса взаимодействия
                if (sqrDst >= smoothingRadius * smoothingRadius) continue;

                float dst = std::sqrt(sqrDst);

                // ВАЖНО: Вычисляем обе плотности за один проход
                density += mass * SmoothingKernel(dst, smoothingRadius);
                nearDensity += mass * NearSmoothingKernel(dst, smoothingRadius);
            }
        }

        // Сохраняем результаты в структуру частицы
        particles[i].density = density;
        particles[i].nearDensity = nearDensity;
    }
}


//суммируетотталкивающий вектор от уплотненных областей
Vec2 CalculatePressureForce(int index, const std::vector<Particle>& particles, const std::vector<Vec2>& predictedPositions) {
    Vec2 force = {0, 0};
    Vec2 pos = predictedPositions[index];
    //Насколько сильно различаются давления и в каком направлении
    // давления для текущей частицы
    float p = (particles[index].density - targetDensity) * pressureMultiplier;
    float pNear = particles[index].nearDensity * nearPressureMultiplier;

    int centreX, centreY;
    PositionToCellCoord(pos.x, pos.y, smoothingRadius, centreX, centreY);

    // Проход по соседним ячейкам
    for (int i = 0; i < 9; i++) {
        unsigned int key = GetKeyFromHash(
                HashCell(centreX + cellOffsets[i][0], centreY + cellOffsets[i][1]),
                particles.size()
        );

        int cellStartIndex = startIndices[key];
        if (cellStartIndex == -1) continue;

        for (int j = cellStartIndex; j < (int)particles.size(); j++) {
            if (spatialLookup[j].cellKey != key) break;

            int neighborIndex = spatialLookup[j].particleIndex;
            if (neighborIndex == index) continue;

            float dx = predictedPositions[neighborIndex].x - pos.x;
            float dy = predictedPositions[neighborIndex].y - pos.y;
            float sqrDst = dx * dx + dy * dy;
            float dst = std::sqrt(sqrDst);

            if (dst >= smoothingRadius) continue;

            // Расчет направления силы
            Vec2 dir = (dst > 0) ? Vec2{dx / dst, dy / dst} : Vec2{0, 1};

            float neighborP = (particles[neighborIndex].density - targetDensity) * pressureMultiplier;
            float neighborPNear = particles[neighborIndex].nearDensity * nearPressureMultiplier;

            float sharedP = (p + neighborP) / 2.0f;
            float sharedPNear = (pNear + neighborPNear) / 2.0f;

            float slope = SmoothingKernelDerivative(dst, smoothingRadius);
            float slopeNear = NearSmoothingKernelDerivative(dst, smoothingRadius);

            // Итоговая величина силы
            // Делим на среднюю плотность обеих частиц для симметричности (третий закон Ньютона)
            float densitySelf = std::max(0.001f, particles[index].density);
            float densityNeighbor = std::max(0.001f, particles[neighborIndex].density);
            float avgDensity = (densitySelf + densityNeighbor) * 0.5f;
            float magnitude = (sharedP * slope + sharedPNear * slopeNear) * mass / avgDensity;

            force.x += dir.x * magnitude;
            force.y += dir.y * magnitude;
        }
    }

    return force;
}

float CalculateSharedPressure(float densityA, float densityB) {
    float pressureA = ConvertDensityToPressure(densityA);
    float pressureB = ConvertDensityToPressure(densityB);
    return (pressureA + pressureB) / 2.0f;
}

float ConvertDensityToPressure(float density) {
    float densityError = density - targetDensity;
    return densityError * pressureMultiplier;
}


float SmoothingKernel(float dst, float radius) {
    if (dst >= radius) return 0;
    float volume = (PI * std::pow(radius, 4.0f)) / 6.0f;
    float diff = radius - dst;
    return (diff * diff) / volume;
}//Насколько сосед влияет на текущую частичку

//Каждый сосед тянет текущую частицу в направлении
// своей скорости пропорционально близости
Vec2 CalculateViscosityForce(int particleIndex, const std::vector<Vec2>& positions, const std::vector<Particle>& particles) {
    Vec2 viscosityForce = {0, 0};
    Vec2 pos = positions[particleIndex];

    int centreX, centreY;
    PositionToCellCoord(pos.x, pos.y, smoothingRadius, centreX, centreY);

    for (int j = 0; j < 9; j++) {
        unsigned int key = GetKeyFromHash(
                HashCell(centreX + cellOffsets[j][0], centreY + cellOffsets[j][1]),
                positions.size()
        );

        int cellStartIndex = startIndices[key];
        if (cellStartIndex == -1) continue;

        for (int i = cellStartIndex; i < (int)positions.size(); i++) {
            if (spatialLookup[i].cellKey != key) break;

            int neighborIndex = spatialLookup[i].particleIndex;
            if (neighborIndex == particleIndex) continue;

            float dx = positions[neighborIndex].x - pos.x;
            float dy = positions[neighborIndex].y - pos.y;
            float dst = std::sqrt(dx * dx + dy * dy);

            if (dst >= smoothingRadius) continue;

            float influence = ViscositySmoothingKernel(dst, smoothingRadius);
            viscosityForce.x += (particles[neighborIndex].vx - particles[particleIndex].vx) * influence;
            viscosityForce.y += (particles[neighborIndex].vy - particles[particleIndex].vy) * influence;
        }
    }
    return {viscosityForce.x * viscosityStrength, viscosityForce.y * viscosityStrength};
}
//мышка
Vec2 CalculateInteractionForce(Vec2 mousePos, Vec2 particlePos, Vec2 velocity) {
    float dx = mousePos.x - particlePos.x;
    float dy = mousePos.y - particlePos.y;
    float dst = std::sqrt(dx * dx + dy * dy);

    if (dst >= interactionRadius || dst < 0.0001f) return { 0, 0 };

    Vec2 dirToMouse = { dx / dst, dy / dst };
    float t = 1.0f - dst / interactionRadius;
    float falloff = t * t;

    return {
            dirToMouse.x * interactionStrength * falloff,
            dirToMouse.y * interactionStrength * falloff
    };
}

void ResolveCollisions(Particle& p, float aspect) {
    float bx = aspect - p.radius;
    float by = 1.0f - p.radius;

    if (std::abs(p.x) > bx) {
        p.x = (p.x > 0 ? bx : -bx);
        p.vx *= -collisionDamping;
    }
    if (std::abs(p.y) > by) {
        p.y = (p.y > 0 ? by : -by);
        p.vy *= -collisionDamping;
    }
}