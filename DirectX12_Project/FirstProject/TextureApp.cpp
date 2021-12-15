#include "TextureApp.h"
#include "RenderItem.h"
#include "d3dUtil.h"

TextureApp::TextureApp(HINSTANCE hInstance):FirstApp(hInstance)
{

}

TextureApp::~TextureApp()
{

}

bool TextureApp::Initialize()
{
	if (!FirstApp::Initialize())
	{
		return false;
	}

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void TextureApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../../Textures/WoodCrate01.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));

	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
}

void TextureApp::BuildRenderItems()
{
	auto SquareRitem = std::make_unique<RenderItem>();
	//存入世界矩阵
	XMStoreFloat4x4(&SquareRitem->World, XMMatrixScaling(0.8f, 0.8f , 0.8f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	SquareRitem->ObjCBIndex = 0;
	SquareRitem->Geo = mGeometries["shapeGeo"].get();
	SquareRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	SquareRitem->IndexCount = SquareRitem->Geo->DrawArgs["Square"].IndexCount;
	SquareRitem->StartIndexLocation = SquareRitem->Geo->DrawArgs["Square"].StartIndexLocation;
	SquareRitem->BaseVertexLocation = SquareRitem->Geo->DrawArgs["Square"].BaseVertexLocation;
	mAllRitems.push_back(std::move(SquareRitem));

	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void TextureApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\colorTexture.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\colorTexture.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TexCoord", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void TextureApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
}

void TextureApp::BuildFrameResources()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void TextureApp::BuildRootSignature()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void TextureApp::UpdateMainPassCBs(const GameTimer& gt)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void TextureApp::UpdateMaterialCBs(const GameTimer& gt)
{
	throw std::logic_error("The method or operation is not implemented.");
}
