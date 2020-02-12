#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>
using namespace glm;

#ifndef PARTICLE_H
#define PARTICLE_H
struct Particle {
	float max_life;
	float current_life;
	vec3 velocity;
	vec3 position;
	Particle();
	Particle(float maxLife, float currentLife, vec3 vel, vec3 pos);
};
#endif // PARTICLE_H

#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H
class ParticleSystem {
private:
	int max_size;
	int current_size = 0;
	vec3 starting_point;
	bool is_exhaust = false;
	bool is_enabled = true;
	bool is_continous = true;
	bool is_sphere = false;
	float scale = 1.f;
	vec3 color;

public:
	// Members
	std::vector<Particle> particles;
	// Constructors
	ParticleSystem(void);
	ParticleSystem(float maxSize, float newScale);
	//ParticleSystem(float maxSize, vec3 startingPoint, bool isContinous, bool isSphere, float newScale);
	ParticleSystem(float maxSize, vec3 startingPoint, bool isContinous, bool isSphere, float newScale, vec3 color);
// Methods
	// -- Getters --
	int getMaxSize(void);
	int getSize(void);
	float getScale(void);
	vec3 getColor(void);
	vec3 getStartingPoint(void);
	bool isExhaust(void);
	bool isEnabled(void);
	bool isContinous(void);
	bool isSphere(void);

	// -- Setters --
	void setEnabled(bool isEnabled);

	// -- Particle --
	void kill(int id);
	void spawn(Particle particle);
	void process_particles(float dt);
};
#endif // PARTICLESYSTEM_H