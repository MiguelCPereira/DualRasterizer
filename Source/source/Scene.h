#pragma once
#include <vector>

#include "ERGBColor.h"

class Mesh;

namespace Elite
{
	class ECamera;
}

class Scene
{
public:
	Scene();
	~Scene();

	Scene(const Scene& other) = delete;
	Scene(Scene&& other) noexcept = delete;
	Scene& operator=(const Scene& other) = delete;
	Scene& operator=(Scene&& other) noexcept = delete;


	void AddMesh(Mesh* newMesh);
	void ClearMeshes();
	std::vector<Mesh*> GetMeshes() const { return m_Meshes; }


	void AddCamera(Elite::ECamera* newCamera);
	void ClearCameras();
	void ChangeCamera(bool changeToNextCam = true);
	Elite::ECamera* GetCurrentCamera() const { if (m_Cameras.empty() == false) { return m_Cameras[m_CurrentCameraIdx]; } return nullptr; }

	Elite::FVector3 GetLightDirection() const { return m_LightDirection; }
	float GetLightIntensity() const { return m_LightIntensity; }
	Elite::FVector3 GetAmbientLight() const { return m_AmbientLight; }
	Elite::RGBColor GetBackgroundColor() const { return m_BackgroundColor; }
	

	void SetLightDirection(const Elite::FVector3& direction) { m_LightDirection = direction; }
	void SetLightIntensity(float intensity) { m_LightIntensity = intensity; }
	void SetAmbientLight(const Elite::FVector3& ambient) { m_AmbientLight = ambient; }
	void SetBackgroundColor(const Elite::RGBColor& backgroundColor) { m_BackgroundColor = backgroundColor; }

	void Update(float elapsedTime, bool leftHandCoordSystem);


private:
	std::vector<Mesh*> m_Meshes;
	std::vector <Elite::ECamera*> m_Cameras;
	int m_CurrentCameraIdx;
	Elite::FVector3 m_LightDirection;
	float m_LightIntensity;
	Elite::FVector3 m_AmbientLight;
	Elite::RGBColor m_BackgroundColor;
};