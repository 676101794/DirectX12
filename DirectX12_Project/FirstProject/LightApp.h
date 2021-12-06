#pragma once
#include "FirstApp.h"

//extern const int gNumFrameResources;


class LightApp :public FirstApp
{
	
public:
	LightApp(HINSTANCE hInstance);
	~LightApp();

	virtual bool Initialize() override;

private:
	virtual void BuildShadersAndInputLayout() override;
	virtual void BuildFrameResources() override;
	virtual void BuildRenderItems() override;

	virtual void UpdateObjectCBs(const GameTimer& gt) override;
	virtual void UpdateMainPassCBs(const GameTimer& gt) override;

	//材质构建
	void BuildMaterials();

private:
	std::unordered_map<std::string, std::unique_ptr<Material>> mAllMaterials;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

protected:
	virtual void BuildSkullGeometry() override;

	virtual void OnKeyboardInput(const GameTimer& gt) override;

};