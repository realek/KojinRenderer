#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct VkLight
{
	vec4 color;
	vec4 position;
	vec4 direction;
	vec4 lightProps; //type, intensity, falloff, angle
	mat4 lightBiasedMVP;

};

const mat4 iMat = 
mat4(
 1,0,0,0,
 0,1,0,0,
 0,0,1,0,
 0,0,0,1
);

layout(location = 0) in vec4 inColor;
layout(location = 1) flat in vec4 inMaterialColor;
layout(location = 2) flat in float inMaterialSpecularity;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) flat in vec4 inAmbientLight;
layout(location = 5) in vec3 inFragPos;
layout(location = 6) in vec3 inFragNormal;
layout(location = 7) in vec4 inVertex;
layout(location = 8) flat in VkLight inLights[6];


layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2DArray depthSampler;


layout(location = 0) out vec4 outColor;

const float gamma = 2.2f;

float textureProj(vec4 P, vec2 off, int arrayIdx)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		if((shadowCoord.x >= 0.0) && (shadowCoord.x <= 1.0f) && (shadowCoord.y >= 0.0) && (shadowCoord.y <= 1.0f) )
		{
			vec3 arrTexCoord = vec3(shadowCoord.xy + off,arrayIdx);
			float dist = texture( depthSampler, arrTexCoord ).r;
			if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
			{
				shadow = 0.1f;
			}
		}

	}
	return shadow;
}

float filterPCF(vec4 sc, int index)
{
	ivec2 texDim = textureSize(depthSampler, 0).xy;
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

void main() 
{
	outColor = inColor * (inMaterialColor*texture(texSampler, inTexCoord));

	vec4 lightColor = vec4(0.0f,0.0f,0.0f,0.0f);
	float diffuseFrac = 1.0 - inAmbientLight.w;

	vec3 N = normalize(inFragNormal);

	for(int i = 0;i < 6; ++i)
	{
		float intensity = inLights[i].lightProps[1];
		vec4 specular = vec4(0.0,0.0,0.0,0.0);
		vec4 diffuse = vec4(0.0,0.0,0.0,0.0);
		vec3 L; // fragment light dir
		vec3 V; // fragment eye 
		vec3 D; // light forward from rotation
		float atten = 1.0f;
		float shadowCoef = 1.0f;
		D = normalize(-inLights[i].direction.xyz);
		
		if(inLights[i].lightProps[0] == 2)
		{
			V = normalize(-inLights[i].direction.xyz - inFragPos);
			L = D;
		}
		else
		{
			L = normalize(inLights[i].position.xyz - inFragPos); 
			V = normalize(-inLights[i].position.xyz);
		}
		
		if(inLights[i].lightProps[0] == 0 || inLights[i].lightProps[0] == 1) //is point or spot
		{
			float dist = length(inLights[i].position.xyz - inFragPos);
			if(dist <= inLights[i].lightProps[2]) //falloff
			{
				atten = clamp(1.0 - (dist*dist)/pow(inLights[i].lightProps[2],2), 0.0, 1.0);
				if(inLights[i].lightProps[0] == 1) // is spot thus extra per vert testing
				{
					float coneAngle = degrees(acos(dot(L, D)));
					if(coneAngle >= inLights[i].lightProps[3]) //angle
					{
						atten = 0.0f;
					}
					else
						atten = clamp(atten - coneAngle/inLights[i].lightProps[3], 0.0, 1.0);
				}
			}
			else
				atten = 0.0f;

		}
		
		if(atten > 0.0f)
		{
			float incidenceAngle = max(0.0,dot(L, N));
			if(incidenceAngle > 0.0)
			{
				diffuse = diffuseFrac * incidenceAngle * inLights[i].color; // diffuse component		
			}
		
			if(inMaterialSpecularity > 0.0)
			{
			
				vec3 H = normalize(L+V);
				float specAngle = max(dot(H, N), 0.0);
				if(specAngle > 0.0)
				{
					specular = pow(specAngle, inMaterialSpecularity) * vec4(1.0f,1.0f,1.0f,1.0f);			
				
				}

			}
		}
		
		if(inLights[i].lightBiasedMVP != iMat)
		{
			vec4 sVertPos = inLights[i].lightBiasedMVP*inVertex;
            shadowCoef = filterPCF(sVertPos,i);
		}
		lightColor += shadowCoef*atten*intensity*(diffuse+specular);



	}
	outColor *=vec4(inAmbientLight.xyz,0.0f)+vec4(lightColor.xyz,1.0f);
	outColor = vec4(clamp(outColor.x,0.0f,1.0f),clamp(outColor.y,0.0f,1.0f),clamp(outColor.z,0.0f,1.0f),outColor.w);
  //SHADOWMAP VISUAL
  //outColor = vec4(1.0-vec3(LinearizeDepth(texture(depthSampler, fragTexCoord).x)), 1.0);
  //  outColor.rgb = pow(outColor.rgb,vec3(1.0f/gamma));
	
}