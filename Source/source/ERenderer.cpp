#include "pch.h"

//External includes
#include "SDL.h"//
#include "SDL_surface.h"//

//Project includes
#include "ERenderer.h"

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>



#include "TransparentMaterial.h"
#include "ECamera.h"
#include "Mesh.h"
#include "Scene.h"
#include "Texture.h"
#include "EMath.h"

Elite::Renderer::Renderer(SDL_Window* pWindow)
	: m_pWindow{ pWindow }
	, m_Width{}
	, m_Height{}
	, m_DirectXInitialized{ false }
	, m_SoftwareInitialized{ false }
	, m_pDevice{ nullptr }
	, m_pDeviceContext{ nullptr }
	, m_pDXGIFactory{ nullptr }
	, m_pSwapChain{ nullptr }
	, m_pDepthStencilBuffer{ nullptr }
	, m_pDepthStencilView{ nullptr }
	, m_pRenderTargetBuffer{ nullptr }
	, m_pRenderTargetView{ nullptr }
	, m_TransformedVerticesTriangle{} // This is a temporary vector to store whatever triangle is currently being calculated in Software Mode - it's stored as a member variable for optimization purposes
{	
	int width, height = 0;
	SDL_GetWindowSize(pWindow, &width, &height);
	m_Width = static_cast<uint32_t>(width);
	m_Height = static_cast<uint32_t>(height);

	// Initialize Software
	if(InitializeSoftware())
	{
		m_SoftwareInitialized = true;
		std::cout << "Software is ready\n";
	}

	// Initialize DirectX pipeline
	if (SUCCEEDED(InitializeDirectX()))
	{
		m_DirectXInitialized = true;
		std::cout << "DirectX is ready\n";
	}
}

Elite::Renderer::~Renderer()
{
	if(m_pRenderTargetView)
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetView = nullptr;
	}

	if(m_pRenderTargetBuffer)
	{
		m_pRenderTargetBuffer->Release();
		m_pRenderTargetBuffer = nullptr;
	}

	if(m_pDepthStencilView)
	{
		m_pDepthStencilView->Release();
		m_pDepthStencilView = nullptr;
	}

	if(m_pDepthStencilBuffer)
	{
		m_pDepthStencilBuffer->Release();
		m_pDepthStencilBuffer = nullptr;
	}

	if(m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}
	
	if(m_pDeviceContext)
	{
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
		m_pDeviceContext->Release();

		m_pDeviceContext = nullptr;
	}

	if(m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}

	if (m_pDXGIFactory)
	{
		m_pDXGIFactory->Release();
		m_pDXGIFactory = nullptr;
	}

	if (m_pDepthBuffer)
	{
		delete m_pDepthBuffer;
		m_pDepthBuffer = nullptr;
	}
}

void Elite::Renderer::Render(Scene* pScene, SAMPLER_FILTER samplerFilter, RENDER_MODE renderMode, CULL_MODE cullMode, bool fireFXVisible, Mesh* pFireMesh)
{
	if (renderMode == RENDER_MODE::DirectX)
		RenderDirectX(pScene, samplerFilter, cullMode, fireFXVisible, pFireMesh);
	else
		RenderSoftware(pScene, cullMode, fireFXVisible, pFireMesh);
}

void Elite::Renderer::RenderDirectX(Scene* pScene, SAMPLER_FILTER samplerFilter, CULL_MODE cullMode, bool fireFXVisible, Mesh* pFireMesh) const
{
	if (!m_DirectXInitialized)
		return;

	// Clear Buffers
	RGBColor clearColor = pScene->GetBackgroundColor();
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render
	const auto aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
	for (auto* mesh : pScene->GetMeshes())
	{
		// Skip the FireFX, if fireFXVisible has been set to false
		if(pFireMesh == nullptr || fireFXVisible || mesh != pFireMesh)
			mesh->RenderDirectX(m_pDeviceContext, pScene->GetCurrentCamera(), aspectRatio, samplerFilter, cullMode, pScene->GetLightDirection(), pScene->GetLightIntensity(), pScene->GetAmbientLight());
	}
	// Present
	m_pSwapChain->Present(0, 0);
}

void Elite::Renderer::RenderSoftware(Scene* pScene, CULL_MODE cullMode, bool fireFXVisible, Mesh* pFireMesh)
{
	if (!m_SoftwareInitialized)
		return;

	// Reset the depth buffer
	for (uint32_t i = 0; i < m_Height * m_Width; i++)
		m_pDepthBuffer[i] = FLT_MAX;
	
	// Reset the backbuffer pixels
	auto sceneAmbient = pScene->GetBackgroundColor();
	uint32_t backgroundColor = SDL_MapRGB(m_pBackBuffer->format, Uint8(sceneAmbient.r * 255.f), Uint8(sceneAmbient.g * 255.f), Uint8(sceneAmbient.b * 255.f));
	SDL_FillRect(m_pBackBuffer, nullptr, backgroundColor);

	SDL_LockSurface(m_pBackBuffer);

	// Get the camera
	ECamera* pCamera = pScene->GetCurrentCamera();

	// Get the scene meshes and go over each one
	const auto& sceneMeshes = pScene->GetMeshes();
	for (auto* mesh : sceneMeshes)
	{
		// Skip the FireFX, if fireFXVisible has been set to false
		if (pFireMesh != nullptr && fireFXVisible == false && mesh == pFireMesh)
			continue;
		
		// Get all the mesh's info
		const auto& primTopology = mesh->GetPrimitiveTopology();
		const auto& vertices = mesh->GetVertexVector();
		const auto& indexes = mesh->GetIndexVector();
		const auto* pDiffuseText = mesh->GetDiffuseTexture();
		const auto* pNormalText = mesh->GetNormalTexture();
		const auto* pSpecularText = mesh->GetSpecularTexture();
		const auto* pGlossText = mesh->GetGlossinessTexture();
		const auto shininess = mesh->GetShininess();

		// Set the transparency bool depending on the Material type
		bool transparencyOn = false;
		if (dynamic_cast<TransparentMaterial*>(mesh->GetMaterial()) != nullptr && pDiffuseText)
			transparencyOn = true;

		// Define the increment for the next loop depending on topology
		int increment, max;
		if (primTopology == D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
		{
			increment = 3;
			max = int(indexes.size());
		}
		else
		{
			increment = 1;
			max = int(indexes.size()) - 2;
		}

		// And loop over all the mesh's triangles
		for (int i = 0; i < max; i += increment)
		{
			// Get the triangle vertices
			const VS_INPUT& v0 = vertices[indexes[i]];
			VS_INPUT v1{ {0.f, 0.f, 0.f} };
			VS_INPUT v2{ {0.f, 0.f, 0.f} };

			// If it's a TriangleStreep and it's an odd i, invert the 2nd and 3rd vertex order
			if (primTopology == D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP && i % 2 != 0)
			{
				v1 = vertices[indexes[size_t(i) + 2]];
				v2 = vertices[indexes[size_t(i) + 1]];
			}
			else
			{
				v1 = vertices[indexes[size_t(i) + 1]];
				v2 = vertices[indexes[size_t(i) + 2]];
			}

			// And convert the vertices to NDC space (storing the transformed result in a separate container)
			const auto& viewMatrix = pCamera->GetViewMatrix();
			m_TransformedVerticesTriangle[0] = v0;
			m_TransformedVerticesTriangle[1] = v1;
			m_TransformedVerticesTriangle[2] = v2;
			const auto cameraPos = pCamera->GetPosition();
			ConvertVerticesScreenSpace(mesh->GetTransformMatrix(false), viewMatrix, pCamera->GetFov(), pCamera->GetFar(), pCamera->GetNear(), cameraPos);

			// Check for culling
			if (cullMode != CULL_MODE::None && mesh != pFireMesh) //// If it's NoCull (or the it's the fireMesh, which requires NoCull), jump this portion of code
			{
				//const auto triangleNormal = GetNormalized(m_TransformedVerticesTriangle[0].Normal + m_TransformedVerticesTriangle[1].Normal +
				//	m_TransformedVerticesTriangle[2].Normal);
				const auto triangleNormal = GetNormalized(Cross(m_TransformedVerticesTriangle[1].Position.xyz - m_TransformedVerticesTriangle[0].Position.xyz,
					m_TransformedVerticesTriangle[2].Position.xyz - m_TransformedVerticesTriangle[0].Position.xyz));
				
				const auto triangleWorldPos = FPoint4(FVector4(m_TransformedVerticesTriangle[0].WorldPosition + FVector4(m_TransformedVerticesTriangle[1].WorldPosition) +
					FVector4(m_TransformedVerticesTriangle[2].WorldPosition)) / 3.f);
				
				const FVector3 cameraToTriangle = GetNormalized(triangleWorldPos.xyz - cameraPos);
				auto dotProd = Dot(triangleNormal, cameraToTriangle);
				if (dotProd > 0.f) //// If the ray hits from the back
				{
					if (cullMode == CULL_MODE::Back) //// If it's BackFacing Mode, return false
						continue;
				}
				else //// If the ray hits from the front
				{
					if (cullMode == CULL_MODE::Front) //// If it's FrontFacing Mode, return false
						continue;
				}
			}
			
			// Ignore triangles with vertexes outside camera frustum (frustum culling)
			bool shouldIgnore = true;
			for (const auto& vertex : m_TransformedVerticesTriangle)
			{
				if (vertex.Position.x >= -1.f && vertex.Position.x <= 1.f && vertex.Position.y >= -1.f && vertex.Position.y <= 1.f && vertex.Position.z >= 0.f && vertex.Position.z <= 1.f)
				{
					shouldIgnore = false;
					break;
				}
			}
			if (shouldIgnore)
				continue;

			// Convert the vertices from NDC space to screenspace (raster space)
			for (auto& vertex : m_TransformedVerticesTriangle)
			{
				vertex.Position.x = (vertex.Position.x + 1.f) / 2.f * m_Width;
				vertex.Position.y = (1 - vertex.Position.y) / 2.f * m_Height;
			}

			// Calculate the triangle edges and the area
			FVector2 edgeA = m_TransformedVerticesTriangle[1].Position.xy - m_TransformedVerticesTriangle[0].Position.xy;
			FVector2 edgeB = m_TransformedVerticesTriangle[2].Position.xy - m_TransformedVerticesTriangle[1].Position.xy;
			FVector2 edgeC = m_TransformedVerticesTriangle[0].Position.xy - m_TransformedVerticesTriangle[2].Position.xy;
			float totalArea = Cross(edgeA, edgeB);

			// Calculate the bounding box
			float minX = m_TransformedVerticesTriangle[0].Position.x;
			if (m_TransformedVerticesTriangle[1].Position.x < minX) minX = m_TransformedVerticesTriangle[1].Position.x;
			if (m_TransformedVerticesTriangle[2].Position.x < minX) minX = m_TransformedVerticesTriangle[2].Position.x;
			float minY = m_TransformedVerticesTriangle[0].Position.y;
			if (m_TransformedVerticesTriangle[1].Position.y < minY) minY = m_TransformedVerticesTriangle[1].Position.y;
			if (m_TransformedVerticesTriangle[2].Position.y < minY) minY = m_TransformedVerticesTriangle[2].Position.y;
			float maxX = m_TransformedVerticesTriangle[0].Position.x;
			if (m_TransformedVerticesTriangle[1].Position.x > maxX) maxX = m_TransformedVerticesTriangle[1].Position.x;
			if (m_TransformedVerticesTriangle[2].Position.x > maxX) maxX = m_TransformedVerticesTriangle[2].Position.x;
			float maxY = m_TransformedVerticesTriangle[0].Position.y;
			if (m_TransformedVerticesTriangle[1].Position.y > maxY) maxY = m_TransformedVerticesTriangle[1].Position.y;
			if (m_TransformedVerticesTriangle[2].Position.y > maxY) maxY = m_TransformedVerticesTriangle[2].Position.y;
			
			// Turn the bounds into uint32_t, rounding them out with a pixel margin
			auto iMinX = uint32_t(minX - 1.f);
			auto iMinY = uint32_t(minY - 1.f);
			auto iMaxX = uint32_t(maxX + 1.f);
			auto iMaxY = uint32_t(maxY + 1.f);

			// Make sure the box doesn't exceed screen boundaries
			iMinX = std::max(iMinX, uint32_t(0));
			iMinY = std::max(iMinY, uint32_t(0));
			iMaxX = std::min(iMaxX, m_Width - 1);
			iMaxY = std::min(iMaxY, m_Height - 1);

			// Loop over only the pixels inside the bounding box
			for (uint32_t r = iMinY; r < iMaxY; ++r)
			{
				for (uint32_t c = iMinX; c < iMaxX; ++c)
				{			
					// Check if the point is inside all the triangle edges
					FVector2 pixelCoordinates = { float(c), float(r) };
						
					const float w0 = Cross(edgeB, pixelCoordinates - FVector2(m_TransformedVerticesTriangle[1].Position.xy)) / totalArea;
					const float w1 = Cross(edgeC, pixelCoordinates - FVector2(m_TransformedVerticesTriangle[2].Position.xy)) / totalArea;
					const float w2 = Cross(edgeA, pixelCoordinates - FVector2(m_TransformedVerticesTriangle[0].Position.xy)) / totalArea;
					if (w0 >= 0.f && w1 >= 0.f && w2 >= 0.f)
					{
						// Calculate the distance between the camera and the hitpoint
						const float zDepth = 1.f / ((1.f / m_TransformedVerticesTriangle[0].Position.z) * w0 + (1.f / m_TransformedVerticesTriangle[1].Position.z) * w1 + (1.f / m_TransformedVerticesTriangle[2].Position.z) * w2);

						// If the point is closer than the one saved in the Depth Buffer
						if (zDepth < m_pDepthBuffer[c + (r * m_Width)])
						{
							// Only replace the value in the buffer if it's not a material with transparency
							if (transparencyOn == false)
								m_pDepthBuffer[c + (r * m_Width)] = zDepth;

							// And calculate the pixel
							CalculatePixel(pDiffuseText, pNormalText, pSpecularText, pGlossText, shininess, w0, w1, w2, c, r, cameraPos,
								pScene->GetLightDirection(), pScene->GetLightIntensity(), pScene->GetAmbientLight(), transparencyOn);
						}
					}
				}
			}
		}
	}

	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Elite::Renderer::ConvertVerticesScreenSpace(const FMatrix4& transformMatrix, const FMatrix4& viewMatrix, float fov, float farPlane, float nearPlane, const FPoint3& cameraPos)
{
	const auto aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);

	// Set up the projection matrix - Right-Hand Coordinate System
	const FMatrix4 projectionMatrix =
	{
		FVector4{1.f / (aspectRatio * fov), 0.f, 0.f, 0.f},
		FVector4{0.f, 1.f / fov, 0.f, 0.f},
		FVector4{0.f, 0.f, -farPlane / (farPlane - nearPlane), -1.f},
		FVector4{0.f, 0.f, -(farPlane * nearPlane) / (farPlane - nearPlane), 0.f}
	};

	// Set up the worldViewProjectionMatrix matrix
	const FMatrix4 worldViewProjectionMatrix = projectionMatrix * viewMatrix * transformMatrix;

	// Transform each vertex of the triangle
	for (auto& vertex : m_TransformedVerticesTriangle)
	{
		// First the world pos (we just adjust it according to the transformation matrix)
		//vertex.WorldPosition = transformMatrix * FPoint4(vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.f); // For some reason, the multiplication operator is not working
		const auto worldPos = FPoint4(vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.f);
		vertex.WorldPosition = FPoint4(
			transformMatrix(0, 0) * worldPos.x + transformMatrix(0, 1) * worldPos.y + transformMatrix(0, 2) * worldPos.z + transformMatrix(0, 3),
			transformMatrix(1, 0) * worldPos.x + transformMatrix(1, 1) * worldPos.y + transformMatrix(1, 2) * worldPos.z + transformMatrix(1, 3),
			transformMatrix(2, 0) * worldPos.x + transformMatrix(2, 1) * worldPos.y + transformMatrix(2, 2) * worldPos.z + transformMatrix(2, 3),
			transformMatrix(3, 0) * worldPos.x + transformMatrix(3, 1) * worldPos.y + transformMatrix(3, 2) * worldPos.z + transformMatrix(3, 3));

		// Then the position (with perspective divide)
		//FPoint4 transformedPos = worldViewProjectionMatrix * vertex.Position; // For some reason, the multiplication operator is not working
		FPoint4 transformedPos(
			worldViewProjectionMatrix(0, 0) * vertex.Position.x + worldViewProjectionMatrix(0, 1) * vertex.Position.y + worldViewProjectionMatrix(0, 2) * vertex.Position.z + worldViewProjectionMatrix(0, 3),
			worldViewProjectionMatrix(1, 0) * vertex.Position.x + worldViewProjectionMatrix(1, 1) * vertex.Position.y + worldViewProjectionMatrix(1, 2) * vertex.Position.z + worldViewProjectionMatrix(1, 3),
			worldViewProjectionMatrix(2, 0) * vertex.Position.x + worldViewProjectionMatrix(2, 1) * vertex.Position.y + worldViewProjectionMatrix(2, 2) * vertex.Position.z + worldViewProjectionMatrix(2, 3),
			worldViewProjectionMatrix(3, 0) * vertex.Position.x + worldViewProjectionMatrix(3, 1) * vertex.Position.y + worldViewProjectionMatrix(3, 2) * vertex.Position.z + worldViewProjectionMatrix(3, 3));

		
		if (transformedPos.w != 0.f)
		{
			transformedPos.x /= transformedPos.w;
			transformedPos.y /= transformedPos.w;
			transformedPos.z /= transformedPos.w;
		}
		vertex.Position = transformedPos;
		

		// Then the normal (again, just the transformation matrix)
		//FPoint4 transformedNorm = transformMatrix * FPoint4(vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 1.f); // For some reason, the multiplication operator is not working
		const auto norm = FPoint4(vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 1.f);
		const FPoint4 transformedNorm(
			transformMatrix(0, 0) * norm.x + transformMatrix(0, 1) * norm.y + transformMatrix(0, 2) * norm.z + transformMatrix(0, 3),
			transformMatrix(1, 0) * norm.x + transformMatrix(1, 1) * norm.y + transformMatrix(1, 2) * norm.z + transformMatrix(1, 3),
			transformMatrix(2, 0) * norm.x + transformMatrix(2, 1) * norm.y + transformMatrix(2, 2) * norm.z + transformMatrix(2, 3),
			transformMatrix(3, 0) * norm.x + transformMatrix(3, 1) * norm.y + transformMatrix(3, 2) * norm.z + transformMatrix(3, 3));
		vertex.Normal = GetNormalized(FVector3(transformedNorm.xyz));

		// And then the tangent (again, just the transformation matrix)
		//FPoint4 transformedTangent = transformMatrix * FPoint4(vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z, 1.f); // For some reason, the multiplication operator is not working
		const auto tan = FPoint4(vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z, 1.f);
		const FPoint4 transformedTangent(
			transformMatrix(0, 0) * tan.x + transformMatrix(0, 1) * tan.y + transformMatrix(0, 2) * tan.z + transformMatrix(0, 3),
			transformMatrix(1, 0) * tan.x + transformMatrix(1, 1) * tan.y + transformMatrix(1, 2) * tan.z + transformMatrix(1, 3),
			transformMatrix(2, 0) * tan.x + transformMatrix(2, 1) * tan.y + transformMatrix(2, 2) * tan.z + transformMatrix(2, 3),
			transformMatrix(3, 0) * tan.x + transformMatrix(3, 1) * tan.y + transformMatrix(3, 2) * tan.z + transformMatrix(3, 3));
		vertex.Tangent = GetNormalized(FVector3(transformedTangent.xyz));
	}
}

void Elite::Renderer::CalculatePixel(const Texture* pDiffuseText, const Texture* pNormalText, const Texture* pSpecularText, const Texture* pGlossText,
	float shininess, float w0, float w1, float w2, int c, int r, const FPoint3& cameraPos,
	const FVector3& lightDirection, float lightIntensity, const FVector3& ambientLight, bool transparencyOn) const
{
	const float wInterp = 1.f / ((1.f / m_TransformedVerticesTriangle[0].Position.w) * w0 + (1.f / m_TransformedVerticesTriangle[1].Position.w) * w1 + (1.f / m_TransformedVerticesTriangle[2].Position.w) * w2);

	// Interpolate the vertices normals, tangents, view direction, UV values and world position - not in NDC space (and normalize the first 3)
	const auto interpNormal = GetNormalized(((m_TransformedVerticesTriangle[0].Normal / m_TransformedVerticesTriangle[0].Position.w) * w0 +
		(m_TransformedVerticesTriangle[1].Normal / m_TransformedVerticesTriangle[1].Position.w) * w1 +
		(m_TransformedVerticesTriangle[2].Normal / m_TransformedVerticesTriangle[2].Position.w) * w2) * wInterp);
	const auto interpTangent = GetNormalized(((m_TransformedVerticesTriangle[0].Tangent / m_TransformedVerticesTriangle[0].Position.w) * w0 +
		(m_TransformedVerticesTriangle[1].Tangent / m_TransformedVerticesTriangle[1].Position.w) * w1 +
		(m_TransformedVerticesTriangle[2].Tangent / m_TransformedVerticesTriangle[2].Position.w) * w2) * wInterp);
	const auto interpUV = ((m_TransformedVerticesTriangle[0].UVCoord / m_TransformedVerticesTriangle[0].Position.w) * w0 +
		(m_TransformedVerticesTriangle[1].UVCoord / m_TransformedVerticesTriangle[1].Position.w) * w1 +
		(m_TransformedVerticesTriangle[2].UVCoord / m_TransformedVerticesTriangle[2].Position.w) * w2) * wInterp;
	const auto interpWorldPosition = ((FVector4(m_TransformedVerticesTriangle[0].WorldPosition) / m_TransformedVerticesTriangle[0].Position.w) * w0 +
		(FVector4(m_TransformedVerticesTriangle[1].WorldPosition) / m_TransformedVerticesTriangle[1].Position.w) * w1 +
		(FVector4(m_TransformedVerticesTriangle[2].WorldPosition) / m_TransformedVerticesTriangle[2].Position.w) * w2) * wInterp;

	const auto viewDir0 = FVector3(m_TransformedVerticesTriangle[0].WorldPosition.xyz) - FVector3(cameraPos);
	const auto viewDir1 = FVector3(m_TransformedVerticesTriangle[1].WorldPosition.xyz) - FVector3(cameraPos);
	const auto viewDir2 = FVector3(m_TransformedVerticesTriangle[2].WorldPosition.xyz) - FVector3(cameraPos);
	const auto interpViewDir = GetNormalized(((viewDir0 / m_TransformedVerticesTriangle[0].Position.w) * w0 +
		(viewDir1 / m_TransformedVerticesTriangle[1].Position.w) * w1 +
		(viewDir2 / m_TransformedVerticesTriangle[2].Position.w) * w2) * wInterp);

	// Calculate the final color
	RGBColor finalColor{ 0.f, 0.f, 0.f };
	
	if (transparencyOn == false) // If it's not transparent, shade normally
	{
		if (pDiffuseText == nullptr) // If the mesh has no texture
		{
			// Interpolate the given colors
			finalColor = ((m_TransformedVerticesTriangle[0].Color / m_TransformedVerticesTriangle[0].Position.w) * w0 +
				(m_TransformedVerticesTriangle[1].Color / m_TransformedVerticesTriangle[1].Position.w) * w1 +
				(m_TransformedVerticesTriangle[2].Color / m_TransformedVerticesTriangle[2].Position.w) * w2) * wInterp;
		}
		else // If it does
		{
			// Just sample according to the interpolated UV
			RGBColor pixelColor = pDiffuseText->Sample(interpUV);

			// Use this new info to calculate the ouputVertex, and use it to shade the pixel
			// The position in screen space irrelevant, so we can just pass a copy of the world position in its place
			VS_OUTPUT outputVertex = { FPoint4(interpWorldPosition), FPoint4(interpWorldPosition), pixelColor, interpUV, interpNormal, interpTangent };
			PixelShading(outputVertex, finalColor, pNormalText, pSpecularText, pGlossText, shininess, interpViewDir, lightDirection, lightIntensity, ambientLight);
		}
	}
	else // If it is transparent, calculate a blend between the old color in the BackBuffer and the new sampled one
	{
		// I don't understand why does this still shows artifacts
		const FVector4 sample = pDiffuseText->SampleWTransparency(interpUV);
		Uint8 oldRUint, oldGUint, oldBUint;
		SDL_GetRGB(m_pBackBufferPixels[c + (r * m_Width)], m_pBackBuffer->format, &oldRUint, &oldGUint, &oldBUint);
		float oldR = static_cast<float>(oldRUint) / 255.f;
		float oldG = static_cast<float>(oldGUint) / 255.f;
		float oldB = static_cast<float>(oldBUint) / 255.f;

		finalColor.r = sample.r * sample.w + oldR * (1.f - sample.w);
		finalColor.g = sample.g * sample.w + oldG * (1.f - sample.w);
		finalColor.b = sample.b * sample.w + oldB * (1.f - sample.w);
	}

	// Set the finalColor as the new pixel color in the BackBuffer
	m_pBackBufferPixels[c + (r * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255.f),
		static_cast<uint8_t>(finalColor.g * 255.f),
		static_cast<uint8_t>(finalColor.b * 255.f));
}

void Elite::Renderer::PixelShading(const VS_OUTPUT& outputVertex, RGBColor& finalColor, const Texture* pNormalText, const Texture* pSpecularText, const Texture* pGlossText, float shininess, const FVector3& interpViewDir,
	const FVector3& lightDirection, float lightIntensity, const FVector3& ambientLight) const
{
	finalColor = { 0.f, 0.f, 0.f };
	auto mappedNormal = outputVertex.Normal;

	if (pNormalText != nullptr) // If the mesh has a normal map
	{
		// Get the pixel sample from the Normal Map and remap it to the correct range [-1,1]
		const auto normalMapSampleCol = pNormalText->Sample(outputVertex.UVCoord);
		auto normalMapSampleVec = FVector3(normalMapSampleCol.r, normalMapSampleCol.g, normalMapSampleCol.b);
		normalMapSampleVec = normalMapSampleVec * 2.f - FVector3{ 1.f, 1.f, 1.f };

		// And put it in tangent space
		const FVector3 binormal = Cross(outputVertex.Tangent, outputVertex.Normal);
		const FMatrix3 tangentSpaceAxis = FMatrix3(outputVertex.Tangent, binormal, outputVertex.Normal);
		//mappedNormal = GetNormalized(tangentSpaceAxis * normalMapSampleVec);  // For some reason, the multiplication operator is not working
		mappedNormal = FVector3(tangentSpaceAxis(0, 0) * normalMapSampleVec.x + tangentSpaceAxis(0, 1) * normalMapSampleVec.y + tangentSpaceAxis(0, 2) * normalMapSampleVec.z,
			tangentSpaceAxis(1, 0)* normalMapSampleVec.x + tangentSpaceAxis(1, 1) * normalMapSampleVec.y + tangentSpaceAxis(1, 2) * normalMapSampleVec.z,
			tangentSpaceAxis(2, 0)* normalMapSampleVec.x + tangentSpaceAxis(2, 1) * normalMapSampleVec.y + tangentSpaceAxis(2, 2) * normalMapSampleVec.z);
		Normalize(mappedNormal);
	}
	
	float specularColor = 0.f;
	float glossiness = 0.f;
	if (pSpecularText != nullptr && pGlossText != nullptr) // If the mesh has a specular and glossiness map
	{
		// Get the pixel sample from the Specular Map
		const auto specMapSampleCol = pSpecularText->Sample(outputVertex.UVCoord);
		specularColor = specMapSampleCol.r;

		// Get the pixel sample from the Glossiness Map and multiply it by the shine variable
		const auto glossMapSampleCol = pGlossText->Sample(outputVertex.UVCoord);
		glossiness = glossMapSampleCol.r * shininess;
	}

	// Calculate the PhongBRDF, if the maps the maps were provided (use inverted light direction, as to adjust for the coordinate system flip)
	const Elite::FVector3 reflectedLightDir = Reflect(mappedNormal, -lightDirection);
	auto phongBRDF = RGBColor{ 0.f, 0.f, 0.f };
	if (pSpecularText != nullptr && pGlossText != nullptr)
	{
		float specularStrength = std::max(0.0f, Dot(interpViewDir, reflectedLightDir));
		specularStrength = powf(specularStrength, glossiness);
		const auto specularReflection = specularColor * specularStrength;
		phongBRDF = { specularReflection, specularReflection, specularReflection };

	}

	// Calculate the LambertBRDF (use inverted light direction, as to adjust for the coordinate system flip)
	//auto diffuseStrength = std::max(Dot(mappedNormal, reflectedLightDir), 0.f);
	auto diffuseStrength = std::max(Dot(mappedNormal, -lightDirection), 0.f);
	diffuseStrength = (diffuseStrength * lightIntensity) / float(M_PI);
	auto lambertBRDF = outputVertex.Color * diffuseStrength;

	// And add the effect of said light to the finalColor
	finalColor = lambertBRDF + phongBRDF + RGBColor(ambientLight.x, ambientLight.y, ambientLight.z);

	// Cap the color value at 1.f
	finalColor.MaxToOne();
}

HRESULT Elite::Renderer::InitializeDirectX()
{
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	uint32_t createDeviceFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create the device and the device context
	HRESULT result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, 0, 0, D3D11_SDK_VERSION, &m_pDevice, &featureLevel, &m_pDeviceContext);
	if (FAILED(result))
		return result;

	// Create the DXGIFactory
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&m_pDXGIFactory));
	if (FAILED(result))
		return result;

	// Create the SwapChain Descriptor
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = m_Width;
	swapChainDesc.BufferDesc.Height = m_Height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Get the handle HWND from the SDL Backbuffer
	SDL_SysWMinfo sysWMInfo{};
	SDL_VERSION(&sysWMInfo.version);
	SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
	swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

	// Create SwapChain and hook it into the handle of the SDL Window
	result = m_pDXGIFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
	if (FAILED(result))
		return result;

	// Set up the descriptor for the Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc{};
	depthStencilDesc.Width = m_Width;
	depthStencilDesc.Height = m_Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// Set up the descriptor for the resource view
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = depthStencilDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// And with the last 2, create the actual resource and matching resource view
	result = m_pDevice->CreateTexture2D(&depthStencilDesc, 0, &m_pDepthStencilBuffer);
	if (FAILED(result))
		return result;
	result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
	if (FAILED(result))
		return result;

	//Create the RenderTargetView
	result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
	if (FAILED(result))
		return result;
	result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, 0, &m_pRenderTargetView);
	if (FAILED(result))
		return result;

	// Bind the Views to the Output Merger Stage
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

	// Set the Viewport
	D3D11_VIEWPORT viewPort{};
	viewPort.Width = static_cast<float>(m_Width);
	viewPort.Height = static_cast<float>(m_Height);
	viewPort.TopLeftX = 0.f;
	viewPort.TopLeftY = 0.f;
	viewPort.MinDepth = 0.f;
	viewPort.MaxDepth = 1.f;
	m_pDeviceContext->RSSetViewports(1, &viewPort);
	
	return S_OK;
}

bool Elite::Renderer::InitializeSoftware()
{
	m_pFrontBuffer = SDL_GetWindowSurface(m_pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;
	m_pDepthBuffer = new float[size_t(m_Width) * m_Height];
	m_TransformedVerticesTriangle.resize(3);
	
	return (m_pFrontBuffer && m_pBackBuffer && m_pBackBufferPixels && m_pDepthBuffer);
}


