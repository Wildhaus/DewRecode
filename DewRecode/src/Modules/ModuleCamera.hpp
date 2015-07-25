#pragma once
#include <ElDorito/ModuleBase.hpp>

#include <ElDorito/Blam/BlamTypes.hpp>
#include <unordered_map>

namespace Modules
{
	class ModuleCamera : public ModuleBase
	{
	public:
		Command* VarCameraCrosshair;
		Command* VarCameraFov;
		Command* VarCameraHideHud;
		Command* VarCameraMode;
		Command* VarCameraSpeed;
		Command* VarCameraSave;
		Command* VarCameraLoad;

		PatchSet* CustomModePatches;

		// patches to disable all camera edits while in static mode
		PatchSet* StaticModePatches;

		Patch* HideHudPatch;
		Patch* CenteredCrosshairPatch;

		ModuleCamera();

		void UpdatePosition();

		//std::unordered_map<std::string, CameraType> CameraTypeStrings;
		//std::unordered_map<CameraType, size_t> CameraTypeFunctions;
	};
}