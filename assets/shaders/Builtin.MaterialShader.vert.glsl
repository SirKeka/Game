#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
	mat4 view;
	vec4 ambient_colour;
} global_ubo;

layout(push_constant) uniform push_constants {

	// Гарантируется только общее количество 128 байт.
	mat4 model; // 64 байта
} u_push_constants;

layout(location = 0) out int out_mode;

// Объект передачи данных
layout(location = 1) out struct dto {
	vec4 ambient;
	vec2 tex_coord;
	vec3 in_normal;
} out_dto;


void main() {
	out_dto.tex_coord = in_texcoord;
	out_dto.in_normal = mat3(u_push_constants.model) * in_normal;
	out_dto.ambient = global_ubo.ambient_colour;
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0);
}