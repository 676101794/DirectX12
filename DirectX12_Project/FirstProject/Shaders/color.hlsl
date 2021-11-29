//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

//单行注释：ctrl+/ 
//多行注释：alt+shift+A

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
};

cbuffer cbPassObject : register(b1)
{
/* 	float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime; */

    float4x4 gViewProj;
    float4 gPulseColor;
    float gTime;
}


struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	//vin.PosL.xy += 0.5f*sin(vin.PosL.x)*sin(3.0f*gTime);
  	//vin.PosL.z *= 0.6f + 0.4f*sin(2.0f*gTime);
	
	// Transform to homogeneous clip space.
    float4 curPos=float4(vin.PosL, 1.0f);
    
    //为啥这个gWorld没有值
    float4 posW = mul(curPos, gWorld);
    vout.PosH = mul(posW, gViewProj);
	
    //vout.PosH =float4(vin.PosL, 1.0f);

	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// const float pi = 3.14159;

	// // 随着时间流逝，令正弦函数的值在[0,1]区间内周期性地变化
	// float s = 0.5f*sin(2*gTimer - 0.25f*pi)+0.5f;

	// // 基于参数s 在pin.Color与gPulseColor之间进行线性插值
	// float4 c = lerp(pin.Color, gPulseColor, s);

    //return pin.Color;
	//return c;

    const float pi = 3.14159;

     // 随着时间流逝，令正弦函数的值在[0,1]区间内周期性地变化
    float s = 0.5f*sin(2*gTime - 0.25f*pi)+0.5f;

     // 基于参数s 在pin.Color与gPulseColor之间进行线性插值
    float4 c = lerp(pin.Color, gPulseColor, s);

    return c;
}
