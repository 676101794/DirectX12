 
 
/*  // Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
};

cbuffer cbPassObject : register(b1)
{
  	//float4x4 gView;
    //float4x4 gInvView;
    //float4x4 gProj;
    //float4x4 gInvProj;
    //float4x4 gViewProj;
    //float4x4 gInvViewProj;
    //float3 gEyePosW;
    //float cbPerObjectPad1;
    //float2 gRenderTargetSize;
    //float2 gInvRenderTargetSize;
    //float gNearZ;
    //float gFarZ;
    //float gTotalTime;
    //float gDeltaTime;  

    float4x4 gViewProj;
    float4 gPulseColor;
    float4 gAmbientLight;
    float3 gEyePosW;
    float gTime;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;                                            
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
}

cbuffer cbMatObject : register(b2)
{
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
	float4x4 gMatTransform;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
    //float3 NormalL : NORMAL;   
    float2 TexC    : TEXCOORD;
};

//关于SV_POSITION : SV即System Value 
//如果这个是绑定在从VS输出的数据结构上的话，意味着这个输出的数据结构包含了最终的转换过的，将用于光栅器的
struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
    float3 NormalW : NORMAL;
    float3 PosW  : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	//vin.PosL.xy += 0.5f*sin(vin.PosL.x)*sin(3.0f*gTime);
    //vin.PosL.z *= 0.6f + 0.4f*sin(2.0f*gTime);
	
    // Indirect lighting. test
    float4 ambient = gAmbientLight*gDiffuseAlbedo;

	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    vout.PosH = mul(posW, gViewProj);
    //vout.PosH =float4(vin.PosL, 1.0f);

	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    //vout.Color=float4(0.0f,0.0f,0.0f,1.0f);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    //测试在PixelShader里使用光照系统

    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// Indirect lighting.
    float4 ambient = gAmbientLight*gDiffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse material.
    litColor.a = gDiffuseAlbedo.a;

    return litColor;
 
    //const float pi = 3.14159;

    //return pin.Color; 

}
 */


//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);
SamplerState gsamLinear  : register(s0);

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorld; 
    float4x4 gTexTransform;
};

cbuffer cbPassObject : register(b0)
{
  	//float4x4 gView;
    //float4x4 gInvView;
    //float4x4 gProj;
    //float4x4 gInvProj;
    //float4x4 gViewProj;
    //float4x4 gInvViewProj;
    //float3 gEyePosW;
    //float cbPerObjectPad1;
    //float2 gRenderTargetSize;
    //float2 gInvRenderTargetSize;
    //float gNearZ;
    //float gFarZ;
    //float gTotalTime;
    //float gDeltaTime;  

    float4x4 gViewProj;
    float4 gPulseColor;
    float4 gAmbientLight;
    float3 gEyePosW;
    float gTime;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;                                            
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
}

cbuffer cbMatObject : register(b2)
{
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
	float4x4 gMatTransform;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
    float3 NormalL : NORMAL;   
    float2 TexC    : TEXCOORD;
};

//关于SV_POSITION : SV即System Value 
//如果这个是绑定在从VS输出的数据结构上的话，意味着这个输出的数据结构包含了最终的转换过的，将用于光栅器的
struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
    float3 NormalW : NORMAL;
    float3 PosW  : POSITION;
    float2 TexC: TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

    // Indirect lighting. test

	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    //vout.PosH=posW;
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    vout.PosH = mul(posW, gViewProj);

    // Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);

    //float4 MulResult=mul(float4(vin.TexC, 0.0f, 1.0f);
    vout.TexC = mul(texC, gMatTransform).xy;

	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    //vout.Color=float4(0.0f,0.0f,0.0f,1.0f);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.TexC) * gDiffuseAlbedo;

    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse material.
    litColor.a = diffuseAlbedo.a;

    return litColor;
}