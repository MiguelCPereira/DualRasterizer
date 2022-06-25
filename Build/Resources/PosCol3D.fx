float4x4 gWorldViewProj : WorldViewProjection;
Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossinessMap : GlossinessMap;
float4x4 gWorldMatrix : World;
float4x4 gViewInverseMatrix : ViewInverse;
float gShininess : Shininess;
float3 gAmbient : AmbientLight;
float3 gLightDirection : LightDirection;
float gLightIntensity : LightIntensity;


// Hardcoded Values
const float gPi = 3.14159265358979323846f;

// -------------
// SamplerStates
// -------------
SamplerState gSamplerPoint
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Border; //Mirror, Clamp or Border
	AddressV = Clamp; //Mirror, Clamp or Border
	BorderColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
};

SamplerState gSamplerLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border; //Mirror, Clamp or Border
	AddressV = Clamp; //Mirror, Clamp or Border
	BorderColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
};

SamplerState gSamplerAnisotropic
{
	Filter = ANISOTROPIC;
	AddressU = Border; //Mirror, Clamp or Border
	AddressV = Clamp; //Mirror, Clamp or Border
	BorderColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
};


// ----------------
// RasterizerStates
// ----------------
RasterizerState gRasterizerStateBackCull
{
	CullMode = back;
	FrontCounterClockwise = true;
};

RasterizerState gRasterizerStateFrontCull
{
	CullMode = front;
	FrontCounterClockwise = true;
};

RasterizerState gRasterizerStateNoCull
{
	CullMode = none;
	FrontCounterClockwise = true;
};


// ----------
// BlendState
// ----------
BlendState gBlendStateOn
{
	BlendEnable[0] = true;
	SrcBlend = src_alpha;
	DestBlend = inv_src_alpha;
	BlendOp = add;
	SrcBlendAlpha = zero;
	DestBlendAlpha = zero;
	BlendOpAlpha = add;
	RenderTargetWriteMask[0] = 0x0F;
};

BlendState gBlendStateOff
{
	BlendEnable[0] = false;
};


// -----------------
// DepthStencilState
// -----------------
DepthStencilState gDepthStencilStateStencilOff
{
	DepthEnable = true;
	DepthWriteMask = zero;
	DepthFunc = less;
	StencilEnable = false;
	
	StencilReadMask = 0x0F;
	StencilWriteMask = 0x0F;
	
	FrontFaceStencilFunc = always;
	BackFaceStencilFunc = always;
	
	FrontFaceStencilDepthFail = keep;
	BackFaceStencilDepthFail = keep;
	
	FrontFaceStencilPass = keep;
	BackFaceStencilPass = keep;
	
	FrontFaceStencilFail = keep;
	BackFaceStencilFail = keep;
};

DepthStencilState gDepthStencilStateStencilOn
{
	DepthEnable = true;
	DepthWriteMask = all;
	DepthFunc = less;
	StencilEnable = false;
	
	StencilReadMask = 0x0F;
	StencilWriteMask = 0x0F;
	
	FrontFaceStencilFunc = always;
	BackFaceStencilFunc = always;
	
	FrontFaceStencilDepthFail = keep;
	BackFaceStencilDepthFail = keep;
	
	FrontFaceStencilPass = keep;
	BackFaceStencilPass = keep;
	
	FrontFaceStencilFail = keep;
	BackFaceStencilFail = keep;
};


// --------------------
// Input/Output structs
// --------------------
struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Color : COLOR;
	float2 UVCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float4 WorldPosition : WORLD_POSITION;
	float3 Color : COLOR;
	//float4 WorldPosition : COLOR;
	float2 UVCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};


// -------------
// Vertex Shader
// -------------
VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul(float4(input.Position, 1.f), gWorldViewProj); 
	output.WorldPosition = mul(float4(input.Position, 1.f), gWorldMatrix);
	output.Color = input.Color;
	output.UVCoord = input.UVCoord;
	output.Normal = mul(normalize(input.Normal), (float3x3)gWorldMatrix);
	output.Tangent = mul(normalize(input.Tangent), (float3x3)gWorldMatrix);
	return output;
}

// -----------------------------
// Pixel Shaders Helper Function
// -----------------------------
float4 Shading(VS_OUTPUT input, float4 difuseSample, float4 normalSample, float4 specularSample, float4 glossinessSample)
{
	// Remap the normal sample to the correct range [-1,1]
	float3 remmapedNormal = normalSample.xyz * 2.f - float3(1.f, 1.f, 1.f);
		
	// And put it in tangent space
	const float3 binormal = cross(input.Normal, input.Tangent);
	const float3x3 tangentSpaceAxis = float3x3(input.Tangent, binormal, input.Normal);
	remmapedNormal = normalize(mul(remmapedNormal, tangentSpaceAxis));
	
	// Calculate specularReflectance and phongExponent through the Specular and Glossiness map samples
	const float specularColor = specularSample.x;
	const float glossiness = glossinessSample.x * gShininess;
	
	// Calculate the view direction and reflected light direction
	const float3 viewDirection = normalize(gViewInverseMatrix[3].xyz - input.WorldPosition.xyz);
	const float3 reflectedLightDir = normalize(reflect(gLightDirection, remmapedNormal));
	
	// Calculate the PhongBRDF
	float specularStrength = saturate(dot(viewDirection, reflectedLightDir));
	specularStrength = pow(specularStrength, glossiness);
	const float specularComponent = specularColor * specularStrength;
	const float3 phongBRDF = float3(specularComponent, specularComponent, specularComponent);
	
	// Calculate the LambertBRDF
	float diffuseStrength = saturate(dot(remmapedNormal, reflectedLightDir));
	diffuseStrength = (diffuseStrength * gLightIntensity) / gPi;
	const float3 lambertBRDF = difuseSample.xyz * diffuseStrength;
	
	// And calculate the finalColor
	float3 finalColor = lambertBRDF + phongBRDF + gAmbient;
	return saturate(float4(finalColor, 1.f));
}

// -------------
// Pixel Shaders
// -------------
float4 PSPoint(VS_OUTPUT input) : SV_TARGET
{
	if(input.UVCoord.x >= 0.f) // Check if the vertex has valid uv coords before sampling
	{
		float4 difuseSample = gDiffuseMap.Sample(gSamplerPoint, input.UVCoord);
		float4 normalSample = gNormalMap.Sample(gSamplerPoint, input.UVCoord);
		float4 specularSample = gSpecularMap.Sample(gSamplerPoint, input.UVCoord);
		float4 glossinessSample = gGlossinessMap.Sample(gSamplerPoint, input.UVCoord);
		return Shading(input, difuseSample, normalSample, specularSample, glossinessSample);
	}
	return float4(input.Color, 1.f); // If they aren't, return the predefined color
}

float4 PSPointDifOnly(VS_OUTPUT input) : SV_TARGET
{
	if(input.UVCoord.x >= 0.f) // Check if the vertex has valid uv coords before sampling
	{
		float4 difuseSample = gDiffuseMap.Sample(gSamplerPoint, input.UVCoord);
		return difuseSample;
	}
	return float4(input.Color, 1.f); // If they aren't, return the predefined color
}

float4 PSLinear(VS_OUTPUT input) : SV_TARGET
{
	if(input.UVCoord.x >= 0.f) // Check if the vertex has valid uv coords before sampling
	{
		float4 difuseSample = gDiffuseMap.Sample(gSamplerLinear, input.UVCoord);
		float4 normalSample = gNormalMap.Sample(gSamplerLinear, input.UVCoord);
		float4 specularSample = gSpecularMap.Sample(gSamplerLinear, input.UVCoord);
		float4 glossinessSample = gGlossinessMap.Sample(gSamplerLinear, input.UVCoord);
		return Shading(input, difuseSample, normalSample, specularSample, glossinessSample);
	}
	return float4(input.Color, 1.f); // If they aren't, return the predefined color
}

float4 PSLinearDifOnly(VS_OUTPUT input) : SV_TARGET
{
	if(input.UVCoord.x >= 0.f) // Check if the vertex has valid uv coords before sampling
	{
		float4 difuseSample = gDiffuseMap.Sample(gSamplerLinear, input.UVCoord);
		return difuseSample;
	}
	return float4(input.Color, 1.f); // If they aren't, return the predefined color
}

float4 PSAnisotropic(VS_OUTPUT input) : SV_TARGET
{
	if(input.UVCoord.x >= 0.f) // Check if the vertex has valid uv coords before sampling
	{
		float4 difuseSample = gDiffuseMap.Sample(gSamplerAnisotropic, input.UVCoord);
		float4 normalSample = gNormalMap.Sample(gSamplerAnisotropic, input.UVCoord);
		float4 specularSample = gSpecularMap.Sample(gSamplerAnisotropic, input.UVCoord);
		float4 glossinessSample = gGlossinessMap.Sample(gSamplerAnisotropic, input.UVCoord);
		return Shading(input, difuseSample, normalSample, specularSample, glossinessSample);
	}
	return float4(input.Color, 1.f); // If they aren't, return the predefined color
}

float4 PSAnisotropicDifOnly(VS_OUTPUT input) : SV_TARGET
{
	if(input.UVCoord.x >= 0.f) // Check if the vertex has valid uv coords before sampling
	{
		float4 difuseSample = gDiffuseMap.Sample(gSamplerAnisotropic, input.UVCoord);
		return difuseSample;
	}
	return float4(input.Color, 1.f); // If they aren't, return the predefined color
}


// ----------
// Techniques
// ----------
technique11 PointTechniqueBackCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateBackCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSPoint() ) );
	}
}

technique11 PointTechniqueFrontCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateFrontCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSPoint() ) );
	}
}

technique11 PointTechniqueNoCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateNoCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSPoint() ) );
	}
}

technique11 TransparentPointTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateNoCull);
		SetDepthStencilState(gDepthStencilStateStencilOff, 0);
		SetBlendState(gBlendStateOn, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSPointDifOnly() ) );
	}
}

technique11 LinearTechniqueBackCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateBackCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSLinear() ) );
	}
}

technique11 LinearTechniqueFrontCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateFrontCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSLinear() ) );
	}
}

technique11 LinearTechniqueNoCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateNoCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSLinear() ) );
	}
}

technique11 TransparentLinearTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateNoCull);
		SetDepthStencilState(gDepthStencilStateStencilOff, 0);
		SetBlendState(gBlendStateOn, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSLinearDifOnly() ) );
	}
}

technique11 AnisotropicTechniqueBackCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateBackCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSAnisotropic() ) );
	}
}

technique11 AnisotropicTechniqueFrontCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateFrontCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSAnisotropic() ) );
	}
}

technique11 AnisotropicTechniqueNoCull
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateNoCull);
		SetDepthStencilState(gDepthStencilStateStencilOn, 0);
		SetBlendState(gBlendStateOff, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSAnisotropic() ) );
	}
}

technique11 TransparentAnisotropicTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerStateNoCull);
		SetDepthStencilState(gDepthStencilStateStencilOff, 0);
		SetBlendState(gBlendStateOn, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PSAnisotropicDifOnly() ) );
	}
}