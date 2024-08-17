#include <Windows.h>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <format>
class GameResourceServiceClientHandler {
private:
	uintptr_t gameResourceServiceClient;

public:
	GameResourceServiceClientHandler() {
		//Load the engine2.dll module
		HMODULE engineModule = GetModuleHandleA("engine2.dll");
		if (!engineModule) {
			std::cerr << "Failed to get handle to engine2.dll" << std::endl;
			gameResourceServiceClient = 0;
			return;
		}

		//Get the CreateInterface function
		auto CreateInterface = (uintptr_t(*)(const char*, void*))GetProcAddress(engineModule, "CreateInterface");
		if (!CreateInterface) {
			std::cerr << "Failed to get CreateInterface function" << std::endl;
			gameResourceServiceClient = 0;
			return;
		}

		//Call CreateInterface to initialize GameResourceServiceClient
		gameResourceServiceClient = CreateInterface("GameResourceServiceClientV001", nullptr);
	}

	uintptr_t getGameResourceServiceClient() const {
		return gameResourceServiceClient;
	}

	bool isValid() const {
		return gameResourceServiceClient != 0;
	}
};

//Class to manage access to CGameEntitySystem
class GameEntitySystemHandler {
private:
	void* gameEntitySystem;

public:
	GameEntitySystemHandler(const GameResourceServiceClientHandler& grsClientHandler) {
		//Check if GameResourceServiceClient is valid
		if (!grsClientHandler.isValid()) {
			std::cerr << "Invalid GameResourceServiceClient" << std::endl;
			gameEntitySystem = nullptr;
			return;
		}

		//Get the CGameEntitySystem pointer
		gameEntitySystem = *(void**)(grsClientHandler.getGameResourceServiceClient() + 0x58);
	}

	void* getGameEntitySystem() const {
		return gameEntitySystem;
	}

	bool isValid() const {
		return gameEntitySystem != nullptr;
	}
};

void HackThread(HMODULE instance) {


	AllocConsole();
	FILE* file;
	freopen_s(&file, "CONOUT$", "w", stdout);


	while (!GetAsyncKeyState(VK_END)) {
		if (GetAsyncKeyState(VK_INSERT) & 1) {


			GameResourceServiceClientHandler grsClientHandler;
			if (!grsClientHandler.isValid()) {
				std::cerr << "Failed to initialize GameResourceServiceClient" << std::endl;
			}

			GameEntitySystemHandler gameEntitySystemHandler(grsClientHandler);
			if (!gameEntitySystemHandler.isValid()) {
				std::cerr << "Failed to get CGameEntitySystem" << std::endl;
			}

			std::cout << "CGameEntitySystem successfully obtained: "
				<< gameEntitySystemHandler.getGameEntitySystem() << std::endl;

			void* CGameEntitySystem = gameEntitySystemHandler.getGameEntitySystem();

			/*auto GameResourceServiceClient =
				((uintptr_t(*)(const char*, void*))GetProcAddress(GetModuleHandleA("engine2.dll"),
					"CreateInterface"))("GameResourceServiceClientV001", nullptr);
			void* CGameEntitySystem = *(void**)(GameResourceServiceClient + 0x58);*/

			//Function to get an entity by its index
			const auto _GetEntityByIndex = [CGameEntitySystem](std::size_t index) -> void*
				{
					constexpr auto ENTITY_SYSTEM_LIST_SIZE = 512;
					const auto entitysystem_lists = (const void**)((const std::uint8_t*)CGameEntitySystem + 0x10);
					const auto list = entitysystem_lists[index / ENTITY_SYSTEM_LIST_SIZE];
					if (list)
					{
						std::cout << CGameEntitySystem << std::endl;
						const auto entry_index = index % ENTITY_SYSTEM_LIST_SIZE;
						constexpr auto sizeof_CEntityIdentity = 0x78;
						const auto identity = (const std::uint8_t*)list + entry_index * sizeof_CEntityIdentity;
						if (identity)
						{
							return *(void**)identity;
						}
					}
					return nullptr;
				};

			//Loop through entities and process them
			for (std::size_t index = 1; index <= 64; ++index)
			{
				void* Entity = _GetEntityByIndex(index);
				if (Entity)
				{
					constexpr auto offset_m_iszPlayerName = 0x628;
					constexpr auto offset_m_hAssignedHero = 0x7e4;
					constexpr int offset_m_iTaggedAsVisibleByTeam = 0xc2c;
					constexpr auto offset_m_hInventory = 0xf30;
					constexpr auto offset_m_hAbilities = 0xae0;
					constexpr auto offset_m_fCooldown = 0x580;
					constexpr auto high_17_bits = 0b11111111111111111000000000000000;
					constexpr auto low_15_bits = 0b00000000000000000111111111111111;

					struct CHandle
					{
						int value;
						int serial() const noexcept
						{
							return value & high_17_bits;
						}
						int index() const noexcept
						{
							return value & low_15_bits;
						}
						bool is_valid() const noexcept
						{
							return value != -1;
						}
					};

					void* EntityHero = _GetEntityByIndex(((*(CHandle*)((uintptr_t)Entity + offset_m_hAssignedHero)).index()));
					{
						for (int i = 0; i < 35; i++)
						{
							void* EntityHeroAbility = _GetEntityByIndex(((*(CHandle*)((uintptr_t)EntityHero + offset_m_hAbilities + i * 0x4)).index()));
							if (EntityHeroAbility) {
								std::cout << "EntityHeroAbility: " << EntityHeroAbility << std::endl;
								std::cout << "EntityHeroAbility Cooldown: " << *(float*)((uintptr_t)EntityHeroAbility + offset_m_fCooldown) << std::endl;
							}
						}

					}
					int EntityHeroTaggedAsVisibleByTeam = *(int*)((uintptr_t)EntityHero + offset_m_iTaggedAsVisibleByTeam);
					std::cout << "Entity: " << Entity << std::endl;
					std::cout << "Entity Name : " << (char*)((uintptr_t)Entity + offset_m_iszPlayerName) << std::endl;
					std::cout << "Entity Assigned Hero : " << ((*(CHandle*)((uintptr_t)Entity + offset_m_hAssignedHero)).index()) << std::endl;
					std::cout << "EntityHero: " << EntityHero << std::endl;
					if (EntityHeroTaggedAsVisibleByTeam && EntityHeroTaggedAsVisibleByTeam != 4) {
						if (EntityHeroTaggedAsVisibleByTeam == 30) {
							std::cout << "is visible: " << EntityHeroTaggedAsVisibleByTeam << std::endl;
						}
						else {
							std::cout << "not visible : " << EntityHeroTaggedAsVisibleByTeam << std::endl;
						}
					}

					OutputDebugStringA(std::format("player {}({}) controls hero indexed {}\n",
						(char*)((uintptr_t)Entity + offset_m_iszPlayerName),
						Entity, ((*(CHandle*)((uintptr_t)Entity + offset_m_hAssignedHero)).index())).data());
				}
			}
		}
		Sleep(200);
	}
	if (file)
		fclose(file);

	FreeConsole();
	FreeLibraryAndExitThread((HMODULE)HackThread, 0);
}

BOOL WINAPI DllMain(HMODULE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);

		const auto thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(HackThread), instance, 0, nullptr);

		if (thread) {
			CloseHandle(thread);
		}
	}
	return TRUE;
}