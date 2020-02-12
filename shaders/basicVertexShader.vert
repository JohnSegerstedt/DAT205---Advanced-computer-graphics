#version 400 compatibility
layout(location = 0) in vec4 particle;
uniform mat4 projectionMatrix;
uniform vec3 particleColor;

out float life;
out vec3 outColor;

void main(){
    life = particle.w;
	outColor = particleColor;

    // Transform position.
    gl_Position  = projectionMatrix * vec4(particle.xyz, 1.0);

}