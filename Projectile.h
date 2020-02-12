#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>
using namespace glm;


#ifndef PROJECTILE_H
#define PROJECTILE_H
class Projectile {
	
private:
	// --- Variables ---
	labhelper::Model *model = nullptr;
	vec3 velocity = vec3(1.f);
	
	float max_range = 100.f;
	float bulletRange = 500.f;
	float asteroidRange = 1000.f;
	float distance_travelled = 0.f;
	bool is_asteroid = false;
	bool is_to_explode = false;
	bool is_to_be_removed = false;
	
	float size = 1.f;

	// --- Methods ---
	
	void setPosition(vec3 setPosition);
	void updatePosition(vec3 positionDelta);
	void scaleModelMatrix(float scale);
	mat3 getRotationMatrix(void);

public:

	bool collided = false;

	mat4 model_matrix = mat4(1.0f);

	// --- Constructors ---
	Projectile();
	Projectile(labhelper::Model *newModel, mat4 modelMatrix, vec3 vel, float diameter);
	Projectile(labhelper::Model *newModel, mat4 modelMatrix, vec3 vel, float diameter, bool is_Asteroid);

	// --- Methods ---
	float getSize(void); 
	vec3 getPosition(void);
	labhelper::Model* getModel(void);
	mat4 getModelMatrix(void);
	vec3 getVelocity(void);
	bool isToExplode(void);
	bool isToBeRemoved(void);
	bool isAsteroid(void);
	void update(float dt);
	void setDestruction(bool destroy);
	void remove(void);
};
#endif // PROJECTILE_H