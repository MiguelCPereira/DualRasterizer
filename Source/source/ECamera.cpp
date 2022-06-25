#include "pch.h"
#include "ECamera.h"
#include <SDL.h>

namespace Elite
{
	ECamera::ECamera(const FPoint3& position, const FVector3& viewForward, bool leftHandCoordSystem, float fovAngle, float nearPlane, float farPlane)
		: m_Fov(tanf((fovAngle* float(E_TO_RADIANS)) / 2.f))
		, m_NearPlane{ nearPlane }
		, m_FarPlane{ farPlane }
		, m_OriginalPosition{ position }
		, m_Position{ position }
		, m_ViewForward{ GetNormalized(viewForward) }
	{
		//Calculate initial matrices based on given parameters (position & target)
		CalculateLookAt(leftHandCoordSystem);
	}

	void ECamera::Reset()
	{
		m_Position = m_OriginalPosition;
		m_AbsoluteRotation.x = m_ViewForward.x;
		m_AbsoluteRotation.y = m_ViewForward.y;
	}

	void ECamera::Update(float elapsedSec, bool leftHandCoordSystem)
	{
		//Capture Input (absolute) Rotation & (relative) Movement
		//*************
		//Keyboard Input
		const uint8_t* pKeyboardState = SDL_GetKeyboardState(0);
		float keyboardSpeed = pKeyboardState[SDL_SCANCODE_LSHIFT] ? m_KeyboardMoveSensitivity * m_KeyboardMoveMultiplier : m_KeyboardMoveSensitivity;
		m_RelativeTranslation.x = (pKeyboardState[SDL_SCANCODE_D] - pKeyboardState[SDL_SCANCODE_A]) * keyboardSpeed * elapsedSec;
		m_RelativeTranslation.y = 0;
		m_RelativeTranslation.z = -(pKeyboardState[SDL_SCANCODE_S] - pKeyboardState[SDL_SCANCODE_W]) * keyboardSpeed * elapsedSec;

		//Mouse Input
		int x, y = 0;
		uint32_t mouseState = SDL_GetRelativeMouseState(&x, &y);
		if (mouseState == SDL_BUTTON_LMASK)
		{
			m_RelativeTranslation.z -= y * m_MouseMoveSensitivity * elapsedSec;
			m_AbsoluteRotation.y += x * m_MouseRotationSensitivity;
		}
		else if (mouseState == SDL_BUTTON_RMASK)
		{
			m_AbsoluteRotation.x += y * m_MouseRotationSensitivity;
			m_AbsoluteRotation.y += x * m_MouseRotationSensitivity;
		}
		else if (mouseState == (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))
		{
			m_RelativeTranslation.y -= y * m_MouseMoveSensitivity * elapsedSec;
		}

		//Update LookAt (view & world matrices)
		//*************
		CalculateLookAt(leftHandCoordSystem);
	}

	void ECamera::CalculateLookAt(bool leftHandCoordSystem)
	{
		//FORWARD (zAxis) with YAW applied
		FMatrix3 yawRotation = MakeRotationY(m_AbsoluteRotation.y * float(E_TO_RADIANS));
		FVector3 zAxis = yawRotation * m_ViewForward;

		//Calculate RIGHT (xAxis) based on transformed FORWARD
		FVector3 xAxis = GetNormalized(Cross(FVector3{ 0.f,1.f,0.f }, zAxis));

		//FORWARD with PITCH applied (based on xAxis)
		FMatrix3 pitchRotation = MakeRotation(m_AbsoluteRotation.x * float(E_TO_RADIANS), xAxis);
		zAxis = pitchRotation * zAxis;
		
		//Calculate UP (yAxis)
		FVector3 yAxis = Cross(zAxis, xAxis);
		
		//Translate based on transformed axis
		m_Position += m_RelativeTranslation.x * xAxis;
		m_Position += m_RelativeTranslation.y * yAxis;
		m_Position += m_RelativeTranslation.z * zAxis;
		
		auto worldMatPosVec = FVector4{ m_Position.x,m_Position.y,m_Position.z,1.f };
		
		//Flip axis and matrix pos to adjust for coordinate system
		if (!leftHandCoordSystem)
		{
			xAxis = -xAxis;
			yAxis = -yAxis;
			worldMatPosVec.x = -worldMatPosVec.x;
			worldMatPosVec.y = -worldMatPosVec.y;
			worldMatPosVec.z = -worldMatPosVec.z;
		}

		//Construct World Matrix
		m_WorldMatrix =
		{
			FVector4{xAxis},
			FVector4{yAxis},
			FVector4{zAxis},
			worldMatPosVec
		};

		//Construct View Matrix
		m_ViewMatrix = Inverse(m_WorldMatrix);
	}
}