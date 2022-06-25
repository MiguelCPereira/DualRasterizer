// Copyright 2021 Elite Engine 2.0
// Authors: Thomas Goussaert
/*=============================================================================*/
// ECamera.h: Base ECamera Implementation with movement
/*=============================================================================*/

#pragma once
#include "EMath.h"

namespace Elite
{
	class ECamera
	{
	public:

		ECamera(const FPoint3& position = { 0.f, 0.f, -10.f }, const FVector3& viewForward = { 0.f, 0.f, 1.f }, bool leftHandCoordSystem = true, float fovAngle = 60.f, float nearPlane = 0.1f, float farPlane = 100.f);
		~ECamera() = default;

		ECamera(const ECamera&) = delete;
		ECamera(ECamera&&) noexcept = delete;
		ECamera& operator=(const ECamera&) = delete;
		ECamera& operator=(ECamera&&) noexcept = delete;

		void Update(float elapsedSec, bool leftHandCoordSystem);

		void Reset();

		const FMatrix4& GetWorldMatrix() const { return m_WorldMatrix; }
		const FMatrix4& GetViewMatrix() const { return m_ViewMatrix; }

		const float GetFov() const { return m_Fov; }
		const float GetFar() const { return m_FarPlane; }
		const float GetNear() const { return m_NearPlane; }
		
		FPoint3 GetPosition() const { return m_Position; }

	private:
		void CalculateLookAt(bool leftHandCoordSystem);

		float m_Fov{};

		const float m_NearPlane;
		const float m_FarPlane;

		const float m_KeyboardMoveSensitivity{ 1.f };
		const float m_KeyboardMoveMultiplier{ 10.f };
		const float m_MouseRotationSensitivity{ .1f };
		const float m_MouseMoveSensitivity{ 2.f };

		FPoint2 m_AbsoluteRotation{}; //Pitch(x) & Yaw(y) only
		FPoint3 m_RelativeTranslation{};

		const FPoint3 m_OriginalPosition{};
		FPoint3 m_Position{};
		const FVector3 m_ViewForward{};

		FMatrix4 m_WorldMatrix{};
		FMatrix4 m_ViewMatrix{};
	};
}