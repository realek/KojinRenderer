#version 450
#extension GL_ARB_separate_shader_objects : enable
#define MAX_LIGHT_NR 4;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform LightingBufferObject {

	vec4 ambientLightColor;
	vec4 perFragmentLightPos[4];
	vec4 perFragmentLightColor[4];
	vec4 perFragmentLightIntensity[4];
	float specularity;

} lightubo;
layout(location = 0) out vec4 outColor;

void main() {
	vec3 N = normalize(-fragNormal);
    outColor = vec4(fragColor,1.0) * texture(texSampler, fragTexCoord);
	vec4 lightColor = vec4(lightubo.ambientLightColor.xyz,1.0);
	float diffuseFrac = 1.0 - lightubo.ambientLightColor.w;
	vec3 specular = vec3(0.0,0.0,0.0);
	vec4 diffuse = vec4(0.0,0.0,0.0,0.0);
	for(int i = 0;i < 4;i++)
	{
		vec3 L = normalize(fragPos - lightubo.perFragmentLightPos[i].xyz); // light dir
		vec3 V = normalize(-fragPos);// eye coord
		vec3 R = reflect(-L, N); // reflection
		
		float incidenceAngle = max(0.0,dot(-L, N));
		
		if(incidenceAngle > 0.0)
		{
			diffuse = diffuseFrac * incidenceAngle * lightubo.perFragmentLightColor[i]; // diffuse component
		}
		
		if(lightubo.specularity > 0.0)
		{
			specular = vec3(pow(max(dot(R, V), 0.0), lightubo.specularity)); // specular component		
		}
		

		
		
		lightColor += (diffuse * lightubo.perFragmentLightIntensity[i]) + vec4(specular,1.0);		
	}
	
	outColor *=lightColor;
}