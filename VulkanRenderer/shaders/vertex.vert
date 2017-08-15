#version 450
#extension GL_ARB_separate_shader_objects : enable

struct VkLight
{
	vec4 color;
	vec4 position;
	vec4 direction;
	vec4 lightProps; //type, intensity, falloff, angle
	mat4 lightBiasedMVP;

};

layout(set = 0, binding = 0) uniform Model {
	
	mat4 model;

} model_data;

layout(set = 0, binding = 1) uniform Material {

	vec4 materialDiffuse;
	float specularity;
	
} material_data;

layout(set = 0, binding = 2) uniform Lights {

	VkLight lights[6];
	vec4 ambientLightColor;
} lights_data;

layout(push_constant) uniform Camera {
	mat4 view;
	mat4 proj;
} camera_data;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) flat out vec4 outMaterialColor;
layout(location = 2) flat out float outMaterialSpecularity;
layout(location = 3) out vec2 outTexCoord;
layout(location = 4) flat out vec4 outAmbientLight;
layout(location = 5) out vec3 outFragPos;
layout(location = 6) out vec3 outFragNormal;
layout(location = 7) flat out VkLight outLights[6];



out gl_PerVertex {
    vec4 gl_Position;
};

void main() {

	vec4 vertex = vec4(inPosition, 1.0);
	mat4 modelView = camera_data.view * model_data.model;
	outColor = vec4(inColor,1.0f);
    outTexCoord = inTexCoord;
	outMaterialColor = material_data.materialDiffuse;
	outAmbientLight = lights_data.ambientLightColor;
	
	vec4 vPos = modelView*vertex;
	outFragPos = vec3(vPos)/vPos.w;
    outFragNormal = vec3(transpose(inverse(modelView)) * vec4(inNormal,1.0));
	VkLight iLights[6] = lights_data.lights;
	for(int i = 0;i < 6;++i)
	{

		iLights[i].position = camera_data.view*lights_data.lights[i].position;
		iLights[i].direction = camera_data.view*lights_data.lights[i].direction;

	}
	outLights = iLights;
    gl_Position =  camera_data.proj * vPos;
}