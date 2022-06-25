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
#include "DiffuseOnlyMaterial.h"

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
			uvs.emplace_back(Elite::FVector2(u, 1.0 - v));
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
		for (uint32_t i = 0; i < vertexBuffer.size(); i++)
			indexBuffer.push_back(i);

		// And add the tangents to the vertexBuffer - inspired in https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
		for (uint32_t i = 0; i < indexBuffer.size(); i += 3)
		{
			int index0 = indexBuffer[i];
			int index1 = indexBuffer[i + 1];
			int index2 = indexBuffer[i + 2];

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

void InitializeCTriangleScene(Scene* scene, ID3D11Device* pDevice)
{
	// Set Up Camera
	scene->AddCamera(new Elite::ECamera());

	// Set Up Ambient Color
	scene->SetAmbientColor(Elite::RGBColor(0.f, 0.f, 0.3f));
	
	// Set Up Mesh
	//// Set the vertices
	const std::vector<VS_INPUT> vertices
	{
		{Elite::FPoint3(0.f, 3.f, 2.f), Elite::RGBColor(1.f, 0.f, 0.f)},
		{Elite::FPoint3(3.f, -3.f, 2.f), Elite::RGBColor(0.f, 0.f, 1.f)},
		{Elite::FPoint3(-3.f, -3.f, 2.f), Elite::RGBColor(0.f, 1.f, 0.f)}
	};
	//// Then the indices
	const std::vector<uint32_t> indices{ 0,1,2 };
	//// Then the material
	const std::wstring assetFile = L"Resources/PosCol3D.fx";
	auto* pMaterial = new DiffuseOnlyMaterial(pDevice, assetFile);
	//// And lastly, initialize the mesh
	auto* pMesh = new Mesh(pDevice, vertices, indices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, pMaterial);
	scene->AddMesh(pMesh);
}

void InitializeQuadScene(Scene* scene, ID3D11Device* pDevice)
{
	// Set Up Camera
	scene->AddCamera(new Elite::ECamera());

	// Set Up Ambient Color
	scene->SetAmbientColor(Elite::RGBColor(0.f, 0.f, 0.3f));

	// Set Up Mesh
	//// Set the vertices
	const std::vector<VS_INPUT> vertices
	{
		{Elite::FPoint3{ -3, 3, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 0.f, 0.f }},
		{Elite::FPoint3{ 0, 3, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 0.5f, 0.f }},
		{Elite::FPoint3{ 3, 3, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 1.f, 0.f }},
		{Elite::FPoint3{  -3, 0, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 0.f, 0.5f }},
		{Elite::FPoint3{ 0, 0, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 0.5f, 0.5f }},
		{Elite::FPoint3{ 3, 0, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 1.f, 0.5f }},
		{Elite::FPoint3{ -3, -3, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 0.f, 1.f }},
		{Elite::FPoint3{ 0, -3, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 0.5f, 1.f }},
		{Elite::FPoint3{ 3, -3, 2}, Elite::RGBColor{ 1.f, 1.f, 1.f }, Elite::FVector2{ 1.f, 1.f }}
	};
	//// Then the indices (could change the topology for a trianglestrip and shortned the indices vector, but for now this works)
	const std::vector<uint32_t> indices{ 0,1,3, 3,1,4, 1,2,4, 4,2,5, 2,5,3, 3,4,6, 6,4,7, 7,4,5, 7,5,8 };
	//// Then the material
	const std::wstring assetFile = L"Resources/PosCol3D.fx";
	auto* pMaterial = new DiffuseOnlyMaterial(pDevice, assetFile);
	//// The diffuse texture path
	const char* diffusePath = "Resources/uv_grid_2.png";
	//// And lastly, initialize the mesh
	auto* pMesh = new Mesh(pDevice, vertices, indices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, pMaterial, Elite::FMatrix4::Identity(), diffusePath);
	scene->AddMesh(pMesh);
}

std::vector<Mesh*> InitializeVehicleScene(Scene* scene, ID3D11Device* pDevice)
{
	// Set Up Camera
	scene->AddCamera(new Elite::ECamera(Elite::FPoint3{ 0.f, 0.f, -64.f }, Elite::FVector3{ 0.f, 0.f, 1.f }, 45.f));

	// Set Up Ambient Color
	scene->SetAmbientColor(Elite::RGBColor(0.f, 0.f, 0.3f));

	// Set Up Vehicle Mesh
	const std::wstring assetFile = L"Resources/PosCol3D.fx";
	auto* pShadedMaterial = new ShadedMaterial(pDevice, assetFile);
	auto* pVehicleMesh = ParseOBJFile("Resources/vehicle.obj", pDevice, pShadedMaterial);
	pVehicleMesh->SetDiffuseTexture("Resources/vehicle_diffuse.png", pDevice);
	pVehicleMesh->SetNormalTexture("Resources/vehicle_normal.png", pDevice);
	pVehicleMesh->SetSpecularTexture("Resources/vehicle_specular.png", pDevice);
	pVehicleMesh->SetGlossinessTexture("Resources/vehicle_gloss.png", pDevice);
	pVehicleMesh->SetShininess(25.f);
	scene->AddMesh(pVehicleMesh);

	// Set Up Fire Mesh
	auto* pDifOnlyMaterial = new DiffuseOnlyMaterial(pDevice, assetFile);
	auto* pFireMesh = ParseOBJFile("Resources/fireFX.obj", pDevice, pDifOnlyMaterial);
	pFireMesh->SetDiffuseTexture("Resources/fireFX_diffuse.png", pDevice);
	scene->AddMesh(pFireMesh);

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
	auto meshVector = InitializeVehicleScene(pVehicleScene, pRenderer->GetDevice());
	scenes.push_back(pVehicleScene);
	auto* pQuadScene = new Scene();
	InitializeQuadScene(pQuadScene, pRenderer->GetDevice());
	scenes.push_back(pQuadScene);
	auto* pColTriangleScene = new Scene();
	InitializeCTriangleScene(pColTriangleScene, pRenderer->GetDevice());
	scenes.push_back(pColTriangleScene);

	//Print extra commands
	std::cout << "\n----------------------------------------------------------------------------\n";
	std::cout << "Extra Commands:\n\n";
	std::cout << "  SPACE -> Switch between scenes\n";
	std::cout << "  F -----> Switch between pixel shading techniques\n";
	std::cout << "  C -----> Restart the current camera to its original position and rotation\n";
	std::cout << "  R -----> Toggle the mesh's rotation on and off\n";
	std::cout << "----------------------------------------------------------------------------\n\n";
	
	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	bool meshRotation = false;
	const float rotationSpeed = E_PI / 6.f;
	int techniqueIndex = 0;  // 0 = Point, 1 = Linear, 2 = Anisotropic

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
				// Restart camera with C
				case SDLK_c:
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
				// Change scene with SPACE
				case SDLK_SPACE:
					if (currentSceneIdx < scenes.size() - 1)
						currentSceneIdx++;
					else
						currentSceneIdx = 0;
					break;
				// Change pixel shading technique (samplerState) with F
				case SDLK_f:

					if (techniqueIndex < 2)
						techniqueIndex++;
					else
						techniqueIndex = 0;

					switch (techniqueIndex)
					{
					case 0:
						std::cout << "Pixel Shading Technique set to Point\n";
						break;
					case 1:
						std::cout << "Pixel Shading Technique set to Linear\n";
						break;
					case 2:
						std::cout << "Pixel Shading Technique set to Anisotropic\n";
						break;
					default:
						std::cout << "Pixel Shading Technique set to Point\n";
						break;
					}
					
					break;
				default:
					break;
				}
				break;
			}
		}

		//--------- Update Current Scene ---------
		scenes[currentSceneIdx]->Update(pTimer->GetElapsed());

		// Update the meshes rotation
		if (meshRotation)
		{
			const float frameRotation = rotationSpeed * pTimer->GetElapsed();
			auto rotationMatrix = Elite::FMatrix4{
				Elite::FVector4(cosf(frameRotation), 0.f, -sinf(frameRotation), 0.f),
				Elite::FVector4(0.f, 1.f, 0.f, 0.f),
				Elite::FVector4(sinf(frameRotation), 0.f, cosf(frameRotation), 0.f),
				Elite::FVector4(0.f, 0.f, 0.f, 1.f) };

			for(auto* mesh : meshVector)
				mesh->SetTransformMatrix(rotationMatrix * mesh->GetTransformMatrix());
		}
		
		//--------- Render Current Scene ---------
		pRenderer->Render(scenes[currentSceneIdx], techniqueIndex);

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