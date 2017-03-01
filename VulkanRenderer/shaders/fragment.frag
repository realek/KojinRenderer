#version 450
#extension GL_ARB_separate_shader_objects : enable

const int MAX_LIGHT_NR = 8;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(set = 1, binding = 0) uniform sampler2D texSampler;

struct Light
{
	vec4 position;
	vec4 color;
	vec4 specularColor;
	float angle;
	float range;
	int lightType;
};

layout(set = 1, binding = 1) uniform LightingBufferObject {

	vec4 ambientLightColor;
	Light lights[MAX_LIGHT_NR];
	float specularity;

} lightubo;
layout(location = 0) out vec4 outColor;

void main() {

    outColor = vec4(fragColor,1.0) * texture(texSampler, fragTexCoord);
	vec4 lightColor = vec4(lightubo.ambientLightColor.xyz,1.0);
	float diffuseFrac = 1.0 - lightubo.ambientLightColor.w;
	vec3 specular = vec3(0.0,0.0,0.0);
	vec4 diffuse = vec4(0.0,0.0,0.0,0.0);
	
	vec3 N = normalize(fragNormal);
	for(int i = 0;i < MAX_LIGHT_NR;i++)
	{
		vec3 L = normalize(lightubo.lights[i].position.xyz - fragPos); // light dir
		vec3 V = normalize(fragPos);// eye coord
		vec3 R = reflect(L, N); // reflection
		
		float incidenceAngle = max(0.0,dot(L, N));
		
		if(incidenceAngle > 0.0)
		{
			diffuse = diffuseFrac * incidenceAngle * lightubo.lights[i].color; // diffuse component
		}
		
		if(lightubo.specularity > 0.0)
		{
			specular = vec3(pow(max(dot(R, V), 0.0), lightubo.specularity)); // specular component		
		}
		

		
		
		lightColor += diffuse + vec4(specular,1.0);//*lightubo.lights[i].specularColor);		
	}
	
	outColor *=lightColor;
}