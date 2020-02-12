#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

uniform float screen_x;
uniform float screen_y;
layout(binding = 0) uniform sampler2D colortexture;

in float life;

void main() 
{
    // Base color.
    gl_FragColor = texture2D(colortexture, gl_PointCoord);

    // Make it darker the older it is.
    gl_FragColor.xyz *= (1.0-life);

    // Make it fade out the older it is, also all particles have a  very low base alpha so they blend together.
    gl_FragColor.w = gl_FragColor.w * (1.0-pow(life, 4.0))*0.15;
    //gl_FragColor.w = gl_FragColor.w * (1.0-pow(life, 4.0))*0.05;
}