#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	
	mat4 model;
	//mat4 modelView;
	//mat4 modelViewProjection;
	//mat4 normal;
} ubo;

layout(push_constant) uniform Camera {
	mat4 view;
	mat4 proj;
} uboCamera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outVertex;
layout(location = 4) out mat4 outView;
layout(location = 8) out mat4 outModelView;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {

	outVertex = vec4(inPosition, 1.0);
	outView = uboCamera.view;
	outNormal = vec4(inNormal,1.0);
	outModelView = uboCamera.view * ubo.model;
	outColor = inColor;
    outTexCoord = inTexCoord;
    gl_Position =  uboCamera.proj * outModelView * outVertex;



}