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
	mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void LightApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
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

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, XM_PIDIV2);
}
