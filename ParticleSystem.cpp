#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>

#include "ParticleSystem.h"

using namespace glm;

Particle::Particle() {}

Particle::Particle(float maxLife, float currentLife, vec3 vel, vec3 pos) {
	max_life = maxLife;
	current_life = currentLife;
	velocity = vel;
	position = pos;
}

ParticleSystem::ParticleSystem(void) {
}

ParticleSystem::ParticleSystem(float maxSize, float newScale) {
	max_size = maxSize;
	scale = newScale;
	is_exhaust = true;
}

/*
ParticleSystem::ParticleSystem(float maxSize, vec3 startingPoint, bool isContinous, bool isSphere) {
	max_size = maxSize;
	starting_point = startingPoint;
	is_continous = isContinous;
	is_sphere = isSphere;
}*/

ParticleSystem::ParticleSystem(float maxSize, vec3 startingPoint, bool isContinous, bool isSphere, float newScale, vec3 newColor) {
	max_size = maxSize;
	starting_point = startingPoint;
	is_continous = isContinous;
	is_sphere = isSphere;
	scale = newScale;
	color = newColor;
}

int ParticleSystem::getMaxSize(void) {
	return max_size;
}

int ParticleSystem::getSize(void) {
	return current_size;
}

vec3 ParticleSystem::getColor(void) {
	return color;
}

vec3 ParticleSystem::getStartingPoint(void) {
	return starting_point;
}

float ParticleSystem::getScale(void) {
	return scale;
}

bool ParticleSystem::isExhaust(void) {
	return is_exhaust;
}

bool ParticleSystem::isEnabled(void) {
	return is_enabled;
}

bool ParticleSystem::isContinous(void) {
	return is_continous;
}

bool ParticleSystem::isSphere(void) {
	return is_sphere;
}

void ParticleSystem::setEnabled(bool isEnabled) {
	is_enabled = isEnabled;
}

void ParticleSystem::kill(int id) {
	particles.erase(particles.begin() + id);
	current_size--;
}

void ParticleSystem::spawn(Particle particle) {
	particles.push_back(particle);
	current_size++;
}

void ParticleSystem::process_particles(float dt) {
	for (int i = 0; i < particles.size(); i++) {
		particles[i].current_life += dt;
		if (particles[i].current_life >= particles[i].max_life) kill(i);
		else particles[i].position += dt * particles[i].velocity;
	}
}