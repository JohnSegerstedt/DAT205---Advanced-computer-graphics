SDL_Event event;

// Various globals
SDL_Window* g_window = nullptr;
float currentTime = 0.0f;

// Shader programs
GLuint backgroundProgram, shaderProgram, postFxShader;

// Environment
float environment_multiplier = 1.0f;
GLuint environmentMap, irradianceMap, reflectionMap;
const std::string envmap_base_name = "001";

// Light source
float point_light_intensity_multiplier = 1000.0f;
vec3 point_light_color = vec3(1.f, 1.f, 1.f);
const vec3 lightPosition = vec3(20.0f, 40.0f, 0.0f);

// Camera parameters.
vec3 securityCamPos = vec3(70.0f, 50.0f, -70.0f);
vec3 securityCamDirection = normalize(-securityCamPos);
vec3 cameraPosition(-70.0f, 50.0f, 70.0f);
vec3 cameraDirection = normalize(vec3(0.0f) - cameraPosition);
vec3 worldUp(0.0f, 1.0f, 0.0f);

// World camera.
vec3 cameraOffset(50.0f, 20.0f, 0.0f);
vec3 cameraOffsetMinimum = cameraOffset * 0.5f;
vec3 cameraOffsetMaximum = cameraOffset * 5.f;
vec3 cameraDirectionOffsetDefault(-1.0f, -0.3f, 0.0f);
vec3 cameraDirectionOffset = cameraDirectionOffsetDefault;
vec3 cameraRight;
vec3 cameraUp;

// Ship Variables
mat4 shipModelMatrix = mat4(1.f);
float shipSpeedDefault = 200.f;
float shipSpeed = shipSpeedDefault;
float shipBoostSpeedDefault = shipSpeedDefault * 2.f;
float shipTurnSpeed = shipSpeedDefault / 100.f;
float shipStrafeSpeed = shipSpeedDefault / 2.f;
float shipUpSpeed = 5.f;
float cameraZoomSpeed = 2.f;
static mat4 T = mat4(1.0f);
static mat4 R = mat4(1.0f);


float Math_PI = 3.141592653589793;


struct PerspectiveParams {
	float fov;
	int w;
	int h;
	float near;
	float far;
};

int windowWidth = 1280;
int windowHeight = 720;
static PerspectiveParams pp = { 60.0f, windowWidth, windowHeight, 0.1f, 10000.0f };

// newer code 0 - Particle System
GLuint vertexArrayObject, basicVAO;
GLuint particleShaderProgram, exhaustShaderProgram;
GLuint positionBuffer;
const unsigned int maxNumberOfParticlesInAParticleSystem = 1000;
const unsigned int minNumberOfParticlesInAParticleSystem = 10;
const unsigned int numberOfParticleSystemsWhenParticlesHaveReachedFloor = 10;
std::vector<ParticleSystem> particleSystems;
vec4 data[maxNumberOfParticlesInAParticleSystem] = {};
GLuint explosionBillboard;

// Models
const std::string model_filename = "../scenes/NewShip.obj";
labhelper::Model *landingpadModel = nullptr;
labhelper::Model *fighterModel = nullptr;
labhelper::Model *sphereModel = nullptr;
labhelper::Model *cameraModel = nullptr;

// Projectile
#include "Projectile.h"
std::vector<Projectile> projectiles = {};
float projectileSpeed = 300.f;
float asteroidSpeed = 100.f;
bool asteroidsEnabled = true;
bool canFire = true;
bool canMove = true;
float shipSize = 10.f;
float shipScale = 0.05f;
bool shipRender = true;
vec3 shipDestructionColor = vec3(250.f, 50.f, 0.f);
bool spawnAsteroid = true;
float shipGunWaited = 0.f;
float shipGunCooldown = 0.3f;
float asteroidWaited = 0.f;
float asteroidCooldown = 2.f;
vec3 projectileColor = vec3(1.f, 0.f, 0.f);
vec3 asteroidColor = vec3(0.f, 0.f, 1.f);

bool playerCollision = true;
float asteroidSize = 15.f; // Hitbox diameter (default=15.f, large=200.f)
float projectileSize = 1.f;
float asteroidScale = asteroidSize * 0.75f; // Mesh diameter, for rendering (default=*0.75f)
float projectileScale = projectileSize * 1.25f;
float exhaustScale = 0.01f;

// GUI
float level = 1;
int asteroidsSpawnedThisLevel = 0;
int asteroidsDestroyedThisLevel = 0;
int maxAsteroidsThisLevel = 10.f;
float score = 0;
float ammo = 0;

float ammoPerSecond = 0.5f;
float ammoPerHit = 1.f;
float pointsPerSecond = 1.f;
float pointsPerHit = 10.f;
float pointsPerLevel = 100.f;

float asteroidFactorPerLevel = 1.2f;
float asteroidCooldownFactorPerLevel = 0.7f;
float asteroidCooldownMinimum = 0.1f;

// Width of pathable map = 800.f   ---   width of collision detection = 1000.f
float pathableWorldWidth = 750.f;
float pathableWorldHalfWidth = pathableWorldWidth * 0.5f;
float worldWidth = 1000.f;
float worldHalfWidth = worldWidth * 0.5f;

bool collisionHasHappened = false;
int asteroidsRemovedThisLevel = 0;

bool gameOver = false;

#define GRIDWIDTH 10					// The number of grid squares in width/height (default=10, small=5, large=15)
#define GRIDSIZE GRIDWIDTH * GRIDWIDTH	// The total number of grid squares
#define SQUARESIZE 100					// Number of collision objects allowed in a single grid