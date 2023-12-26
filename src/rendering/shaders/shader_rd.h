#ifndef SHADER_RD_H
#define SHADER_RD_H

#include <cstdint>
#include <vector>

class ShaderRD {
private:
	enum StageType {
		STAGE_TYPE_VERTEX,
		STAGE_TYPE_FRAGMENT,
		STAGE_TYPE_COMPUTE,
		STAGE_TYPE_MAX,
	};

	std::vector<uint32_t> processShader(const char *glslCode, StageType stage);
	std::vector<uint32_t> spirv[STAGE_TYPE_MAX];

protected:
	ShaderRD(){};
	void setup(const char *vertexCode, const char *fragmentCode, const char *computeCode, const char *name);

public:
	std::vector<uint32_t> getVertexCode();
	std::vector<uint32_t> getFragmentCode();
	std::vector<uint32_t> getComputeCode();
};

#endif // !SHADER_RD_H
