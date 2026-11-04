#include "renderer_utils.h"

bool UniformTypeIsSampler(ShaderUniformType type)
{
    switch (type) {
        case ShaderUniformType::Sampler1D:
        case ShaderUniformType::Sampler2D:
        case ShaderUniformType::Sampler3D:
        case ShaderUniformType::SamplerCube:
        case ShaderUniformType::Sampler1DArray:
        case ShaderUniformType::Sampler2DArray:
        case ShaderUniformType::SamplerCubeArray:
            return true;
        default:
            return false;
    }
}