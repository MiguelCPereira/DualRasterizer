#pragma once
#include <vector>

class Texture;
class BaseMaterial;

namespace Elite
{
	class ECamera;
}

enum class SAMPLER_FILTER
{
	Point,
	Linear,
	Anisotropic
};

enum class CULL_MODE
{
	Back,
	Front,
	None
};

struct VS_INPUT
{
	Elite::FPoint3 Position;
	Elite::RGBColor Color;
	Elite::FVector2 UVCoord;
	Elite::FVector3 Normal;
	Elite::FVector3 Tangent;

	VS_INPUT(const Elite::FPoint3& pos, const Elite::RGBColor& col = { 1.f, 1.f, 1.f }, const Elite::FVector2& uvCoord = {-10.f, -10.f},
		const Elite::FVector3& normal = { -10.f, -10.f, -10.f }, const Elite::FVector3& tangent = { -10.f, -10.f, -10.f })
		: Position(pos)
		, Color(col)
		, UVCoord(uvCoord)
		, Normal(normal)
		, Tangent(tangent)
	{
	}
};

struct VS_OUTPUT
{
	Elite::FPoint4 Position;
	Elite::FPoint4 WorldPosition;
	Elite::RGBColor Color;
	Elite::FVector2 UVCoord;
	Elite::FVector3 Normal;
	Elite::FVector3 Tangent;

	VS_OUTPUT()
		: Position(Elite::FPoint4(0.f, 0.f, 0.f, 1.f))
		, WorldPosition(Elite::FPoint4(0.f, 0.f, 0.f, 1.f))
		, Color(Elite::RGBColor(0.f, 0.f, 0.f))
		, UVCoord(Elite::FVector2(0.f, 0.f))
		, Normal(Elite::FVector3(0.f, 0.f ,0.f))
		, Tangent(Elite::FVector3(0.f, 0.f, 0.f))
	{
	}
	
	VS_OUTPUT(VS_INPUT input)
		: Position(Elite::FPoint4(input.Position.x, input.Position.y, input.Position.z, 1.f))
		, WorldPosition(input.Position)
		, Color(input.Color)
		, UVCoord(input.UVCoord)
		, Normal(input.Normal)
		, Tangent(input.Tangent)
	{
	}
	
	VS_OUTPUT(const Elite::FPoint4& pos, const Elite::FPoint4& worldPos, const Elite::RGBColor& col = { 1.f, 1.f, 1.f }, const Elite::FVector2& uvCoord = { -10.f, -10.f },
		const Elite::FVector3& normal = { -10.f, -10.f, -10.f }, const Elite::FVector3& tangent = { -10.f, -10.f, -10.f })
		: Position(pos)
		, WorldPosition(worldPos)
		, Color(col)
		, UVCoord(uvCoord)
		, Normal(normal)
		, Tangent(tangent)
	{
	}
};

class Mesh
{
public:
	Mesh(ID3D11Device* pDevice, const std::vector<VS_INPUT>& vertices, const std::vector<uint32_t>& indices, D3D_PRIMITIVE_TOPOLOGY primTopology, BaseMaterial* pMaterial,
		const Elite::FMatrix4& transform = Elite::FMatrix4::Identity(), const char* diffuseTextPath = nullptr, const char* normalTextPath = nullptr,
		const char* specularTextPath = nullptr, const char* glossTextPath = nullptr);
	~Mesh();

	Mesh(const Mesh& other) = delete;
	Mesh(Mesh&& other) noexcept = delete;
	Mesh& operator=(const Mesh& other) = delete;
	Mesh& operator=(Mesh&& other) noexcept = delete;

	void RenderDirectX(ID3D11DeviceContext* pDeviceContext, Elite::ECamera* pCamera, float aspectRatio, SAMPLER_FILTER samplerFilter,
		CULL_MODE cullMode, Elite::FVector3 lightDirection, float lightIntensity, Elite::FVector3 ambientLight) const;
	float* GetWorldViewProjMatrix(Elite::ECamera* pCamera, float aspectRatio) const;
	Elite::FMatrix4 GetTransformMatrix(bool leftHandCoordSystem) const;
	std::vector<VS_INPUT> GetVertexVector() const { return m_VertexVector; }
	std::vector<uint32_t> GetIndexVector() const { return m_IndexVector; }
	Texture* GetDiffuseTexture() const { return m_pDiffuseText; }
	Texture* GetNormalTexture() const { return m_pNormalText; }
	Texture* GetSpecularTexture() const { return m_pSpecularText; }
	Texture* GetGlossinessTexture() const { return m_pGlossinessText; }
	float GetShininess() const { return m_Shininess; }
	D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimTopology; }

	BaseMaterial* GetMaterial() const { return m_pMaterial; }

	void SetShininess(float newValue) { m_Shininess = newValue; }
	void SetDiffuseTexture(const char* diffuseTextPath, ID3D11Device* pDevice);
	void SetNormalTexture(const char* normalTextPath, ID3D11Device* pDevice);
	void SetSpecularTexture(const char* specularTextPath, ID3D11Device* pDevice);
	void SetGlossinessTexture(const char* glossTextPath, ID3D11Device* pDevice);
	void SetTransformMatrix(const Elite::FMatrix4& transform) { m_TransformMatrix = transform; }

private:
	std::vector<VS_INPUT> m_VertexVector;
	std::vector<uint32_t> m_IndexVector;
	
	BaseMaterial* m_pMaterial;
	ID3D11InputLayout* m_pVertexLayout;
	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	uint32_t m_AmountIndices;
	Elite::FMatrix4 m_TransformMatrix;
	D3D_PRIMITIVE_TOPOLOGY m_PrimTopology;
	float m_Shininess;

	Texture* m_pDiffuseText;
	Texture* m_pNormalText;
	Texture* m_pSpecularText;
	Texture* m_pGlossinessText;
};
