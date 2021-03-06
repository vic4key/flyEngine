#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

// Shader constant
uniform mat4 VP;

// Model constants
uniform mat4 M;
uniform mat3 M_i;

out vec3 pos_world;
out vec3 normal_world;
out vec2 uv_out;
out vec3 tangent_world;
out vec3 bitangent_world;

uniform float t;
// Global wind params
uniform vec2 wd; // Wind direction
uniform vec2 wm; // Wind movement
uniform float wf; // Wind frequency
uniform float ws; // Wind strength
// Wind params per mesh
uniform float wp; // Wind pivot
uniform float we; // Weight exponent
uniform vec2 bb_mi; // AABB min xz
uniform vec2 bb_ma; // AABB max xz

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(12.9898f, 78.233f))) * 43758.5453f);
}

float noise(vec2 p)
{
	vec2 start = floor(p);
	vec2 end = start + 1.f;
	vec2 weights = smoothstep(start, end, p);
	return mix(mix(hash(start), hash(vec2(end.x, start.y)), weights.x), mix(hash(vec2(start.x, end.y)), hash(end), weights.x), weights.y);
}

void main()
{
	pos_world = (M * vec4(position, 1.f)).xyz;
	float weight = pow(abs(wp - pos_world.y), we);
	pos_world.xz += wd * (noise(pos_world.xz * wf + t * wm) * 2.f - 1.f) * ws * weight;
	pos_world.xz = clamp(pos_world.xz, bb_mi, bb_ma);
	gl_Position = VP * vec4(pos_world, 1.f);
	normal_world = normalize(M_i * normal);
	uv_out = uv;
	tangent_world = normalize(M_i * tangent);
	bitangent_world = normalize(M_i * bitangent);
}