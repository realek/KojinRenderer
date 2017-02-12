#version 450
#extension GL_ARB_separate_shader_objects : enable
#define MAX_LIGHT_NR 4;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragEye;
layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform LightingBufferObject {

	vec4 ambientLightColor;
	vec4 perFragmentLightPos[4];
	vec4 perFragmentLightColor[4];
	float specularity;

} lightubo;
layout(location = 0) out vec4 outColor;

void main() {
	vec3 N = normalize(fragNormal);
    outColor = vec4(fragColor,1.0) * texture(texSampler, fragTexCoord);
	vec3 lightColor = lightubo.ambientLightColor.xyz;
	float diffuseFrac = 1.0 - lightubo.ambientLightColor.w;
	float atten = 1.0;
	for(int i = 0;i < 4;i++)
	{
		vec3 L = normalize(lightubo.perFragmentLightPos[i].xyz); // light dir
		vec3 V = normalize(fragEye);// eye coord
		vec3 R = reflect(-L, N); // reflection
		vec3 diffuse = max(dot(N, L), 0.0) * lightubo.perFragmentLightColor[i].xyz; // diffuse component
		vec3 specular = vec3(pow(max(dot(R, V), 0.0), lightubo.specularity) * outColor.a); // specular component
		
		
		lightColor += diffuse + specular;		
	}
	
	outColor *=vec4(lightColor,1.0);
}