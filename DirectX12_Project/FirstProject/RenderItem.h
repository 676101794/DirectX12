#pragma once
#include <DirectXMath.h>
#include "d3dUtil.h"

//class MeshGeometry;
using namespace DirectX;

// 存储绘制图形所需参数的轻量级结构体。它会随着不同的应用程序而有所差别
struct RenderItem
{
	RenderItem() = default;

	// 描述物体局部空间相对于世界空间的世界矩阵
	// 它定义了物体位于世界空间中的位置、朝向以及大小
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	// 用已更新标志（dirty flag）来表示物体的相关数据已发生改变，这意味着我们此时需要更新常量缓
	// 冲区。由于每个FrameResource中都有一个物体常量缓冲区，所以我们必须对每个FrameResource
	// 都进行更新。即，当我们修改物体数据的时候，应当按NumFramesDirty = gNumFrameResources
	// 进行设置，从而使每个帧资源都得到更新
	// RenderItem中存有世界矩阵，为了用于为每帧都更新常量缓冲区，所以需要NumFramesDirty与NumFrameResources保持一致。
	int NumFramesDirty = gNumFrameResources;

	// 该索引指向的GPU常量缓冲区对应于当前渲染项中的物体常量缓冲区
	// 当前RenderItem的常量缓冲区的索引偏移。
	UINT ObjCBIndex = -1;

	// 此渲染项参与绘制的几何体。注意，绘制一个几何体可能会用到多个渲染项
	// 暂时没发现有什么用，暂时先不管。
	MeshGeometry* Geo = nullptr;

	//图元拓扑
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	//DrawIndexedInstanced方法的参数
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};