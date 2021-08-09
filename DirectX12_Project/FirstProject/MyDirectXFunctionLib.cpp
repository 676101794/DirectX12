#include "MyDirectXFunctionLib.h"

using namespace DirectX;
float CalculateAngleFromTwoVec(DirectX::XMVECTOR vec1, DirectX::XMVECTOR vec2)
{
	DirectX::XMVECTOR angelVec = DirectX::XMVector3AngleBetweenVectors(vec1, vec2);
	float angleRadians = XMVectorGetX(angelVec);
	float angleDegrees = XMConvertToDegrees(angleRadians);
	return angleDegrees;
}

