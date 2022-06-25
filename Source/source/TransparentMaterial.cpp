#include "pch.h"
#include "TransparentMaterial.h"

TransparentMaterial::TransparentMaterial(ID3D11Device* pDevice, const std::wstring& assetFile)
	: BaseMaterial(pDevice, assetFile)
	, m_pPointTechnique{}
	, m_pLinearTechnique{}
	, m_pAnisotropicTechnique{}
	, m_pDiffuseMapVariable{}
{
	if (m_pEffect)
	{
		m_pPointTechnique = m_pEffect->GetTechniqueByName("TransparentPointTechnique");
		if (!m_pPointTechnique->IsValid())
			std::wcout << L"Point Technique not valid\n";

		m_pLinearTechnique = m_pEffect->GetTechniqueByName("TransparentLinearTechnique");
		if (!m_pLinearTechnique->IsValid())
			std::wcout << L"Linear Technique not valid\n";

		m_pAnisotropicTechnique = m_pEffect->GetTechniqueByName("TransparentAnisotropicTechnique");
		if (!m_pAnisotropicTechnique->IsValid())
			std::wcout << L"Anisotropic Technique not valid\n";

		m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
		if (!m_pDiffuseMapVariable->IsValid())
			std::wcout << L"m_pDiffuseMapVariable not valid\n";
	}
	else
	{
		std::wcout << L"Effect from TransparentMaterial was not initialized by the parent class properly\n";
	}
}

TransparentMaterial::~TransparentMaterial()
{
	if (m_pPointTechnique)
	{
		m_pPointTechnique->Release();
		m_pPointTechnique = nullptr;
	}

	if (m_pLinearTechnique)
	{
		m_pLinearTechnique->Release();
		m_pLinearTechnique = nullptr;
	}

	if (m_pAnisotropicTechnique)
	{
		m_pAnisotropicTechnique->Release();
		m_pAnisotropicTechnique = nullptr;
	}

	if (m_pDiffuseMapVariable)
	{
		m_pDiffuseMapVariable->Release();
		m_pDiffuseMapVariable = nullptr;
	}
}

void TransparentMaterial::SetDiffuseMap(ID3D11ShaderResourceView* pResourceView) const
{
	if (m_pDiffuseMapVariable->IsValid())
		m_pDiffuseMapVariable->SetResource(pResourceView);
}



