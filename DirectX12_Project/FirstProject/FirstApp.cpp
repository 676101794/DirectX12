#include "FirstApp.h"
#include <DirectXColors.h>

FirstApp::FirstApp(HINSTANCE hInstance):D3DApp(hInstance)
{

}

FirstApp::~FirstApp()
{

}

bool FirstApp::Initialize()
{
	if(!D3DApp::Initialize())
	{
		return false;
	}

	return true;
}

void FirstApp::OnResize()
{
	D3DApp::OnResize();
}

void FirstApp::Update(const GameTimer& gt)
{
	
}

void FirstApp::Draw(const GameTimer& gt)
{
	// 重复使用记录命令的相关内存
  // 只有当与GPU关联的命令列表执行完成时，我们才能将其重置
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// 在通过ExecuteCommandList方法将某个命令列表加入命令队列后，我们便可以重置该命令列表。以
	// 此来复用命令列表及其内存
	ThrowIfFailed(mCommandList->Reset(
		mDirectCmdListAlloc.Get(), nullptr));

	// 对资源的状态进行转换，将资源从呈现状态转换为渲染目标状态
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 设置视口和裁剪矩形。它们需要随着命令列表的重置而重置
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

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

	// 再次对资源状态进行转换，将资源从渲染目标状态转换回呈现状态
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	// 完成命令的记录
	ThrowIfFailed(mCommandList->Close());

	// 将待执行的命令列表加入命令队列
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换后台缓冲区和前台缓冲区
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 等待此帧的命令执行完毕。当前的实现没有什么效率，也过于简单
	// 我们在后面将重新组织渲染部分的代码，以免在每一帧都要等待
	FlushCommandQueue();
}
