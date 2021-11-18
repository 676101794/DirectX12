//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "FirstApp.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

// class BoxApp : public D3DApp
// {
// public:
// 	BoxApp(HINSTANCE hInstance);
//     BoxApp(const BoxApp& rhs) = delete;
//     BoxApp& operator=(const BoxApp& rhs) = delete;
// 	~BoxApp();
// 
// 	virtual bool Initialize()override;
// 
// private:
//     virtual void OnResize()override;
//     virtual void Update(const GameTimer& gt)override;
//     virtual void Draw(const GameTimer& gt)override;
// 
//     virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//     virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//     virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
// 
//     void BuildDescriptorHeaps();
// 	void BuildConstantBuffers();
//     void BuildRootSignature();
//     void BuildShadersAndInputLayout();
//     void BuildBoxGeometry();
//     void BuildPSO();
// 
// private:
//     
//     ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//     ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
// 
//     std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
// 
// 	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
// 
//     ComPtr<ID3DBlob> mvsByteCode = nullptr;
//     ComPtr<ID3DBlob> mpsByteCode = nullptr;
// 
//     std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
// 
//     ComPtr<ID3D12PipelineState> mPSO = nullptr;
// 
//     XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
//     XMFLOAT4X4 mView = MathHelper::Identity4x4();
//     XMFLOAT4X4 mProj = MathHelper::Identity4x4();
// 
//     float mTheta = 1.5f*XM_PI;
//     float mPhi = XM_PIDIV4;
//     float mRadius = 5.0f;
// 
//     POINT mLastMousePos;
// };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        FirstApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}
