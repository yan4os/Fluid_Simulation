//
// Created by MacBook on 13/04/2026.
//

#ifndef FLUIDSIMULATION_PHYSICS_H
#define FLUIDSIMULATION_PHYSICS_H

#include "Physics.h"
#include "Common.h"

extern float gravity;
extern float targetDensity;
extern float pressureMultiplier;
extern float smoothingRadius;
extern float viscosityStrength;
extern float collisionDamping;
extern float interactionRadius;
extern float interactionStrength;

extern float nearPressureMultiplier;

// Функции
void UpdateSpatialLookup(const std::vector<Vec2>& positions, float radius);

float CalculateDensity(int particleIndex, const std::vector<Vec2>& positions);

Vec2 CalculatePressureForce(int index, const std::vector<Particle>& particles, const std::vector<Vec2>& predictedPositions);
Vec2 CalculateViscosityForce(int particleIndex, const std::vector<Vec2>& positions, const std::vector<Particle>& particles);

Vec2 CalculateInteractionForce(Vec2 mousePos, Vec2 particlePos, Vec2 velocity);

void ResolveCollisions(Particle& p, float aspect);

float SmoothingKernel(float dst, float radius);


float ConvertDensityToPressure(float density);

float CalculateSharedPressure(float densityA, float densityB);

unsigned int GetKeyFromHash(unsigned int hash, int numParticles);

void ResizePhysicsData(int numParticles);

float NearSmoothingKernel(float dst, float radius);

float NearSmoothingKernelDerivative(float dst, float radius);

void CalculateAllDensities(const std::vector<Vec2>& predictedPositions, std::vector<Particle>& particles);

float SmoothingKernelDerivative(float dst, float radius);

float ViscositySmoothingKernel(float dst, float radius);

void PositionToCellCoord(float x, float y, float radius, int& cellX, int& cellY);

unsigned int HashCell(int cellX, int cellY);

#endif //FLUIDSIMULATION_PHYSICS_H
