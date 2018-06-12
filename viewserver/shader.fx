//--------------------------------------------------------------------------------------
// File: Tutorial022.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
Texture2D texZero : register(t0);

Texture2D texOne : register(t1);

SamplerState samLinear : register(s0);

cbuffer VS_CONSTANT_BUFFER : register(b0)
	{
	float offsetx1;
	float offsety1;

	matrix world;
	matrix view;
	matrix projection;

	float4 campos;

	float4 offsets;

	float4 waveOffSet;
	};

struct VS_MODEL
	{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
	};

struct PS_MODEL
	{
	float4 Pos : SV_POSITION;
	float4 WorldPos : POSITION1;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
	};
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_MODEL VShader(VS_MODEL input)
	{
	PS_MODEL output;
	float4 pos = input.Pos;

	pos = mul(world, pos);	
	output.WorldPos = pos;
	pos = mul(view, pos);
	pos = mul(projection, pos);	
	
	matrix w = world;
	w._14 = 0;
	w._24 = 0;
	w._34 = 0;
	float4 norm;
	norm.xyz = input.Norm;
	norm.w = 1;
	norm = mul(w,norm);
	output.Norm = normalize(norm.xyz);
	output.Pos = pos;
	output.Tex = input.Tex;
	return output;
	}


PS_MODEL VShaderSun(VS_MODEL input)
{
	PS_MODEL output;
	float4 pos = input.Pos;

	pos = mul(world, pos);
	//USE THE COLOR AS A HEIGHT MULTIPLIER -  ALONG THE Y AXIS
	float4 color = texOne.SampleLevel(samLinear, input.Tex +float2(offsetx1, offsety1), 0);
	//heightmapping - using the red color of RGB
	pos.y += color.r; // 
										
	pos = mul(view, pos);
	//output.ViewPos = pos.xyz;
	pos = mul(projection, pos);
	output.Pos = pos;
	output.Tex = input.Tex;
	
	return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
//normal pixel shader
float4 PS(PS_MODEL input) : SV_Target
	{
	
	float4 texture1color = texZero.Sample(samLinear,input.Tex);

	//ambient lighting 
	float4 amb = (0.0005, 0.0005, 0.0005, 0.1) * texture1color;

	float3 lightposition = float3(1000, 1000, 1000);

	float3 normal = normalize(input.Norm);

	float3 worldpos = input.WorldPos.xyz / input.WorldPos.w;

	//diffuse light:
	float3 lightdirection = lightposition - worldpos;
	lightdirection = normalize(lightdirection);
	float diffuselight=dot(-normal, lightdirection);
	diffuselight = saturate(diffuselight);//saturate(x) ... if(x<0)x=0;else if(x>1) x=1;
	
	//specular light:
	float specular = 0;
	if (dot(lightdirection, normal) <= 0) {
		float3 r = reflect(lightdirection, normal);
		float3 camdir = campos - worldpos;
		camdir = normalize(camdir);
		specular = dot(r, camdir);
		specular = saturate(specular);
		specular = pow(specular, 30);
	}

	//light calculation:
	//diffuselight = diffuselight + specular;
	float4 SpecLightColor = float4(1, 1, 1, 1);//light color

	float4 color = texture1color * diffuselight + SpecLightColor*specular + amb;
	//float4 color = texture1color * diffuselight;
	color.a = 1;
	return color;
	}


//shader for the sky sphere
float4 PSsky(PS_MODEL input) : SV_Target
	{
	//return float4(input.Tex.y,input.Tex.y,0,1);
	float4 color = texZero.Sample(samLinear, input.Tex);
	//color.a = 1;
	return color;
	}

	struct VS_INPUT
	{
		float4 Pos : POSITION;
		float2 Tex : TEXCOORD0;
	};
	struct PS_INPUT
	{
		float4 Pos : SV_Position;
		float2 Tex : TEXCOORD0;
		float3 ViewPos : VIEW_POS;
	};

	//terrain shaders
	PS_INPUT vs_terrain(VS_INPUT input)
	{
		PS_INPUT output;
		output.Pos = mul(world, input.Pos);
		output.Pos.y += (texZero.SampleLevel(samLinear, input.Tex , 0).r * 15 - 5);
		output.Pos = mul(view, output.Pos);
		output.ViewPos = output.Pos.xyz; //<--- camera is at (0, 0, 0)
		output.Pos = mul(projection, output.Pos);

		output.Tex = input.Tex;
		return output;
	}

	float4 ps_terrain(PS_INPUT input) : SV_Target
	{
		float4 color = texOne.Sample(samLinear, input.Tex  * 80);
		float h = texZero.Sample(samLinear, input.Tex + offsets.xy).r;
		h *= 2.0f;
		color.rgb *= h;

		float dist = length(input.ViewPos.xz) / 250.0f;

		float4 far = float4(float3(1, 1, 1) *0.3, 1);
		//linear interpolation:

		float4 finalColor = lerp(color, far, saturate(dist*dist)); //<-- saturate faster than min
		return finalColor;
	}


	//	//water
	//	PS_INPUT vs_water(VS_INPUT input)
	//{
	//	PS_INPUT output;
	//	float4 pos = input.Pos;

	//	pos = mul(world, pos);

	//	//USE THE COLOR AS A HEIGHT MULTIPLIER -  ALONG THE Y AXIS
	//	float4 color1 = texZero.SampleLevel(samLinear, input.Tex + float2(offsetWave.x, offsetWave.y), 0);
	//	float4 color2 = texOne.SampleLevel(samLinear, input.Tex + float2(offsetWave.z, offsetWave.w), 0);

	//	//using the red color of RGB
	//	pos.y += (color1.r + color2.r)*2.0; // heightmapping!!!!
	//										//pos.y += (color1.r) * 10 - 10; 
	//	pos = mul(view, pos);

	//	output.ViewPos = pos.xyz;

	//	pos = mul(projection, pos);

	//	output.Pos = pos;
	//	output.Tex = input.Tex;
	//	//output.Tex1 = input.Tex1;
	//	return output;
	//}

	//float4 ps_water(PS_INPUT input) : SV_Target
	//{
	//	//return float4(input.Tex.y,input.Tex.y,0,1);

	//	float4 color = texZero.Sample(samLinear,  (input.Tex + offsets.xy));
	//	float4 color2 = texOne.Sample(samLinear, (input.Tex + offsets.xy));

	//	/*float3 vpos = input.ViewPos;
	//	vpos.y = 0;
	//	float dist = length(vpos);
	//	dist /= 225.0;
	//	if (dist > 1) dist = 1.0;
	//	float4 farcolor = float4(0.8, 0.9, 1.0, 1.0);
	//	farcolor.r *= dist;
	//	farcolor.g *= dist;
	//	farcolor.b *= dist;*/

	//	//return float4(dist, dist, dist, 1);

	//	color.r = (color.r + color2.r) / 5.0f;
	//	color.g = (color.g + color2.g) / 4.0f;
	//	color.b = (color.b + color2.b) / 7.0f;
	//	
	//	// 2 states: color and farcolor

	//	float4 far = float4(float3(1, 1, 1) *0.3, 1);
	//	//linear interpolation:
	//	float dist = length(input.ViewPos.xz) / 250.0f;
	//	float4 finalColor = lerp(color, far, saturate(dist*dist)); //<-- saturate faster than min

	//															   //float4 resultcolor = color *(1 - dist) + farcolor*dist;
	//															   //float4 resultcolor = farcolor*dist;
	//	finalColor.a = 1;
	//	return finalColor;
	//}

