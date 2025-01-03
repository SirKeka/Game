#version 450

layout(location = 0) out vec4 out_colour;

struct directional_light {
    vec4 colour;
    vec3 direction;
};

struct point_light {
    vec4 colour;
    vec3 position;
    // Обычно 1, следите за тем, чтобы знаменатель никогда не был меньше 1.
    float constant;
    // Линейно снижает интенсивность света
    float linear;
    // Заставляет свет падать медленнее на больших расстояниях.
    float quadratic;
    float padding;
};

const int MAX_POINT_LIGHTS = 10;

layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_colour;
    directional_light dir_light;
    point_light p_lights[MAX_POINT_LIGHTS];
    int num_p_lights;
    float specular;
} object_ubo;

// Samplers, diffuse, spec
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

// layout(set=0, binding = 0) uniform TheStruct { vec4 theMember; };

layout(location = 0) flat in int in_mode;
// Data Transfer Object
layout(location = 1) in struct dto {
    vec4 ambient;
	vec2 tex_coord;
    vec3 normal;
    vec3 view_position;
	vec3 frag_position;
    vec4 colour;
	vec3 tangent;
} in_dto;

mat3 TBN;

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction);
vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction);

void main() {
    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent.xyz;
    tangent = (tangent - dot(tangent, normal) *  normal);
    vec3 bitangent = cross(in_dto.normal, in_dto.tangent);
    TBN = mat3(tangent, bitangent, normal);

    // Обновите нормаль, чтобы использовать образец из карты нормалей.
    vec3 localNormal = 2.0 * texture(samplers[SAMP_NORMAL], in_dto.tex_coord).rgb - 1.0;
    normal = normalize(TBN * localNormal);

    if(in_mode == 0 || in_mode == 1) {
        vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);

        out_colour = calculate_directional_light(object_ubo.dir_light, normal, view_direction);

        for(int i = 0; i < object_ubo.num_p_lights; ++i) {
            out_colour += calculate_point_light(object_ubo.p_lights[i], normal, in_dto.frag_position, view_direction);
        }
        // out_colour += calculate_point_light(object_ubo.p_light_1, normal, in_dto.frag_position, view_direction);
    } else if(in_mode == 2) {
        out_colour = vec4(abs(normal), 1.0);
    }
}

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction) {
    float diffuse_factor = max(dot(normal, -light.direction.xyz), 0.0);

    vec3 half_direction = normalize(view_direction - light.direction.xyz);
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), object_ubo.specular);

    vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
    vec4 ambient = vec4(vec3(in_dto.ambient * object_ubo.diffuse_colour), diff_samp.a);
    vec4 diffuse = vec4(vec3(light.colour * diffuse_factor), diff_samp.a);
    vec4 specular = vec4(vec3(light.colour * specular_factor), diff_samp.a);
    
    if(in_mode == 0) {
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    return (ambient + diffuse + specular);
}

vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction) {
    vec3 light_direction =  normalize(light.position.xyz - frag_position);
    float diff = max(dot(normal, light_direction), 0.0);

    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), object_ubo.specular);

    // Рассчитайте затухание или затухание света с расстоянием.
    float distance = length(light.position.xyz - frag_position);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec4 ambient  = in_dto.ambient;
    vec4 diffuse  = light.colour * diff;
    vec4 specular = light.colour * spec;
    
    if(in_mode == 0) {
        vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}
