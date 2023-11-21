#include "shader_rd.h"

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <iostream>
#include <stdexcept>

void ShaderRD::setup(const char *vertexCode, const char *fragmentCode, const char *computeCode, const char *name) {
	const char *stage[] = { vertexCode, fragmentCode, computeCode };

	for (int i = 0; i < STAGE_TYPE_MAX; i++) {
		spirv[i] = processShader(stage[i], StageType(i));
	}
}

std::vector<uint32_t> ShaderRD::processShader(const char *glslCode, StageType stage) {
	if (glslCode == nullptr) {
		return std::vector<uint32_t>();
	}

	glslang::InitializeProcess();

	EShLanguage glslStage;
	bool validStage = false;

	switch (stage) {
		case STAGE_TYPE_VERTEX:
			glslStage = EShLangVertex;
			validStage = true;
			break;
		case STAGE_TYPE_FRAGMENT:
			glslStage = EShLangFragment;
			validStage = true;
			break;
		case STAGE_TYPE_COMPUTE:
			glslStage = EShLangCompute;
			validStage = true;
			break;
		default:
			break;
	}

	if (!validStage) {
		return std::vector<uint32_t>();
	}

	glslang::TShader glslShader(glslStage);
	glslShader.setStrings(&glslCode, 1);

	glslang::EShTargetClientVersion targetApiVersion = glslang::EShTargetVulkan_1_0;
	glslShader.setEnvClient(glslang::EShClientVulkan, targetApiVersion);

	glslang::EShTargetLanguageVersion spirvVersion = glslang::EShTargetSpv_1_0;
	glslShader.setEnvTarget(glslang::EshTargetSpv, spirvVersion);

	glslShader.setEntryPoint("main");

	const TBuiltInResource *resources = GetDefaultResources();

	const int defaultVersion = 450;
	const bool forwardCompatible = false;
	const EShMessages messageFlags = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
	EProfile defaultProfile = ENoProfile;

	glslang::TShader::ForbidIncluder includer;

	std::string preprocessedStr;
	if (!glslShader.preprocess(resources, defaultVersion, defaultProfile, false, forwardCompatible, messageFlags, &preprocessedStr, includer)) {
		throw std::runtime_error("failed to preprocess GLSL shader!");
	}

	const char *preprocessedSources[1] = { preprocessedStr.c_str() };
	glslShader.setStrings(preprocessedSources, 1);

	if (!glslShader.parse(resources, defaultVersion, defaultProfile, false, forwardCompatible, messageFlags, includer)) {
		std::cout << glslShader.getInfoLog() << "\n";
		throw std::runtime_error("failed to parse GLSL shader!");
	}

	glslang::TProgram program;
	program.addShader(&glslShader);
	if (!program.link(messageFlags)) {
		throw std::runtime_error("failed to link shader!");
	}

	glslang::TIntermediate &intermediateRef = *(program.getIntermediate(glslStage));
	std::vector<uint32_t> spirv;
	glslang::SpvOptions options{};
	options.validate = true;
	glslang::GlslangToSpv(intermediateRef, spirv, &options);

	glslang::FinalizeProcess();

	return spirv;
}

std::vector<uint32_t> ShaderRD::getVertexCode() {
	return spirv[STAGE_TYPE_VERTEX];
}

std::vector<uint32_t> ShaderRD::getFragmentCode() {
	return spirv[STAGE_TYPE_FRAGMENT];
}

std::vector<uint32_t> ShaderRD::getComputeCode() {
	return spirv[STAGE_TYPE_COMPUTE];
}
