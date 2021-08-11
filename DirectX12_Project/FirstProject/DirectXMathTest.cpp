#include <iostream>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "MyDirectXFunctionLib.h"
using namespace DirectX;

std::ostream& XM_CALLCONV operator<<(std::ostream& os, DirectX::FXMVECTOR v)
{
	DirectX::XMFLOAT3 dest;
	XMStoreFloat3(&dest, v);

	os << "(" << dest.x << "," << dest.y << "," << dest.z << ")";
	return os;
}

std::ostream& XM_CALLCONV operator<<(std::ostream& os, DirectX::XMMATRIX v)
{
	DirectX::XMFLOAT4X4 dest;
	XMStoreFloat4x4(&dest, v);

	os << dest.m[0][0] << "," << dest.m[0][1] << "," << dest.m[0][2] <<  "," << dest.m[0][3] << '\n' 
		<< dest.m[1][0] << "," << dest.m[1][1] << "," << dest.m[1][2] << "," << dest.m[1][3] << '\n'
		<< dest.m[2][0] << "," << dest.m[2][1] << "," << dest.m[2][2] << "," << dest.m[2][3] << '\n'
		<< dest.m[3][0] << "," << dest.m[3][1] << "," << dest.m[3][2] << "," << dest.m[3][3] << '\n'
		<< std::endl;
	return os;
}

int main()
{

	DirectX::XMFLOAT3 vec(1.0f,0.0f,0.0f);
	DirectX::XMFLOAT3 vec1(0.0f, 0.0f,1.0f);

	DirectX::XMVECTOR totalvec = XMLoadFloat3(&vec);
	DirectX::XMVECTOR totalvec1 = XMLoadFloat3(&vec1);

	std::cout << XMVector3Dot(totalvec, totalvec1) << std::endl;
	std::cout << XMVector3Length(totalvec) << std::endl;
	std::cout << XMVector3Length(totalvec1) << std::endl;

 
	float angleDegrees=CalculateAngleFromTwoVec(totalvec, totalvec1);
	std::cout << angleDegrees << std::endl;

	std::cout << XMVector3Cross(totalvec1,totalvec) << std::endl;

	//XMVector3ComponentsFromNormal()

	XMVECTOR v1{ 1,2,3,4 };
	XMVECTOR v2{ 2,3,4,5 };
	XMVECTOR v3{ 3,4,5,6 };
	XMVECTOR v4{ 4,5,6,7 };
	XMMATRIX M1{ v1,v2,v3,v4 };

	std::cout << XMMatrixDeterminant(M1) << std::endl;
	//XMMatrixInverse(&v1,);
	
	std::cout << M1 << std::endl;

	
}
