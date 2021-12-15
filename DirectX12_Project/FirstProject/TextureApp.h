#pragma once
#include "FirstApp.h"
class TextureApp : public FirstApp
{
public:
	TextureApp(HINSTANCE hInstance);
	~TextureApp();


	virtual bool Initialize() override;

protected:
	virtual void LoadTextures() override;

	virtual void BuildRenderItems() override;

	virtual void BuildShadersAndInputLayout() override;

	virtual void BuildDescriptorHeaps() override;

	virtual void BuildFrameResources() override;

	virtual void BuildRootSignature() override;

	virtual void UpdateMainPassCBs(const GameTimer& gt) override;

	virtual void UpdateMaterialCBs(const GameTimer& gt) override;

private:
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
};