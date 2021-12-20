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
	virtual void UpdateObjectCBs(const GameTimer& gt) override;

	virtual void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems) override;
	virtual void BuildBoxGeometry() override;

private:
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	virtual void Draw(const GameTimer& gt) override;

private:
	UINT mCbvSrvDescriptorSize = 0;
};