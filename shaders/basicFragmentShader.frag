#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

layout(location = 0) out vec4 fragmentColor;

in float life;
in vec3 outColor;

void main() 
{
	fragmentColor.xyz = pow((1.f - life), 3) * outColor;
}