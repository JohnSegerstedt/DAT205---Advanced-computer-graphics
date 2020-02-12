#include <labhelper.h>
#include <Model.h>
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include "Projectile.h"
using namespace glm;

// --- Constructors ---
Projectile::Projectile() {}

Projectile::Projectile(labhelper::Model *newModel, mat4 modelMatrix , vec3 vel, float diameter) {
	model = newModel;
	model_matrix = modelMatrix;
	velocity = vel;
	size = diameter;
}

Projectile::Projectile(labhelper::Model *newModel, mat4 modelMatrix, vec3 vel, float diameter, bool isAsteroid) {
	model = newModel;
	model_matrix = modelMatrix;
	velocity = vel;
	size = diameter;
	is_asteroid = isAsteroid;
	max_range = isAsteroid ? asteroidRange : bulletRange;
}

// --- Methods ---
float Projectile::getSize(void) {
	return size;
}

labhelper::Model* Projectile::getModel(void) {
	return model;
}

mat4 Projectile::getModelMatrix(void) {
	return model_matrix;
}

vec3 Projectile::getVelocity(void) {
	return velocity;
}

void Projectile::update(float dt) {
	updatePosition(dt*getVelocity());
}

vec3 Projectile::getPosition(void) {
	return inverse(getRotationMatrix()) * vec3(model_matrix[3]);
}

mat3 Projectile::getRotationMatrix(void) {
	return mat3(model_matrix);
}

void Projectile::setPosition(vec3 newPosition) {
	model_matrix[3][0] = newPosition.x;
	model_matrix[3][1] = newPosition.y;
	model_matrix[3][2] = newPosition.z;
}

void Projectile::scaleModelMatrix(float scale) {
	mat4 scalationMatrix = mat4(1.f / scale);
	scalationMatrix[3][3] = 1.f;
	model_matrix = model_matrix * scalationMatrix;
}

void Projectile::updatePosition(vec3 positionDelta) {
	distance_travelled += length(positionDelta);
	model_matrix[3] += vec4(positionDelta, 0.f);
	if (distance_travelled > max_range) is_to_explode = true;
}

bool Projectile::isToExplode(void) {
	return is_to_explode;
}

bool Projectile::isToBeRemoved(void) {
	return is_to_be_removed;
}


bool Projectile::isAsteroid(void) {
	return is_asteroid;
}

void Projectile::setDestruction(bool explode) {
	is_to_explode = explode;
}

void Projectile::remove(void) {
	is_to_be_removed = true;
}
