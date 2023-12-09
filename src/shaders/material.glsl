#[VERTEX]

#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
	vec4 data;
	mat4 model;
} constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec2 fragTexCoord;

void main() {
	mat4 model = constants.model;
	mat4 view = ubo.view;
	mat4 proj = ubo.proj;

	fragPosition = vec3(view * model * vec4(inPosition, 1.0));
	fragNormal = normalize(mat3(transpose(inverse(view * model))) * inNormal); 
	fragColor = inColor;
	fragTexCoord = inTexCoord;
	gl_Position = proj * view * model * vec4(inPosition, 1.0f);
}

#[FRAGMENT]

#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = texture(texSampler, fragTexCoord);
}