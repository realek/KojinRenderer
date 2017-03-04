#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;


struct VkLightProps
{
	int lightType;
	int intensity;
	float falloff;
	float angle;
};

struct VkLight
{
	vec4 color;
	vec4 position;
	vec4 direction;
	VkLightProps lightProps;
//	float range;

};

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform FragUbo {

	//vec4 cameraPos;
	VkLight lights[4];
	vec4 ambientLightColor;
	float specularity;
} ubo;

layout(location = 0) out vec4 outColor;

void main() 
{


    outColor = vec4(fragColor,1.0) * texture(texSampler, fragTexCoord);
	vec4 lightColor = vec4(ubo.ambientLightColor.xyz,1.0f);
	float diffuseFrac = 1.0 - ubo.ambientLightColor.w;
	vec4 specular = vec4(0.0,0.0,0.0,0.0);
	vec4 diffuse = vec4(0.0,0.0,0.0,0.0);
	float atten = 1.0f;
	vec3 N = normalize(fragNormal);
	vec3 L;
	vec3 V;
	for(int i = 0;i < 4;i++)
	{
		if(ubo.lights[i].lightProps.lightType == 2)
		{
			L = normalize(-ubo.lights[i].direction.xyz); // light dir
			V = normalize(-ubo.lights[i].direction.xyz - fragPos);
		}
		else
		{
			L = normalize(ubo.lights[i].position.xyz - fragPos); // light dir
			V = normalize(-ubo.lights[i].position.xyz);
		}


	    float intensity = ubo.lights[i].lightProps.intensity;
		
		
		float incidenceAngle = max(0.0,dot(L, N));
		if(incidenceAngle > 0.0)
		{
			diffuse = diffuseFrac * incidenceAngle * ubo.lights[i].color; // diffuse component		
		}
		
		if(ubo.specularity > 0.0)
		{
			
			vec3 H = normalize(L+V);
			float specAngle = max(dot(H, N), 0.0);
			if(specAngle > 0.0)
			{
				specular = pow(specAngle, ubo.specularity) * vec4(1.0f,1.0f,1.0f,1.0f);			
				
			}

		}
		
		if(ubo.lights[i].lightProps.lightType == 0 || ubo.lights[i].lightProps.lightType == 1) //is point or spot
		{
			float dist = length(ubo.lights[i].position.xyz - fragPos);
			if(dist <= ubo.lights[i].lightProps.falloff)
			{
				atten = clamp(1.0 - pow(dist,2)/pow(ubo.lights[i].lightProps.falloff,2), 0.0, 1.0);
				if(ubo.lights[i].lightProps.lightType == 1) // is spot thus extra per fragment testing
				{
					
				}
			}
			else
				atten = 0.0f;



		}

		
		
		lightColor += atten*intensity*(diffuse + specular);
	}
	
	outColor *=lightColor;
}