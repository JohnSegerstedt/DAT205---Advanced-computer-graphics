#version 400 compatibility
layout(location = 0) in vec4 particle;
uniform mat4 projectionMatrix;
uniform float screen_x;
uniform float screen_y;
out float life;

void main()
{

    life = particle.w;

    // Calculate one projected corner of a quad at the particles view space depth.
    vec4 proj_quad    = projectionMatrix * vec4(1.0, 1.0, particle.z, 1.0);

    // Calculate the projected pixel size.
    vec2 proj_pixel   = vec2(screen_x, screen_y) * proj_quad.xy / proj_quad.w;

    // Use scale factor as sum of x and y sizes.
    float scale_factor = (proj_pixel.x+proj_pixel.y);

    // Transform position.
    gl_Position  = projectionMatrix * vec4(particle.xyz, 1.0);

    // Scale the point with regard to the previosly defined scale_factor
    // and the life (it will get larger the older it is)
    gl_PointSize = scale_factor * (pow(5, life)-1);
    //gl_PointSize = scale_factor * mix(0.0, 5.0, pow(life, 1.0/4.0));
}