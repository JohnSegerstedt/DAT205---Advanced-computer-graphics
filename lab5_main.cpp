#include <labhelper.h>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <stb_image.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <iostream>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <Model.h>
#include <list>
#include "hdr.h"

#include "lab5_main.h"
#include "ParticleSystem.h"
#include "variables.h"
#include "Projectile.h"

/*

// --- ASK --- //
1. Passande sätt att få med Ship i kollisionshanteringen? Polymorfism?
2. Måste man ladda om .OBJ-en för varje objekt? --> Pool av modeller?

// --- TODO --- //
implement advanced collision detection w/Ship
implement Ship class?

*/

using namespace glm;
using std::min;
using std::max;

// Framebuffers
struct FboInfo;
std::vector<FboInfo> fboList;
struct FboInfo {
	GLuint framebufferId;
	GLuint colorTextureTarget;
	GLuint depthBuffer;
	int width;
	int height;
	bool isComplete;

	FboInfo(int w, int h) {
		isComplete = false;
		width = w;
		height = h;
		// Generate two textures and set filter parameters (no storage allocated yet)
		glGenTextures(1, &colorTextureTarget);
		glBindTexture(GL_TEXTURE_2D, colorTextureTarget);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenTextures(1, &depthBuffer);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// allocate storage for textures
		resize(width, height);

		///////////////////////////////////////////////////////////////////////
		// Generate and bind framebuffer
		///////////////////////////////////////////////////////////////////////
		// >>> @task 1
		glGenFramebuffers(1, &framebufferId);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

		// bind the texture as color attachment 0 (to the currently bound framebuffer)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureTarget, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		// bind the texture as depth attachment (to the currently bound framebuffer)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

		// check if framebuffer is complete
		isComplete = checkFramebufferComplete();

		// bind default framebuffer, just in case.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// if no resolution provided
	FboInfo() : isComplete(false), framebufferId(UINT32_MAX), colorTextureTarget(UINT32_MAX), depthBuffer(UINT32_MAX), width(0), height(0){};

	void resize(int w, int h) {
		width = w;
		height = h;
		// Allocate a texture
		glBindTexture(GL_TEXTURE_2D, colorTextureTarget);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		// generate a depth texture
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}

	bool checkFramebufferComplete(void) {
		// Check that our FBO is correctly set up, this can fail if we have
		// incompatible formats in a buffer, or for example if we specify an
		// invalid drawbuffer, among things.
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			labhelper::fatal_error("Framebuffer not complete");
		}

		return (status == GL_FRAMEBUFFER_COMPLETE);
	}
};


void initGL() {
	// enable Z-buffering
	glEnable(GL_DEPTH_TEST);

	// enable backface culling
	glEnable(GL_CULL_FACE);

	// Load some models.
	landingpadModel = labhelper::loadModelFromOBJ("../scenes/landingpad.obj");
	//cameraModel = labhelper::loadModelFromOBJ("../scenes/camera.obj");
	fighterModel = labhelper::loadModelFromOBJ("../scenes/NewShip.obj");

	// load and set up default shader
	backgroundProgram = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/background.vert", "../lab5-rendertotexture/shaders/background.frag");
	shaderProgram = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/simple.vert", "../lab5-rendertotexture/shaders/simple.frag");
	postFxShader = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/postFx.vert", "../lab5-rendertotexture/shaders/postFx.frag");
	exhaustShaderProgram = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/particle.vert", "../lab5-rendertotexture/shaders/particle.frag");
	particleShaderProgram = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/basicVertexShader.vert", "../lab5-rendertotexture/shaders/basicFragmentShader.frag");

	// Load environment map
	const int roughnesses = 8;
	std::vector<std::string> filenames;
	for (int i = 0; i < roughnesses; i++)
		filenames.push_back("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");

	reflectionMap = labhelper::loadHdrMipmapTexture(filenames);
	environmentMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + ".hdr");
	irradianceMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");
	sphereModel = labhelper::loadModelFromOBJ("../scenes/sphere.obj");

	// EXPLOSION BILLBOARD FOR PARTICLE SYSTEM
	int wi, he, comp;
	unsigned char* image = stbi_load("../lab5-rendertotexture/explosion.png", &wi, &he, &comp, STBI_rgb_alpha);
	glGenTextures(1, &explosionBillboard);
	glBindTexture(GL_TEXTURE_2D, explosionBillboard);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wi, he, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	free(image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Setup Framebuffers
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);
	const int numFbos = 5;
	for (int i = 0; i < numFbos; i++)
		fboList.push_back(FboInfo(w, h));

	// newer code 1 - BINDING BUFFERS AND SHADERS
	const float positions[] = {
		//	 X      Y     Z
		-0.5f,   -0.5f, 0.0f,		// v0
		0.5f,  -0.5f, 0.0f,			// v1
		0.0f,  0.5f, 0.0f,			// v2
	};

	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer); // Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, 100000, nullptr, GL_STATIC_DRAW); // Send the vertex position data to the current buffer
	glGenVertexArrays(1, &basicVAO); 	// Bind the vertex array object - The following calls will affect this vertex array object.
	glBindVertexArray(basicVAO);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer); // Makes positionBuffer the current array buffer for subsequent calls.
	glVertexAttribPointer(0, 4 /*vec4*/, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/); 	// Attaches positionBuffer to vertexArrayObject, in the 0th attribute location
	glEnableVertexAttribArray(0); // Enable the vertex position attribute
}

void drawScene(const mat4 &view, const mat4 &projection)
{
	glUseProgram(backgroundProgram);
	labhelper::setUniformSlow(backgroundProgram, "environment_multiplier", environment_multiplier);
	labhelper::setUniformSlow(backgroundProgram, "inv_PV", inverse(projection * view));
	labhelper::setUniformSlow(backgroundProgram, "camera_pos", cameraPosition);
	labhelper::drawFullScreenQuad();

	glUseProgram(shaderProgram);
	// Light source
	vec4 viewSpaceLightPosition = view * vec4(lightPosition, 1.0f);
	labhelper::setUniformSlow(shaderProgram, "point_light_color", point_light_color);
	labhelper::setUniformSlow(shaderProgram, "point_light_intensity_multiplier", point_light_intensity_multiplier);
	labhelper::setUniformSlow(shaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));

	// Environment
	labhelper::setUniformSlow(shaderProgram, "environment_multiplier", environment_multiplier);

	// camera
	labhelper::setUniformSlow(shaderProgram, "viewInverse", inverse(view));

	// landing pad 
	mat4 modelMatrix(1.0f);
	modelMatrix[3] += vec4(0.0f, -30.0f, 0.0f, 0.0f);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", (projection * view * modelMatrix) * scale(vec3(10.f)));
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", view * modelMatrix);
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * modelMatrix)));
	labhelper::render(landingpadModel);

	// Fighter
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", (projection * view * shipModelMatrix));
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", view * shipModelMatrix);
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * shipModelMatrix)));
	if(shipRender) labhelper::render(fighterModel);

	// todo look at this code - What is it for?
	glUseProgram(shaderProgram);
	labhelper::setUniformSlow(shaderProgram, "viewProjectionMatrix", projection * view);

	// newer code 2
	glUseProgram(exhaustShaderProgram);
	labhelper::setUniformSlow(exhaustShaderProgram, "projectionMatrix", projection);
	labhelper::setUniformSlow(exhaustShaderProgram, "screen_x", float(windowWidth));
	labhelper::setUniformSlow(exhaustShaderProgram, "screen_y", float(windowHeight));

	glUseProgram(particleShaderProgram);
	labhelper::setUniformSlow(particleShaderProgram, "projectionMatrix", projection);

	glUseProgram(0);
}

void display(){
	// Check if any framebuffer needs to be resized
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);

	for (int i = 0; i < fboList.size(); i++) {
		if (fboList[i].width != w || fboList[i].height != h)
			fboList[i].resize(w, h);
	}

	// setup matrices
	mat4 securityCamViewMatrix = lookAt(securityCamPos, securityCamPos + securityCamDirection, worldUp);
	mat4 securityCamProjectionMatrix = perspective(radians(30.0f), float(w) / float(h), 15.0f, 1000.0f);
	//mat4 projectionMatrix = perspective(radians(45.0f), float(w) / float(h), 10.0f, 1000.0f);

	cameraRight = normalize(cross(cameraDirection, worldUp));
	cameraUp = normalize(cross(cameraRight, cameraDirection));
	mat3 cameraBaseVectorsWorldSpace(cameraRight, cameraUp, -cameraDirection);

	mat4 cameraRotation = mat4(transpose(cameraBaseVectorsWorldSpace));
	mat4 viewMatrix = cameraRotation * translate(-cameraPosition);;
	mat4 projectionMatrix = perspective(radians(pp.fov), float(pp.w) / float(pp.h), pp.near, pp.far);
	//mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraDirection, worldUp);


	// Bind the environment map(s) to unused texture units
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);

	// draw scene from security camera - @task 2
	FboInfo &securityFB0 = fboList[0]; // normal camera
	FboInfo &securityFB1 = fboList[1]; // security camera

	glBindFramebuffer(GL_FRAMEBUFFER, securityFB1.framebufferId);

	glViewport(0, 0, securityFB1.width, securityFB1.height);
	glClearColor(0.2, 0.2, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene(securityCamViewMatrix, securityCamProjectionMatrix); // using both shaderProgram and backgroundProgram

	// draw scene from camera
	glBindFramebuffer(GL_FRAMEBUFFER, securityFB0.framebufferId); // to be replaced with another framebuffer when doing post processing
	glViewport(0, 0, w, h);
	glClearColor(0.2, 0.2, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene(viewMatrix, projectionMatrix); // using both shaderProgram and backgroundProgram

	///////// TV SCREEN - SECURITY CAMERA
	labhelper::Material &screen = landingpadModel->m_materials[8];
	screen.m_emission_texture.gl_id = securityFB1.colorTextureTarget;

	// projectiles
	glUseProgram(shaderProgram);

	vec3 scaleVec = vec3(1.f);
	for (Projectile projectile : projectiles) {
		if (projectile.isAsteroid()) scaleVec = vec3(asteroidScale);
		else scaleVec = vec3(projectileScale);
		labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * inverse(projectile.getModelMatrix()) * scale(scaleVec));
		labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", viewMatrix * inverse(projectile.getModelMatrix()));
		labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(viewMatrix * inverse(projectile.getModelMatrix()))));
		labhelper::render(projectile.getModel());
	} 

	glUseProgram(0);


	renderParticleSystem(viewMatrix); // newer code 3


	// Post processing pass(es)
	// task 3 - WHAT WE ARE SEEING (NOW CAMERA 0 - NORMAL)
	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, securityFB0.colorTextureTarget);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(postFxShader);
	labhelper::drawFullScreenQuad();

}

bool handleEvents(double dt){
	
	handleCooldowns(dt);
	handleAsteroids();
	handleCollisions();
	keyInputs(float(dt));
	processParticleSystem(float(dt));
	processProjectiles(float(dt));
	handleProjectileRemoval();

	if (asteroidsDestroyedThisLevel >= maxAsteroidsThisLevel) completeLevel();


	// check events (keyboard among other)
	SDL_Event event;
	bool quitEvent = false;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)) quitEvent = true;
		ImGui_ImplSdlGL3_ProcessEvent(&event); // Allow ImGui to capture events.
		mouseInputs(event);
	}

	return quitEvent;
}

void gui() {
	// Inform imgui of new frame
	ImGui_ImplSdlGL3_NewFrame(g_window);
	ImVec2 guiSize = ImVec2(40.f, 40.f);

	ImGui::Text("Level: %.0f\n", level);
	ImGui::Text("Score: %.0f\n", score);
	ImGui::Text("Ammo: %.0f\n", ammo);


	ImGui::SetNextWindowSize(guiSize, 0);

	// Render the GUI.
	ImGui::Render();
}

int main(int argc, char *argv[]) {
	g_window = labhelper::init_window_SDL("OpenGL Lab 5");

	particleSystems.push_back(ParticleSystem(1000, exhaustScale)); // ExhaustParticleSystem


	initGL();

	bool stopRendering = false;
	auto startTime = std::chrono::system_clock::now();

	double oldTime = 0.0f;
	double newTime = 0.01f;

	while (!stopRendering) {
		std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
		currentTime = timeSinceStart.count(); //update currentTime
		display(); // render to window
		gui(); // Render overlay GUI.
		SDL_GL_SwapWindow(g_window); // Swap front and back buffer. This frame will now been displayed.
		oldTime = newTime;
		newTime = currentTime;
		if (newTime - oldTime > 1.f) std::cout << "----------INVALID DELTATIME: " << newTime - oldTime << "!----------\n";
		stopRendering = handleEvents(newTime - oldTime); // check events (keyboard among other)
		//std::cout << ".";
	}

	// Free Models
	labhelper::freeModel(landingpadModel);
	//labhelper::freeModel(cameraModel);
	labhelper::freeModel(fighterModel);
	labhelper::freeModel(sphereModel);
	
	//for (Projectile projectile : projectiles) labhelper::freeModel(projectile.getModel());

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}

void handleCooldowns(float dt) {
	if (!gameOver) {
		shipGunWaited += dt;
		asteroidWaited += dt;
		if (shipGunWaited > shipGunCooldown) {
			canFire = true;
			//shipGunWaited = 0;
		}
		if (asteroidWaited > asteroidCooldown) {
			spawnAsteroid = true;
			asteroidWaited = 0;
		}
		score += (dt * pointsPerSecond);
		ammo += (dt * ammoPerSecond);
	}
}

void handleAsteroids(void) {
	if (spawnAsteroid && asteroidsEnabled) {
		spawnAsteroid = false;

		if (asteroidsSpawnedThisLevel++ >= maxAsteroidsThisLevel) return;
		std::cout << "   -    Spawned: " << asteroidsSpawnedThisLevel << "/" << maxAsteroidsThisLevel << "\n";
		labhelper::Model *tmpModel = sphereModel;

		double theta = labhelper::uniform_randf(0.f, 2.f * M_PI);
		vec3 projectilePosition = glm::vec3(cos(theta), 0.f, sin(theta));
		float spawnScale = 500.f;
		projectilePosition *= spawnScale;

		mat4 projectileMatrix = mat4(1.f);
		projectileMatrix[3][0] = projectilePosition.x;
		projectileMatrix[3][1] = projectilePosition.y;
		projectileMatrix[3][2] = projectilePosition.z;
		vec3 shipPosition = vec3(-shipModelMatrix[3][0], -shipModelMatrix[3][1], -shipModelMatrix[3][2]);
		vec3 projectileVelocity = asteroidSpeed * vec4(normalize(shipPosition - projectilePosition), 1.f);


		Projectile asteroid = Projectile(tmpModel, projectileMatrix, projectileVelocity, asteroidSize, true);
		projectiles.push_back(asteroid);
	}
}

void mouseInputs(SDL_Event event) {
	if (event.type == SDL_MOUSEMOTION && !ImGui::IsMouseHoveringAnyWindow()) {
		// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
		static int prev_xcoord = event.motion.x;
		static int prev_ycoord = event.motion.y;
		int delta_x = event.motion.x - prev_xcoord;
		int delta_y = event.motion.y - prev_ycoord;
		if (event.button.button & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			float rotationSpeed = 0.005f;
			mat4 yaw = rotate(rotationSpeed * -delta_x, worldUp);
			mat4 pitch = rotate(rotationSpeed * -delta_y, normalize(cross(cameraDirectionOffset, worldUp)));
			cameraDirectionOffset = vec3(pitch * yaw * vec4(cameraDirectionOffset, 0.0f));
			printf("Mouse motion while left button down (%i, %i)\n", event.motion.x, event.motion.y);
		}
		prev_xcoord = event.motion.x;
		prev_ycoord = event.motion.y;
	}
}

void keyInputs(float dt) {
	const uint8_t *state = SDL_GetKeyboardState(nullptr);
	// --- CAMERA ---
	if (state[SDL_SCANCODE_UP]) cameraOffset /= (1.f + (dt * cameraZoomSpeed));
	if (state[SDL_SCANCODE_DOWN]) cameraOffset *= (1.f + (dt * cameraZoomSpeed));
	if (length(cameraOffset) < length(cameraOffsetMinimum)) cameraOffset = cameraOffsetMinimum;
	if (length(cameraOffset) > length(cameraOffsetMaximum)) cameraOffset = cameraOffsetMaximum;

	if (canMove) {
		// --- MOVEMENT ---
		if (state[SDL_SCANCODE_LSHIFT]) shipSpeed = dt * shipBoostSpeedDefault;
		else shipSpeed = dt * shipSpeedDefault;
		if (state[SDL_SCANCODE_W]) T[3] -= shipSpeed * R[0];
		if (state[SDL_SCANCODE_S]) T[3] += shipSpeed * R[0];
		if (state[SDL_SCANCODE_A]) T[3] += dt * shipStrafeSpeed * R[2];
		if (state[SDL_SCANCODE_D]) T[3] -= dt * shipStrafeSpeed * R[2];
		if (length(T[3]) > pathableWorldHalfWidth) T[3] = vec4(vec3(normalize(T[3]) * pathableWorldHalfWidth), 1.f);
		if (length(T[3]) < -pathableWorldHalfWidth) T[3] = vec4(vec3(normalize(T[3]) * -pathableWorldHalfWidth), 1.f);

		// --- ROTATION ---
		if (state[SDL_SCANCODE_Q]) R[0] -= dt * shipTurnSpeed * R[2];
		if (state[SDL_SCANCODE_E]) R[0] += dt * shipTurnSpeed * R[2];

		// --- SHOOT ---
		if (state[SDL_SCANCODE_SPACE]) shoot();
	}

	// --- UPDATE ---
	R[0] = normalize(R[0]);
	R[2] = vec4(cross(vec3(R[0]), vec3(R[1])), 0.0f);
	cameraPosition = vec3(T[3]) + mat3(R) * cameraOffset;
	cameraDirection = normalize(mat3(R) * cameraDirectionOffset);
		

	shipModelMatrix = T * R;
}

void shoot(void) {
	if (canFire && ammo > 1.f) {
		ammo--;
		canFire = false;
		shipGunWaited = 0;

		labhelper::Model *tmpModel = sphereModel;

		mat4 projectileMatrix = mat4(1.f);
		projectileMatrix[3] = -shipModelMatrix[3];
		projectileMatrix[3][3] = 1.f;
		mat4 rotationMatrix = R;
		float projectileRotationOffset = 45.f;
		rotationMatrix[0] += projectileRotationOffset * R[2];
		rotationMatrix[0] = normalize(rotationMatrix[0]);
		rotationMatrix[2] = vec4(cross(vec3(rotationMatrix[0]), vec3(rotationMatrix[1])), 0.0f);
		vec3 projectileVelocity = projectileSpeed * vec3(0.f, 0.f, -1.f);

		Projectile newProjectile = Projectile(tmpModel, inverse(rotationMatrix) * projectileMatrix, projectileVelocity, projectileSize, false);
		projectiles.push_back(newProjectile);
	}
}

void onHit(void) {
	score += pointsPerHit;
	ammo += ammoPerHit;
}

void processParticleSystem(float dt) {
	int indicesOfParticleSystemsToBeRemoved[100] = {};
	int pointerArrayToBeRemoved = 0;
	int index = -1;
	for (ParticleSystem& particleSystem : particleSystems) {
		index++;

		// --- Processing (and Killing) Particles ---
		particleSystem.process_particles(dt);

		if (particleSystem.isEnabled() || particleSystem.isContinous()) {
			// --- Spawning Particles ---
			const float scaleVelocity = 20.0f;
			const float scalePosition = particleSystem.getScale() * 0.5f;
			//const float scalePosition = 0.1f;
			vec3 shipExhaustOffset = vec3(13.0f, 1.0f, .0f);
			vec3 particleStartPoint;
			mat3 rotationMatrix;
			if (particleSystem.isExhaust() && shipRender) {
				particleStartPoint = shipModelMatrix[3];
				rotationMatrix = shipModelMatrix;
				particleStartPoint += (rotationMatrix * shipExhaustOffset);
			}else{
				particleStartPoint = particleSystem.getStartingPoint();
				rotationMatrix = mat4(1.f);
			}


			for (int i = 0; i < particleSystem.getMaxSize() - particleSystem.getSize(); i++) {
				double theta = labhelper::uniform_randf(0.f, 2.f * M_PI);
				double u = 0.0f;
				glm::vec3 pos = vec3(1.0f);



				float maxTime = labhelper::uniform_randf(1.5f, 2.0f);
				float currentTime = labhelper::uniform_randf(0.0f, maxTime);

				if (particleSystem.isSphere()) {
					u = labhelper::uniform_randf(-1.f, 1.f);
					pos = glm::vec3(sqrt(1.f - u * u) * cosf(theta), u, sqrt(1.f - u * u) * sinf(theta));
					pos *= scaleVelocity;
					maxTime *= 2.f;
				}else{
					u = labhelper::uniform_randf(0.95f, 1.f);
					pos = glm::vec3(labhelper::uniform_randf(.5f, 10.f), labhelper::uniform_randf(.1f, .2f), labhelper::uniform_randf(.1f, .2f));
				}

				pos = rotationMatrix * pos;
				particleStartPoint.x += pos.x * scalePosition;
				particleStartPoint.y += pos.y * scalePosition;
				particleStartPoint.z += pos.z * scalePosition;
				//particleStartPoint.x += labhelper::uniform_randf(-1.f, 1.f) * scalePosition;
				//particleStartPoint.y += labhelper::uniform_randf(-1.f, 1.f) * scalePosition;
				//particleStartPoint.z += labhelper::uniform_randf(-1.f, 1.f) * scalePosition;

				Particle spawnedParticle = Particle(maxTime, currentTime, pos, particleStartPoint);
				particleSystem.spawn(spawnedParticle);
			}
			if (!particleSystem.isContinous()) particleSystem.setEnabled(false);
		}
		if (particleSystem.getSize() == 0) indicesOfParticleSystemsToBeRemoved[pointerArrayToBeRemoved++] = index;
	}
	for (int i = (pointerArrayToBeRemoved-1); i >= 0; i--) particleSystems.erase(particleSystems.begin() + indicesOfParticleSystemsToBeRemoved[i]);
}

void processProjectiles(float dt) {
	for (Projectile& projectile : projectiles) {
		projectile.update(dt);
		if (projectile.isToExplode()) {
			mat4 projectileMatrix = projectile.getModelMatrix();
			float scale = projectile.isAsteroid() ? asteroidScale : projectileScale;
			vec3 color = projectile.isAsteroid() ? asteroidColor : projectileColor;
			asteroidsDestroyedThisLevel += projectile.isAsteroid() ? 1.0 : 0.f;

			// Fewer particles in a ParticleSystem if there are many Asteroids
			float a = maxNumberOfParticlesInAParticleSystem;
			float b = minNumberOfParticlesInAParticleSystem;
			float c = 1.f / (numberOfParticleSystemsWhenParticlesHaveReachedFloor);
			float d = particleSystems.size();
			int numberOfParticles = (d == 0) ? a : b + (a - b) * glm::max((1.f - c*d), 0.f);
			
			particleSystems.push_back(ParticleSystem(numberOfParticles, inverse(projectileMatrix)[3], false, true, scale * 0.01f, color));
			projectile.remove();
			if(projectile.isAsteroid()) std::cout << "Exploded: " << asteroidsDestroyedThisLevel << "/" << maxAsteroidsThisLevel << "\n";
		}
	}
}

void renderParticleSystem(mat4 viewMatrix) {
	for (ParticleSystem& particleSystem : particleSystems) {
		vec4 data[maxNumberOfParticlesInAParticleSystem] = {};
		std::vector<glm::vec4> stdData;

		mat4 basicShaderModel = mat4(1.0f);
		basicShaderModel[3] += vec4(0.0f, 0.0f, 0.0f, 0.0f);

		// --- Fetching Data ---
		std::vector<Particle> particles = particleSystem.particles;
		for (int i = 0; i < particleSystem.getSize(); i++) {
			//stdData.push_back((glm::vec4(particles[i].position, particles[i].current_life / particles[i].max_life)));
			glm::vec4 particlePositions = glm::vec4(particles[i].position, 1.f);
			glm::vec3 transformedPositions = viewMatrix * basicShaderModel * particlePositions;
			stdData.push_back(glm::vec4(transformedPositions, particles[i].current_life / particles[i].max_life));
		}

		std::sort(stdData.begin(), std::next(stdData.begin(), particleSystem.getSize()),
			[](const vec4 &lhs, const vec4 &rhs) { return lhs.z > rhs.z; });

		// todo optimize this
		for (int i = 0; i < particleSystem.getSize(); i++) {
			data[i] = stdData.back();
			stdData.pop_back();
		}


		// --- Rendering ---
		glDisable(GL_CULL_FACE);
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		// Send the vertex position data to the current buffer
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);

		// Shader Program
		if (particleSystem.isExhaust()) {
			glUseProgram(exhaustShaderProgram);	// Set the shader program to use for this draw call
			glBindVertexArray(basicVAO);		// Bind the vertex array object that contains all the vertex data.
			glEnable(GL_PROGRAM_POINT_SIZE); 	// Enable shader program point size modulation.
			glEnable(GL_BLEND); 				// Enable blending.
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, explosionBillboard);
			glDepthMask(GL_FALSE);
		} else {
			glUseProgram(particleShaderProgram);// Set the shader program to use for this draw call
			labhelper::setUniformSlow(particleShaderProgram, "particleColor", particleSystem.getColor());
			glBindVertexArray(basicVAO);		// Bind the vertex array object that contains all the vertex data.
		}
		glDrawArrays(GL_POINTS, 0, particleSystem.getSize());
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glUseProgram(0);
	}
}

void completeLevel(void) {
	if(!gameOver){
		std::cout << "---level completed---\n";
		level++;
		asteroidsSpawnedThisLevel = 0;
		asteroidsDestroyedThisLevel = 0;
		asteroidsRemovedThisLevel = 0;
		score += pointsPerLevel;
		maxAsteroidsThisLevel = int(glm::max(maxAsteroidsThisLevel * asteroidFactorPerLevel, float(maxAsteroidsThisLevel + 1)));
		if (asteroidCooldown - 0.01f < asteroidCooldownMinimum) asteroidCooldown = asteroidCooldownMinimum;
		else asteroidCooldown = glm::min(asteroidCooldown * asteroidCooldownFactorPerLevel, float(asteroidCooldown - 0.01f));
		
	}
}

void handleCollisions(void) {
	std::vector<std::vector<Projectile*>> grids(GRIDSIZE, std::vector<Projectile*>(SQUARESIZE));
	int numberOfProjectiles[GRIDSIZE] = { 0 };

	for (Projectile& projectile : projectiles)	insertProjectileIntoGridsDynamic(grids, numberOfProjectiles, projectile); // Checks dynamic grid structure
	//for (Projectile& projectile : projectiles)	insertProjectileIntoGridsStatic(grids, numberOfProjectiles, projectile); // Checks 3x3 grid structure

	// Projectile - Projectile Collision
	for (int i = 0; i < GRIDSIZE; i++) {
		if (numberOfProjectiles[i] > 1) {
			for (int index1 = 0; index1 < numberOfProjectiles[i]; index1++) {
				for (int index2 = 0; index2 < numberOfProjectiles[i]; index2++) {
					if (index1 != index2 && !grids[i][index1]->isToBeRemoved() && !grids[i][index2]->isToBeRemoved()) {
						handleProjectilesCollision(grids[i][index1], grids[i][index2]);
					}
				}
			}
		}
	}

	// Player - Projectile Collision
	for (int grid : getShipGrids()) {
		for (int projectileIndex = 0; projectileIndex < numberOfProjectiles[grid]; projectileIndex++) {
			if (!grids[grid][projectileIndex]->isToBeRemoved()) {
				handlePlayerCollision(grids[grid][projectileIndex]);
			}
		}
	}
}


// Takes all projectiles and inserts them into the grids[] system
void insertProjectileIntoGridsDynamic(std::vector<std::vector<Projectile*>>& grids, int numberOfProjectiles[], Projectile& object) {

	vec3 position = object.getPosition();
	float x = toGridPosition(position.x);
	float z = toGridPosition(position.z);

	// Insert object into the grid it is most centered in
	int centerGrid = int(x * GRIDWIDTH + z);
	grids[centerGrid][numberOfProjectiles[centerGrid]++] = &object;

	// Advanced cases, handles if object also is within adjacent grid squares
	int gridCircleIndex = -1;
	int gridListIndex = 0;
	bool insertedIntoGridInThisCircleLevel = false;

	// Handle each adjacent grid going in circles around the centerGrid
	for (int circleLevel = 1; circleLevel < sqrt(GRIDSIZE) - 1; circleLevel++) {				// Loop through each level of circles
		insertedIntoGridInThisCircleLevel = false;
		while (!isNewCircleLevel(circleLevel, ++gridCircleIndex)) {								// Loop through each grid in current circle level
			gridListIndex = getGridListIndex(x, z, gridCircleIndex);										// Get index of this grid in grids[]
			if (correctGridListIndex(gridListIndex) && isObjectInGrid(object, gridListIndex)){	// If this object is in this grid...
				grids[gridListIndex][numberOfProjectiles[gridListIndex]++] = &object;			// Insert the object into grids[]
				insertedIntoGridInThisCircleLevel = true;										
			}
		}
		if (!insertedIntoGridInThisCircleLevel) return;	// If no insertions were done in a circle level -> terminate
	}
}

// Is this object even partially inside this grid?
bool isObjectInGrid(Projectile& object, int gridListIndex) {
	float halfWidthOfSingleGrid = worldHalfWidth / sqrt(GRIDSIZE);
	int xIndex = ((int)(gridListIndex / (int)(sqrt(GRIDSIZE))));
	int zIndex = ((int)(gridListIndex % (int)(sqrt(GRIDSIZE))));
	float xPosition = -worldHalfWidth + halfWidthOfSingleGrid + worldWidth * xIndex / (int)(sqrt(GRIDSIZE));
	float zPosition = -worldHalfWidth + halfWidthOfSingleGrid + worldWidth * zIndex / (int)(sqrt(GRIDSIZE));
	vec3 centerOfGridInWorldPosition = vec3(xPosition, 0.f, zPosition);
	vec3 objectInWorldPosition = object.getPosition();
	float distanceBetweenGridAndObject = glm::abs(glm::length((centerOfGridInWorldPosition - objectInWorldPosition)));
	float objectRadius = (object.getSize() / 2.f);
	if (distanceBetweenGridAndObject < objectRadius + halfWidthOfSingleGrid * sqrt(2)) return true;
	else return false;
}

// Simple input validation
bool correctGridListIndex(int gridListIndex) {
	if (gridListIndex < 0) return false;
	if (gridListIndex > GRIDSIZE) return false;
	return true;
}

// Gets the index of "grid" in grids[] from a "gridCircleIndex"
int getGridListIndex(int x, int z, int gridCircleIndex) {
	x += getCircleTraversalInfo(gridCircleIndex, 1);
	z += getCircleTraversalInfo(gridCircleIndex, 2);
	return int(x * 10.f + z);
}

// Is this grid in a different circle level than the providied circle level?
bool isNewCircleLevel(int oldCircleLevel, int newGrid) {
	int newCircleLevel = getCircleTraversalInfo(newGrid, 0);
	return (newCircleLevel > oldCircleLevel) ? true : false;
}

// X = the index of a grid surrounding a centering grid, 
// returnValue==0:the circlelevel of X, returnValue==1:X diff from center, returnValue==2:Y diff from center
int getCircleTraversalInfo(int x, int returnValue) {
	int width = 1;
	int circleLevel = 1;
	int currentGrid = 0;
	int currentX = 0;
	int currentY = 1;
	int direction = 0;
	while (currentGrid++ != x) {
		switch (direction) {
		case 0: // Going east
			currentX++;
			if (currentX == width) direction++;
			break;
		case 1: // Going south
			currentY--;
			if (currentY == -width) direction++;
			break;
		case 2: // Going west
			currentX--;
			if (currentX == -width) direction++;
			break;
		case 3: // Going north
			currentY++;
			if (currentY == width + 1) { // We have gone full circle --> new circle level
				direction = 0;
				circleLevel++;
				width += 1;
			}
			break;
		}
	}

	switch (returnValue) {
	case 0:
		return circleLevel;
	case 1:
		return currentX;
	case 2:
		return currentY;
	}


	return circleLevel;
}





void insertProjectileIntoGridsStatic(std::vector<std::vector<Projectile*>>& grids, int numberOfProjectiles[], Projectile& object) {

	vec3 position = object.getPosition();
	float x = toGridPosition(position.x);
	float z = toGridPosition(position.z);
	float xp = toGridPosition(position.x + object.getSize());
	float zp = toGridPosition(position.z + object.getSize());
	float xn = toGridPosition(position.x - object.getSize());
	float zn = toGridPosition(position.z - object.getSize());

	int grid = int(x * 10.f + z);
	grids[grid][numberOfProjectiles[grid]++] = &object;

	// Advanced cases, handles if object is within multiple adjacent grid squares
	if (xp > x && zp > z)	grids[int(xp * 10.f + zp)][numberOfProjectiles[int(xp * 10.f + zp)]++] = &object;
	if (zp > z)				grids[int(x  * 10.f + zp)][numberOfProjectiles[int(x  * 10.f + zp)]++] = &object;
	if (xn < x && zp > z)	grids[int(xn * 10.f + zp)][numberOfProjectiles[int(xn * 10.f + zp)]++] = &object;
	if (xn < x)				grids[int(xn * 10.f + z )][numberOfProjectiles[int(xn * 10.f + z )]++] = &object;
	if (xn < x && zn < z)	grids[int(xn * 10.f + zn)][numberOfProjectiles[int(xn * 10.f + zn)]++] = &object;
	if (zn < z)				grids[int(x  * 10.f + zn)][numberOfProjectiles[int(x  * 10.f + zn)]++] = &object;
	if (xp > x && zn < z)	grids[int(xp * 10.f + zn)][numberOfProjectiles[int(xp * 10.f + zn)]++] = &object;
	if (xp > x)				grids[int(xp * 10.f + z )][numberOfProjectiles[int(xp * 10.f + z )]++] = &object;
}

std::vector<int> getShipGrids(void) {

	std::vector<int> result;

	vec3 shipPosition = vec3(-shipModelMatrix[3][0], -shipModelMatrix[3][1], -shipModelMatrix[3][2]);
	float x = toGridPosition(shipPosition.x);
	float z = toGridPosition(shipPosition.z);
	float xp = toGridPosition(shipPosition.x + shipSize);
	float zp = toGridPosition(shipPosition.z + shipSize);
	float xn = toGridPosition(shipPosition.x - shipSize);
	float zn = toGridPosition(shipPosition.z - shipSize);


	result.push_back(int(x * 10.f + z));

	// Advanced cases, handles if object is within multiple adjacent grid squares
	if (xp > x && zp > z)	result.push_back(int(xp * 10.f + zp));
	if (zp > z)				result.push_back(int(x  * 10.f + zp));
	if (xn < x && zp > z)	result.push_back(int(xn * 10.f + zp));
	if (xn < x)				result.push_back(int(xn * 10.f + z ));
	if (xn < x && zn < z)	result.push_back(int(xn * 10.f + zn));
	if (zn < z)				result.push_back(int(x  * 10.f + zn));
	if (xp > x && zn < z)	result.push_back(int(xp * 10.f + zn));
	if (xp > x)				result.push_back(int(xp * 10.f + z ));

	return result;
}

int toGridPosition(float worldPosition) {
	return glm::min(glm::max(float(int((worldPosition + worldHalfWidth) / float(GRIDSIZE))), 0.f), float(glm::sqrt(GRIDSIZE) - 1));
}

void handleProjectilesCollision(Projectile* projectile1, Projectile* projectile2) {
	float distanceBetweenProjectiles = glm::abs(length((*projectile1).getPosition() - (*projectile2).getPosition()));
	if (distanceBetweenProjectiles < (*projectile1).getSize() + (*projectile2).getSize()) {
		onProjectilesCollision(projectile1, projectile2);
	}
}

void handlePlayerCollision(Projectile* projectile) {
	if (!projectile->isAsteroid()) return;
	vec3 shipPosition = vec3(-shipModelMatrix[3][0], -shipModelMatrix[3][1], -shipModelMatrix[3][2]);;
	float distanceBetweenProjectiles = glm::abs(length((shipPosition - projectile->getPosition())));
	if (!gameOver && playerCollision && distanceBetweenProjectiles < shipSize + projectile->getSize()) {
		onPlayerCollision();
		onProjectileCollision(projectile);
	}
}

void handleProjectileRemoval() {

	int index = 0;
	int numberOfProjectilesToBeRemoved = 0;
	int indicesOfProjectilesToBeRemoved[100] = {};
	int arrayIndex = 0;

	for (Projectile& projectile : projectiles) {
		try {
			projectile.isToBeRemoved();
		}catch(const std::exception& e) {
				std::cout << "shit";
		}
		if (projectile.isToBeRemoved()) {
			indicesOfProjectilesToBeRemoved[arrayIndex] = index;
			arrayIndex++;
			numberOfProjectilesToBeRemoved++;
		}
		index++;
	}

	for (int i = (numberOfProjectilesToBeRemoved-1); i >= 0; i--) {
		projectiles.erase(projectiles.begin() + indicesOfProjectilesToBeRemoved[i]);
	} 
}

void onProjectilesCollision(Projectile* p1, Projectile* p2) {
	if (!(*p1).isToExplode() && !(*p2).isToExplode()) {
		if ((*p1).isAsteroid() + (*p2).isAsteroid() % 2 != 0) {
			onHit();
		}
		onProjectileCollision(p1);
		onProjectileCollision(p2);
	}
}

void onProjectileCollision(Projectile* projectile) {
	projectile->collided = true;
	projectile->setDestruction(true);
}

void onPlayerCollision(void) {
	// animation of player ship destruction
	// "game over" screen
	
	if(!gameOver) onPlayerDestruction(); 
	
	gameOver = true;
	asteroidsEnabled = false;
	canFire = false;
	canMove = false;
}

void onPlayerDestruction(void) {

	vec3 shipPosition = vec3(shipModelMatrix[3][0], shipModelMatrix[3][1], shipModelMatrix[3][2]);;
	
	float scale = shipScale;
	vec3 color = shipDestructionColor;

	int numberOfParticles = maxNumberOfParticlesInAParticleSystem;

	particleSystems.push_back(ParticleSystem(numberOfParticles, shipPosition, false, true, scale, color));

	shipRender = false;
}