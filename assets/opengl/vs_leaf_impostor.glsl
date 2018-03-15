#version 330
 
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tan;
layout (location = 4) in vec3 bitan;
layout (location = 5) in mat4 transform; // per instance

uniform mat4 MVP;
uniform vec2 idx_2d;

out vec2 tex_coord;

void main()
{
	tex_coord = uv * 0.5f + idx_2d * 0.5f;
	gl_Position = MVP * vec4(vertex, 1.f);
}