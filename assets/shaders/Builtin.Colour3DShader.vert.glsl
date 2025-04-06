#version 450

layout(location = 0) in vec4 in_position; // ПРИМЕЧАНИЕ: w игнорируется.
layout(location = 1) in vec4 in_colour;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
	mat4 view;
} global_ubo;

layout(push_constant) uniform push_constants {
	
	// Гарантируется всего 128 байт.
	mat4 model; // 64 байта
} local_ubo;


// Объект передачи данных
layout(location = 1) out struct dto {
	vec4 colour;
} out_dto;

void main() {
	out_dto.colour = in_colour;
	gl_Position = global_ubo.projection * global_ubo.view * local_ubo.model * vec4(in_position.xyz, 1.0);
}