#pragma once
#include "BaseMaterial.h"

enum class CULL_MODE;

class TransparentMaterial final : public BaseMaterial
{
public:
	TransparentMaterial(ID3D11Device* pDevice, const std::wstring& assetFile);
	~TransparentMaterial();
	
	TransparentMaterial(const TransparentMaterial& other) = delete;
	TransparentMaterial(TransparentMaterial&& other) noexcept = delete;
	TransparentMaterial& operator=(const TransparentMaterial& other) = delete;
	TransparentMaterial& operator=(TransparentMaterial&& other) noexcept = delete;

	void SetDiffuseMap(ID3D11ShaderResourceView* pResourceView) const override;

	ID3DX11EffectTechnique* GetPointTechnique(CULL_MODE cullMode) const override { return m_pPointTechnique; }
	ID3DX11EffectTechnique* GetLinearTechnique(CULL_MODE cullMode) const override { return m_pLinearTechnique; }
	ID3DX11EffectTechnique* GetAnisotropicTechnique(CULL_MODE cullMode) const override { return m_pAnisotropicTechnique; }

private:
	ID3DX11EffectTechnique* m_pPointTechnique;
	ID3DX11EffectTechnique* m_pLinearTechnique;
	ID3DX11EffectTechnique* m_pAnisotropicTechnique;
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
};
