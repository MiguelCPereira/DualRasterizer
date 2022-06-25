#include "pch.h"
#include "BaseMaterial.h"
#include <string>
#include <sstream>

BaseMaterial::BaseMaterial(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	m_pEffect = LoadEffect(pDevice, assetFile);

	m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (!m_pMatWorldViewProjVariable->IsValid())
		std::wcout << L"m_pMatWorldViewProjVariable not valid\n";

	m_pShininessVariable = m_pEffect->GetVariableByName("gShininess")->AsScalar();
	if (!m_pShininessVariable->IsValid())
		std::wcout << L"m_pShininessVariable not valid\n";
}

BaseMaterial::~BaseMaterial()
{
	if(m_pMatWorldViewProjVariable)
	{
		m_pMatWorldViewProjVariable->Release();
		m_pMatWorldViewProjVariable = nullptr;
	}

	if (m_pShininessVariable)
	{
		m_pShininessVariable->Release();
		m_pShininessVariable = nullptr;
	}

	if (m_pEffect)
	{
		m_pEffect->Release();
		m_pEffect = nullptr;
	}
}


ID3DX11Effect* BaseMaterial::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	HRESULT result = S_OK;
	ID3D10Blob* pErrorBlob = nullptr;
	ID3DX11Effect* pEffect;

	DWORD shaderFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile(assetFile.c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		pDevice,
		&pEffect,
		&pErrorBlob);

	if(FAILED(result))
	{
		if(pErrorBlob != nullptr)
		{
			char* pErrors = (char*)pErrorBlob->GetBufferPointer();

			std::wstringstream ss;
			for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); i++)
				ss << pErrors[i];

			OutputDebugStringW(ss.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << ss.str() << std::endl;
		}
		else
		{
			std::wstringstream ss;
			ss << "EffectLoader: Failed to CreateEffectFromFile\nPath: " << assetFile;
			std::wcout << ss.str() << std::endl;
			return nullptr;
		}
	}

	return pEffect;
}

void BaseMaterial::SetWorldViewProjMatrix(float* pMatrix) const
{
	if(m_pMatWorldViewProjVariable->IsValid())
		m_pMatWorldViewProjVariable->SetMatrix(pMatrix);
}

void BaseMaterial::SetShininess(float shininess) const
{
	if (m_pShininessVariable->IsValid())
		m_pShininessVariable->SetFloat(shininess);
}








