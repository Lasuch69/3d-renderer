#ifndef VK_SHADER_H
#define VK_SHADER_H

#include <string>
#include <vector>

#include <glslang/Public/ShaderLang.h>

#include "vk_types.h"

VkShaderModule loadShader(const VkDevice &device, const std::string &path, EShLanguage stage);

#endif // !VK_SHADER_H
