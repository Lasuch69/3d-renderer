#include <fstream>

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include "vk_shader.h"

VkShaderModule loadShader(const VkDevice &device, const std::string &path, EShLanguage stage) {
	// Load GLSL shader from disk
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	// Compile GLSL to SPIRV
	glslang::InitializeProcess();

	glslang::TShader shader(stage);

	const char *sources[1] = { buffer.data() };
	shader.setStrings(sources, 1);

	glslang::EShTargetClientVersion targetApiVersion = glslang::EShTargetVulkan_1_0;
	shader.setEnvClient(glslang::EShClientVulkan, targetApiVersion);

	glslang::EShTargetLanguageVersion spirvVersion = glslang::EShTargetSpv_1_0;
	shader.setEnvTarget(glslang::EshTargetSpv, spirvVersion);

	shader.setEntryPoint("main");

	const TBuiltInResource *resources = GetDefaultResources();

	const int defaultVersion = 450;
	const bool forwardCompatible = false;
	const EShMessages messageFlags = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
	EProfile defaultProfile = ENoProfile;

	glslang::TShader::ForbidIncluder includer;

	std::string preprocessedStr;
	if (!shader.preprocess(resources, defaultVersion, defaultProfile, false, forwardCompatible, messageFlags, &preprocessedStr, includer)) {
		throw std::runtime_error("failed to preprocess GLSL shader!");
	}

	const char *preprocessedSources[1] = { preprocessedStr.c_str() };
	shader.setStrings(preprocessedSources, 1);

	if (!shader.parse(resources, defaultVersion, defaultProfile, false, forwardCompatible, messageFlags, includer)) {
		throw std::runtime_error("failed to parse GLSL shader!");
	}

	glslang::TProgram program;
	program.addShader(&shader);
	if (!program.link(messageFlags)) {
		throw std::runtime_error("failed to link shader!");
	}

	glslang::TIntermediate &intermediateRef = *(program.getIntermediate(stage));
	std::vector<uint32_t> spirv;
	glslang::SpvOptions options{};
	options.validate = true;
	glslang::GlslangToSpv(intermediateRef, spirv, &options);

	glslang::FinalizeProcess();

	// Create shader module
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}
