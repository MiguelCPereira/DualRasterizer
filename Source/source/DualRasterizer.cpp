#include "pch.h"
//#undef main

//Standard includes
#include <iostream>

//Project includes
#include <string>
#include <fstream>
#include <sstream>

#include "ETimer.h"
#include "ERenderer.h"
#include "ECamera.h"
#include "Scene.h"
#include "Mesh.h"
#include "ShadedMaterial.h"
#include "TransparentMaterial.h"

#ifdef _DEBUG
#include <vld.h>
#endif

Mesh* ParseOBJFile(const std::string& filePath, ID3D11Device* pDevice, BaseMaterial* pMaterial)
{
	std::vector<Elite::FPoint3> positions{};
	std::vector<Elite::FVector2> uvs{};
	std::vector<Elite::FVector3> normals{};

	std::vector<int> vertexIndices{};
	std::vector<int> uvIndices{};
	std::vector<int> normalIndices{};


	// Open the file
	std::ifstream in(filePath, std::ios::in);
	if (!in)
	{
		std::cout << "An error occurred while opening the obj file." << std::endl;
		return new Mesh(pDevice, std::vector<VS_INPUT>(), std::vector<uint32_t>(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, pMaterial);
	}

	// Go over every line
	std::string line;
	while (std::getline(in, line))
	{
		// Save the vertex positions
		if (line.substr(0, 2) == "v ")
		{
			std::istringstream v(line.substr(2));
			float x, y, z;
			v >> x;
			v >> y;
			v >> z;
			// Invert the z axis (since it's a Left-Handed Coord System)
			Elite::FPoint3 vertex(x, y, -z);
			positions.push_back(vertex);
		}

		// Save the indexes
		else if (line.substr(0, 2) == "f ")
		{
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			const char* chh = line.c_str();
			sscanf_s(chh, "f %i/%i/%i %i/%i/%i %i/%i/%i", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);

			vertexIndices.push_back(vertexIndex[0] - 1);
			vertexIndices.push_back(vertexIndex[1] - 1);
			vertexIndices.push_back(vertexIndex[2] - 1);
			uvIndices.push_back(uvIndex[0] - 1);
			uvIndices.push_back(uvIndex[1] - 1);
			uvIndices.push_back(uvIndex[2] - 1);
			normalIndices.push_back(normalIndex[0] - 1);
			normalIndices.push_back(normalIndex[1] - 1);
			normalIndices.push_back(normalIndex[2] - 1);
		}

		// Save the normals
		else if (line.substr(0, 2) == "vn")
		{
			std::istringstream vn(line.substr(3));
			float x, y, z;
			vn >> x;
			vn >> y;
			vn >> z;
			// Invert the z axis (since it's a Left-Handed Coord System)
			normals.emplace_back(Elite::FVector3(x, y, -z));
		}

		// Save the uvs
		else if (line.substr(0, 2) == "vt")
		{
			std::istringstream vt(line.substr(3));
			double u, v;
			vt >> u;
			vt >> v;
			uvs.emplace_back(Elite::FVector2(float(u), float(1.0 - v)));
		}
	}

	std::vector<VS_INPUT> vertexBuffer{};
	std::vector<uint32_t> indexBuffer{};

	// If the file is in a valid format
	if (vertexIndices.size() == uvIndices.size() && vertexIndices.size() == normalIndices.size())
	{
		// Set up the vertexBuffer
		for (int i = 0; i < vertexIndices.size(); i++)
			vertexBuffer.push_back(VS_INPUT(positions[vertexIndices[i]], Elite::RGBColor{ 1.f, 1.f, 1.f }, uvs[uvIndices[i]], normals[normalIndices[i]]));

		// Set up the indexBuffer
		for (int i = 0; i < int(vertexBuffer.size()); i++)
			indexBuffer.push_back(i);

		// And add the tangents to the vertexBuffer - inspired in https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
		for (int i = 0; i < int(indexBuffer.size()); i += 3)
		{
			int index0 = indexBuffer[i];
			int index1 = indexBuffer[size_t(i) + 1];
			int index2 = indexBuffer[size_t(i) + 2];

			const Elite::FPoint4& p0 = vertexBuffer[index0].Position;
			const Elite::FPoint4& p1 = vertexBuffer[index1].Position;
			const Elite::FPoint4& p2 = vertexBuffer[index2].Position;
			const Elite::FVector2& uv0 = vertexBuffer[index0].UVCoord;
			const Elite::FVector2& uv1 = vertexBuffer[index1].UVCoord;
			const Elite::FVector2& uv2 = vertexBuffer[index2].UVCoord;

			const Elite::FVector4 edge0 = p1 - p0;
			const Elite::FVector4 edge1 = p2 - p0;
			const Elite::FVector2 diffX = Elite::FVector2(uv1.x - uv0.x, uv2.x - uv0.x);
			const Elite::FVector2 diffY = Elite::FVector2(uv1.y - uv0.y, uv2.y - uv0.y);
			float r = 1.f / Cross(diffX, diffY);

			Elite::FVector4 tangent = (edge0 * diffY.y - edge1 * diffY.x) * r;
			vertexBuffer[index0].Tangent += tangent.xyz;
			vertexBuffer[index1].Tangent += tangent.xyz;
			vertexBuffer[index2].Tangent += tangent.xyz;
		}
		for (auto& vertex : vertexBuffer)
			vertex.Tangent = Elite::GetNormalized(Elite::Reject(vertex.Tangent, vertex.Normal));

		return new Mesh(pDevice, vertexBuffer, indexBuffer, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, pMaterial);
	}

	// If it's not in a valid format
	std::cout << "The obj file is not written in a readable format." << std::endl;
	return new Mesh(pDevice, vertexBuffer, indexBuffer, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, pMaterial);
}


void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

std::vector<Mesh*> InitializeVehicleScene(Scene* scene, ID3D11Device* pDevice)
{
	// Set Up Camera
	scene->AddCamera(new Elite::ECamera(Elite::FPoint3{ 0.f, 0.f, 0.f }, Elite::FVector3{ 0.f, 0.f, 1.f }, true, 45.f, 0.1f, 100.f));

	// Set Up Background Color
	scene->SetBackgroundColor(Elite::RGBColor(.1f, .1f, .1f));
	
	// Set Up Vehicle Mesh
	const std::wstring assetFile = L"Resources/PosCol3D.fx";
	auto* pShadedMaterial = new ShadedMaterial(pDevice, assetFile);
	auto* pVehicleMesh = ParseOBJFile("Resources/vehicle.obj", pDevice, pShadedMaterial);
	pVehicleMesh->SetDiffuseTexture("Resources/vehicle_diffuse.png", pDevice);
	pVehicleMesh->SetNormalTexture("Resources/vehicle_normal.png", pDevice);
	pVehicleMesh->SetSpecularTexture("Resources/vehicle_specular.png", pDevice);
	pVehicleMesh->SetGlossinessTexture("Resources/vehicle_gloss.png", pDevice);
	pVehicleMesh->SetShininess(25.f);
	auto transformMatrix = Elite::FMatrix4::Identity();
	transformMatrix[3][2] = 50.f;
	pVehicleMesh->SetTransformMatrix(transformMatrix);
	scene->AddMesh(pVehicleMesh);
	
	// Set Up Fire Mesh
	auto* pTransparentMaterial = new TransparentMaterial(pDevice, assetFile);
	auto* pFireMesh = ParseOBJFile("Resources/fireFX.obj", pDevice, pTransparentMaterial);
	pFireMesh->SetDiffuseTexture("Resources/fireFX_diffuse.png", pDevice);
	pFireMesh->SetTransformMatrix(transformMatrix);
	scene->AddMesh(pFireMesh);
	
	// Set Up Light
	scene->SetAmbientLight({ 0.025f, 0.025f, 0.025f });
	scene->SetLightDirection({ 0.577f, -0.577f, 0.577f });
	scene->SetLightIntensity(7.f);
	
	// Set up the return value
	std::vector<Mesh*> returnVector;
	returnVector.push_back(pVehicleMesh);
	returnVector.push_back(pFireMesh);
	return returnVector;
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;
	SDL_Window* pWindow = SDL_CreateWindow(
		"DirectX - Miguel Pereira",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	auto pTimer{ std::make_unique<Elite::Timer>() };
	auto pRenderer{ std::make_unique<Elite::Renderer>(pWindow) };

	// Initialize Scenes
	std::vector<Scene*> scenes;
	int currentSceneIdx = 0;
	auto* pVehicleScene = new Scene();
	auto vehicleMeshVector = InitializeVehicleScene(pVehicleScene, pRenderer->GetDevice());
	scenes.push_back(pVehicleScene);

	//Print extra commands
	std::cout << "\n----------------------------------------------------------------------------\n";
	std::cout << "Commands:\n\n";
	std::cout << "  C -----> Toggle between cull modes\n";
	std::cout << "  E -----> Switch between render modes\n";
	std::cout << "  F -----> Switch between sampler filters (only in DirectX)\n";
	std::cout << "  R -----> Toggle the mesh's rotation on and off\n";
	std::cout << "  T -----> Hide/show the fireFX mesh\n";
	std::cout << "  V -----> Restart the current camera to its original position and rotation\n";
	std::cout << "  SPACE -> Switch between scenes (there's only one scene currently)\n\n\n";
	std::cout << "Extra Implementations:\n\n";
	std::cout << "  - Transparency in Software Mode\n";
	std::cout << "  - Editable single Directional Light through Scene class (not hardcoded values)\n";
	std::cout << "        -> This does not entail light structures or multiple light sources\n\n\n";
	std::cout << "Note: The shading in Software Mode doesn't behave as intended.\n";
	std::cout << "----------------------------------------------------------------------------\n\n";

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	RENDER_MODE renderMode = RENDER_MODE::DirectX;
	CULL_MODE cullMode = CULL_MODE::Back;
	bool fireFXVisible = true;
	bool meshRotation = true;
	const auto rotationSpeed = float(E_PI / 4.0); //45º per second
	SAMPLER_FILTER samplerFilter = SAMPLER_FILTER::Point;
	int cullModeIndex = 0;  // 0 = Point, 1 = Linear, 2 = Anisotropic

	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			// Get the current camera
			Elite::ECamera* pCamera = scenes[currentSceneIdx]->GetCurrentCamera();

			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				switch (e.key.keysym.sym)
				{
					// Toggle between cull modes with C
				case SDLK_c:
					if (cullMode == CULL_MODE::Back)
					{
						cullMode = CULL_MODE::Front;
						std::cout << "Cull Mode changed to FrontCulling\n";
					}
					else if(cullMode == CULL_MODE::Front)
					{
						cullMode = CULL_MODE::None;
						std::cout << "Cull Mode changed to None\n";
					}
					else
					{
						cullMode = CULL_MODE::Back;
						std::cout << "Cull Mode changed to BackCulling\n";
					}
					break;
					// Restart camera with V
				case SDLK_v:
					pCamera->Reset();
					std::cout << "Camera reset\n";
					break;
					// Toggle mesh rotation with R
				case SDLK_r:
					if (currentSceneIdx == 0)
					{
						meshRotation = !meshRotation;
						std::cout << "Mesh rotation ";
						if (meshRotation) std::cout << "on\n";
						else std::cout << "off\n";
					}
					break;
					// Change render mode with E
				case SDLK_e:
					if (renderMode == RENDER_MODE::DirectX)
					{
						renderMode = RENDER_MODE::Software;
						std::cout << "Render Mode changed to Software\n";
					}
					else
					{
						renderMode = RENDER_MODE::DirectX;
						std::cout << "Render Mode changed to DirectX\n";
					}
					break;
					// Change scene with SPACE
				case SDLK_SPACE:
					if (currentSceneIdx < scenes.size() - 1)
						currentSceneIdx++;
					else
						currentSceneIdx = 0;
					break;
					// Change pixel shading technique (samplerState) with F
				case SDLK_f:
					if (renderMode == RENDER_MODE::DirectX)
					{
						switch (samplerFilter)
						{
						case SAMPLER_FILTER::Point:
							samplerFilter = SAMPLER_FILTER::Linear;
							std::cout << "Sampler Filter set to Linear\n";
							break;
						case SAMPLER_FILTER::Linear:
							samplerFilter = SAMPLER_FILTER::Anisotropic;
							std::cout << "Sampler Filter set to Anisotropic\n";
							break;
						case SAMPLER_FILTER::Anisotropic:
							samplerFilter = SAMPLER_FILTER::Point;
							std::cout << "Sampler Filter set to Point\n";
							break;
						}
					}
					break;
					// Hide/show the FireFX mesh with T
				case SDLK_t:
					fireFXVisible = !fireFXVisible;
					if(fireFXVisible)
						std::cout << "Fire mesh visible\n";
					else
						std::cout << "Fire mesh hidden\n";
					break;
				default:
					break;
				}
				break;
			}
		}

		//--------- Update Current Scene ---------
		bool leftHandCoordSystem = true;
		if (renderMode == RENDER_MODE::Software)
			leftHandCoordSystem = !leftHandCoordSystem;
		scenes[currentSceneIdx]->Update(pTimer->GetElapsed(), leftHandCoordSystem);

		// Update the meshes rotation
		if (meshRotation)
		{
			const float frameRotation = rotationSpeed * pTimer->GetElapsed();
			auto rotationMatrix = Elite::FMatrix4{
				Elite::FVector4(cosf(frameRotation), 0.f, -sinf(frameRotation), 0.f),
				Elite::FVector4(0.f, 1.f, 0.f, 0.f),
				Elite::FVector4(sinf(frameRotation), 0.f, cosf(frameRotation), 0.f),
				Elite::FVector4(0.f, 0.f, 0.f, 1.f) };

			for (auto* mesh : vehicleMeshVector)
				mesh->SetTransformMatrix(mesh->GetTransformMatrix(true) * rotationMatrix);
		}

		//--------- RenderDirectX Current Scene ---------
		pRenderer->Render(scenes[currentSceneIdx], samplerFilter, renderMode, cullMode, fireFXVisible, vehicleMeshVector[1]);
		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			printTimer = 0.f;
			std::cout << "FPS: " << pTimer->GetFPS() << std::endl;
		}

	}
	pTimer->Stop();

	// Delete all the scenes
	for (auto* scene : scenes)
	{
		delete scene;
		scene = nullptr;
	}

	//Shutdown "framework"
	ShutDown(pWindow);
	return 0;
}