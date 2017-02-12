#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	mat3 normal;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragEye;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {

	vec4 position = vec4(inPosition, 1.0);
    gl_Position = ubo.modelViewProjection * position;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	fragNormal = vec3(ubo.normal * inNormal);
	fragEye = vec3(ubo.modelView * -position);
}