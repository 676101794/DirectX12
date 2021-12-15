#include "LightApp.h"
#include "RenderItem.h"

LightApp::LightApp(HINSTANCE hInstance):FirstApp(hInstance)
{

}

LightApp::~LightApp()
{

}

bool LightApp::Initialize()
{
	if (!D3DApp::Initialize())
	{
		return false;
	}

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildMaterials();
	BuildRootSignature();
	BuildShadersAndInputLayout();
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

void LightApp::BuildShadersAndInputLayout()
{
	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\colorLight.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\colorLight.hlsl", nullptr, "PS", "ps_5_0");

	//修改布局结构为带有法线的类型
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void LightApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mAllMaterials.size()));
		//(UINT)mAllMaterials.size()
	}
}

void LightApp::BuildRenderItems()
{
	//骷髅的RenderItem
	auto SkullRItem = std::make_unique<RenderItem>();
	//存入世界矩阵
	XMStoreFloat4x4(&SkullRItem->World, XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	
	//Skull的ObjConstantIndex;
	SkullRItem->ObjCBIndex = 0;
	SkullRItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	SkullRItem->Mat = mAllMaterials["grass"].get();
	SkullRItem->Geo = mGeometries["skullGeo"].get();
	SkullRItem->IndexCount = SkullRItem->Geo->DrawArgs["skull"].IndexCount;
	SkullRItem->BaseVertexLocation = SkullRItem->Geo->DrawArgs["skull"].BaseVertexLocation;
	SkullRItem->StartIndexLocation = SkullRItem->Geo->DrawArgs["skull"].StartIndexLocation;
	mAllRitems.push_back(std::move(SkullRItem));

	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void LightApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();

	//看需要有多少帧要更新这个东西
	for (std::unique_ptr<RenderItem>& e : mAllRitems)
	{
		//先不做DirtyNumFrame的处理，每帧都进行世界坐标的更新

		//更新常量缓冲区
// 		if (e->NumFramesDirty > 0)
// 		{
			//XMMATRIX world = XMLoadFloat4x4(&e->World);

			XMFLOAT4X4 NewFloat4x4;
			XMStoreFloat4x4(&NewFloat4x4, XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixTranslation(XOffset, YOffset, ZOffset));

			XMMATRIX world = XMLoadFloat4x4(&NewFloat4x4);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.WorldMatrix, XMMatrixTranspose(world));

			//根据每个RenderItem的物体常量缓冲区的偏移Index，将数据拷贝到对应区域里面去。
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			//当前对象当前帧已经进行世界矩阵的拷贝。暂时每帧都进行拷贝
// 			e->NumFramesDirty--;
// 		}
	}
}

void LightApp::UpdateMainPassCBs(const GameTimer& gt)
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

	mMainPassCB.gEyePos = mEyePos;
 
 	XMMATRIX world = XMLoadFloat4x4(&mWorld);
 	XMMATRIX proj = XMLoadFloat4x4(&mProj);
 	XMMATRIX worldViewProj = world * view * proj;
 
 	// Update the constant buffer with the latest worldViewProj matrix.
 	XMStoreFloat4x4(&mMainPassCB.WorldViewProj, XMMatrixTranspose(worldViewProj));
 	mMainPassCB.gTimer = gt.TotalTime();
 
 	//更新灯光
 	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
 	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
 
 	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	//mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 1.0f };

	float value = sinf(gt.TotalTime() * 4.0f) * 0.5f + 0.5f;
	mMainPassCB.Lights[0].Strength = { value * 2.0f + 1.0f, 1.0f, 1.0f };
 
 	auto currPassCB = mCurrFrameResource->PassCB.get();
 	currPassCB->CopyData(0, mMainPassCB);
}

void LightApp::BuildMaterials()
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

	mAllMaterials["grass"] = std::move(grass);
	mAllMaterials["water"] = std::move(water);
}

void LightApp::BuildSkullGeometry()
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
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;//normal不读取
		vertices[i].Color = XMFLOAT4(DirectX::Colors::Blue);//设置顶点色
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
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

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

void LightApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		mSunTheta -= 1.0f * dt;

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		mSunTheta += 1.0f * dt;

	if (GetAsyncKeyState(VK_UP) & 0x8000)
		mSunPhi -= 1.0f * dt;

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		mSunPhi += 1.0f * dt;

	if (GetAsyncKeyState('D') & 0x8000)
		XOffset += increment;

	if (GetAsyncKeyState('A') & 0x8000)
		XOffset -= increment;

	if (GetAsyncKeyState('W') & 0x8000)
		YOffset += increment;

	if (GetAsyncKeyState('S') & 0x8000)
		YOffset -= increment;

	if (GetAsyncKeyState('Q') & 0x8000)
		ZOffset += increment;

	if (GetAsyncKeyState('Z') & 0x8000)
		ZOffset -= increment;

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, XM_PIDIV2);
}

void LightApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	//第一个根参数用于存放各种需要经常变化的参数。
	// Create a single descriptor table of CBVs.
// 	CD3DX12_DESCRIPTOR_RANGE PassObjectTable;
// 	//第三个参数为要绑定的寄存器，没有显示地指定则直接为0
// 	PassObjectTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
// 	slotRootParameter[0].InitAsDescriptorTable(1, &PassObjectTable);

	//使用描述符用于根参数
	slotRootParameter[0].InitAsConstantBufferView(1);

	//用于描述世界矩阵  寄存器绑定为0
// 	CD3DX12_DESCRIPTOR_RANGE WorldObjectTable;
// 	WorldObjectTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
// 	slotRootParameter[1].InitAsDescriptorTable(1, &WorldObjectTable);
	
	slotRootParameter[1].InitAsConstantBufferView(0);

	//描述符表有问题，直接绑定描述符
	
	//用于描述材质 寄存器绑定为2
// 	CD3DX12_DESCRIPTOR_RANGE MaterialTable;
// 	MaterialTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
// 	slotRootParameter[2].InitAsDescriptorTable(1, &MaterialTable);

	//使用描述符作为根参数
	slotRootParameter[2].InitAsConstantBufferView(2);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr,
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

void LightApp::BuildConstantBufferViews()
{
	//创建描述符  PassCB的描述符 ObjectCB的描述符 MaterialCB的描述符

	//根据多少帧，以及每帧需要绘制的物体，来创建描述符！
// 	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
// 
// 	UINT objCount = (UINT)mOpaqueRitems.size();
// 
// 	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
// 	{
// 		auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
// 
// 		for (UINT i = 0; i < objCount; ++i)
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

	//计算passCB的大小
// 	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
// 
// 	for (int frameIndex = 0; frameIndex < gNumFrameResources; frameIndex++)
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

	//计算MaterialCB的大小
// 	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
// 	for (int frameIndex = 0; frameIndex < gNumFrameResources; frameIndex++)
// 	{
// 		auto materialCB = mFrameResources[frameIndex]->MaterialCB->Resource();
// 		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = materialCB->GetGPUVirtualAddress();
// 
// 		//因为每帧的MatCB都一样，而不需要像ObjectConstantBuffer一样针对每个物体都进行处理。所以只需要对帧进行偏移。
// 		int heapIndex = mMaterialCbvOffset + frameIndex;
// 		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
// 		handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);
// 
// 		//在堆的该偏移上设置描述符
// 		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
// 		cbvDesc.BufferLocation = cbAddress;
// 		cbvDesc.SizeInBytes = matCBByteSize;
// 
// 		md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
// 	}


}

void LightApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mAllMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}