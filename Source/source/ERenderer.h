/*=============================================================================*/
// Copyright 2017-2019 Elite Engine
// Authors: Matthieu Delaere
/*=============================================================================*/
// ERenderer.h: class that holds the surface to render too + DirectX initialization.
/*=============================================================================*/
#ifndef ELITE_RAYTRACING_RENDERER
#define	ELITE_RAYTRACING_RENDERER

#include <cstdint>
#include <vector>

enum class SAMPLER_FILTER;
enum class CULL_MODE;
class Mesh;
struct VS_INPUT;
struct VS_OUTPUT;
class Texture;
class Scene;
struct SDL_Window;
struct SDL_Surface;

enum class RENDER_MODE
{
	DirectX,
	Software
};

namespace Elite
{
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Render(Scene* pScene, SAMPLER_FILTER samplerFilter, RENDER_MODE renderMode, CULL_MODE cullMode, bool fireFXVisible, Mesh* pFireMesh = nullptr);
		void RenderDirectX(Scene* pScene, SAMPLER_FILTER samplerFilter, CULL_MODE cullMode, bool fireFXVisible, Mesh* pFireMesh) const;
		void RenderSoftware(Scene* pScene, CULL_MODE cullMode, bool fireFXVisible, Mesh* pFireMesh);


		void ConvertVerticesScreenSpace(const FMatrix4& transformMatrix, const FMatrix4& viewMatrix, float fov, float farPlane, float nearPlane, const FPoint3& cameraPos);
		void CalculatePixel(const Texture* pDiffuseText, const Texture* pNormalText, const Texture* pSpecularText, const Texture* pGlossText,
			float shininess, float w0, float w1, float w2, int c, int r, const FPoint3& cameraPos,
			const FVector3& lightDirection, float lightIntensity, const FVector3& ambientLight, bool transparencyOn) const;
		void PixelShading(const VS_OUTPUT& outputVertex, RGBColor& finalColor, const Texture* pNormalText, const Texture* pSpecularText, const Texture* pGlossText, float shininess, const FVector3& interpViewDir,
			const FVector3& lightDirection, float lightIntensity, const FVector3& ambientLight) const;

		
		HRESULT InitializeDirectX();
		bool InitializeSoftware();

		ID3D11Device* GetDevice() const { return m_pDevice; }

	private:
		SDL_Window* m_pWindow;
		uint32_t m_Width;
		uint32_t m_Height;

		bool m_DirectXInitialized;
		bool m_SoftwareInitialized;

		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pDeviceContext;
		IDXGIFactory* m_pDXGIFactory;
		IDXGISwapChain* m_pSwapChain;
		ID3D11Texture2D* m_pDepthStencilBuffer;
		ID3D11DepthStencilView* m_pDepthStencilView;
		ID3D11Resource* m_pRenderTargetBuffer;
		ID3D11RenderTargetView* m_pRenderTargetView;

		SDL_Surface* m_pFrontBuffer = nullptr;
		SDL_Surface* m_pBackBuffer = nullptr;
		float* m_pDepthBuffer = nullptr;
		uint32_t* m_pBackBufferPixels = nullptr;

		std::vector<VS_OUTPUT> m_TransformedVerticesTriangle; // This is a temporary vector to store whatever triangle is currently being calculated in Software Mode - it's stored as a member variable for optimization purposes
	};
}

#endif