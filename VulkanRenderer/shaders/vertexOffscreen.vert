#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform Model {
	
	mat4 model;

} model_data;

layout(push_constant) uniform Camera {
	mat4 view;
	mat4 proj;
} camera_data;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

 
void main()
{
	gl_Position =  camera_data.proj * camera_data.view * model_data.model*vec4(inPosition, 1.0);
}