#include "pch.h"
#include "ShadedMaterial.h"
#include "Mesh.h"

ShadedMaterial::ShadedMaterial(ID3D11Device* pDevice, const std::wstring& assetFile)
	: BaseMaterial(pDevice, assetFile)
	, m_pPointTechniqueBackCull{}
	, m_pLinearTechniqueBackCull{}
	, m_pAnisotropicTechniqueBackCull{}
	, m_pPointTechniqueFrontCull{}
	, m_pLinearTechniqueFrontCull{}
	, m_pAnisotropicTechniqueFrontCull{}
	, m_pPointTechniqueNoCull{}
	, m_pLinearTechniqueNoCull{}
	, m_pAnisotropicTechniqueNoCull{}
	, m_pDiffuseMapVariable{}
	, m_pNormalMapVariable{}
	, m_pSpecularMapVariable{}
	, m_pGlossinessMapVariable{}
	, m_pMatWorldMatrixVariable{}
	, m_pMatViewInverseMatrixVariable{}
	, m_pLightDirectionVariable{}
	, m_pLightIntensityVariable{}
	, m_pAmbientLightVariable{}
{
	if (m_pEffect)
	{
		m_pPointTechniqueBackCull = m_pEffect->GetTechniqueByName("PointTechniqueBackCull");
		if (!m_pPointTechniqueBackCull->IsValid())
			std::wcout << L"Point BackCull Technique not valid\n";

		m_pLinearTechniqueBackCull = m_pEffect->GetTechniqueByName("LinearTechniqueBackCull");
		if (!m_pLinearTechniqueBackCull->IsValid())
			std::wcout << L"Linear BackCull Technique not valid\n";

		m_pAnisotropicTechniqueBackCull = m_pEffect->GetTechniqueByName("AnisotropicTechniqueBackCull");
		if (!m_pAnisotropicTechniqueBackCull->IsValid())
			std::wcout << L"Anisotropic BackCull Technique not valid\n";

		m_pPointTechniqueFrontCull = m_pEffect->GetTechniqueByName("PointTechniqueFrontCull");
		if (!m_pPointTechniqueFrontCull->IsValid())
			std::wcout << L"Point FrontCull Technique not valid\n";

		m_pLinearTechniqueFrontCull = m_pEffect->GetTechniqueByName("LinearTechniqueFrontCull");
		if (!m_pLinearTechniqueFrontCull->IsValid())
			std::wcout << L"Linear FrontCull Technique not valid\n";

		m_pAnisotropicTechniqueFrontCull = m_pEffect->GetTechniqueByName("AnisotropicTechniqueFrontCull");
		if (!m_pAnisotropicTechniqueFrontCull->IsValid())
			std::wcout << L"Anisotropic FrontCull Technique not valid\n";

		m_pPointTechniqueNoCull = m_pEffect->GetTechniqueByName("PointTechniqueNoCull");
		if (!m_pPointTechniqueNoCull->IsValid())
			std::wcout << L"Point NoCull Technique not valid\n";

		m_pLinearTechniqueNoCull = m_pEffect->GetTechniqueByName("LinearTechniqueNoCull");
		if (!m_pLinearTechniqueNoCull->IsValid())
			std::wcout << L"Linear NoCull Technique not valid\n";

		m_pAnisotropicTechniqueNoCull = m_pEffect->GetTechniqueByName("AnisotropicTechniqueNoCull");
		if (!m_pAnisotropicTechniqueNoCull->IsValid())
			std::wcout << L"Anisotropic NoCull Technique not valid\n";

		
		

		m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
		if (!m_pDiffuseMapVariable->IsValid())
			std::wcout << L"m_pDiffuseMapVariable not valid\n";

		m_pNormalMapVariable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
		if (!m_pNormalMapVariable->IsValid())
			std::wcout << L"m_pNormalMapVariable not valid\n";

		m_pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
		if (!m_pSpecularMapVariable->IsValid())
			std::wcout << L"m_pSpecularMapVariable not valid\n";

		m_pGlossinessMapVariable = m_pEffect->GetVariableByName("gGlossinessMap")->AsShaderResource();
		if (!m_pGlossinessMapVariable->IsValid())
			std::wcout << L"m_pGlossinessMapVariable not valid\n";


		

		m_pMatWorldMatrixVariable = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
		if (!m_pMatWorldMatrixVariable->IsValid())
			std::wcout << L"m_pMatWorldMatrixVariable not valid\n";

		m_pMatViewInverseMatrixVariable = m_pEffect->GetVariableByName("gViewInverseMatrix")->AsMatrix();
		if (!m_pMatViewInverseMatrixVariable->IsValid())
			std::wcout << L"m_pMatViewInverseMatrixVariable not valid\n";


		

		m_pLightDirectionVariable = m_pEffect->GetVariableByName("gLightDirection")->AsVector();
		if (!m_pLightDirectionVariable->IsValid())
			std::wcout << L"m_pLightDirectionVariable not valid\n";

		m_pLightIntensityVariable = m_pEffect->GetVariableByName("gLightIntensity")->AsScalar();
		if (!m_pLightIntensityVariable->IsValid())
			std::wcout << L"m_pLightIntensityVariable not valid\n";

		m_pAmbientLightVariable = m_pEffect->GetVariableByName("gAmbient")->AsVector();
		if (!m_pAmbientLightVariable->IsValid())
			std::wcout << L"m_pAmbientLightVariable not valid\n";
	}
	else
	{
		std::wcout << L"Effect from ShadedMaterial was not initialized by the parent class properly\n";
	}
}

ShadedMaterial::~ShadedMaterial()
{
	if (m_pPointTechniqueBackCull)
	{
		m_pPointTechniqueBackCull->Release();
		m_pPointTechniqueBackCull = nullptr;
	}

	if (m_pLinearTechniqueBackCull)
	{
		m_pLinearTechniqueBackCull->Release();
		m_pLinearTechniqueBackCull = nullptr;
	}

	if (m_pAnisotropicTechniqueBackCull)
	{
		m_pAnisotropicTechniqueBackCull->Release();
		m_pAnisotropicTechniqueBackCull = nullptr;
	}

	if (m_pPointTechniqueFrontCull)
	{
		m_pPointTechniqueFrontCull->Release();
		m_pPointTechniqueFrontCull = nullptr;
	}

	if (m_pLinearTechniqueFrontCull)
	{
		m_pLinearTechniqueFrontCull->Release();
		m_pLinearTechniqueFrontCull = nullptr;
	}

	if (m_pAnisotropicTechniqueFrontCull)
	{
		m_pAnisotropicTechniqueFrontCull->Release();
		m_pAnisotropicTechniqueFrontCull = nullptr;
	}

	if (m_pPointTechniqueNoCull)
	{
		m_pPointTechniqueNoCull->Release();
		m_pPointTechniqueNoCull = nullptr;
	}

	if (m_pLinearTechniqueNoCull)
	{
		m_pLinearTechniqueNoCull->Release();
		m_pLinearTechniqueNoCull = nullptr;
	}

	if (m_pAnisotropicTechniqueNoCull)
	{
		m_pAnisotropicTechniqueNoCull->Release();
		m_pAnisotropicTechniqueNoCull = nullptr;
	}

	if (m_pMatWorldMatrixVariable)
	{
		m_pMatWorldMatrixVariable->Release();
		m_pMatWorldMatrixVariable = nullptr;
	}

	if (m_pMatViewInverseMatrixVariable)
	{
		m_pMatViewInverseMatrixVariable->Release();
		m_pMatViewInverseMatrixVariable = nullptr;
	}

	if (m_pDiffuseMapVariable)
	{
		m_pDiffuseMapVariable->Release();
		m_pDiffuseMapVariable = nullptr;
	}

	if (m_pNormalMapVariable)
	{
		m_pNormalMapVariable->Release();
		m_pNormalMapVariable = nullptr;
	}

	if (m_pSpecularMapVariable)
	{
		m_pSpecularMapVariable->Release();
		m_pSpecularMapVariable = nullptr;
	}

	if (m_pGlossinessMapVariable)
	{
		m_pGlossinessMapVariable->Release();
		m_pGlossinessMapVariable = nullptr;
	}

	if(m_pLightDirectionVariable)
	{
		m_pLightDirectionVariable->Release();
		m_pLightDirectionVariable = nullptr;
	}

	if(m_pLightIntensityVariable)
	{
		m_pLightIntensityVariable->Release();
		m_pLightIntensityVariable = nullptr;
	}

	if(m_pAmbientLightVariable)
	{
		m_pAmbientLightVariable->Release();
		m_pAmbientLightVariable = nullptr;
	}
}

void ShadedMaterial::SetDiffuseMap(ID3D11ShaderResourceView* pResourceView) const
{
	if (m_pDiffuseMapVariable->IsValid())
		m_pDiffuseMapVariable->SetResource(pResourceView);
}

void ShadedMaterial::SetNormalMap(ID3D11ShaderResourceView* pResourceView) const
{
	if (m_pNormalMapVariable->IsValid())
		m_pNormalMapVariable->SetResource(pResourceView);
}

void ShadedMaterial::SetSpecularMap(ID3D11ShaderResourceView* pResourceView) const
{
	if (m_pSpecularMapVariable->IsValid())
		m_pSpecularMapVariable->SetResource(pResourceView);
}

void ShadedMaterial::SetGlossinessMap(ID3D11ShaderResourceView* pResourceView) const
{
	if (m_pGlossinessMapVariable->IsValid())
		m_pGlossinessMapVariable->SetResource(pResourceView);
}

void ShadedMaterial::SetWorldMatrix(float* pMatrix) const
{
	if (m_pMatWorldMatrixVariable->IsValid())
		m_pMatWorldMatrixVariable->SetMatrix(pMatrix);
}

void ShadedMaterial::SetViewInverseMatrix(float* pMatrix) const
{
	if (m_pMatViewInverseMatrixVariable->IsValid())
		m_pMatViewInverseMatrixVariable->SetMatrix(pMatrix);
}

void ShadedMaterial::SetLightDirection(float* lightDirection) const
{
	if (m_pLightDirectionVariable->IsValid())
		m_pLightDirectionVariable->SetFloatVector(lightDirection);
}

void ShadedMaterial::SetLightIntensity(float lightIntensity) const
{
	if (m_pLightIntensityVariable->IsValid())
		m_pLightIntensityVariable->SetFloat(lightIntensity);
}

void ShadedMaterial::SetAmbientLight(float* ambientLight) const
{
	if (m_pAmbientLightVariable->IsValid())
		m_pAmbientLightVariable->SetFloatVector(ambientLight);
}

ID3DX11EffectTechnique* ShadedMaterial::GetPointTechnique(CULL_MODE cullMode) const
{
	switch(cullMode)
	{
	case CULL_MODE::Back:
		return m_pPointTechniqueBackCull;
		break;
	case CULL_MODE::Front:
		return m_pPointTechniqueFrontCull;
		break;
	case CULL_MODE::None:
		return m_pPointTechniqueNoCull;
		break;
	}

	return nullptr;
}

ID3DX11EffectTechnique* ShadedMaterial::GetLinearTechnique(CULL_MODE cullMode) const
{
	switch (cullMode)
	{
	case CULL_MODE::Back:
		return m_pLinearTechniqueBackCull;
		break;
	case CULL_MODE::Front:
		return m_pLinearTechniqueFrontCull;
		break;
	case CULL_MODE::None:
		return m_pLinearTechniqueNoCull;
		break;
	}

	return nullptr;
}

ID3DX11EffectTechnique* ShadedMaterial::GetAnisotropicTechnique(CULL_MODE cullMode) const
{
	switch (cullMode)
	{
	case CULL_MODE::Back:
		return m_pAnisotropicTechniqueBackCull;
		break;
	case CULL_MODE::Front:
		return m_pAnisotropicTechniqueFrontCull;
		break;
	case CULL_MODE::None:
		return m_pAnisotropicTechniqueNoCull;
		break;
	}

	return nullptr;
}











