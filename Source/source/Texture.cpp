#include "pch.h"
#include "Texture.h"

#include <iostream>
#include <SDL_image.h>

Texture::Texture(ID3D11Device* pDevice, const char* filePath)
	: m_pTexture{}
	, m_pTexResourceView{}
	, m_Resource{} // aka, the surface
	, m_ResourcePixel{}
{
	m_Resource = IMG_Load(filePath);
	
	if (m_Resource == nullptr)
	{
		std::cout << "Unable to load texture file into Surface\n";
	}
	else
	{
		m_ResourcePixel = static_cast<uint32_t*> (m_Resource->pixels);


		D3D11_TEXTURE2D_DESC desc;
		desc.Width = m_Resource->w;
		desc.Height = m_Resource->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = m_Resource->pixels;
		initData.SysMemPitch = static_cast<UINT>(m_Resource->pitch);
		initData.SysMemSlicePitch = static_cast<UINT>(m_Resource->h * m_Resource->pitch);

		HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pTexture);
		if (FAILED(hr))
			std::cout << "Unable to create Texture2D\n";

		// We don't release surface in the initializer anymore, as we need it for the software mode
		// SDL_FreeSurface(m_Resource);

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		if (m_pTexture)
			hr = pDevice->CreateShaderResourceView(m_pTexture, &SRVDesc, &m_pTexResourceView);

		if (FAILED(hr))
			std::cout << "Unable to create Shader Resource View\n";
	}
}

Texture::~Texture()
{
	SDL_FreeSurface(m_Resource);
	m_Resource = nullptr;
	
	if (m_pTexture)
	{
		m_pTexture->Release();
		m_pTexture = nullptr;
	}

	if (m_pTexResourceView)
	{
		m_pTexResourceView->Release();
		m_pTexResourceView = nullptr;
	}
}

Elite::RGBColor Texture::Sample(const Elite::FVector2& uv) const
{
	const auto width = m_Resource->w;
	const auto height = m_Resource->h;
	const auto col = int(uv.x * width);
	const auto row = int(uv.y * height);
	const Uint32 pixelCoord = col + (row * width);

	Uint8 r{}, g{}, b{};

	const Uint32 size = height * width;
	if (pixelCoord < size)
		SDL_GetRGB(m_ResourcePixel[pixelCoord], m_Resource->format, &r, &g, &b);

	return Elite::RGBColor{ static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f };
}

Elite::FVector4 Texture::SampleWTransparency(const Elite::FVector2& uv) const
{
	const auto width = m_Resource->w;
	const auto height = m_Resource->h;
	const auto col = int(uv.x * width);
	const auto row = int(uv.y * height);
	const Uint32 pixelCoord = col + (row * width);

	Uint8 r{}, g{}, b{}, a{};
	
	const Uint32 size = height * width;
	if (pixelCoord < size)
		SDL_GetRGBA(m_ResourcePixel[pixelCoord], m_Resource->format, &r, &g, &b, &a);

	return Elite::FVector4{ static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f,  static_cast<float>(a) / 255.f };
}


