#include "Core.hpp"
#include "../../ElDorito.hpp"
namespace
{
	__declspec(naked) void FovHook()
	{
		__asm
		{
			// Override the FOV that the memmove before this sets
			mov eax, ds:[0x189D42C]
			mov ds : [0x2301D98], eax
			mov ecx, [edi + 0x18]
			push 0x50CA08
			ret
		}
	}

	int __cdecl DualWieldHook(unsigned short objectIndex)
	{
		auto& dorito = ElDorito::Instance();

		Pointer &objectHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];
		uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + objectIndex * 0x10;
		uint32_t objectDataAddress = *(uint32_t*)objectAddress;
		uint32_t index = *(uint32_t*)objectDataAddress;

		typedef char* (*GetTagAddressPtr)(int groupTag, uint32_t index);
		auto GetTagAddress = reinterpret_cast<GetTagAddressPtr>(0x503370);

		char* tagAddr = GetTagAddress(0x70616577, index);

		return ((*(uint32_t*)(tagAddr + 0x1D4) >> 22) & 1) == 1;
	}

	void EquipmentHookImpl(unsigned short playerIndex, unsigned short equipmentIndex)
	{
		BYTE unkData[0x40];
		BYTE b69Data[0x48];

		auto& dorito = ElDorito::Instance();

		Pointer& playerHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::Players::TLSOffset)[0];
		uint32_t playerStructAddress = playerHeaderPtr(0x44).Read<uint32_t>() + playerIndex * GameGlobals::Players::PlayerEntryLength;
		uint32_t playerObjectDatum = *(uint32_t*)(playerStructAddress + 0x30);

		Pointer &objectHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];
		uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + equipmentIndex * 0x10;
		uint32_t objectDataAddress = *(uint32_t*)objectAddress;
		uint32_t index = *(uint32_t*)objectDataAddress;

		memset(b69Data, 0, 0x48);
		*(uint32_t*)(b69Data) = 0x3D; // object type?
		*(uint32_t*)(b69Data + 4) = equipmentIndex;

		// add equipment to the player, also removes the object from gameworld
		typedef char(__cdecl* Objects_AttachPtr)(int a1, void* a2);
		auto Objects_Attach = reinterpret_cast<Objects_AttachPtr>(0xB69C50);
		Objects_Attach(playerObjectDatum, b69Data); // 82182C48

		// prints text to the HUD, taken from HO's code
		{
			typedef int(__thiscall* sub_589680Ptr)(void* thisPtr, int a2);
			auto sub_589680 = reinterpret_cast<sub_589680Ptr>(0x589680);
			sub_589680(&unkData, playerIndex);

			typedef BOOL(__thiscall* sub_589770Ptr)(void* thisPtr);
			auto sub_589770 = reinterpret_cast<sub_589770Ptr>(0x589770);
			while ((unsigned __int8)sub_589770(&unkData))
			{
				typedef int(__thiscall* sub_589760Ptr)(void* thisPtr);
				auto sub_589760 = reinterpret_cast<sub_589760Ptr>(0x589760);
				int v9 = sub_589760(&unkData);

				typedef int(__cdecl* sub_A95850Ptr)(unsigned int a1, short a2);
				auto sub_A95850 = reinterpret_cast<sub_A95850Ptr>(0xA95850);
				sub_A95850(v9, index);
			}
		}

		// unsure what these do, taken from HO code
		{
			typedef int(__cdecl *sub_B887B0Ptr)(unsigned short a1, unsigned short a2);
			auto sub_B887B0 = reinterpret_cast<sub_B887B0Ptr>(0xB887B0);
			sub_B887B0(playerIndex, index); // sub_82437A08

			typedef void(_cdecl *sub_4B31C0Ptr)(unsigned short a1, unsigned short a2);
			auto sub_4B31C0 = reinterpret_cast<sub_4B31C0Ptr>(0x4B31C0);
			sub_4B31C0(playerObjectDatum, index); // sub_8249A1A0
		}

		// called by powerup pickup func, deletes the item but also crashes the game when used with equipment
		// not needed since Objects_Attach removes it from the game world
		/*
		typedef int(__cdecl *Objects_DeletePtr)(int objectIndex);
		auto Objects_Delete = reinterpret_cast<Objects_DeletePtr>(0xB57090);
		Objects_Delete(equipmentIndex);*/

	}

	__declspec(naked) void EquipmentHook()
	{
		__asm
		{
			mov edx, 0x531D70
			call edx
			mov esi, [ebp+8]
			push edi
			push esi
			test al, al
			jz DoEquipmentHook
			mov edx, 0x4B4A20
			call edx
			add esp, 8
			push 0x5397D8
			ret
		DoEquipmentHook:
			call EquipmentHookImpl
			add esp, 8
			push 0x5397D8
			ret
		}
	}
}
namespace Modules
{
	PatchModuleCore::PatchModuleCore() : ModuleBase("Patches.Core")
	{
		AddModulePatches(
		{
			// Enable tag edits
			Patch("TagEdits1", 0x501A5B, { 0xEB }),
			Patch("TagEdits2", 0x502874, 0x90, 2),
			Patch("TagEdits3", 0x5030AA, 0x90, 2),

			// No --account args patch
			Patch("AccountArgs1", 0x83731A, { 0xEB, 0x0E }),
			Patch("AccountArgs2", 0x8373AD, { 0xEB, 0x03 }),

			// Prevent game variant weapons from being overridden
			Patch("VariantOverride1", 0x5A315F, { 0xEB }),
			Patch("VariantOverride2", 0x5A31A4, { 0xEB }),

			// Level load patch (?)
			Patch("LevelLoad", 0x6D26DF, 0x90, 5),

			/* TODO: find out if one of these patches is breaking game prefs
			// Remove preferences.dat hash check
			Patch("PrefsDatHashCheck", 0x50C99A, 0x90, 6),

			// Patch to allow spawning AI through effects
			Patch("SpawnAIWithEffects", 0x1433321, 0x90, 2),

			// Prevent FOV from being overridden when the game loads
			Patch("FOVOverride1", 0x65FA79, 0x90, 10),
			Patch("FOVOverride2", 0x65FA86, 0x90, 5)*/
		},
		{
			/*Hook("FOVHook", 0x50CA02, FovHook, HookType::Jmp)*/

			Hook("DualWieldHook", 0xB61550, DualWieldHook, HookType::Jmp),
			Hook("EquipmentHook", 0x539888, EquipmentHook, HookType::JmpIfNotEqual)
		});
	}
}
