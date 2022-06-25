#pragma once

enum class CULL_MODE;

class BaseMaterial
{
public:
	BaseMaterial(ID3D11Device* pDevice, const std::wstring& assetFile);
	virtual ~BaseMaterial();

	BaseMaterial(const BaseMaterial& other) = delete;
	BaseMaterial(BaseMaterial&& other) noexcept = delete;
	BaseMaterial& operator=(const BaseMaterial& other) = delete;
	BaseMaterial& operator=(BaseMaterial&& other) noexcept = delete;

	static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);

	void SetWorldViewProjMatrix(float* pMatrix) const;
	void SetShininess(float shininess) const;
	virtual void SetDiffuseMap(ID3D11ShaderResourceView* pResourceView) const {}
	virtual void SetNormalMap(ID3D11ShaderResourceView* pResourceView) const {}
	virtual void SetSpecularMap(ID3D11ShaderResourceView* pResourceView) const {}
	virtual void SetGlossinessMap(ID3D11ShaderResourceView* pResourceView) const {}
	virtual void SetWorldMatrix(float* pMatrix) const {}
	virtual void SetViewInverseMatrix(float* pMatrix) const {}
	virtual void SetLightDirection(float* lightDirection) const {}
	virtual void SetLightIntensity(float lightIntensity) const {}
	virtual void SetAmbientLight(float* ambientLight) const {}
	
	ID3DX11Effect* GetEffect() const { return m_pEffect; }
	virtual ID3DX11EffectTechnique* GetPointTechnique(CULL_MODE cullMode) const = 0;
	virtual ID3DX11EffectTechnique* GetLinearTechnique(CULL_MODE cullMode) const = 0;
	virtual ID3DX11EffectTechnique* GetAnisotropicTechnique(CULL_MODE cullMode) const = 0;

protected:
	ID3DX11Effect* m_pEffect;

private:
	ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable;
	ID3DX11EffectScalarVariable* m_pShininessVariable;
};
