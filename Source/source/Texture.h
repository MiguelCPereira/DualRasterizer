#pragma once
#include <SDL_surface.h>
#include "EMath.h"
#include "ERGBColor.h"

class Texture
{
public:
	Texture(ID3D11Device* pDevice, const char* filePath);
	~Texture();

	Texture(const Texture& other) = delete;
	Texture(Texture&& other) noexcept = delete;
	Texture& operator=(const Texture& other) = delete;
	Texture& operator=(Texture&& other) noexcept = delete;

	ID3D11ShaderResourceView* GetResourceView() const { return m_pTexResourceView; }

	Elite::RGBColor Sample(const Elite::FVector2& uv) const;
	Elite::FVector4 SampleWTransparency(const Elite::FVector2& uv) const;

private:
	ID3D11Texture2D* m_pTexture;
	ID3D11ShaderResourceView* m_pTexResourceView;
	SDL_Surface* m_Resource;
	uint32_t* m_ResourcePixel;
};