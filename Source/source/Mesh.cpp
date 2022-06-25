#include "pch.h"
#include "Mesh.h"
#include "BaseMaterial.h"
#include "ECamera.h"
#include "Scene.h"
#include "Texture.h";


Mesh::Mesh(ID3D11Device* pDevice, const std::vector<VS_INPUT>& vertices, const std::vector<uint32_t>& indices, D3D_PRIMITIVE_TOPOLOGY primTopology, BaseMaterial* pMaterial,
           const Elite::FMatrix4& transform, const char* diffuseTextPath, const char* normalTextPath, const char* specularTextPath, const char* glossTextPath)
	: m_VertexVector{vertices}
	, m_IndexVector{ indices }
	, m_pMaterial{ pMaterial }
	, m_pVertexLayout{}
	, m_pVertexBuffer{}
	, m_pIndexBuffer{}
	, m_AmountIndices{}
	, m_TransformMatrix{ transform }
	, m_PrimTopology{ primTopology }
	, m_Shininess{ 25.f }
	, m_pDiffuseText{}
	, m_pNormalText{}
	, m_pSpecularText{}
	, m_pGlossinessText{}
{	
	// Create Diffuse Texture (if a path was provided)
	SetDiffuseTexture(diffuseTextPath, pDevice);

	// Create Normal Texture (if a path was provided)
	SetNormalTexture(normalTextPath, pDevice);

	// Create Specular Texture (if a path was provided)
	SetSpecularTexture(specularTextPath, pDevice);

	// Create Gloss Texture (if a path was provided)
	SetGlossinessTexture(glossTextPath, pDevice);
	
	// Create Vertex Layout
	HRESULT result = S_OK;
	static const uint32_t numElements(5);
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "COLOR";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "TEXCOORD";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 24;
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "NORMAL";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32;
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[4].SemanticName = "TANGENT";
	vertexDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[4].AlignedByteOffset = 44;
	vertexDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;


	// Create Input Layout
	D3DX11_PASS_DESC passDesc;
	m_pMaterial->GetPointTechnique(CULL_MODE::Back)->GetPassByIndex(0)->GetDesc(&passDesc);
	result = pDevice->CreateInputLayout(
		vertexDesc,
		numElements,
		passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize,
		&m_pVertexLayout);


	// Create Vertex Buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(VS_INPUT) * (uint32_t)vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData = { 0 };
	initData.pSysMem = vertices.data();
	result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
	if (FAILED(result))
		return;


	// Create Index Buffer
	m_AmountIndices = (uint32_t)indices.size();
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(uint32_t) * m_AmountIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	initData.pSysMem = indices.data();
	result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
	if (FAILED(result))
		return;
}

Mesh::~Mesh()
{
	if(m_pVertexLayout)
	{
		m_pVertexLayout->Release();
		m_pVertexLayout = nullptr;
	}

	if(m_pVertexBuffer)
	{
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}
	
	if(m_pIndexBuffer)
	{
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
	}

	if (m_pMaterial)
	{
		delete m_pMaterial;
		m_pMaterial = nullptr;
	}

	if(m_pDiffuseText)
	{
		delete m_pDiffuseText;
		m_pDiffuseText = nullptr;
	}
	
	if (m_pNormalText)
	{
		delete m_pNormalText;
		m_pNormalText = nullptr;
	}
	
	if (m_pSpecularText)
	{
		delete m_pSpecularText;
		m_pSpecularText = nullptr;
	}

	if (m_pGlossinessText)
	{
		delete m_pGlossinessText;
		m_pGlossinessText = nullptr;
	}
}

void Mesh::RenderDirectX(ID3D11DeviceContext* pDeviceContext, Elite::ECamera* pCamera, float aspectRatio, SAMPLER_FILTER samplerFilter,
	CULL_MODE cullMode, Elite::FVector3 lightDirection, float lightIntensity, Elite::FVector3 ambientLight) const
{
	// Calculate the World View Projection Matrix
	// And set it in the GPU (to convert the vertices to NDC space)
	const auto& worldViewProjMat = GetWorldViewProjMatrix(pCamera, aspectRatio);
	m_pMaterial->SetWorldViewProjMatrix(worldViewProjMat);

	// Set the World Matrix in the GPU
	Elite::FMatrix4 worldMatrix = m_TransformMatrix;
	auto* pWorldMat = reinterpret_cast<float*>(&worldMatrix);
	m_pMaterial->SetWorldMatrix(pWorldMat);

	// Set the View Inverse Matrix in the GPU
	Elite::FMatrix4 viewInvMatrix = pCamera->GetWorldMatrix();
	auto* pViewInverseMat = reinterpret_cast<float*>(&viewInvMatrix);
	m_pMaterial->SetViewInverseMatrix(pViewInverseMat);

	
	// Set the Diffuse Map in the GPU
	if(m_pDiffuseText != nullptr)
		m_pMaterial->SetDiffuseMap(m_pDiffuseText->GetResourceView());

	// Set the Normal Map in the GPU
	if (m_pNormalText != nullptr)
		m_pMaterial->SetNormalMap(m_pNormalText->GetResourceView());

	// Set the Specular Map in the GPU
	if (m_pSpecularText != nullptr)
		m_pMaterial->SetSpecularMap(m_pSpecularText->GetResourceView());

	// Set the Glossiness Map in the GPU
	if (m_pGlossinessText != nullptr)
		m_pMaterial->SetGlossinessMap(m_pGlossinessText->GetResourceView());

	// Set the Shininess in the GPU
	m_pMaterial->SetShininess(m_Shininess);

	// Set the single light's info in the GPU
	auto* pLightDir = reinterpret_cast<float*>(&lightDirection);
	m_pMaterial->SetLightDirection(pLightDir);
	m_pMaterial->SetLightIntensity(lightIntensity);
	auto* pAmbient = reinterpret_cast<float*>(&ambientLight);
	m_pMaterial->SetAmbientLight(pAmbient);

	
	// Set Vertex Buffer
	UINT stride = sizeof(VS_INPUT);
	UINT offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Set Index Buffer
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set Input Layout
	pDeviceContext->IASetInputLayout(m_pVertexLayout);

	// Set Primitive Topology
	pDeviceContext->IASetPrimitiveTopology(m_PrimTopology);

	
	// Render Each Mesh Triangle
	ID3DX11EffectTechnique* pCurrentTechnique = nullptr;
	switch(samplerFilter)
	{
	case SAMPLER_FILTER::Point:
		pCurrentTechnique = m_pMaterial->GetPointTechnique(cullMode);
		break;
	case SAMPLER_FILTER::Linear:
		pCurrentTechnique = m_pMaterial->GetLinearTechnique(cullMode);
		break;
	case SAMPLER_FILTER::Anisotropic:
		pCurrentTechnique = m_pMaterial->GetAnisotropicTechnique(cullMode);
		break;
	}

	if (pCurrentTechnique)
	{
		D3DX11_TECHNIQUE_DESC techDesc;
		pCurrentTechnique->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			pCurrentTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexed(m_AmountIndices, 0, 0);
		}
	}
}

float* Mesh::GetWorldViewProjMatrix(Elite::ECamera* pCamera, float aspectRatio) const
{
	// Set up the projection matrix - Left-Hand Coordinate System
	const Elite::FMatrix4 projectionMatrix =
	{
		Elite::FVector4{1.f / (aspectRatio * pCamera->GetFov()), 0.f, 0.f, 0.f},
		Elite::FVector4{0.f, 1.f / pCamera->GetFov(), 0.f, 0.f},
		Elite::FVector4{0.f, 0.f, pCamera->GetFar() / (pCamera->GetFar() - pCamera->GetNear()), 1.f},
		Elite::FVector4{0.f, 0.f, -(pCamera->GetFar() * pCamera->GetNear()) / (pCamera->GetFar() - pCamera->GetNear()), 0.f}
	};

	// Set up the worldViewProjectionMatrix matrix
	Elite::FMatrix4 worldViewProjectionMatrix = projectionMatrix * pCamera->GetViewMatrix() * m_TransformMatrix;

	// Convert it to a float pointer
	auto* pReturnValue = reinterpret_cast<float*>(&worldViewProjectionMatrix);
	
	return pReturnValue;
}

void Mesh::SetDiffuseTexture(const char* diffuseTextPath, ID3D11Device* pDevice)
{
	if (diffuseTextPath != nullptr)
		m_pDiffuseText = new Texture(pDevice, diffuseTextPath);
}

void Mesh::SetNormalTexture(const char* normalTextPath, ID3D11Device* pDevice)
{
	if (normalTextPath != nullptr)
		m_pNormalText = new Texture(pDevice, normalTextPath);
}

void Mesh::SetSpecularTexture(const char* specularTextPath, ID3D11Device* pDevice)
{
	if (specularTextPath != nullptr)
		m_pSpecularText = new Texture(pDevice, specularTextPath);
}

void Mesh::SetGlossinessTexture(const char* glossTextPath, ID3D11Device* pDevice)
{
	if (glossTextPath != nullptr)
		m_pGlossinessText = new Texture(pDevice, glossTextPath);
}


Elite::FMatrix4 Mesh::GetTransformMatrix(bool leftHandCoordSystem) const
{
	if(leftHandCoordSystem)
		return m_TransformMatrix;

	auto rightHandTransformMat = m_TransformMatrix;
	rightHandTransformMat[3][2] = -rightHandTransformMat[3][2];

	auto rotationMatrixX = Elite::FMatrix4{
				Elite::FVector4(1.f, 0.f, 0.f, 0.f),
				Elite::FVector4(0.f, cosf(float(M_PI)), sinf(float(M_PI)), 0.f),
				Elite::FVector4(0.f, -sinf(float(M_PI)), cosf(float(M_PI)), 0.f),
				Elite::FVector4(0.f, 0.f, 0.f, 1.f) };
	
	auto rotationMatrixY = Elite::FMatrix4{
				Elite::FVector4(cosf(float(M_PI)), 0.f, -sinf(float(M_PI)), 0.f),
				Elite::FVector4(0.f, 1.f, 0.f, 0.f),
				Elite::FVector4(sinf(float(M_PI)), 0.f, cosf(float(M_PI)), 0.f),
				Elite::FVector4(0.f, 0.f, 0.f, 1.f) };

	auto scaleMatrix = Elite::FMatrix4{
				Elite::FVector4(1.f, 0.f, 0.f, 0.f),
				Elite::FVector4(0.f, 1.f, 0.f, 0.f),
				Elite::FVector4(0.f, 0.f, -1.f, 0.f),
				Elite::FVector4(0.f, 0.f, 0.f, 1.f) };

	rightHandTransformMat = rightHandTransformMat* rotationMatrixX * rotationMatrixY * scaleMatrix;

	return rightHandTransformMat;
}



