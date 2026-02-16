#pragma once

#include <mq/Plugin.h>
#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>
#include <map>
#include <unordered_map>
#include <deque>
#include <string>
#include <chrono>

// Shared data structures and globals used across MQ2Mono compilation units

struct monoAppDomainInfo
{
	std::string m_appDomainName;
	// app domain we have created for e3
	MonoDomain* m_appDomain = nullptr;
	// core.dll information so we can bind to it
	MonoAssembly* m_csharpAssembly = nullptr;
	MonoImage* m_coreAssemblyImage = nullptr;
	MonoClass* m_classInfo = nullptr;
	MonoObject* m_classInstance = nullptr;
	// methods that we call in C# if they are available
	MonoMethod* m_OnPulseMethod = nullptr;
	MonoMethod* m_OnSpawnAdd = nullptr;
	MonoMethod* m_OnZoned = nullptr;
	MonoMethod* m_OnWriteChatColor = nullptr;
	MonoMethod* m_OnIncomingChat = nullptr;
	MonoMethod* m_OnInit = nullptr;
	MonoMethod* m_OnUpdateImGui = nullptr;
	MonoMethod* m_OnStop = nullptr;
	MonoMethod* m_OnCommand = nullptr;
	MonoMethod* m_OnSetSpawns = nullptr;
	MonoMethod* m_OnSetSpawnsViaCallback = nullptr;
	MonoMethod* m_OnQuery = nullptr;

	std::map<std::string, bool> m_IMGUI_OpenWindows;
	std::map<std::string, bool> m_IMGUI_CheckboxValues;
	std::map<std::string, std::string> m_IMGUI_InputTextValues;
	std::map<std::string, std::array<float, 4>> m_IMGUI_InputColorValues;
	std::map<std::string, int> m_IMGUI_InputIntValues;
	std::map<std::string, bool> m_IMGUI_RadioButtonValues;
	std::string m_CurrentWindow;
	bool m_IMGUI_Open = true;
	int m_delayTime = 0; // amount of time in milliseconds that was set by C#
	std::chrono::steady_clock::time_point m_delayTimer = std::chrono::steady_clock::now(); // the time this was issued + m_delayTime
	std::deque<std::string> m_CommandList;

	bool AddCommand(std::string commandName)
	{
		if (!IsCommand(commandName.c_str()))
		{
			m_CommandList.push_back(commandName);

			mq::AddCommand(commandName.c_str(), [this, commandName](PlayerClient*, const char* args) -> void
			{
				if (this->m_appDomain)
				{
					if (this->m_OnCommand)
					{
						std::string line = args;
						line = commandName + " " + line;
						mono_domain_set(this->m_appDomain, false);
						MonoString* monoLine = mono_string_new(this->m_appDomain, line.c_str());
						void* params[1] =
						{
							monoLine
						};
						mono_runtime_invoke(this->m_OnCommand, this->m_classInstance, params, nullptr);
					}
				}
			});
			return true;
		}
		return false;
	}

	void RemoveCommand(std::string command)
	{
		int count = 0;
		int processCount = static_cast<int>(this->m_CommandList.size());
		while (count < processCount)
		{
			count++;
			std::string currentKey = this->m_CommandList.front();
			m_CommandList.pop_front();
			if (!ci_equals(currentKey, command))
			{
				m_CommandList.push_back(currentKey);
			}
			else
			{
				mq::RemoveCommand(currentKey.c_str());
			}
		}
	}

	void ClearCommands()
	{
		while (this->m_CommandList.size() > 0)
		{
			mq::RemoveCommand(m_CommandList.front().c_str());
			m_CommandList.pop_front();
		}
	}
};

// Globals defined in MQ2Mono.cpp, used by ImGui wrappers
extern std::map<std::string, monoAppDomainInfo> monoAppDomains;
extern std::map<MonoDomain*, std::string> monoAppDomainPtrToString;
