#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragEye;
layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform LightingBufferObject {

	vec4 ambientLightColor;
	vec4 perFragmentLightPos[4];
	vec4 perFragmentLightColor[4];

} lightubo;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor,1.0) * texture(texSampler, fragTexCoord);
	vec3 lightColor = lightubo.ambientLightColor.xyz;
	float diffuseFrac = 1.0 - lightubo.ambientLightColor.w;
	
	vec3 lightDir = normalize(lightubo.perFragmentLightPos[0].xyz);

    float atten = 1.0;
	float diffuseLight = max(0.0,dot(lightDir, fragNormal));
	
        if (diffuseLight > 0.0){
           lightColor += (atten * diffuseFrac * diffuseLight) * lightubo.perFragmentLightColor[0].xyz;
        }
	
	
	
	
	outColor = outColor*vec4(lightColor,1.0);
}