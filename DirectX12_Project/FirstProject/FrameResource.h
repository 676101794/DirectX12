#pragma once

#include "d3dUtil.h"
#include "UploadBuffer.h"
#include <DirectXMath.h>

using namespace DirectX;

/*因为是通过偏移来找到位置，所以和寄存器中的结构体数据一定要一一对应起来*/
struct ObjectConstants
{
	XMFLOAT4X4 WorldMatrix = MathHelper::Identity4x4();
    XMFLOAT4X4 gTexTransform = MathHelper::Identity4x4();
};

struct PassConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
    XMFLOAT4 gPulseColor = XMFLOAT4(Colors::Green);
	//环境光
	XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
    XMFLOAT3 gEyePos = { 0.0f, 0.0f, 0.0f };
    float gTimer;

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
struct FrameResource
{
public:
    
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

	// 在GPU处理完与此命令分配器相关的命令之前，我们不能对它进行重置。
	// 所以每一帧都要有它们自己的命令分配器
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// 在GPU执行完引用此常量缓冲区的命令之前，我们不能对它进行更新。
	// 因此每一帧都要有它们自己的常量缓冲区
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;

    // 物体常量缓冲区
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // 通过围栏值将命令标记到此围栏点，这使我们可以检测到GPU是否还在使用这些帧资源
    UINT64 Fence = 0;
};