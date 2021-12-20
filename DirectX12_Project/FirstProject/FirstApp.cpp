#include "FirstApp.h"
#include "RenderItem.h"
#include "GeometryGenerator.h"
#include "LightApp.h"
#include "TextureApp.h"
//#include <DirectXColors.h>

//这儿为啥放在.h文件会出问题。
extern const int gNumFrameResources = 3;

FirstApp::FirstApp(HINSTANCE hInstance):D3DApp(hInstance)
{

}

FirstApp::~FirstApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool FirstApp::Initialize()
{
	if(!D3DApp::Initialize())
	{
		return false;
	}

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildSkullGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBufferViews();
	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void FirstApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void FirstApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// 循环整个mFrameResource数据，并进行更新
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// GPU是否已经执行当前帧资源的所有命令？如果没有，等待GPU执行到这个栅栏点。
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMainPassCBs(gt);
	UpdateMaterialCBs(gt);
}

void FirstApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// 重复使用记录命令的相关内存
	// 只有当与GPU关联的命令列表执行完成时，我们才能将其重置
	ThrowIfFailed(cmdListAlloc->Reset());

	// 在通过ExecuteCommandList方法将某个命令列表加入命令队列后，我们便可以重置该命令列表。以
	// 此来复用命令列表及其内存
	ThrowIfFailed(mCommandList->Reset(
		cmdListAlloc.Get(),mPSO.Get()));

	// 设置视口和裁剪矩形。它们需要随着命令列表的重置而重置
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 对资源的状态进行转换，将资源从呈现状态转换为渲染目标状态
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 清除后台缓冲区和深度缓冲区
	mCommandList->ClearRenderTargetView(
		CurrentBackBufferView(),
		DirectX::Colors::LightSteelBlue, 0, nullptr);

	mCommandList->ClearDepthStencilView(
		DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH |
		D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 指定将要渲染的缓冲区
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(),
		true, &DepthStencilView());

	//描述符堆不一定要必须指定。

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	//使用根描述符设置PassCB参数
	UINT PassCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto PassCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS PassCBAddress = PassCB->GetGPUVirtualAddress() + 0 * PassCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(0,PassCBAddress);

	//设置材质的根
	SetMaterialParamater();

	//绘制RenderItems
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// 完成命令的记录
	ThrowIfFailed(mCommandList->Close());

	// 将待执行的命令列表加入命令队列
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换后台缓冲区和前台缓冲区
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// 等待此帧的命令执行完毕。当前的实现没有什么效率，也过于简单
	// 我们在后面将重新组织渲染部分的代码，以免在每一帧都要等待
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void FirstApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void FirstApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void FirstApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx =  (0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void FirstApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../../Textures/WoodCrate01.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));

	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
}

void FirstApp::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)mOpaqueRitems.size();

	// 建立描述符堆，描述堆的描述符容量为帧数*（每帧所需要绘制的物体数量+1），+2是因为每帧都还需要绘制一个passCB和一个MaterialCB
	UINT numDescriptors = (objCount + 1) * gNumFrameResources;

	// 因为会首先建立ObjConstantBuffer，使用同一个描述符堆，PassConstantsBuffer会有一个偏移值。偏移值的量为ObjCount*gNumFrameResource;
	// 然后再建立PassConstantBuffer，MaterialConstantsBuffer会有一个偏移值，偏移量gNumFrameResource;
	mPassCbvOffset = objCount * gNumFrameResources;
	//mMaterialCbvOffset = mPassCbvOffset + gNumFrameResources;
	
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap)));
}

void FirstApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	//grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	grass->DiffuseAlbedo = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.5f, 0.5f, 0.5f);
	grass->Roughness = 0.125f;

	// This is not a good water material definition, but we do not have all the rendering
	// tools we need (transparency, environment reflection), so we fake it for now.
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	auto woodCrate = std::make_unique<Material>();
	woodCrate->Name = "woodCrate";
	woodCrate->MatCBIndex = 0;
	woodCrate->DiffuseSrvHeapIndex = 0;
	woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodCrate->Roughness = 0.2f;

	mAllMaterials["woodCrate"] = std::move(woodCrate);
	mAllMaterials["grass"] = std::move(grass);
	mAllMaterials["water"] = std::move(water);
}

void FirstApp::SetMaterialParamater()
{

}

void FirstApp::BuildConstantBufferViews()
{
	//创建描述符  PassCB的描述符 ObjectCB的描述符 MaterialCB的描述符

	//根据多少帧，以及每帧需要绘制的物体，来创建描述符！
// 	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
// 
// 	UINT objCount = (UINT)mOpaqueRitems.size();
// 
// 	for(int frameIndex=0; frameIndex < gNumFrameResources; ++frameIndex)
// 	{
// 		auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
// 
// 		for(UINT i=0;i<objCount;++i)
// 		{
// 			//获取到当前帧的常量缓冲区的GPU地址
// 			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
// 
// 			//偏移到第i个对象的常量缓存区地址
// 			cbAddress += i * objCBByteSize;
// 
// 			//计算出在描述符堆中的偏移。
// 			int heapIndex = frameIndex * objCount + i;
// 			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
// 			handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);
// 
// 			//在该偏移处创建描述符
// 			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
// 			cbvDesc.BufferLocation = cbAddress;
// 			cbvDesc.SizeInBytes = objCBByteSize;
// 
// 			md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
// 		}
// 	}
// 
// 	//计算passCB的大小
// 	UINT passCBByteSize= d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
// 
// 	for(int frameIndex=0;frameIndex<gNumFrameResources;frameIndex++)
// 	{
// 		auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
// 		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();
// 
// 		//因为每帧的PassCB都一样，而不需要像ObjectConstantBuffer一样针对每个物体都进行处理。所以只需要对帧进行偏移。
// 		int heapIndex = mPassCbvOffset + frameIndex;
// 		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
// 		handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);
// 
// 		//在堆的该偏移上设置描述符
// 		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
// 		cbvDesc.BufferLocation = cbAddress;
// 		cbvDesc.SizeInBytes = passCBByteSize;
// 
// 		md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
// 	}
}

void FirstApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	//第一个根参数用于存放各种需要经常变化的参数。
	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;

	//第三个参数为要绑定的寄存器，没有显示地指定则直接为0
// 	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
// 	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
// 
// 	//用于描述世界矩阵  寄存器绑定为0
// 	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
// 	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
// 	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	//使用描述符用于根参数
	slotRootParameter[0].InitAsConstantBufferView(1);

	slotRootParameter[1].InitAsConstantBufferView(0);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void FirstApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void FirstApp::BuildBoxGeometry()
{
	//通过Position和Index进行几何的绘制。
	std::vector<MyApp::Vertex> SquareVertices
	{
		MyApp::Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		MyApp::Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		MyApp::Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		MyApp::Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		MyApp::Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		MyApp::Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		MyApp::Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		MyApp::Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	//点和颜色
	std::vector<std::uint16_t> SquareIndices
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7

	};

	UINT SquareVertexOffset = 0;
	UINT SquareIndexOffset = 0;

	//顶点步长，格式等等。
	SubmeshGeometry SquareSubMesh;
	SquareSubMesh.IndexCount = (UINT)SquareIndices.size();
	SquareSubMesh.StartIndexLocation = SquareVertexOffset;
	SquareSubMesh.BaseVertexLocation = SquareIndexOffset;

	//通过GeometryGenerator创建球形
// 	GeometryGenerator geoGen;
// 	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);//geoGen.CreateSphere(50.0f, 20, 20);
// 	UINT SphereVertexOffset = SquareVertexOffset + (UINT)SquareVertices.size();
// 	UINT SphereIndexOffset = SquareIndexOffset + (UINT)SquareIndices.size();

// 	SubmeshGeometry SphereSubmesh;
// 	SphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
// 	SphereSubmesh.StartIndexLocation = SphereIndexOffset;
// 	SphereSubmesh.BaseVertexLocation = SphereVertexOffset;

	auto totalVertexCount = SquareVertices.size();
	std::vector<MyApp::Vertex> vertices(totalVertexCount);
	UINT k = 0;
	for(size_t i=0;i<SquareVertices.size();i++,k++)
	{
		vertices[k].Pos = SquareVertices[i].Pos;
		vertices[k].Color = SquareVertices[i].Color;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(),std::begin(SquareIndices),std::end(SquareIndices));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(MyApp::Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexBufferByteSize = vbByteSize;
	geo->VertexByteStride = sizeof(MyApp::Vertex);
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["Square"] = SquareSubMesh;

	mGeometries[geo->Name] = std::move(geo);
}

void FirstApp::BuildSkullGeometry()
{
	//加载骷髅头模型
	std::ifstream fin("Resource/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Resource/skull.txt not found", 0, 0);
		return;
	}

	//顶点读写
	UINT vertexCount = 0;
	UINT triangleCount = 0;
	std::string ignore;
	fin >> ignore >> vertexCount;
	fin >> ignore >> triangleCount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<MyApp::Vertex> vertices(vertexCount);//初始化顶点列表
	//顶点列表赋值
	for (UINT i = 0; i < vertexCount; i++)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;//读取顶点坐标
		fin >> ignore >> ignore >> ignore;//normal不读取
		vertices[i].Color = XMFLOAT4(DirectX::Colors::CadetBlue);//设置顶点色
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::int32_t> indices(triangleCount * 3);//初始化索引列表
	//索引列表赋值
	for (UINT i = 0; i < triangleCount; i++)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();//关闭输入流

	const UINT vbByteSize = vertices.size() * sizeof(MyApp::Vertex);//顶点缓存大小
	const UINT ibByteSize = indices.size() * sizeof(std::int32_t);//索引缓存大小

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skullGeo";

	//将顶点和索引数据复制到CPU系统内存上
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	//将顶点和索引数据从CPU内存复制到GPU缓存上
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(),  vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize,  geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(MyApp::Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;

	//绘制三参数
	SubmeshGeometry skullSubmesh;
	skullSubmesh.BaseVertexLocation = 0;
	skullSubmesh.StartIndexLocation = 0;
	skullSubmesh.IndexCount = (UINT)indices.size();

	geo->DrawArgs["skull"] = skullSubmesh;

	mGeometries["skullGeo"] = std::move(geo);
}

void FirstApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; i++)
	{
		//暂时还没有ObjectCount
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRitems.size()));
	}
}

void FirstApp::BuildRenderItems()
{
	auto SquareRitem = std::make_unique<RenderItem>();
	//存入世界矩阵
	XMStoreFloat4x4(&SquareRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	SquareRitem->ObjCBIndex = 0;
	SquareRitem->Geo = mGeometries["shapeGeo"].get();
	SquareRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	SquareRitem->IndexCount = SquareRitem->Geo->DrawArgs["Square"].IndexCount;
	SquareRitem->StartIndexLocation = SquareRitem->Geo->DrawArgs["Square"].StartIndexLocation;
	SquareRitem->BaseVertexLocation = SquareRitem->Geo->DrawArgs["Square"].BaseVertexLocation;
	mAllRitems.push_back(std::move(SquareRitem));

	//球形的RenderItem
// 	auto SphereRItem = std::make_unique<RenderItem>();
// 	//存入世界矩阵
// 	XMStoreFloat4x4(&SphereRItem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 2.0f));
// 	SphereRItem->ObjCBIndex = 1;
// 	SphereRItem->Geo = mGeometries["shapeGeo"].get();
// 	SphereRItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
// 	SphereRItem->IndexCount = SphereRItem->Geo->DrawArgs["Sphere"].IndexCount;
// 	SphereRItem->StartIndexLocation = SphereRItem->Geo->DrawArgs["Sphere"].StartIndexLocation;
// 	SphereRItem->BaseVertexLocation = SphereRItem->Geo->DrawArgs["Sphere"].BaseVertexLocation;
// 	mAllRitems.push_back(std::move(SphereRItem));

	//骷髅的RenderItem
	auto SkullRItem = std::make_unique<RenderItem>();
	//存入世界矩阵
	XMStoreFloat4x4(&SkullRItem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 0.0f, 2.0f));
	//skull常量数据（world矩阵）在objConstantBuffer索引2上
	SkullRItem->ObjCBIndex = 2;
	SkullRItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	SkullRItem->Geo = mGeometries["skullGeo"].get();
	SkullRItem->IndexCount = SkullRItem->Geo->DrawArgs["skull"].IndexCount;
	SkullRItem->BaseVertexLocation = SkullRItem->Geo->DrawArgs["skull"].BaseVertexLocation;
	SkullRItem->StartIndexLocation = SkullRItem->Geo->DrawArgs["skull"].StartIndexLocation;
	//mAllRitems.push_back(std::move(SkullRItem));

	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void FirstApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	//对象常量的大小
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	//获取当前帧的ObjectCB的Resource
	auto ObjectCB = mCurrFrameResource->ObjectCB->Resource();

	//渲染每一个物体
	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

// 		//偏移到缓冲区此物体常量的地址
// 		UINT cbvIndex = mCurrFrameResourceIndex * (UINT)mOpaqueRitems.size() + ri->ObjCBIndex;
// 		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
// 		cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);
// 
// 		//给根描述符的参数进行赋值填充
// 		cmdList->SetGraphicsRootDescriptorTable(1, cbvHandle);

		//使用根描述符可以更直接地设置参数
		UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		auto objCB = mCurrFrameResource->ObjectCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(1, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void FirstApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	//暂时设置为不裁剪
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void FirstApp::OnKeyboardInput(const GameTimer& gt)
{
	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	else
		mIsWireframe = false;
}

void FirstApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void FirstApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	
	//看需要有多少帧要更新这个东西
	for (std::unique_ptr<RenderItem>& e : mAllRitems)
	{
		//更新常量缓冲区
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.WorldMatrix, XMMatrixTranspose(world));

			//根据每个RenderItem的物体常量缓冲区的偏移Index，将数据拷贝到对应区域里面去。
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			//当前对象当前帧已经进行世界矩阵的拷贝。暂时每帧都进行拷贝
			e->NumFramesDirty--;
		}
	}
}

void FirstApp::UpdateMainPassCBs(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	XMStoreFloat4x4(&mMainPassCB.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mMainPassCB.gTimer = gt.TotalTime();

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void FirstApp::UpdateMaterialCBs(const GameTimer& gt)
{

}