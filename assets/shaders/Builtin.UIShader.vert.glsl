#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
	mat4 view;
} global_ubo;

layout(push_constant) uniform push_constants {

	// Гарантируется всего 128 байт.
	mat4 model; // 64 байт
} u_push_constants;

layout(location = 0) out int out_mode;

// Объект передачи данных
layout(location = 1) out struct dto {
	vec2 tex_coord;
} out_dto;

void main() {
	// ПРИМЕЧАНИЕ: намеренно переверните координату текстуры y. Это, наряду с перевернутой орто-матрицей, помещает [0, 0] 
    // в верхний левый угол вместо нижнего левого и корректирует координаты текстуры для отображения в правильном направлении.
	out_dto.tex_coord = vec2(in_texcoord.x, 1.0 - in_texcoord.y);
	gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 0.0, 1.0);
}