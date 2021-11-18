// #pragma once
// #include "d3dApp.h"
// #include "FrameResource.h"
// 
// using Microsoft::WRL::ComPtr;
// using namespace DirectX;
// using namespace DirectX::PackedVector;
// 
// #pragma comment(lib, "d3dcompiler.lib")
// #pragma comment(lib, "D3D12.lib")
// 
// const int gNumFrameResources = 3;
// 
// // Lightweight structure stores parameters to draw a shape.  This will
// // vary from app-to-app.
// struct RenderItem
// {
// 	RenderItem() = default;
// 
// 	// World matrix of the shape that describes the object's local space
// 	// relative to the world space, which defines the position, orientation,
// 	// and scale of the object in the world.
// 	XMFLOAT4X4 World = MathHelper::Identity4x4();
// 
// 	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
// 
// 	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
// 	// Because we have an object cbuffer for each FrameResource, we have to apply the
// 	// update to each FrameResource.  Thus, when we modify obect data we should set 
// 	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
// 	int NumFramesDirty = gNumFrameResources;
// 
// 	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
// 	UINT ObjCBIndex = -1;
// 
// 	Material* Mat = nullptr;
// 	MeshGeometry* Geo = nullptr;
// 
// 	// Primitive topology.
// 	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
// 
// 	// DrawIndexedInstanced parameters.
// 	UINT IndexCount = 0;
// 	UINT StartIndexLocation = 0;
// 	int BaseVertexLocation = 0;
// };
// 
// class TextureLoadClassApp : public D3DApp
// {
// public:
// 	TextureLoadClassApp(HINSTANCE hInstance);
// 	~TextureLoadClassApp();
// 
// 	virtual bool Initialize() override;
// 
// private:
// 	virtual void OnResize() override;
// 
// 	virtual void Update(const GameTimer& gt) override;
// 
// 	virtual void Draw(const GameTimer& gt) override;
// 
// 	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
// 	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
// 	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
// 
// 	void OnKeyboardInput(const GameTimer& gt);
// 	void UpdateCamera(const GameTimer& gt);
// 	void AnimateMaterials(const GameTimer& gt);
// 	void UpdateObjectCBs(const GameTimer& gt);
// 	void UpdateMaterialCBs(const GameTimer& gt);
// 	void UpdateMainPassCB(const GameTimer& gt);
// 
// 	void LoadTextures();
// 	void BuildRootSignature();
// 	void BuildDescriptorHeaps();
// 	void BuildShadersAndInputLayout();
// 	void BuildShapeGeometry(); 
// 	void BuildMaterials();
// 	void BuildRenderItems();
// 	void BuildFrameResources();
// 	void BuildPSOs();
// 	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
// 
// 	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
// 
// private:
// 	FrameResource* mCurrFrameResource = nullptr;
// 	int mCurrFrameResourceIndex = 0;
// 
// 	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
// 	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
// 	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
// 
// 	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
// 	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
// 	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
// 	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
// 
// 	// List of all the render items.
// 	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
// 
// 	// Render items divided by PSO.
// 	std::vector<RenderItem*> mOpaqueRitems;
// 
// 	PassConstants mMainPassCB;
// 
// 	ComPtr<ID3DBlob> mvsByteCode = nullptr;
// 	ComPtr<ID3DBlob> mpsByteCode = nullptr;
// 
// 	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
// 
// 	ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;
// 
// 	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
// 
// 	ComPtr<ID3D12PipelineState> mPSO = nullptr;
// 
// 	float mRadius = 5.0f;
// 	float mPhi = XM_PIDIV4;
// 	float mTheta = 1.5f * XM_PI;
// 
// 	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
// 	XMFLOAT4X4 mView = MathHelper::Identity4x4();
// 	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
// 	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
// 
// 	POINT mLastMousePos;
// 
// 	UINT mCbvSrvDescriptorSize = 0;
// };

