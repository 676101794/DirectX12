#pragma once
#include "d3dApp.h"
#include "FrameResource.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class RenderItem;

const int gNumFrameResources = 3;

namespace MyApp
{
	struct Vertex
	{
		XMFLOAT3 Pos;
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

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void LoadTextures();
	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildFrameResources();
	void BuildRenderItems();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	//Pipeline State Object
	void BuildPSO();

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCBs(const GameTimer& gt);

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

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

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;

	int mCurrFrameResourceIndex = 0;
	FrameResource* mCurrFrameResource = nullptr;

	bool mIsWireframe = false;
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	UINT mPassCbvOffset = 0;
private:
	//存有所有渲染项的向量
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	//根据PSO来划分渲染项
	std::vector<RenderItem*> mOpaqueRitems;
	std::vector<RenderItem*> mTransparentRitems;

	PassConstants mMainPassCB;
	//test
	D3D12_VERTEX_BUFFER_VIEW vbv;
};