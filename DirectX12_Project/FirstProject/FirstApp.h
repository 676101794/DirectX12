#pragma once
#include "d3dApp.h"
#include "FrameResource.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
class RenderItem;


namespace MyApp
{
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
		XMFLOAT3 Normal;
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

protected:
	virtual void BuildShadersAndInputLayout();
	virtual void BuildFrameResources();
	virtual void BuildRenderItems();
	virtual void BuildSkullGeometry();
	virtual void BuildRootSignature();

	void LoadTextures();
	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuildBoxGeometry();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	//Pipeline State Object
	void BuildPSO();

	void UpdateCamera(const GameTimer& gt);

	virtual void OnKeyboardInput(const GameTimer& gt);
	virtual void UpdateObjectCBs(const GameTimer& gt);
	virtual void UpdateMainPassCBs(const GameTimer& gt);
	virtual void UpdateMaterialCBs(const GameTimer& gt);

protected:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	//几何体的映射
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	//堆描述符
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	//输入布局的
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	float mRadius = 5.0f;
	float mPhi = XM_PIDIV4;
	float mTheta = 1.5f * XM_PI;

	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;

	UINT mPassCbvOffset = 0;
	UINT mMaterialCbvOffset = 0;

	//存有所有渲染项的向量
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	//根据PSO来划分渲染项
	std::vector<RenderItem*> mOpaqueRitems;
	std::vector<RenderItem*> mTransparentRitems;

	PassConstants mMainPassCB;

	int mCurrFrameResourceIndex = 0;
	FrameResource* mCurrFrameResource = nullptr;
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

private:
	//纹理
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	//管线状态对象
	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	POINT mLastMousePos;

	bool mIsWireframe = false;

};