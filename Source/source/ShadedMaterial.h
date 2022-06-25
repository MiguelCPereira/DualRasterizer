#pragma once
#include "BaseMaterial.h"

enum class CULL_MODE;

class ShadedMaterial final : public BaseMaterial
{
public:
	ShadedMaterial(ID3D11Device* pDevice, const std::wstring& assetFile);
	~ShadedMaterial();

	ShadedMaterial(const ShadedMaterial& other) = delete;
	ShadedMaterial(ShadedMaterial&& other) noexcept = delete;
	ShadedMaterial& operator=(const ShadedMaterial& other) = delete;
	ShadedMaterial& operator=(ShadedMaterial&& other) noexcept = delete;

	void SetDiffuseMap(ID3D11ShaderResourceView* pResourceView) const override;
	void SetNormalMap(ID3D11ShaderResourceView* pResourceView) const override;
	void SetSpecularMap(ID3D11ShaderResourceView* pResourceView) const override;
	void SetGlossinessMap(ID3D11ShaderResourceView* pResourceView) const override;
	void SetWorldMatrix(float* pMatrix) const override;
	void SetViewInverseMatrix(float* pMatrix) const override;

	void SetLightDirection(float* lightDirection) const override;
	void SetLightIntensity(float lightIntensity) const override;
	void SetAmbientLight(float* ambientLight) const override;

	ID3DX11EffectTechnique* GetPointTechnique(CULL_MODE cullMode) const override;
	ID3DX11EffectTechnique* GetLinearTechnique(CULL_MODE cullMode) const override;
	ID3DX11EffectTechnique* GetAnisotropicTechnique(CULL_MODE cullMode) const override;

private:
	ID3DX11EffectTechnique* m_pPointTechniqueBackCull;
	ID3DX11EffectTechnique* m_pLinearTechniqueBackCull;
	ID3DX11EffectTechnique* m_pAnisotropicTechniqueBackCull;
	ID3DX11EffectTechnique* m_pPointTechniqueFrontCull;
	ID3DX11EffectTechnique* m_pLinearTechniqueFrontCull;
	ID3DX11EffectTechnique* m_pAnisotropicTechniqueFrontCull;
	ID3DX11EffectTechnique* m_pPointTechniqueNoCull;
	ID3DX11EffectTechnique* m_pLinearTechniqueNoCull;
	ID3DX11EffectTechnique* m_pAnisotropicTechniqueNoCull;
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pGlossinessMapVariable;
	ID3DX11EffectMatrixVariable* m_pMatWorldMatrixVariable;
	ID3DX11EffectMatrixVariable* m_pMatViewInverseMatrixVariable;
	ID3DX11EffectVectorVariable* m_pLightDirectionVariable;
	ID3DX11EffectScalarVariable* m_pLightIntensityVariable;
	ID3DX11EffectVectorVariable* m_pAmbientLightVariable;
};
