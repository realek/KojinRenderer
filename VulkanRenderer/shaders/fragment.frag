#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec4 shadowFragPos;

struct VkLightProps
{
	int lightType;
	float intensity;
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
layout(set = 1, binding = 1) uniform sampler2D depthSampler;
layout(set = 1, binding = 2) uniform FragUbo {

	//vec4 cameraPos;
	VkLight lights[6];
	vec4 ambientLightColor;
	vec4 materialDiffuse;
	float specularity;
} ubo;

layout(location = 0) out vec4 outColor;

const float gamma = 2.2f;

float textureProj(vec4 P, vec2 off)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		if((shadowCoord.x >= 0.0) && (shadowCoord.x <= 1.0f) && (shadowCoord.y >= 0.0) && (shadowCoord.y <= 1.0f) )
		{
			float dist = texture( depthSampler, shadowCoord.xy + off ).r;
			if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
			{
				shadow = 0.1f;
			}
		}

	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(depthSampler, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

float ComputeShadow(vec4 shadowCoord)
{
	float bias = 0.005;
	float visibility = 1.0;
	
	if((shadowCoord.x >= 0.0) && (shadowCoord.x <= 1.0f) && (shadowCoord.y >= 0.0) && (shadowCoord.y <= 1.0f) )
		{
				if ( texture( depthSampler, (shadowCoord.xy/shadowCoord.w) ).z < (shadowCoord.z-bias)/shadowCoord.w ){
					visibility = 0.1;
				}
		}
	return visibility;
}

float LinearizeDepth(float depth)
{
  float n = 1.0; // camera z near
  float f = 128.0; // camera z far
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

void main() 
{


    outColor = vec4(fragColor,1.0) * (ubo.materialDiffuse*texture(texSampler, fragTexCoord));
	vec4 lightColor = vec4(0.0f,0.0f,0.0f,0.0f);
	float diffuseFrac = 1.0 - ubo.ambientLightColor.w;
	vec4 specular = vec4(0.0,0.0,0.0,0.0);
	vec4 diffuse = vec4(0.0,0.0,0.0,0.0);
	float atten = 1.0f;
	vec3 N = normalize(fragNormal);
	vec3 L; // fragment light dir
	vec3 V; // fragment eye 
	vec3 D; // light forward from rotation
	float shadow = filterPCF(shadowFragPos/shadowFragPos.w);
	//float shadow = ComputeShadow(shadowFragPos/shadowFragPos.w);
	for(int i = 0;i < 6;i++)
	{
		D = normalize(-ubo.lights[i].direction.xyz);
		if(ubo.lights[i].lightProps.lightType == 2)
		{
			V = normalize(-ubo.lights[i].direction.xyz - fragPos);
			L = D;
		}
		else
		{
			L = normalize(ubo.lights[i].position.xyz - fragPos); 
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
				atten = clamp(1.0 - (dist*dist)/pow(ubo.lights[i].lightProps.falloff,2), 0.0, 1.0);
				if(ubo.lights[i].lightProps.lightType == 1) // is spot thus extra per fragment testing
				{
					float coneAngle = degrees(acos(dot(L, D)));
					if(coneAngle >= ubo.lights[i].lightProps.angle)
					{
						atten = 0.0f;
					}
					else
						atten = clamp(atten - coneAngle/ubo.lights[i].lightProps.angle,0.0,1.0);
				}
			}
			else
				atten = 0.0f;

		}

		
		
		lightColor += shadow*(atten*(intensity*(diffuse + specular)));
	}
		//outColor *=vec4(ubo.ambientLightColor.xyz,0.0f)+vec4(lightColor.xyz,1.0f);
		outColor *=vec4(ubo.ambientLightColor.xyz,0.0f)+vec4(lightColor.xyz,1.0f);
		//SHADOWMAP VISUAL
		//outColor = vec4(1.0-vec3(LinearizeDepth(texture(depthSampler, fragTexCoord).x)), 1.0);
		//outColor.rgb = pow(outColor.rgb,vec3(1.0f/gamma));
	
}