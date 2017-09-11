#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct VkLight
{
	vec4 color;
	vec4 position;
	vec4 direction;
	vec4 lightProps; //type, intensity, falloff, angle
};

layout(location = 0) in vec4 inColor;
layout(location = 1) flat in vec4 inMaterialColor;
layout(location = 2) flat in float inMaterialSpecularity;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) flat in vec4 inAmbientLight;
layout(location = 5) in vec3 inFragPos;
layout(location = 6) in vec3 inN;
layout(location = 7) in vec4 inVertex;
layout(location = 8) flat in VkLight inLights[6];


layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2DArray depthSampler;
layout(set = 1, binding = 2) uniform BiasMatrices
{
	mat4 biasVP[6];
	
} shadow_data;

layout(location = 0) out vec4 outColor;

const float gamma = 2.2f;

float textureProj(vec4 P, vec2 off, int arrayIdx)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( depthSampler, vec3(shadowCoord.xy + off,arrayIdx) ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = inAmbientLight.a;
		}

	}
	return shadow;
}


vec4 vec4_eq(vec4 x, vec4 y) {
  return 1.0 - abs(sign(x - y));
}


float filterPCF(ivec2 texDim, vec4 sc, int index)
{
	float scale = 1.0f;
	float dx = scale / float(texDim.x);
	float dy = scale / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y), index);
			count++;
		}
	
	}
	return shadowFactor / count;
}

//float LinearizeDepth(float depth)
//{
// float n = 1.0; // camera z near
//  float f = 128.0; // camera z far
//  float z = depth;
//  return (2.0 * n) / (f + n - z * (f - n));	
//}

vec3 Clamp(vec3 value, float min, float max)
{
	value.x = clamp(value.x, min, max);
	value.y = clamp(value.y, min, max);
	value.z = clamp(value.z, min, max);
	return value;
}

void main() 
{
	outColor = inColor * (inMaterialColor*texture(texSampler, inTexCoord));

	vec4 lightColor = vec4(0.0f,0.0f,0.0f,0.0f);
	float diffuseFrac = 1.0 - inAmbientLight.w;

	ivec2 texDim = textureSize(depthSampler, 0).xy;
	
	for(int i = 0;i < 6; ++i)
	{
		vec4 specular = vec4(0.0,0.0,0.0,0.0);
		vec4 diffuse = vec4(0.0,0.0,0.0,0.0);
		vec3 L; // fragment light dir
		vec3 V; // fragment eye 
		vec3 D; // light forward from rotation
		float atten = 1.0f;
		
		D = normalize(-inLights[i].direction.xyz);
		V = normalize(-inLights[i].direction.xyz - inFragPos);
		int lightType = int(inLights[i].lightProps[0]);
		
		if(lightType == 2)
		{
			
			L = D;
		}
		else
		{
			L = normalize(inLights[i].position.xyz - inFragPos); 
		}
		
		if(lightType != 2) //is point or spot
		{
			float dist = length(inLights[i].position.xyz - inFragPos);
			float falloff = inLights[i].lightProps[2];
			if(dist < falloff) //falloff
			{
				atten = max(0.0, 1.0 - dist*dist/(pow(falloff,2)));
				atten*=atten;
				if(lightType == 1) // is spot thus extra per vert testing
				{
					float coneAngle = degrees(acos(dot(L, D)));
					float angle = inLights[i].lightProps[3];
					if(coneAngle < angle) //angle
					{
						atten = max(0, atten - (coneAngle/angle));
						atten*atten;

					}
					else
						atten = 0.0f;						
				}
			}
			else
				atten = 0.0f;

		}
		
		if(atten > 0)
		{
			diffuse = diffuseFrac * max(0.0,dot(L, inN)) * inLights[i].color; // diffuse component
			if(inMaterialSpecularity > 0)
			{
				specular = vec4(pow(max(dot(reflect(-L,inN), V), 0.0), inMaterialSpecularity));	
			}
			vec4 shc = shadow_data.biasVP[i] * inVertex;
			shc = shc/shc.w;
			atten *= filterPCF(texDim, shc,i); //meh performance loss
			lightColor += atten*(diffuse+specular);
		}
        




	}
	outColor.rgb = pow(outColor.rgb,vec3(1.0f/gamma));
	outColor *=vec4(inAmbientLight.xyz,0.0f)+lightColor;
    outColor = clamp(outColor,vec4(0),vec4(1));
  //SHADOWMAP VISUAL
  //outColor = vec4(1.0-vec3(LinearizeDepth(texture(depthSampler, fragTexCoord).x)), 1.0);

	
}