#include <labhelper.h>
#include <glm/glm.hpp>
#include "Projectile.h"

#pragma once
using namespace glm;

#ifndef MAIN
#define MAIN

void mouseInputs(SDL_Event event);
void keyInputs(float dt);

void processParticleSystem(float dt);
void processProjectiles(float dt);
void renderParticleSystem(mat4 viewMatrix);

void handleCooldowns(float dt);
void handleAsteroids(void);
void handleProjectileRemoval(void);
void shoot(void);
void onHit(void);
void completeLevel(void);

void handleCollisions(void);
int toGridPosition(float worldPosition);
void insertProjectileIntoGridsStatic(std::vector<std::vector<Projectile*>>& grids, int numberOfProjectiles[], Projectile& objectAddress);
std::vector<int> getShipGrids(void);
void handleProjectilesCollision(Projectile* projectile1, Projectile* projectile2);
void handlePlayerCollision(Projectile* projectile);

void onProjectilesCollision(Projectile* projectile1, Projectile* projectile2);
void onProjectileCollision(Projectile* projectile);
void onPlayerCollision(void);
void onPlayerDestruction(void);

// --- NEW ADVANCED COLLISION HANDLING ---
void insertProjectileIntoGridsDynamic(std::vector<std::vector<Projectile*>>& grids, int numberOfProjectiles[], Projectile& objectAddress);
bool isObjectInGrid(Projectile& object, int gridListIndex);
bool correctGridListIndex(int gridListIndex);
int getGridListIndex(int x, int z, int gridCircleIndex);
bool isNewCircleLevel(int oldCircleLevel, int newGrid);
int getCircleTraversalInfo(int x, int returnValue);
// -----------------------------------------

#endif // MAIN