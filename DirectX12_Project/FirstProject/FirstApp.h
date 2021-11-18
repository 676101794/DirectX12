#pragma once
#include "d3dApp.h"
#include "MathHelper.h"
//#include "FrameResource.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


namespace MyApp
{
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};

	struct VertexColor
	{
		XMFLOAT4 Color;
	};
}

class FirstApp :public D3DApp
{
public:
	FirstApp(HINSTANCE hInstance);
	~FirstApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	//std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	//纹理
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	//几何体的映射
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;

	//堆描述符
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	//输入布局的
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	//盒子几何体
	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

	//管线状态对象
	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	float mRadius = 5.0f;
	float mPhi = XM_PIDIV4;
	float mTheta = 1.5f * XM_PI;

	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	POINT mLastMousePos;

	const int gNumFrameResources = 3;
	//std::vector<std::unique_ptr<FrameResource>> mFrameResources;
};

