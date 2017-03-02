#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;



struct VkLight
{
	vec4 color;
	vec4 position;
//	float range;
//	int lightType;
};

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform FragUbo {

	vec4 cameraPos;
	VkLight lights[4];
	float specularity;
	vec4 ambientLightColor;



} ubo;

layout(location = 0) out vec4 outColor;

void main() 
{


    outColor = vec4(fragColor,1.0) * texture(texSampler, fragTexCoord);
	vec4 lightColor = vec4(ubo.ambientLightColor.xyz,1.0f);
	float diffuseFrac = 1.0 - ubo.ambientLightColor.w;
	vec4 specular = vec4(0.0,0.0,0.0,0.0);
	vec4 diffuse = vec4(0.0,0.0,0.0,0.0);
	
	vec3 N = normalize(fragNormal);
	vec3 V = normalize(cross(vec3(fragPos),vec3(ubo.cameraPos)));
	
	for(int i = 0;i < 4;i++)
	{
		vec3 L = normalize(ubo.lights[i].position.xyz - fragPos); // light dir

		
		float incidenceAngle = max(0.0,dot(L, N));

		if(incidenceAngle > 0.0)
		{
			diffuse = diffuseFrac * incidenceAngle * ubo.lights[i].color; // diffuse component		
		}
		
		if(ubo.specularity > 0.0)
		{
			
			vec3 H = normalize(L+V);
			float specAngle = max(dot(H, N), 0.0);
			specular = pow(specAngle, ubo.specularity) * vec4(1.0f,1.0f,1.0f,1.0f);	
		}
		

		
		
		lightColor += diffuse + specular;
	}
	
	outColor *=lightColor;
}