#version 450

layout(location = 0) out vec4 out_colour;

struct directional_light {
    vec4 colour;
    vec4 direction;
};

struct point_light {
    vec4 colour;
    vec4 position;
    // Обычно 1, убедитесь, что знаменатель никогда не становится меньше 1.
    float constant_f;
    // Линейно уменьшает интенсивность света.
    float linear;
    // Замедляет затухание света на больших расстояниях.
    float quadratic;
    float padding;
};

const int MAX_POINT_LIGHTS = 10;
const int MAX_SHADOW_CASCADES = 4;

struct pbr_properties {
    vec4 diffuse_colour;
    vec3 padding;
    float shininess;
};

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
	mat4 view;
	mat4 light_space[MAX_SHADOW_CASCADES];
    vec4 cascade_splits; // ПРИМЕЧАНИЕ: 4 разделения.
	vec3 view_position;
	int mode;
    int use_pcf;
    float bias;
    vec2 padding;
} global_ubo;

layout(set = 1, binding = 0) uniform instance_uniform_object {
    directional_light dir_light;
    point_light p_lights[MAX_POINT_LIGHTS]; // ЗАДАЧА: переместить после реквизита.
    pbr_properties properties;
    int num_p_lights;
} instance_ubo;

const int PBR_MATERIAL_TEXTURE_COUNT = 3;

// Индексы текстур материалов.
const int SAMP_ALBEDO = 0;
const int SAMP_NORMAL = 1;
const int SAMP_COMBINED = 2;

const float PI = 3.14159265359;
// Текстуры материалов: альбедо, нормали, комбинированные (металличность, шероховатость, AO).
layout(set = 1, binding = 1) uniform sampler2D material_textures[3];
// Карты теней.
layout(set = 1, binding = 2) uniform sampler2DArray shadow_texture;
// Карта окружения находится по последнему индексу.
layout(set = 1, binding = 3) uniform samplerCube irradiance_texture;

layout(location = 0) flat in int in_mode;
layout(location = 1) flat in int use_pcf;
// Объект передачи данных.
layout(location = 2) in struct dto {
	vec4 light_space_frag_pos[MAX_SHADOW_CASCADES];
    vec4 cascade_splits;
	vec2 tex_coord;
	vec3 normal;
	vec3 view_position;
	vec3 frag_position;
    vec4 colour;
	vec3 tangent;
    float bias;
    vec3 padding;
} in_dto;


mat3 TBN;

// Фильтрация по проценту приближения.
float calculate_pcf(vec3 projected, int cascade_index) {
    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(shadow_texture, 0).xy;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcf_depth = texture(shadow_texture, vec3(projected.xy + vec2(x, y) * texel_size, cascade_index)).r;
            shadow += projected.z - in_dto.bias > pcf_depth ? 1.0 : 0.0;
        }
    }
    shadow /= 9;
    return 1.0 - shadow;
}

float calculate_unfiltered(vec3 projected, int cascade_index) {
    // Сэмплирование карты теней.
    float map_depth = texture(shadow_texture, vec3(projected.xy, cascade_index)).r;

    // ЗАДАЧА: отбросить/удалить ветвь.
    float shadow = projected.z - in_dto.bias > map_depth ? 0.0 : 1.0;
    return shadow;
}

// Сравните положение фрагмента с буфером глубины, и если он находится дальше, 
// чем карта теней, он находится в тени. 0,0 = в тени, 1,0 = нет.
float calculate_shadow(vec4 light_space_frag_pos, vec3 normal, directional_light light, int cascade_index) {
    // Перспективное деление — обратите внимание, 
    // что хотя для ортопроекции это бессмысленно, 
    // для перспективной проекции это потребуется.
    vec3 projected = light_space_frag_pos.xyz / light_space_frag_pos.w;
    // Необходимо инвертировать y
    projected.y = 1.0-projected.y;

    // ПРИМЕЧАНИЕ: Преобразование в NDC не требуется для Vulkan, но потребуется для OpenGL.
    // projected.xy = projected.xy * 0.5 + 0.5;

    if(use_pcf == 1) {
        return calculate_pcf(projected, cascade_index);
    } 

    return calculate_unfiltered(projected, cascade_index);
}

// На основе комбинации GGX и приближения Шлика-Бекмана для расчета вероятности затенения микрограней.
float geometry_schlick_ggx(float normal_dot_direction, float roughness) {
    roughness += 1.0;
    float k = (roughness * roughness) / 8.0;
    return normal_dot_direction / (normal_dot_direction * (1.0 - k) + k);
}

vec3 calculate_point_light_radiance(point_light light, vec3 view_direction, vec3 frag_position_xyz);
vec3 calculate_directional_light_radiance(directional_light light, vec3 view_direction);
vec3 calculate_reflectance(vec3 albedo, vec3 normal, vec3 view_direction, vec3 light_direction, float metallic, float roughness, vec3 base_reflectivity, vec3 radiance);

void main() {
    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent;
    tangent = (tangent - dot(tangent, normal) *  normal);
    vec3 bitangent = cross(in_dto.normal, in_dto.tangent);
    TBN = mat3(tangent, bitangent, normal);

    // Обновите нормаль, используя образец из карты нормалей.
    vec3 local_normal = 2.0 * texture(material_textures[SAMP_NORMAL], in_dto.tex_coord).rgb - 1.0;
    normal = normalize(TBN * local_normal);

    vec4 albedo_samp = texture(material_textures[SAMP_ALBEDO], in_dto.tex_coord);
    vec3 albedo = pow(albedo_samp.rgb, vec3(2.2));

    vec4 combined = texture(material_textures[SAMP_COMBINED], in_dto.tex_coord);
    float metallic = combined.r;
    float roughness = combined.g;
    float ao = combined.b;

    // Сгенерируйте значение тени на основе текущей позиции фрагмента относительно карты теней.
    // Свет и нормаль также учитываются в случае, если необходимо использовать смещение.
    vec4 frag_position_view_space = global_ubo.view * vec4(in_dto.frag_position, 1.0f);
    float depth = abs(frag_position_view_space).z;
    // Получите индекс каскада из текущей позиции фрагмента.
    int cascade_index = -1;
    for(int i = 0; i < MAX_SHADOW_CASCADES; ++i) {
        if(depth < in_dto.cascade_splits[i]) {
            cascade_index = i;
            break;
        }
    }
    if(cascade_index == -1) {
        cascade_index = MAX_SHADOW_CASCADES;
    }
    float shadow = calculate_shadow(in_dto.light_space_frag_pos[cascade_index], normal, instance_ubo.dir_light, cascade_index);

    // Рассчитайте коэффициент отражения при нормальном падении; 
    // если это диэлектрик (например, пластик), используйте базовый коэффициент отражения 0,04, 
    // а если это металл, используйте цвет альбедо в качестве базового коэффициента отражения (для металлов).
    vec3 base_reflectivity = vec3(0.04); 
    base_reflectivity = mix(base_reflectivity, albedo, metallic);

    if(in_mode == 0 || in_mode == 1 || in_mode == 3) {
        vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);

        // Не включайте альбедо в режим 1 (только освещение). Для этого используйте белый цвет, умноженный на режим (режим 1 даст белый цвет, режим 0 — черный), 
        // затем добавьте этот цвет к альбедо и ограничьте его. В результате альбедо в режиме 1 будет чистым белым, а в режиме 0 — обычным альбедо, без каких-либо ветвлений.
        albedo += (vec3(1.0) * in_mode);         
        albedo = clamp(albedo, vec3(0.0), vec3(1.0));

        // Это основано на двунаправленной функции распределения отражательной способности Кука-Торранса (BRDF).
        // Здесь используется модель микрограней для учета шероховатости и металлических свойств материалов с целью получения физически точного представления отражательной способности материала.

        // Общая отражательная способность.
        vec3 total_reflectance = vec3(0.0);

        // Яркость направленного света.
        {
            directional_light light = instance_ubo.dir_light;
            vec3 light_direction = normalize(-light.direction.xyz);
            vec3 radiance = calculate_directional_light_radiance(light, view_direction);

            // Карта теней должна влиять только на направленный свет.
            total_reflectance += (shadow * calculate_reflectance(albedo, normal, view_direction, light_direction, metallic, roughness, base_reflectivity, radiance));
        }

        // Яркость точечного источника света.
        for(int i = 0; i < instance_ubo.num_p_lights; ++i) {
            point_light light = instance_ubo.p_lights[i];
            vec3 light_direction = normalize(light.position.xyz - in_dto.frag_position.xyz);
            vec3 radiance = calculate_point_light_radiance(light, view_direction, in_dto.frag_position.xyz);

            total_reflectance += calculate_reflectance(albedo, normal, view_direction, light_direction, metallic, roughness, base_reflectivity, radiance);
        }

        // Интенсивность излучения содержит весь рассеянный непрямой свет сцены. Используйте нормаль поверхности для выборки из него.
        vec3 irradiance = texture(irradiance_texture, normal).rgb;

        // Объедините интенсивность излучения с альбедо и затенением окружающей среды.
        // Также добавьте общее накопленное отражение.
        vec3 ambient = irradiance * albedo * ao;
        // Измените общее отражение в соответствии с цветом окружающей среды.
        vec3 colour = ambient + total_reflectance;

        // Тональное отображение HDR
        colour = colour / (colour + vec3(1.0));
        // Гамма-коррекция
        colour = pow(colour, vec3(1.0 / 2.2));

        if(in_mode == 3) {
            switch(cascade_index) {
                case 0:
                    colour *= vec3(1.0, 0.25, 0.25);
                    break;
                case 1:
                    colour *= vec3(0.25, 1.0, 0.25);
                    break;
                case 2:
                    colour *= vec3(0.25, 0.25, 1.0);
                    break;
                case 3:
                    colour *= vec3(1.0, 1.0, 0.25);
                    break;
            }
        }

        // Убедитесь, что альфа-канал основан на исходном значении альфа-канала альбедо.
        out_colour = vec4(colour, albedo_samp.a);
    } else if(in_mode == 2) {
        out_colour = vec4(abs(normal), 1.0);
    }
}

vec3 calculate_reflectance(vec3 albedo, vec3 normal, vec3 view_direction, vec3 light_direction, float metallic, float roughness, vec3 base_reflectivity, vec3 radiance) {
    vec3 halfway = normalize(view_direction + light_direction);

    // Это основано на двунаправленной функции распределения отражения Кука-Торранса (BRDF).

    // Нормальное распределение — приблизительно определяет количество микрограней поверхности, выровненных по центральному вектору. На это напрямую влияет шероховатость поверхности. 
    // Чем больше выровненных микрограней, тем блестящая поверхность, чем меньше — тем матовая поверхность/меньше отражения.
    float roughness_sq = roughness*roughness;
    float roughness_sq_sq = roughness_sq * roughness_sq;
    float normal_dot_halfway = max(dot(normal, halfway), 0.0);
    float normal_dot_halfway_squared = normal_dot_halfway * normal_dot_halfway;
    float denom = (normal_dot_halfway_squared * (roughness_sq_sq - 1.0) + 1.0);
    denom = PI * denom * denom;
    float normal_distribution = (roughness_sq_sq / denom);

    // Функция геометрии, которая вычисляет самозатенение на микрогранях (более выраженное на шероховатых поверхностях).
    float normal_dot_view_direction = max(dot(normal, view_direction), 0.0);
    // Масштабирование света с помощью скалярного произведения нормали и направления света.
    float normal_dot_light_direction = max(dot(normal, light_direction), 0.0);
    float ggx_0 = geometry_schlick_ggx(normal_dot_view_direction, roughness);
    float ggx_1 = geometry_schlick_ggx(normal_dot_light_direction, roughness);
    float geometry = ggx_1 * ggx_0;

    // Приближение Френеля-Шлика для эффекта Френеля. Это позволяет получить отношение отражения от поверхности при разных углах. 
    // Во многих случаях отражательная способность может быть выше при более экстремальных углах.
    float cos_theta = max(dot(halfway, view_direction), 0.0);
    vec3 fresnel = base_reflectivity + (1.0 - base_reflectivity) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);

    // Возьмите распределение нормалей * геометрию * эффект Френеля и рассчитайте зеркальное отражение.
    vec3 numerator = normal_distribution * geometry * fresnel;
    float denominator = 4.0 * max(dot(normal, view_direction), 0.0) + 0.0001; // prevent div by 0 
    vec3 specular = numerator / denominator;

    // Для сохранения энергии диффузный и зеркальный свет не могут быть больше 1,0 (если только поверхность не излучает свет); 
    // для сохранения этого соотношения диффузная составляющая должна равняться 1,0 минус френелевская составляющая.
    vec3 refraction_diffuse = vec3(1.0) - fresnel;
    // Умножьте диффузное освещение на обратную величину металличности так, чтобы рассеянный свет был только у неметаллов, 
    // или используйте линейное смешение, если металл частично присутствует (чистые металлы не имеют рассеянного света).
    refraction_diffuse *= 1.0 - metallic;	  

    // В итоге получается коэффициент отражения, который добавляется к общему значению и отслеживается звонящим.
    return (refraction_diffuse * albedo / PI + specular) * radiance * normal_dot_light_direction;  
}

vec3 calculate_point_light_radiance(point_light light, vec3 view_direction, vec3 frag_position_xyz) {
    // Яркость каждого источника света определяется на основе затухания точечного источника света.
    float distance = length(light.position.xyz - frag_position_xyz);
    float attenuation = 1.0 / (light.constant_f + light.linear * distance + light.quadratic * (distance * distance));
    return light.colour.rgb * attenuation;
}

vec3 calculate_directional_light_radiance(directional_light light, vec3 view_direction) {
    // Для направленных источников света яркость совпадает с цветом самого источника света.
    return light.colour.rgb;
}

