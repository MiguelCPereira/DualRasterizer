#include "pch.h"
#include "Scene.h"
#include "Mesh.h"
#include "ECamera.h"

Scene::Scene()
	: m_Meshes()
	, m_Cameras()
	, m_CurrentCameraIdx(0)
	, m_AmbientLight()
	, m_LightDirection()
	, m_LightIntensity()
	, m_BackgroundColor(0.f, 0.f, 0.f)
{
}

Scene::~Scene()
{
	ClearMeshes();
	ClearCameras();
}


void Scene::AddMesh(Mesh* newMesh)
{
	m_Meshes.push_back(newMesh);
}

void Scene::ClearMeshes()
{
	for (auto* mesh : m_Meshes)
	{
		delete mesh;
		mesh = nullptr;
	}

	m_Meshes.clear();
}

void Scene::AddCamera(Elite::ECamera* newCamera)
{
	m_Cameras.push_back(newCamera);
}

void Scene::ClearCameras()
{
	for (auto* camera : m_Cameras)
	{
		delete camera;
		camera = nullptr;
	}

	m_Cameras.clear();
}


void Scene::ChangeCamera(bool changeToNextCam)
{
	if (changeToNextCam)
	{
		if (m_CurrentCameraIdx + 1 >= int(m_Cameras.size()))
			m_CurrentCameraIdx = 0;
		else
			m_CurrentCameraIdx++;
	}
	else
	{
		if (m_CurrentCameraIdx <= 0)
			m_CurrentCameraIdx = int(m_Cameras.size()) - 1;
		else
			m_CurrentCameraIdx--;
	}
}

void Scene::Update(float elapsedTime, bool leftHandCoordSystem)
{
	m_Cameras[m_CurrentCameraIdx]->Update(elapsedTime, leftHandCoordSystem);
}
