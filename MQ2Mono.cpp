// MQ2Mono.cpp : Defines the entry point for the DLL application.
// Created by Rekka of Lazarus

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

#include <mq/Plugin.h>
//mono includes so all the mono_ methods and objects compile
#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>
#include <map>
#include <unordered_map>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mq/imgui/Widgets.h>
#include "MQ2MonoImGui.h"
#include "MQ2MonoShared.h"
PreSetup("MQ2Mono");

// ImGui wrappers moved to MQ2MonoImGui.h / MQ2MonoImGui.cpp
PLUGIN_VERSION(0.35);

/**
 * Avoid Globals if at all possible, since they persist throughout your program.
 * But if you must have them, here is the place to put them.
 */
 bool ShowMQ2MonoWindow = true;
 std::string monoDir;
 std::string rootDir;
 std::string monoRuntimeDir;
 bool initialized = false;
 bool previousCommand = false; //a command was issued on this call.

 //define methods exposde to the plugin to be executed
 void mono_Echo(MonoString* string);
 MonoString* mono_ParseTLO(MonoString* string);
 void mono_DoCommand(MonoString* string);
 void mono_DoCommandDelayed(MonoString* string);
 void mono_Delay(int milliseconds);
 bool mono_AddCommand(MonoString* string);
 void mono_ClearCommands();
 void mono_RemoveCommand(MonoString* text);
 bool InitAppDomain(std::string appDomainName);
 bool UnloadAppDomain(std::string appDomainName, bool updateCollections);
 void UnloadAllAppDomains();
 void mono_ExecuteCommand(unsigned int commandID, bool holdKey);
 void mono_ExecuteCommandByName(MonoString* name, bool holdKey);
 void mono_DoCommandDelayed(MonoString* text);
 void mono_LookAt(FLOAT X, FLOAT Y, FLOAT Z);
 //spell data methods
 int mono_GetSpellDataEffectCount(MonoString* query);
 MonoString* mono_GetSpellDataEffect(MonoString* query, int line);

 //yes there are two of them, yes there is a reason due to compatabilty reasons of e3n
 void mono_GetSpawns();
 void mono_GetSpawns2();

 //not sure if realy needed anymore but eh, its there
 bool mono_GetRunNextCommand();

 //used to get the currently focused window element
 MonoString* mono_GetFocusedWindowName();
 //used to get the currently focused window element
 MonoString* mono_GetHoverWindowName();

 MonoString* mono_GetMQ2MonoVersion();
 std::string version = "0.35";
 
 /// <summary>
 /// Main data structure that has information on each individual app domain that we create and informatoin
 /// we need to keep track of.
 /// </summary>
#include "MQ2MonoShared.h"

std::map<std::string, monoAppDomainInfo> monoAppDomains;
std::map<MonoDomain*, std::string> monoAppDomainPtrToString;
 //used to keep a revolving list of who is valid to process. 
 std::deque<std::string> appDomainProcessQueue;
 uint32_t bmUpdateMonoOnPulse = 0;

//to be replaced later with collections of multilpe domains, etc.
//domains where the code is run
//root domain to contain app domains
MonoDomain* _rootDomain;

int _milisecondsToProcess = 20;


//simple timer to limit puse calls to the .net onpulse.
static std::chrono::steady_clock::time_point PulseTimer = std::chrono::steady_clock::now() + std::chrono::seconds(5);

/**
 * @fn InitializePlugin
 *
 * This is called once on plugin initialization and can be considered the startup
 * routine for the plugin.
 */

void InitMono()
{

	if (initialized) return;
	
	//setup mono macro directory + runtime directory
	rootDir = std::filesystem::path(gPathMQRoot).u8string();
	monoDir = rootDir + "\\Mono";
	
	bool sgenExists = std::filesystem::exists(rootDir +"\\mono-2.0-sgen.dll");
	if (!sgenExists)
	{
		WriteChatf("\arMQ2Mono\au::\at Cannot find mono-2.0-sgen.dll, cannot load.");
		return;
	}


	#if !defined(_M_AMD64)
	monoRuntimeDir = rootDir + "\\resources\\mono\\32bit";
	#else
	monoRuntimeDir = rootDir + "\\resources\\mono\\64bit";
	#endif

	std::filesystem::path monoRuntimeDirPath = monoRuntimeDir;

	bool runtimeExists = std::filesystem::is_directory(monoRuntimeDirPath.parent_path());
	if (!runtimeExists)
	{
		WriteChatf("\arMQ2Mono\au::\at Cannot find mono runtime in resources folder (\\resources\\mono).");
		return;
	}

	mono_set_dirs((monoRuntimeDir + "\\lib").c_str(), (monoRuntimeDir + "\\etc").c_str());
	_rootDomain = mono_jit_init("Mono_Domain");
	mono_domain_set(_rootDomain, false);

	//Namespace.Class::Method + a Function pointer with the actual definition
	//the namespace/class binding too is hard coded to namespace: MonoCore
	//Class: Core
	mono_add_internal_call("MonoCore.Core::mq_Echo", &mono_Echo);
	mono_add_internal_call("MonoCore.Core::mq_ParseTLO", &mono_ParseTLO);
	mono_add_internal_call("MonoCore.Core::mq_DoCommand", &mono_DoCommand);
	mono_add_internal_call("MonoCore.Core::mq_DoCommandDelayed", &mono_DoCommandDelayed);
	mono_add_internal_call("MonoCore.Core::mq_Delay", &mono_Delay);
	mono_add_internal_call("MonoCore.Core::mq_AddCommand", &mono_AddCommand);
	mono_add_internal_call("MonoCore.Core::mq_ClearCommands", &mono_ClearCommands);
	mono_add_internal_call("MonoCore.Core::mq_RemoveCommand", &mono_RemoveCommand);
	mono_add_internal_call("MonoCore.Core::mq_GetSpellDataEffectCount", &mono_GetSpellDataEffectCount);
	mono_add_internal_call("MonoCore.Core::mq_GetSpellDataEffect", &mono_GetSpellDataEffect);
	mono_add_internal_call("MonoCore.Core::mq_GetSpawns", &mono_GetSpawns);
	mono_add_internal_call("MonoCore.Core::mq_GetSpawns2", &mono_GetSpawns2);
	mono_add_internal_call("MonoCore.Core::mq_GetRunNextCommand", &mono_GetRunNextCommand);
	mono_add_internal_call("MonoCore.Core::mq_GetFocusedWindowName", &mono_GetFocusedWindowName);
	mono_add_internal_call("MonoCore.Core::mono_GetHoverWindowName", &mono_GetHoverWindowName);
	mono_add_internal_call("MonoCore.Core::mq_GetMQ2MonoVersion", &mono_GetMQ2MonoVersion);

	mono_add_internal_call("MonoCore.Core::mq_ExecuteCommandByName", &mono_ExecuteCommandByName);
	mono_add_internal_call("MonoCore.Core::mq_ExecuteCommand", &mono_ExecuteCommand);
	mono_add_internal_call("MonoCore.Core::mq_LookAt", &mono_LookAt);
	
	//ImGui stuff
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Begin", &mono_ImGUI_Begin);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Button", &mono_ImGUI_Button);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_ButtonEx", &mono_ImGUI_ButtonEx);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_SmallButton", &mono_ImGUI_SmallButton);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_End", &mono_ImGUI_End);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Begin_OpenFlagSet", &mono_ImGUI_Begin_OpenFlagSet);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Begin_OpenFlagGet", &mono_ImGUI_Begin_OpenFlagGet);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Text", &mono_ImGUI_Text);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Separator", &mono_ImGUI_Separator);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_SameLine", &mono_ImGUI_SameLine);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_SameLineEx", &mono_ImGUI_SameLineEx);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Checkbox", &mono_ImGUI_Checkbox);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Checkbox_Clear", &mono_ImGUI_Checkbox_Clear);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Checkbox_Get", &mono_ImGUI_Checkbox_Get);

	mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginTabBar", &mono_ImGUI_BeginTabBar);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_EndTabBar", &mono_ImGUI_EndTabBar);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginTabItem", &mono_ImGUI_BeginTabItem);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_EndTabItem", &mono_ImGUI_EndTabItem);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_PushID", &mono_ImGUI_PushID);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_PopID", &mono_ImGUI_PopID);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginChild", &mono_ImGUI_BeginChild);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_EndChild", &mono_ImGUI_EndChild);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_Selectable", &mono_ImGUI_Selectable);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetContentRegionAvailX", &mono_ImGUI_GetContentRegionAvailX);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetContentRegionAvailY", &mono_ImGUI_GetContentRegionAvailY);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_InputText", &mono_ImGUI_InputText);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_InputText_Clear", &mono_ImGUI_InputText_Clear);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_CalcTextSizeX", &mono_ImGUI_CalcTextSizeX);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowContentRegionMinX", &mono_ImGUI_GetWindowContentRegionMin_X);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowContentRegionMinY", &mono_ImGUI_GetWindowContentRegionMin_Y);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowContentRegionMaxX", &mono_ImGUI_GetWindowContentRegionMax_X);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowContentRegionMaxY", &mono_ImGUI_GetWindowContentRegionMax_Y);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_SetNextWindowPos", &mono_ImGUI_SetNextWindowPos);

	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowPosX", &mono_ImGUI_GetWindowPosX);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowPosY", &mono_ImGUI_GetWindowPosY);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowSizeX", &mono_ImGUI_GetWindowSizeX);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowSizeY", &mono_ImGUI_GetWindowSizeY);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_SetNextWindowFocus", &mono_ImGUI_SetNextWindowFocus);
	//mono_ImGUI_InputTextClear
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_InputTextMultiline", &mono_ImGUI_InputTextMultiline);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_InputText_Get", &mono_ImGUI_InputText_Get);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_SetNextItemWidth", &mono_ImGUI_SetNextItemWidth);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowHeight", &mono_ImGUI_GetWindowHeight);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowWidth", &mono_ImGUI_GetWindowWidth);


    // Combo wrappers
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginCombo", &mono_ImGUI_BeginCombo);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_EndCombo", &mono_ImGUI_EndCombo);
    // Right align helper
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_RightAlignButton", &mono_ImGUI_RightAlignButton);

    // Context menus / popups
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginPopupContextItem", &mono_ImGUI_BeginPopupContextItem);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginPopupContextWindow", &mono_ImGUI_BeginPopupContextWindow);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_EndPopup", &mono_ImGUI_EndPopup);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_MenuItem", &mono_ImGUI_MenuItem);

    // Tables
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginTable", &mono_ImGUI_BeginTable);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginTableS", &mono_ImGUI_BeginTableSimple);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_EndTable", &mono_ImGUI_EndTable);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TableSetupColumn", &mono_ImGUI_TableSetupColumn);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_TableSetColumnIndex", &mono_ImGUI_TableSetColumnIndex);

    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TableHeadersRow", &mono_ImGUI_TableHeadersRow);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TableNextRow", &mono_ImGUI_TableNextRow);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TableNextColumn", &mono_ImGUI_TableNextColumn);

    // Colors / styled text
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TextColored", &mono_ImGUI_TextColored);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PushStyleColor", &mono_ImGUI_PushStyleColor);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PopStyleColor", &mono_ImGUI_PopStyleColor);

    // Style variables
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PushStyleVarFloat", &mono_ImGUI_PushStyleVarFloat);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PushStyleVarVec2", &mono_ImGUI_PushStyleVarVec2);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PopStyleVar", &mono_ImGUI_PopStyleVar);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetStyleVarFloat", &mono_ImGUI_GetStyleVarFloat);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetStyleVarVec2", &mono_ImGUI_GetStyleVarVec2);

    // Text wrapping and window sizing
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TextWrapped", &mono_ImGUI_TextWrapped);
    // Expose unformatted text render to avoid printf-style format crashes
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TextUnformatted", &mono_ImGUI_TextUnformatted);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PushTextWrapPos", &mono_ImGUI_PushTextWrapPos);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PopTextWrapPos", &mono_ImGUI_PopTextWrapPos);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_SetNextWindowSizeConstraints", &mono_ImGUI_SetNextWindowSizeConstraints);
    
    // New window control functions
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_SetNextWindowBgAlpha", &mono_ImGUI_SetNextWindowBgAlpha);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_SetNextWindowSize", &mono_ImGUI_SetNextWindowSize);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_SetNextWindowSizeWithCond", &mono_ImGUI_SetNextWindowSizeWithCond);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_IsWindowHovered", &mono_ImGUI_IsWindowHovered);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowWidth", &mono_ImGUI_GetWindowWidth);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowHeight", &mono_ImGUI_GetWindowHeight);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_IsMouseClicked", &mono_ImGUI_IsMouseClicked);
    
	// Input int
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_InputInt", &mono_ImGUI_InputInt);
	//mono_ImGUI_InputTextClear
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_InputInt_Get", &mono_ImGUI_InputInt_Get);
	mono_add_internal_call("MonoCore.E3ImGUI::imgui_InputInt_Clear", &mono_ImGUI_InputInt_Clear);
	//bool mono_ImGUI_InputIntClear(MonoString* id)

	//mono_ImGUI_InputInt

	// Sliders
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_SliderInt", &mono_ImGUI_SliderInt);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_SliderDouble", &mono_ImGUI_SliderDouble);
    
    // Tree nodes
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TreeNode", &mono_ImGUI_TreeNode);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TreeNodeEx", &mono_ImGUI_TreeNodeEx);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_TreePop", &mono_ImGUI_TreePop);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_CollapsingHeader", &mono_ImGUI_CollapsingHeader);

    // Tooltips and hover detection
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_IsItemHovered", &mono_ImGUI_IsItemHovered);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_BeginTooltip", &mono_ImGUI_BeginTooltip);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_EndTooltip", &mono_ImGUI_EndTooltip);

    // Image display
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_Image", &mono_ImGUI_Image);

    // Fonts
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_AddFontFromFileTTF", &mono_ImGUI_AddFontFromFileTTF);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PushFont", &mono_ImGUI_PushFont);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PopFont", &mono_ImGUI_PopFont);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_PushMaterialIconsFont", &mono_ImGUI_PushMaterialIconsFont);

    // Spell icon drawing
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_DrawSpellIconByIconIndex", &mono_ImGUI_DrawSpellIconByIconIndex);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_DrawSpellIconBySpellID", &mono_ImGUI_DrawSpellIconBySpellID);

    // Drawing functions for custom backgrounds
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetCursorPosY", &mono_ImGUI_GetCursorPosY);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetCursorScreenPosX", &mono_ImGUI_GetCursorScreenPosX);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetCursorScreenPosY", &mono_ImGUI_GetCursorScreenPosY);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetTextLineHeightWithSpacing", &mono_ImGUI_GetTextLineHeightWithSpacing);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetFrameHeight", &mono_ImGUI_GetFrameHeight);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowDrawList_AddRectFilled", &mono_ImGUI_GetWindowDrawList_AddRectFilled);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetWindowDrawList_AddText", &mono_ImGUI_GetWindowDrawList_AddText);

    // Item rect + color helpers
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetItemRectMinX", &mono_ImGUI_GetItemRectMinX);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetItemRectMinY", &mono_ImGUI_GetItemRectMinY);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetItemRectMaxX", &mono_ImGUI_GetItemRectMaxX);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetItemRectMaxY", &mono_ImGUI_GetItemRectMaxY);
    mono_add_internal_call("MonoCore.E3ImGUI::imgui_GetColorU32", &mono_ImGUI_GetColorU32);

    // Texture creation from raw data
    mono_add_internal_call("MonoCore.E3ImGUI::mq_CreateTextureFromData", &mono_CreateTextureFromData);
    mono_add_internal_call("MonoCore.E3ImGUI::mq_DestroyTexture", &mono_DestroyTexture);

	
	bmUpdateMonoOnPulse = AddMQ2Benchmark("UpdateMonoOnPulse");
	initialized = true;

}

bool UnloadAppDomain(std::string appDomainName, bool updateCollections=true)
{		 
	MonoDomain* domainToUnload = nullptr;
	//check to see if its registered, if so update ptr
	if (monoAppDomains.count(appDomainName) > 0)
	{
		domainToUnload = monoAppDomains[appDomainName].m_appDomain;
	}
	//verify its not the root domain and this is a valid domain pointer
	if (domainToUnload && domainToUnload != mono_get_root_domain())
	{
		if (monoAppDomains[appDomainName].m_OnStop)
		{
			mono_domain_set(domainToUnload, false);
			mono_runtime_invoke(monoAppDomains[appDomainName].m_OnStop, monoAppDomains[appDomainName].m_classInstance, nullptr, nullptr);
		}
		//give time for the threads to die
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		//clean up any commands we have registered
		monoAppDomains[appDomainName].ClearCommands();


		if (updateCollections)
		{
			monoAppDomains.erase(appDomainName);
			monoAppDomainPtrToString.erase(domainToUnload);
			//remove from the process queue
			int count = 0;
			int processCount = static_cast<int>(appDomainProcessQueue.size());

			while (count < processCount)
			{
				count++;
				std::string currentKey = appDomainProcessQueue.front();
				appDomainProcessQueue.pop_front();
				if (!ci_equals(currentKey, appDomainName))
				{
					appDomainProcessQueue.push_back(currentKey);
				}
			}
		}
		//unload the commands

		mono_domain_set(mono_get_root_domain(), false);

		//mono_thread_pop_appdomain_ref();
		mono_domain_unload(domainToUnload);
		
		return true;
	}
	return false;
}

void UnloadAllAppDomains()
{
	//map may be removed
	//https://stackoverflow.com/questions/8234779/how-to-remove-from-a-map-while-iterating-it
	for (auto i = monoAppDomains.cbegin(); i != monoAppDomains.cend() /* not hoisted */; /* no increment */)
	{
		//unload without modifying the collections
		UnloadAppDomain(i->second.m_appDomainName,false);

		//now remove from all the collections
		monoAppDomainPtrToString.erase(i->second.m_appDomain);
		//remove from the process queue
		int count = 0;
		int processCount = static_cast<int>(appDomainProcessQueue.size());

		while (count < processCount)
		{
			count++;
			std::string currentKey = appDomainProcessQueue.front();
			appDomainProcessQueue.pop_front();
			if (!ci_equals(currentKey, i->second.m_appDomainName))
			{
				appDomainProcessQueue.push_back(currentKey);
			}
		}
		//modify the map using the iterator
		monoAppDomains.erase(i++); // or "i = m.erase(it)" since C++11
	}
}
bool InitAppDomain(std::string appDomainName)
{
	UnloadAppDomain(appDomainName);
	
	//app domain we have created for e3
	MonoDomain* appDomain;
	appDomain = mono_domain_create_appdomain((char*)appDomainName.c_str() , nullptr);
	
	//core.dll information so we can bind to it
	MonoAssembly* csharpAssembly;
	MonoImage* coreAssemblyImage;
	MonoClass* classInfo;
	MonoObject* classInstance;
	//methods that we call in C# if they are available
	MonoMethod* OnPulseMethod;
	MonoMethod* OnWriteChatColor;
	MonoMethod* OnIncomingChat;
	MonoMethod* OnInit;
	MonoMethod* OnStop;
	MonoMethod* OnCommand;
	MonoMethod* OnUpdateImGui;
	MonoMethod* OnSetSpawns;
	MonoMethod* OnQuery;

	std::map<std::string, bool> IMGUI_OpenWindows;

	//everything below needs to be moved out to a per application run
	mono_domain_set(appDomain, false);


	std::string fileName = (appDomainName+".dll");
	std::string assemblypath = (monoDir + "\\macros\\" + appDomainName + "\\");

	bool filepathExists = std::filesystem::exists(assemblypath + fileName);

	if (!filepathExists)
	{
		UnloadAppDomain(appDomainName, false);
		return false;
	}
	
	//shadow directory work, copy over dlls to the new folder
	std::string charName(pLocalPC->Name);
	std::string shadowDirectory = assemblypath + charName+"\\";

	namespace fs = std::filesystem;
	if (!fs::is_directory(shadowDirectory) || !fs::exists(shadowDirectory)) { // Check if src folder exists
		fs::create_directory(shadowDirectory); // create src folder
	}

	//copy it to a new directory
	try
	{
		std::filesystem::copy(assemblypath, shadowDirectory, std::filesystem::copy_options::update_existing);

	}
	catch(...)
	{
		WriteChatf("\arMQ2Mono\au::\at Cannot copy data to %s , is it locked by the OS?", shadowDirectory.c_str());
		return false;
	}

	
	csharpAssembly = mono_domain_assembly_open(appDomain, (shadowDirectory + fileName).c_str());
	
	if (!csharpAssembly)
	{
		UnloadAppDomain(appDomainName, false);
		return false;
	}
	coreAssemblyImage = mono_assembly_get_image(csharpAssembly);
	classInfo = mono_class_from_name(coreAssemblyImage, "MonoCore", "Core");
	classInstance = mono_object_new(appDomain, classInfo);
	OnPulseMethod = mono_class_get_method_from_name(classInfo, "OnPulse", 0);
	OnWriteChatColor = mono_class_get_method_from_name(classInfo, "OnWriteChatColor", 1);
	OnIncomingChat = mono_class_get_method_from_name(classInfo, "OnIncomingChat", 1);
	OnInit = mono_class_get_method_from_name(classInfo, "OnInit", 0);
	OnStop = mono_class_get_method_from_name(classInfo, "OnStop", 0);
	OnUpdateImGui = mono_class_get_method_from_name(classInfo, "OnUpdateImGui", 0);
	OnCommand = mono_class_get_method_from_name(classInfo, "OnCommand", 1);
	OnSetSpawns= mono_class_get_method_from_name(classInfo, "OnSetSpawns", 2);
	OnQuery = mono_class_get_method_from_name(classInfo, "OnQuery", 1);

	//add it to the collection

	monoAppDomainInfo domainInfo;
	domainInfo.m_appDomainName = appDomainName;
	domainInfo.m_appDomain = appDomain;
	domainInfo.m_csharpAssembly = csharpAssembly;
	domainInfo.m_coreAssemblyImage = coreAssemblyImage;
	domainInfo.m_classInfo = classInfo;
	domainInfo.m_classInstance = classInstance;
	domainInfo.m_OnPulseMethod = OnPulseMethod;
	domainInfo.m_OnWriteChatColor = OnWriteChatColor;
	domainInfo.m_OnIncomingChat = OnIncomingChat;
	domainInfo.m_OnInit = OnInit;
	domainInfo.m_OnStop = OnStop;
	domainInfo.m_OnUpdateImGui = OnUpdateImGui;
	domainInfo.m_OnCommand = OnCommand;
	domainInfo.m_OnSetSpawns = OnSetSpawns;
	domainInfo.m_OnQuery = OnQuery;

	monoAppDomains[appDomainName] = domainInfo;
	monoAppDomainPtrToString[appDomain] = appDomainName;
	appDomainProcessQueue.push_back(appDomainName);
	

	//call the Init
	if (OnInit)
	{
		mono_runtime_invoke(OnInit, classInstance, nullptr, nullptr);
	}

	return true;
	
}
void MonoCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	char szParam1[MAX_STRING] = { 0 };
	char szParam2[MAX_STRING] = { 0 };
		
	GetArg(szParam1, szLine, 1);

	if ((strlen(szParam1))) {

		GetArg(szParam2, szLine, 2);
	}

	if (!initialized)
	{
		InitMono();
		if (!initialized)
		{
			return;
		}
	}
	//WriteChatf("\arMQ2Mono\au::\at Command issued.");
	//WriteChatf(szParam1);
	//WriteChatf(szParam2);
	if (ci_equals(szParam1, "list"))
	{
		WriteChatf("\arMQ2Mono\au::\at List Running Processes.");
		for (auto i : monoAppDomains)
		{
			WriteChatf(i.second.m_appDomainName.c_str());
		}
		WriteChatf("\arMQ2Mono\au::\at End List.");
		return;
	
	}
	if (ci_equals(szParam1, "version"))
	{
		WriteChatf(("\arMQ2Mono\au::\at Version:"+version).c_str());
		return;

	}
	if (ci_equals(szParam1, "unload") || ci_equals(szParam1, "stop"))
	{
		if (strlen(szParam2))
		{
			if (ci_equals(szParam2, "ALL"))
			{
				UnloadAllAppDomains();
				WriteChatf("\arMQ2Mono\au::\at Finished Unloading all scripts.");
			}
			else
			{
				WriteChatf("\arMQ2Mono\au::\at Unloading %s", szParam2);
				std::string input(szParam2);
				if (UnloadAppDomain(input))
				{
					WriteChatf("\arMQ2Mono\au::\at Finished Unloading %s", szParam2);
				}
				else
				{
					WriteChatf("\arMQ2Mono\au::\at Cannot find %s", szParam2);
				}

			}
			

		}
		else
		{
			WriteChatf("\arMQ2Mono\au::\at What should I load?");

		}
	}
	else if (ci_equals(szParam1, "load") || ci_equals(szParam1, "run") || ci_equals(szParam1, "start"))
	{
		if (strlen(szParam2))
		{
			WriteChatf("\arMQ2Mono\au::\at Loading %s",szParam2);
			std::string input(szParam2);
			if (InitAppDomain(input))
			{
				WriteChatf("\arMQ2Mono\au::\at Finished Loading %s", szParam2);
			}
			else
			{
				WriteChatf("\arMQ2Mono\au::\at Cannot find %s", szParam2);

			}

		}
		else
		{
			WriteChatf("\arMQ2Mono\au::\at What should I load?");

		}


	}
	else
	{
		if (strlen(szParam1))
		{
			WriteChatf("\arMQ2Mono\au::\at Loading %s", szParam1);
			std::string input(szParam1);
			if (InitAppDomain(input))
			{
				WriteChatf("\arMQ2Mono\au::\at Finished Loading %s", szParam1);
			}
			else
			{
				WriteChatf("\arMQ2Mono\au::\at Cannot find %s", szParam1);

			}

		}
	}
	

}


/// <summary>
/// This is the MQ2Mono TLO, used to access the Query method for OnQuery calls to the mono application specified
/// /echo ${MQ2Mono.Query[e3,E3Bots(Necro01).Query(ShowClass)]}
/// here we send to the "e3" application the string "E3Bots(Necro01).Query(ShowClass)"
/// note, you cannot do "E3Bots[Necro01].Query[ShowClass]" as [] are special characters and will truncate your data
/// </summary>
class MQ2MonoMethods* pMonoQuery = nullptr;
class MQ2MonoMethods : public MQ2Type
{
public:
	enum M2MonoMembers
	{
		Query
	};

	MQ2MonoMethods() :MQ2Type("MQ2Mono")
	{
		TypeMember(Query);
	}

	virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
	{
		MQTypeMember* pMember = MQ2MonoMethods::FindMember(Member);
		if (!pMember)
			return false;
	
		char tmp[MAX_STRING] = { 0 };
		DataTypeTemp[0] = '\0';
		switch ((M2MonoMembers)pMember->ID)
		{
			case Query:
				if (appDomainProcessQueue.size() < 1) return true;
				if (monoAppDomains.size() == 0) return true;
				if (char* Arg = Index)
				{
					if (char* pDest = strchr(Arg, ','))
					{
						pDest[0] = '\0';
						auto domainName = trim(Arg);
						pDest++;
						auto expressionValue = trim(pDest);
						

						for (auto i : monoAppDomains)
						{
							if (i.second.m_appDomainName == domainName)
							{
								//Call the main method in this code
								if (i.second.m_appDomain && i.second.m_OnQuery)
								{
									mono_domain_set(i.second.m_appDomain, false);
									std::string param = std::string(expressionValue);
									MonoString* monoLine = mono_string_new(i.second.m_appDomain, param.c_str());
									void* params[1] =
									{
										monoLine

									};
									MonoString *result = (MonoString*)mono_runtime_invoke(i.second.m_OnQuery, i.second.m_classInstance, params, nullptr);
									char* cppString = mono_string_to_utf8(result);
									std::string str(cppString);
									DataTypeTemp[0] = '\0'; //clear it out again in case the invoke also used it.
									strcat_s(DataTypeTemp, cppString);
									mono_free(cppString);
									break;
								}
							}
						}
					}
				}
				Dest.Ptr = &DataTypeTemp[0];
				Dest.Type = mq::datatypes::pStringType;
				return true;
		}
		return false;
	}

	bool ToString(MQVarPtr VarPtr, char* Destination) override
	{
		strcpy_s(Destination, MAX_STRING, "TRUE");
		return true;
	}
};

bool dataMQ2Mono(const char* szName, MQTypeVar& Dest)
{
	Dest.DWord = 1;
	Dest.Type = pMonoQuery;
	return true;
}


PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("MQ2Mono::Initializing version %f", MQ2Version);
	AddCommand("/mono", MonoCommand, 0, 1, 1);
	pMonoQuery = new MQ2MonoMethods;
	AddMQ2Data("MQ2Mono", dataMQ2Mono);
}
/**
 * @fn ShutdownPlugin
 *
 * This is called once when the plugin has been asked to shutdown.  The plugin has
 * not actually shut down until this completes.
 */
PLUGIN_API void ShutdownPlugin()
{
	// DebugSpewAlways("MQ2Mono::OnUnloadPlugin(%s)", Name);
	/// <summary>
	/// for future me
	/// https://github.com/mono/mono/issues/13557
	/// In general, to reload assemblies you should be running the assembly in a new appdomain 
	/// (this will affect how you organize your application) and then unloading the domain to unload the old assemblies. 
	/// Anything else is not supported by Mono and you may have unpredictable results.
	/// </summary>
	
	if (initialized)
	{
		UnloadAllAppDomains();
		mono_jit_cleanup(mono_get_root_domain());
	}
	RemoveMQ2Benchmark(bmUpdateMonoOnPulse);
	RemoveCommand("/mono");
	RemoveMQ2Data("MQ2Mono");
	delete pMonoQuery;
}

/**
 * @fn OnCleanUI
 *
 * This is called once just before the shutdown of the UI system and each time the
 * game requests that the UI be cleaned.  Most commonly this happens when a
 * /loadskin command is issued, but it also occurs when reaching the character
 * select screen and when first entering the game.
 *
 * One purpose of this function is to allow you to destroy any custom windows that
 * you have created and cleanup any UI items that need to be removed.
 */
PLUGIN_API void OnCleanUI()
{
	// DebugSpewAlways("MQ2Mono::OnCleanUI()");
}

/**
 * @fn OnReloadUI
 *
 * This is called once just after the UI system is loaded. Most commonly this
 * happens when a /loadskin command is issued, but it also occurs when first
 * entering the game.
 *
 * One purpose of this function is to allow you to recreate any custom windows
 * that you have setup.
 */
PLUGIN_API void OnReloadUI()
{
	// DebugSpewAlways("MQ2Mono::OnReloadUI()");
}

/**
 * @fn OnDrawHUD
 *
 * This is called each time the Heads Up Display (HUD) is drawn.  The HUD is
 * responsible for the net status and packet loss bar.
 *
 * Note that this is not called at all if the HUD is not shown (default F11 to
 * toggle).
 *
 * Because the net status is updated frequently, it is recommended to have a
 * timer or counter at the start of this call to limit the amount of times the
 * code in this section is executed.
 */
PLUGIN_API void OnDrawHUD()
{
/*
	static std::chrono::steady_clock::time_point DrawHUDTimer = std::chrono::steady_clock::now();
	// Run only after timer is up
	if (std::chrono::steady_clock::now() > DrawHUDTimer)
	{
		// Wait half a second before running again
		DrawHUDTimer = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
		DebugSpewAlways("MQ2Mono::OnDrawHUD()");
	}
*/
}

/**
 * @fn SetGameState
 *
 * This is called when the GameState changes.  It is also called once after the
 * plugin is initialized.
 *
 * For a list of known GameState values, see the constants that begin with
 * GAMESTATE_.  The most commonly used of these is GAMESTATE_INGAME.
 *
 * When zoning, this is called once after @ref OnBeginZone @ref OnRemoveSpawn
 * and @ref OnRemoveGroundItem are all done and then called once again after
 * @ref OnEndZone and @ref OnAddSpawn are done but prior to @ref OnAddGroundItem
 * and @ref OnZoned
 *
 * @param GameState int - The value of GameState at the time of the call
 */
PLUGIN_API void SetGameState(int GameState)
{
	// DebugSpewAlways("MQ2Mono::SetGameState(%d)", GameState);
}


/**
 * @fn OnPulse
 *
 * This is called each time MQ2 goes through its heartbeat (pulse) function.
 *
 * Because this happens very frequently, it is recommended to have a timer or
 * counter at the start of this call to limit the amount of times the code in
 * this section is executed.
 */
PLUGIN_API void OnPulse()
{	
	if (!initialized) return;

	previousCommand = false;
	if (appDomainProcessQueue.size() < 1) return;

	std::chrono::steady_clock::time_point proccessingTimer = std::chrono::steady_clock::now(); //the time this was issued + m_delayTime



	int gameState = GetGameState();
	/*constexpr int GAMESTATE_PRECHARSELECT  = -1;
	constexpr int GAMESTATE_CHARSELECT     = 1;
	constexpr int GAMESTATE_CHARCREATE     = 2;
	constexpr int GAMESTATE_POSTCHARSELECT = 3;
	constexpr int GAMESTATE_SOMETHING      = 4;
	constexpr int GAMESTATE_INGAME         = 5;
	constexpr int GAMESTATE_LOGGINGIN      = 253;
	constexpr int GAMESTATE_UNLOADING      = 255;*/
	// If we have left the world and have apps running, unload them all
	if (gameState != GAMESTATE_INGAME)
	{
		if (monoAppDomains.size() > 0)
		{
			//unload all app domains to terminate all scripts
			UnloadAllAppDomains();
		}
		return;
	}
	std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now(); //the time this was issued + m_delayTime


	size_t count = 0;
	size_t processQueueSize = appDomainProcessQueue.size();
	while (count< processQueueSize)
	{
		std::string domainKey = appDomainProcessQueue.front();
		appDomainProcessQueue.pop_front();
		appDomainProcessQueue.push_back(domainKey);
		count++;
		//get pointer to struct

		auto i = monoAppDomains.find(domainKey);
		if (i != monoAppDomains.end())
		{
			//if we have a delay check to see if we can reset it
			if (i->second.m_delayTime > 0 && std::chrono::steady_clock::now() > i->second.m_delayTimer)
			{
				i->second.m_delayTime = 0;
			}
			//check to make sure we are not in an delay
			if (i->second.m_delayTime == 0) {
				//if not, do work
				if (i->second.m_appDomain && i->second.m_OnPulseMethod)
				{
					MQScopedBenchmark bm1(bmUpdateMonoOnPulse);
					mono_domain_set(i->second.m_appDomain, false);
					mono_runtime_invoke(i->second.m_OnPulseMethod, i->second.m_classInstance, nullptr, nullptr);
				}
			}
			if (previousCommand)
			{	//we did a command on this pulse, break out so it can be executed and then the next script can get a chance to run
				break;
			}
			//check if we have spent the specified time N-ms, or we have processed the entire queue kick out
			if (std::chrono::steady_clock::now() > (proccessingTimer + std::chrono::milliseconds(_milisecondsToProcess)) || count >= appDomainProcessQueue.size())
			{
				break;
			}
		}
		else
		{
			//should never be ever to get here, but if so.
			//get rid of the bad domainKey
			appDomainProcessQueue.pop_back();
		}

	}

}

/**
 * @fn OnWriteChatColor
 *
 * This is called each time WriteChatColor is called (whether by MQ2Main or by any
 * plugin).  This can be considered the "when outputting text from MQ" callback.
 *
 * This ignores filters on display, so if they are needed either implement them in
 * this section or see @ref OnIncomingChat where filters are already handled.
 *
 * If CEverQuest::dsp_chat is not called, and events are required, they'll need to
 * be implemented here as well.  Otherwise, see @ref OnIncomingChat where that is
 * already handled.
 *
 * For a list of Color values, see the constants for USERCOLOR_.  The default is
 * USERCOLOR_DEFAULT.
 *
 * @param Line const char* - The line that was passed to WriteChatColor
 * @param Color int - The type of chat text this is to be sent as
 * @param Filter int - (default 0)
 */
PLUGIN_API void OnWriteChatColor(const char* Line, int Color, int Filter)
{	
	if (!initialized) return;

	if (monoAppDomains.size() == 0) return;

	//stolen from the lua plugin
	std::string_view line = Line;
	char line_char[MAX_STRING] = { 0 };

	if (line.find_first_of('\x12') != std::string::npos)
	{
		CXStr line_str(line);
		line_str = CleanItemTags(line_str, false);
		StripMQChat(line_str, line_char);
	}
	else
	{
		StripMQChat(line, line_char);
	}

	// since we initialized to 0, we know that any remaining members will be 0, so just in case we Get an overflow, re-set the last character to 0
	line_char[MAX_STRING - 1] = 0;
	//end of of the robbery. 

	for (auto i : monoAppDomains)
	{
		//Call the main method in this code
		if (i.second.m_appDomain && i.second.m_OnWriteChatColor)
		{
			mono_domain_set(i.second.m_appDomain, false);

			MonoString* monoLine = mono_string_new(i.second.m_appDomain, line_char);
			void* params[1] =
			{
				monoLine

			};

			mono_runtime_invoke(i.second.m_OnWriteChatColor, i.second.m_classInstance, params, nullptr);
		}
	}
	 //DebugSpewAlways("MQ2Mono::OnWriteChatColor(%s, %d, %d)", Line, Color, Filter);
}

/**
 * @fn OnIncomingChat
 *
 * This is called each time a line of chat is shown.  It occurs after MQ filters
 * and chat events have been handled.  If you need to know when MQ2 has sent chat,
 * consider using @ref OnWriteChatColor instead.
 *
 * For a list of Color values, see the constants for USERCOLOR_. The default is
 * USERCOLOR_DEFAULT.
 *
 * @param Line const char* - The line of text that was shown
 * @param Color int - The type of chat text this was sent as
 *
 * @return bool - Whether to filter this chat from display
 */
PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (!initialized) return false;
	if (monoAppDomains.size() == 0) return false;

	//stolen from the lua plugin
	std::string_view line = Line;
	char line_char[MAX_STRING] = { 0 };

	if (line.find_first_of('\x12') != std::string::npos)
	{
		CXStr line_str(line);
		line_str = CleanItemTags(line_str, false);
		StripMQChat(line_str, line_char);
	}
	else
	{
		StripMQChat(line, line_char);
	}

	// since we initialized to 0, we know that any remaining members will be 0, so just in case we Get an overflow, re-set the last character to 0
	line_char[MAX_STRING - 1] = 0;
	//end of of the robbery. 


	for (auto i : monoAppDomains)
	{
		//Call the main method in this code
		if (i.second.m_appDomain && i.second.m_OnIncomingChat)
		{
			mono_domain_set(i.second.m_appDomain, false);
			MonoString* monoLine = mono_string_new(i.second.m_appDomain, line_char);
			void* params[1] =
			{
				monoLine

			};
			
			mono_runtime_invoke(i.second.m_OnIncomingChat, i.second.m_classInstance, params, nullptr);
		}
	}
	// DebugSpewAlways("MQ2Mono::OnIncomingChat(%s, %d)", Line, Color);
	return false;
}

/**
 * @fn OnAddSpawn
 *
 * This is called each time a spawn is added to a zone (ie, something spawns). It is
 * also called for each existing spawn when a plugin first initializes.
 *
 * When zoning, this is called for all spawns in the zone after @ref OnEndZone is
 * called and before @ref OnZoned is called.
 *
 * @param pNewSpawn PSPAWNINFO - The spawn that was added
 */
PLUGIN_API void OnAddSpawn(PSPAWNINFO pNewSpawn)
{
	// DebugSpewAlways("MQ2Mono::OnAddSpawn(%s)", pNewSpawn->Name);
}

/**
 * @fn OnRemoveSpawn
 *
 * This is called each time a spawn is removed from a zone (ie, something despawns
 * or is killed).  It is NOT called when a plugin shuts down.
 *
 * When zoning, this is called for all spawns in the zone after @ref OnBeginZone is
 * called.
 *
 * @param pSpawn PSPAWNINFO - The spawn that was removed
 */
PLUGIN_API void OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	// DebugSpewAlways("MQ2Mono::OnRemoveSpawn(%s)", pSpawn->Name);
}

/**
 * @fn OnAddGroundItem
 *
 * This is called each time a ground item is added to a zone (ie, something spawns).
 * It is also called for each existing ground item when a plugin first initializes.
 *
 * When zoning, this is called for all ground items in the zone after @ref OnEndZone
 * is called and before @ref OnZoned is called.
 *
 * @param pNewGroundItem PGROUNDITEM - The ground item that was added
 */
PLUGIN_API void OnAddGroundItem(PGROUNDITEM pNewGroundItem)
{
	// DebugSpewAlways("MQ2Mono::OnAddGroundItem(%d)", pNewGroundItem->DropID);
}

/**
 * @fn OnRemoveGroundItem
 *
 * This is called each time a ground item is removed from a zone (ie, something
 * despawns or is picked up).  It is NOT called when a plugin shuts down.
 *
 * When zoning, this is called for all ground items in the zone after
 * @ref OnBeginZone is called.
 *
 * @param pGroundItem PGROUNDITEM - The ground item that was removed
 */
PLUGIN_API void OnRemoveGroundItem(PGROUNDITEM pGroundItem)
{
	// DebugSpewAlways("MQ2Mono::OnRemoveGroundItem(%d)", pGroundItem->DropID);
}

/**
 * @fn OnBeginZone
 *
 * This is called just after entering a zone line and as the loading screen appears.
 */
PLUGIN_API void OnBeginZone()
{
	// DebugSpewAlways("MQ2Mono::OnBeginZone()");

}

/**
 * @fn OnEndZone
 *
 * This is called just after the loading screen, but prior to the zone being fully
 * loaded.
 *
 * This should occur before @ref OnAddSpawn and @ref OnAddGroundItem are called. It
 * always occurs before @ref OnZoned is called.
 */
PLUGIN_API void OnEndZone()
{
	// DebugSpewAlways("MQ2Mono::OnEndZone()");
}

/**
 * @fn OnZoned
 *
 * This is called after entering a new zone and the zone is considered "loaded."
 *
 * It occurs after @ref OnEndZone @ref OnAddSpawn and @ref OnAddGroundItem have
 * been called.
 */
PLUGIN_API void OnZoned()
{
	// DebugSpewAlways("MQ2Mono::OnZoned()");
}

/**
 * @fn OnUpdateImGui
 *
 * This is called each time that the ImGui Overlay is rendered. Use this to render
 * and update plugin specific widgets.
 *
 * Because this happens extremely frequently, it is recommended to move any actual
 * work to a separate call and use this only for updating the display.
 */
PLUGIN_API void OnUpdateImGui()
{
	// Allow managed domains to render their ImGui each frame
	if (!initialized) return;
	if (monoAppDomains.size() == 0) return;

	for (auto i : monoAppDomains)
	{
		//Call the main method in this code
		if (i.second.m_appDomain && i.second.m_OnUpdateImGui)
		{
			mono_domain_set(i.second.m_appDomain, false);
			mono_runtime_invoke(i.second.m_OnUpdateImGui, i.second.m_classInstance, nullptr, nullptr);
		}
	}
	
}

/**
 * @fn OnMacroStart
 *
 * This is called each time a macro starts (ex: /mac somemacro.mac), prior to
 * launching the macro.
 *
 * @param Name const char* - The name of the macro that was launched
 */
PLUGIN_API void OnMacroStart(const char* Name)
{
	// DebugSpewAlways("MQ2Mono::OnMacroStart(%s)", Name);
}

/**
 * @fn OnMacroStop
 *
 * This is called each time a macro stops (ex: /endmac), after the macro has ended.
 *
 * @param Name const char* - The name of the macro that was stopped.
 */
PLUGIN_API void OnMacroStop(const char* Name)
{
	// DebugSpewAlways("MQ2Mono::OnMacroStop(%s)", Name);
}

/**
 * @fn OnLoadPlugin
 *
 * This is called each time a plugin is loaded (ex: /plugin someplugin), after the
 * plugin has been loaded and any associated -AutoExec.cfg file has been launched.
 * This means it will be executed after the plugin's @ref InitializePlugin callback.
 *
 * This is also called when THIS plugin is loaded, but initialization tasks should
 * still be done in @ref InitializePlugin.
 *
 * @param Name const char* - The name of the plugin that was loaded
 */
PLUGIN_API void OnLoadPlugin(const char* Name)
{
	// DebugSpewAlways("MQ2Mono::OnLoadPlugin(%s)", Name);
}

/**
 * @fn OnUnloadPlugin
 *
 * This is called each time a plugin is unloaded (ex: /plugin someplugin unload),
 * just prior to the plugin unloading.  This means it will be executed prior to that
 * plugin's @ref ShutdownPlugin callback.
 *
 * This is also called when THIS plugin is unloaded, but shutdown tasks should still
 * be done in @ref ShutdownPlugin.
 *
 * @param Name const char* - The name of the plugin that is to be unloaded
 */
PLUGIN_API void OnUnloadPlugin(const char* Name)
{
	
	
	
}
void OnCommand(std::string command)
{

}
#pragma region
static bool mono_AddCommand(MonoString* text)
{
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	mono_free(cppString);
	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];
		return domainInfo.AddCommand(str);
	}

	return false;
}

static void mono_ClearCommands()
{	
	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];
		domainInfo.ClearCommands();
	}
}
static void mono_RemoveCommand(MonoString* text)
{
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	mono_free(cppString);
	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];
		domainInfo.RemoveCommand(str);
	}
}
#pragma endregion

static void mono_Delay(int milliseconds)
{
	MonoDomain* currentDomain = mono_domain_get();
	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];
		//do domnain lookup via its pointer
		domainInfo.m_delayTimer = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
		domainInfo.m_delayTime = milliseconds;
	}
	//WriteChatf("Mono_Delay called with %d", m_delayTime);
	//WriteChatf("Mono_Delay delaytimer %ld", m_delayTimer);
}
static void mono_Echo(MonoString* string)
{
	char* cppString = mono_string_to_utf8(string);
	std::string str(cppString);
	WriteChatColor(str.c_str());
	//WriteChatf("%s", str.c_str());
	mono_free(cppString);
}
static void mono_DoCommand(MonoString* text)
{
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	mono_free(cppString);
	HideDoCommand(pLocalPlayer, str.c_str(), false);
	previousCommand = true;

}

static void mono_ExecuteCommandByName(MonoString* name, bool holdKey)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);

	unsigned int commandID = FindMappableCommand(str.c_str());
	if (commandID != -1)
	{
		mq::ExecuteCmd(commandID, holdKey);
	}
}

static void mono_ExecuteCommand(unsigned int commandID, bool holdKey)
{
	mq::ExecuteCmd(commandID, holdKey);

}

static void mono_LookAt(FLOAT X, FLOAT Y, FLOAT Z)
{
	if (PCHARINFO pChar = GetCharInfo())
	{
		if (pChar->pSpawn)
		{
			float angle = (atan2(X - pChar->pSpawn->X, Y - pChar->pSpawn->Y) * 256.0f / (float)PI);
			if (angle >= 512.0f)
				angle -= 512.0f;
			if (angle < 0.0f)
				angle += 512.0f;
			pControlledPlayer->Heading = (FLOAT)angle;
			gFaceAngle = 10000.0f;
			if (pChar->pSpawn->FeetWet) {
				float locdist = GetDistance(pChar->pSpawn->X, pChar->pSpawn->Y, X, Y);
				pChar->pSpawn->CameraAngle = (atan2(Z + 0.0f * 0.9f - pChar->pSpawn->Z - pChar->pSpawn->AvatarHeight * 0.9f, locdist) * 256.0f / (float)PI);
			}
			else if (pChar->pSpawn->mPlayerPhysicsClient.Levitate == 2) {
				if (Z < pChar->pSpawn->Z - 5)
					pChar->pSpawn->CameraAngle = -64.0f;
				else if (Z > pChar->pSpawn->Z + 5)
					pChar->pSpawn->CameraAngle = 64.0f;
				else
					pChar->pSpawn->CameraAngle = 0.0f;
			}
			else
				pChar->pSpawn->CameraAngle = 0.0f;
			gLookAngle = 10000.0f;
		}
	}
}

static void mono_DoCommandDelayed(MonoString* text)
{
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	mono_free(cppString);
	HideDoCommand(pLocalPlayer, str.c_str(), true);
	previousCommand = true;

}
static MonoString* mono_ParseTLO(MonoString* text)
{
	char buffer[MAX_STRING] = { 0 };
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	strncpy_s(buffer, str.c_str(), sizeof(buffer));
	mono_free(cppString);

	auto old_parser = std::exchange(gParserVersion, 2);
	MonoString* returnValue;
	if (ParseMacroData(buffer, sizeof(buffer))) {
		//allocate string on the current domain call
		returnValue = mono_string_new_wrapper(buffer);
	}
	else {
		//allocate string on the current domain call
		returnValue = mono_string_new_wrapper("NULL");
	}
	gParserVersion = old_parser;
	return returnValue;
}
//used to get the global boolean 
bool mono_GetRunNextCommand()
{

	return bRunNextCommand;

}

static int mono_GetSpellDataEffectCount(MonoString* query)
{
	char buffer[MAX_STRING] = { 0 };
	char* cppString = mono_string_to_utf8(query);
	std::string str(cppString);
	strncpy_s(buffer, str.c_str(), sizeof(buffer));
	mono_free(cppString);
	eqlib::SPELL* pSpell = nullptr;
	MonoString* returnValue;
	IsNumber(buffer) ? pSpell = GetSpellByID(GetIntFromString(buffer, 0)) : pSpell = GetSpellByName(buffer);
	if (!pSpell)
	{
		return -1;
	}

	return GetSpellNumEffects(pSpell);

}
static MonoString* mono_GetSpellDataEffect(MonoString* query, int line)
{
	char buffer[MAX_STRING] = { 0 };
	char* cppString = mono_string_to_utf8(query);
	std::string str(cppString);
	strncpy_s(buffer, str.c_str(), sizeof(buffer));
	mono_free(cppString);

	eqlib::SPELL* pSpell = nullptr;
	MonoString* returnValue;
	IsNumber(buffer) ? pSpell = GetSpellByID(GetIntFromString(buffer, 0)) : pSpell = GetSpellByName(buffer);
	if (!pSpell)
	{
		returnValue = mono_string_new_wrapper("NULL");
		return returnValue;
	}
	char szBuff[MAX_STRING] = { 0 };
	char szTemp[MAX_STRING] = { 0 };
	int numberOfEffects = GetSpellNumEffects(pSpell);
	
	if (numberOfEffects > line && line>-1)
	{
		strcat_s(szBuff, ParseSpellEffect(pSpell, line, szTemp, sizeof(szTemp)));
		return  mono_string_new_wrapper(szBuff);
	}
	return  mono_string_new_wrapper("NULL");
}
static MonoString* mono_GetMQ2MonoVersion()
{	
	return mono_string_new_wrapper(version.c_str());
}
static MonoString* mono_GetFocusedWindowName()
{

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)
	{
		return mono_string_new_wrapper("IMGUI");
	}
	if (pWndMgr != nullptr && pWndMgr->FocusWindow != nullptr)
	{

		if (CXMLData* pXMLData = pWndMgr->FocusWindow->GetXMLData())
		{			
			MonoString* returnValue;
			
			returnValue = mono_string_new_wrapper(pXMLData->Name.c_str());
			return returnValue;
		}
	}
	return mono_string_new_wrapper("NULL");
}
static MonoString* mono_GetHoverWindowName()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)
	{
		return mono_string_new_wrapper("IMGUI");
	}
	if (pWndMgr != nullptr && pWndMgr->LastMouseOver != nullptr)
	{
		if(pWndMgr->LastMouseOver==0) return mono_string_new_wrapper("MAIN");
		
		if (CXMLData* pXMLData = pWndMgr->LastMouseOver->GetXMLData())
		{
			MonoString* returnValue;
			returnValue = mono_string_new_wrapper(pXMLData->Name.c_str());
			return returnValue;
		}
	}
	return mono_string_new_wrapper("NULL");
}
/// <summary>
/// seralize the spawn data to be sent into mono
/// why does v2 exist? because I am dumb :P had to get the correct size of some of the values and don't want to break existing things.
/// </summary>
static void mono_GetSpawns2()
{

	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];

		if (!domainInfo.m_OnSetSpawns)
		{
			//no spawn method set.
			return;
		}

		unsigned char buffer[1024];
		int bufferSize = 0;

		if (pSpawnManager)
		{
		
			
			auto spawn = pSpawnManager->FirstSpawn;
			//get a pointer to the buffer
			unsigned char* pBuffer = buffer;

			char tempBuffer[MAX_STRING];
			//assuming I cannot reuse this as there is no free once you call the invoke
			MonoArray* data = mono_array_new(currentDomain, mono_get_byte_class(), sizeof(buffer));

			while (spawn != nullptr)
			{
				//reset pointer
				pBuffer = buffer;
				//reset size
				bufferSize = 0;

				//fill the buffer
				int ID = spawn->SpawnID;
				memcpy(pBuffer, &ID, sizeof(ID));
				pBuffer += sizeof(ID);
				bufferSize += sizeof(ID);

				bool isAFK = (spawn->AFK != 0);
				memcpy(pBuffer, &isAFK, sizeof(isAFK));
				pBuffer += sizeof(isAFK);
				bufferSize += sizeof(isAFK);

				bool isAggressive = (spawn->PlayerState & 0x4 || spawn->PlayerState & 0x8);
				memcpy(pBuffer, &isAggressive, sizeof(isAggressive));
				pBuffer += sizeof(isAggressive);
				bufferSize += sizeof(isAggressive);

				bool isAnon = (spawn->Anon == 1);
				memcpy(pBuffer, &isAnon, sizeof(isAnon));
				pBuffer += sizeof(isAnon);
				bufferSize += sizeof(isAnon);

				int isBlind = spawn->Blind;
				memcpy(pBuffer, &isBlind, sizeof(isBlind));
				pBuffer += sizeof(isBlind);
				bufferSize += sizeof(isBlind);

				int BodyTypeID = GetBodyType(spawn);
				memcpy(pBuffer, &BodyTypeID, sizeof(BodyTypeID));
				pBuffer += sizeof(BodyTypeID);
				bufferSize += sizeof(BodyTypeID);


				const char* BodyDesc = GetBodyTypeDesc(BodyTypeID);
				int BodyDescLength = static_cast<int>(strlen(BodyDesc));
				//copy the size
				memcpy(pBuffer, &BodyDescLength, sizeof(BodyDescLength));
				pBuffer += sizeof(BodyDescLength);
				bufferSize += sizeof(BodyDescLength);
				//copy the data
				memcpy(pBuffer, BodyDesc, BodyDescLength);
				pBuffer += BodyDescLength;
				bufferSize += BodyDescLength;

				bool isBuyer = (spawn->Buyer != 0);
				memcpy(pBuffer, &isBuyer, sizeof(isBuyer));
				pBuffer += sizeof(isBuyer);
				bufferSize += sizeof(isBuyer);

				int ClassID = spawn->GetClass();
				memcpy(pBuffer, &ClassID, sizeof(ClassID));
				pBuffer += sizeof(ClassID);
				bufferSize += sizeof(ClassID);

				strcpy_s(tempBuffer, spawn->Name);
				CleanupName(tempBuffer, sizeof(tempBuffer), false, false);
				std::string tCleanName(tempBuffer);
				int tCleanNameLength = static_cast<int>(tCleanName.size());
				//copy the size
				memcpy(pBuffer, &tCleanNameLength, sizeof(tCleanNameLength));
				pBuffer += sizeof(tCleanNameLength);
				bufferSize += sizeof(tCleanNameLength);
				//copy the data
				memcpy(pBuffer, tCleanName.c_str(), tCleanNameLength);
				pBuffer += tCleanNameLength;
				bufferSize += tCleanNameLength;


				int conColorID = ConColor(spawn);
				memcpy(pBuffer, &conColorID, sizeof(conColorID));
				pBuffer += sizeof(conColorID);
				bufferSize += sizeof(conColorID);

				int currentEndurance = spawn->GetCurrentEndurance();
				memcpy(pBuffer, &currentEndurance, sizeof(currentEndurance));
				pBuffer += sizeof(currentEndurance);
				bufferSize += sizeof(currentEndurance);

				int64_t currentHps = spawn->HPCurrent;
				memcpy(pBuffer, &currentHps, sizeof(currentHps));
				pBuffer += sizeof(currentHps);
				bufferSize += sizeof(currentHps);

				int currentMana = spawn->GetCurrentMana();
				memcpy(pBuffer, &currentMana, sizeof(currentMana));
				pBuffer += sizeof(currentMana);
				bufferSize += sizeof(currentMana);

				bool dead = (spawn->StandState == STANDSTATE_DEAD);
				memcpy(pBuffer, &dead, sizeof(dead));
				pBuffer += sizeof(dead);
				bufferSize += sizeof(dead);

				std::string displayName(spawn->DisplayedName);
				int displayNameLength = static_cast<int>(displayName.length());
				//copy the size
				memcpy(pBuffer, &displayNameLength, sizeof(displayNameLength));
				pBuffer += sizeof(displayNameLength);
				bufferSize += sizeof(displayNameLength);
				//copy the data
				memcpy(pBuffer, displayName.c_str(), displayNameLength);
				pBuffer += displayNameLength;
				bufferSize += displayNameLength;

				bool ducking = (spawn->StandState == STANDSTATE_DUCK);
				memcpy(pBuffer, &ducking, sizeof(ducking));
				pBuffer += sizeof(ducking);
				bufferSize += sizeof(ducking);

				bool feigning = (spawn->StandState == STANDSTATE_FEIGN);
				memcpy(pBuffer, &feigning, sizeof(feigning));
				pBuffer += sizeof(feigning);
				bufferSize += sizeof(feigning);

				int genderId = spawn->GetGender();
				memcpy(pBuffer, &genderId, sizeof(genderId));
				pBuffer += sizeof(genderId);
				bufferSize += sizeof(genderId);

				bool GM = (spawn->GM != 0);
				memcpy(pBuffer, &GM, sizeof(GM));
				pBuffer += sizeof(GM);
				bufferSize += sizeof(GM);

				int64_t guildID = spawn->GuildID;
				memcpy(pBuffer, &guildID, sizeof(guildID));
				pBuffer += sizeof(guildID);
				bufferSize += sizeof(guildID);

				float heading = spawn->Heading * 0.703125f;
				memcpy(pBuffer, &heading, sizeof(heading));
				pBuffer += sizeof(heading);
				bufferSize += sizeof(heading);

				float height = spawn->AvatarHeight;
				memcpy(pBuffer, &height, sizeof(height));
				pBuffer += sizeof(height);
				bufferSize += sizeof(height);

				

				bool Invs = (spawn->HideMode != 0);
				memcpy(pBuffer, &Invs, sizeof(Invs));
				pBuffer += sizeof(Invs);
				bufferSize += sizeof(Invs);

				bool isSummoned = spawn->bSummoned;
				memcpy(pBuffer, &isSummoned, sizeof(isSummoned));
				pBuffer += sizeof(isSummoned);
				bufferSize += sizeof(isSummoned);

				int level = spawn->Level;
				memcpy(pBuffer, &level, sizeof(level));
				pBuffer += sizeof(level);
				bufferSize += sizeof(level);

				//TODO:possible bug? default to false for now
				// https://github.com/macroquest/macroquest/issues/632
				//bool levitation = (spawn->mPlayerPhysicsClient.Levitate == 2);
				bool levitation = false;
				memcpy(pBuffer, &levitation, sizeof(levitation));
				pBuffer += sizeof(levitation);
				bufferSize += sizeof(levitation);

				bool linkDead = spawn->Linkdead;
				memcpy(pBuffer, &linkDead, sizeof(linkDead));
				pBuffer += sizeof(linkDead);
				bufferSize += sizeof(linkDead);

				float look = spawn->CameraAngle;
				memcpy(pBuffer, &look, sizeof(look));
				pBuffer += sizeof(look);
				bufferSize += sizeof(look);

				int master = spawn->MasterID;
				memcpy(pBuffer, &master, sizeof(master));
				pBuffer += sizeof(master);
				bufferSize += sizeof(master);

				int maxEndurance = spawn->GetMaxEndurance();
				memcpy(pBuffer, &maxEndurance, sizeof(maxEndurance));
				pBuffer += sizeof(maxEndurance);
				bufferSize += sizeof(maxEndurance);

				auto spawnType = GetSpawnType(spawn);

				float maxRange = 0.0;
				if (spawnType != ITEM)
				{
					maxRange = get_melee_range(spawn, pControlledPlayer);
				}
				memcpy(pBuffer, &maxRange, sizeof(maxRange));
				pBuffer += sizeof(maxRange);
				bufferSize += sizeof(maxRange);

				float maxRanageTo = 0.0;
				if (spawnType != ITEM)
				{
					maxRanageTo = get_melee_range(pControlledPlayer, spawn);
				}
				memcpy(pBuffer, &maxRanageTo, sizeof(maxRanageTo));
				pBuffer += sizeof(maxRanageTo);
				bufferSize += sizeof(maxRanageTo);

				bool mount = false;
				if (spawn->Mount)
				{
					mount = true;
				}
				memcpy(pBuffer, &mount, sizeof(mount));
				pBuffer += sizeof(mount);
				bufferSize += sizeof(mount);

				bool moving = (fabs(spawn->SpeedRun) > 0.0f);
				memcpy(pBuffer, &moving, sizeof(moving));
				pBuffer += sizeof(moving);
				bufferSize += sizeof(moving);

				std::string name(spawn->Name);
				int tNameLength = static_cast<int>(name.size());
				//copy the size
				memcpy(pBuffer, &tNameLength, sizeof(tNameLength));
				pBuffer += sizeof(tNameLength);
				bufferSize += sizeof(tNameLength);
				//copy the data
				memcpy(pBuffer, name.c_str(), tNameLength);
				pBuffer += tNameLength;
				bufferSize += tNameLength;

				bool isNamed = IsNamed(spawn);
				memcpy(pBuffer, &isNamed, sizeof(isNamed));
				pBuffer += sizeof(isNamed);
				bufferSize += sizeof(isNamed);

				int64_t pctHPs = spawn->HPMax == 0 ? 0 : spawn->HPCurrent * 100 / spawn->HPMax;
				memcpy(pBuffer, &pctHPs, sizeof(pctHPs));
				pBuffer += sizeof(pctHPs);
				bufferSize += sizeof(pctHPs);

				int pctMana = 0;
				if (int maxmana = spawn->GetMaxMana())
				{
					if (maxmana > 0)
					{
						pctMana = spawn->GetCurrentMana() * 100 / maxmana;
					}
				}
				memcpy(pBuffer, &pctMana, sizeof(pctMana));
				pBuffer += sizeof(pctMana);
				bufferSize += sizeof(pctMana);

				int petID = spawn->PetID;
				memcpy(pBuffer, &petID, sizeof(petID));
				pBuffer += sizeof(petID);
				bufferSize += sizeof(petID);

				int playerState = spawn->PlayerState;
				memcpy(pBuffer, &playerState, sizeof(playerState));
				pBuffer += sizeof(playerState);
				bufferSize += sizeof(playerState);

				int raceID = spawn->GetRace();
				memcpy(pBuffer, &raceID, sizeof(raceID));
				pBuffer += sizeof(raceID);
				bufferSize += sizeof(raceID);

				std::string raceName(spawn->GetRaceString());
				int tRaceNameLength = static_cast<int>(raceName.size());
				//copy the size
				memcpy(pBuffer, &tRaceNameLength, sizeof(tRaceNameLength));
				pBuffer += sizeof(tRaceNameLength);
				bufferSize += sizeof(tRaceNameLength);
				//copy the data
				memcpy(pBuffer, raceName.c_str(), tRaceNameLength);
				pBuffer += tRaceNameLength;
				bufferSize += tRaceNameLength;

				bool rolePlaying = (spawn->Anon == 2);
				memcpy(pBuffer, &rolePlaying, sizeof(rolePlaying));
				pBuffer += sizeof(rolePlaying);
				bufferSize += sizeof(rolePlaying);

				bool sitting = (spawn->StandState == STANDSTATE_SIT);
				memcpy(pBuffer, &sitting, sizeof(sitting));
				pBuffer += sizeof(sitting);
				bufferSize += sizeof(sitting);

				bool sneaking = (spawn->Sneak);
				memcpy(pBuffer, &sneaking, sizeof(sneaking));
				pBuffer += sizeof(sneaking);
				bufferSize += sizeof(sneaking);

				bool standing = (spawn->StandState == STANDSTATE_STAND);
				memcpy(pBuffer, &standing, sizeof(standing));
				pBuffer += sizeof(standing);
				bufferSize += sizeof(standing);

				bool stunned = false;
				if (spawn->PlayerState & 0x20)
				{
					stunned = true;
				}
				memcpy(pBuffer, &stunned, sizeof(stunned));
				pBuffer += sizeof(stunned);
				bufferSize += sizeof(stunned);

				std::string suffix(spawn->Suffix);
				int tsuffixLength = static_cast<int>(suffix.size());
				//copy the size
				memcpy(pBuffer, &tsuffixLength, sizeof(tsuffixLength));
				pBuffer += sizeof(tsuffixLength);
				bufferSize += sizeof(tsuffixLength);
				//copy the data
				memcpy(pBuffer, suffix.c_str(), tsuffixLength);
				pBuffer += tsuffixLength;
				bufferSize += tsuffixLength;

				bool targetable = spawn->Targetable;
				memcpy(pBuffer, &targetable, sizeof(targetable));
				pBuffer += sizeof(targetable);
				bufferSize += sizeof(targetable);

				int targetoftargetID = spawn->TargetOfTarget;
				memcpy(pBuffer, &targetoftargetID, sizeof(targetoftargetID));
				pBuffer += sizeof(targetoftargetID);
				bufferSize += sizeof(targetoftargetID);

				bool trader = (spawn->Trader != 0);
				memcpy(pBuffer, &trader, sizeof(trader));
				pBuffer += sizeof(trader);
				bufferSize += sizeof(trader);

				std::string typeDescription(GetTypeDesc(spawnType));
				int ttypeDescriptionLength = static_cast<int>(typeDescription.size());
				//copy the size
				memcpy(pBuffer, &ttypeDescriptionLength, sizeof(ttypeDescriptionLength));
				pBuffer += sizeof(ttypeDescriptionLength);
				bufferSize += sizeof(ttypeDescriptionLength);
				//copy the data
				memcpy(pBuffer, typeDescription.c_str(), ttypeDescriptionLength);
				pBuffer += ttypeDescriptionLength;
				bufferSize += ttypeDescriptionLength;

				bool underwater = (spawn->UnderWater == LiquidType_Water);
				memcpy(pBuffer, &underwater, sizeof(underwater));
				pBuffer += sizeof(underwater);
				bufferSize += sizeof(underwater);

				float x = spawn->X;
				memcpy(pBuffer, &x, sizeof(x));
				pBuffer += sizeof(x);
				bufferSize += sizeof(x);

				float y = spawn->Y;
				memcpy(pBuffer, &y, sizeof(y));
				pBuffer += sizeof(y);
				bufferSize += sizeof(y);

				float z = spawn->Z;
				memcpy(pBuffer, &z, sizeof(z));
				pBuffer += sizeof(z);
				bufferSize += sizeof(z);
	
				//so distance calculations can be done
				float playerx = pControlledPlayer->X;
				memcpy(pBuffer, &playerx, sizeof(playerx));
				pBuffer += sizeof(playerx);
				bufferSize += sizeof(playerx);

				float playery = pControlledPlayer->Y;
				memcpy(pBuffer, &playery, sizeof(playery));
				pBuffer += sizeof(playery);
				bufferSize += sizeof(playery);

				float playerz = pControlledPlayer->Z;
				memcpy(pBuffer, &playerz, sizeof(playerz));
				pBuffer += sizeof(playerz);
				bufferSize += sizeof(playerz);
				//needed to tell NPC vs PC corpses
				int deity = spawn->Deity;
				memcpy(pBuffer, &deity, sizeof(deity));
				pBuffer += sizeof(deity);
				bufferSize += sizeof(deity);


				//copy over the array
				for (auto i = 0; i < bufferSize; i++) {
					mono_array_set(data, uint8_t, i, buffer[i]);
				}
				void* params[2] =
				{
					data,
					&bufferSize
				};
				mono_runtime_invoke(domainInfo.m_OnSetSpawns, domainInfo.m_classInstance, params, nullptr);

				spawn = spawn->GetNext();
			}
		}

		
	}

	
}
//original version i made for EMU, keeping for now for older versions of E3N that haven't updated yet.
//will eventually get rid of it, as its technically wrong.
static void mono_GetSpawns()
{

	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];

		if (!domainInfo.m_OnSetSpawns)
		{
			//no spawn method set.
			return;
		}

		unsigned char buffer[1024];
		int bufferSize = 0;

		if (pSpawnManager)
		{


			auto spawn = pSpawnManager->FirstSpawn;
			//get a pointer to the buffer
			unsigned char* pBuffer = buffer;

			char tempBuffer[MAX_STRING];
			//assuming I cannot reuse this as there is no free once you call the invoke
			MonoArray* data = mono_array_new(currentDomain, mono_get_byte_class(), sizeof(buffer));

			while (spawn != nullptr)
			{
				//reset pointer
				pBuffer = buffer;
				//reset size
				bufferSize = 0;

				//fill the buffer
				int ID = spawn->SpawnID;
				memcpy(pBuffer, &ID, sizeof(ID));
				pBuffer += sizeof(ID);
				bufferSize += sizeof(ID);

				bool isAFK = (spawn->AFK != 0);
				memcpy(pBuffer, &isAFK, sizeof(isAFK));
				pBuffer += sizeof(isAFK);
				bufferSize += sizeof(isAFK);

				bool isAggressive = (spawn->PlayerState & 0x4 || spawn->PlayerState & 0x8);
				memcpy(pBuffer, &isAggressive, sizeof(isAggressive));
				pBuffer += sizeof(isAggressive);
				bufferSize += sizeof(isAggressive);

				bool isAnon = (spawn->Anon == 1);
				memcpy(pBuffer, &isAnon, sizeof(isAnon));
				pBuffer += sizeof(isAnon);
				bufferSize += sizeof(isAnon);

				int isBlind = spawn->Blind;
				memcpy(pBuffer, &isBlind, sizeof(isBlind));
				pBuffer += sizeof(isBlind);
				bufferSize += sizeof(isBlind);

				int BodyTypeID = GetBodyType(spawn);
				memcpy(pBuffer, &BodyTypeID, sizeof(BodyTypeID));
				pBuffer += sizeof(BodyTypeID);
				bufferSize += sizeof(BodyTypeID);


				const char* BodyDesc = GetBodyTypeDesc(BodyTypeID);
				int BodyDescLength = static_cast<int>(strlen(BodyDesc));
				//copy the size
				memcpy(pBuffer, &BodyDescLength, sizeof(BodyDescLength));
				pBuffer += sizeof(BodyDescLength);
				bufferSize += sizeof(BodyDescLength);
				//copy the data
				memcpy(pBuffer, BodyDesc, BodyDescLength);
				pBuffer += BodyDescLength;
				bufferSize += BodyDescLength;

				bool isBuyer = (spawn->Buyer != 0);
				memcpy(pBuffer, &isBuyer, sizeof(isBuyer));
				pBuffer += sizeof(isBuyer);
				bufferSize += sizeof(isBuyer);

				int ClassID = spawn->GetClass();
				memcpy(pBuffer, &ClassID, sizeof(ClassID));
				pBuffer += sizeof(ClassID);
				bufferSize += sizeof(ClassID);

				strcpy_s(tempBuffer, spawn->Name);
				CleanupName(tempBuffer, sizeof(tempBuffer), false, false);
				std::string tCleanName(tempBuffer);
				int tCleanNameLength = static_cast<int>(tCleanName.size());
				//copy the size
				memcpy(pBuffer, &tCleanNameLength, sizeof(tCleanNameLength));
				pBuffer += sizeof(tCleanNameLength);
				bufferSize += sizeof(tCleanNameLength);
				//copy the data
				memcpy(pBuffer, tCleanName.c_str(), tCleanNameLength);
				pBuffer += tCleanNameLength;
				bufferSize += tCleanNameLength;


				int conColorID = ConColor(spawn);
				memcpy(pBuffer, &conColorID, sizeof(conColorID));
				pBuffer += sizeof(conColorID);
				bufferSize += sizeof(conColorID);

				int currentEndurance = spawn->GetCurrentEndurance();
				memcpy(pBuffer, &currentEndurance, sizeof(currentEndurance));
				pBuffer += sizeof(currentEndurance);
				bufferSize += sizeof(currentEndurance);

				int currentHps = static_cast<int>(spawn->HPCurrent);
				memcpy(pBuffer, &currentHps, sizeof(currentHps));
				pBuffer += sizeof(currentHps);
				bufferSize += sizeof(currentHps);

				int currentMana = spawn->GetCurrentMana();
				memcpy(pBuffer, &currentMana, sizeof(currentMana));
				pBuffer += sizeof(currentMana);
				bufferSize += sizeof(currentMana);

				bool dead = (spawn->StandState == STANDSTATE_DEAD);
				memcpy(pBuffer, &dead, sizeof(dead));
				pBuffer += sizeof(dead);
				bufferSize += sizeof(dead);

				std::string displayName(spawn->DisplayedName);
				int displayNameLength = static_cast<int>(displayName.length());
				//copy the size
				memcpy(pBuffer, &displayNameLength, sizeof(displayNameLength));
				pBuffer += sizeof(displayNameLength);
				bufferSize += sizeof(displayNameLength);
				//copy the data
				memcpy(pBuffer, displayName.c_str(), displayNameLength);
				pBuffer += displayNameLength;
				bufferSize += displayNameLength;

				bool ducking = (spawn->StandState == STANDSTATE_DUCK);
				memcpy(pBuffer, &ducking, sizeof(ducking));
				pBuffer += sizeof(ducking);
				bufferSize += sizeof(ducking);

				bool feigning = (spawn->StandState == STANDSTATE_FEIGN);
				memcpy(pBuffer, &feigning, sizeof(feigning));
				pBuffer += sizeof(feigning);
				bufferSize += sizeof(feigning);

				int genderId = spawn->GetGender();
				memcpy(pBuffer, &genderId, sizeof(genderId));
				pBuffer += sizeof(genderId);
				bufferSize += sizeof(genderId);

				bool GM = (spawn->GM != 0);
				memcpy(pBuffer, &GM, sizeof(GM));
				pBuffer += sizeof(GM);
				bufferSize += sizeof(GM);

				int guildID = static_cast<int>(spawn->GuildID);
				memcpy(pBuffer, &guildID, sizeof(guildID));
				pBuffer += sizeof(guildID);
				bufferSize += sizeof(guildID);

				float heading = spawn->Heading * 0.703125f;
				memcpy(pBuffer, &heading, sizeof(heading));
				pBuffer += sizeof(heading);
				bufferSize += sizeof(heading);

				float height = spawn->AvatarHeight;
				memcpy(pBuffer, &height, sizeof(height));
				pBuffer += sizeof(height);
				bufferSize += sizeof(height);



				bool Invs = (spawn->HideMode != 0);
				memcpy(pBuffer, &Invs, sizeof(Invs));
				pBuffer += sizeof(Invs);
				bufferSize += sizeof(Invs);

				bool isSummoned = spawn->bSummoned;
				memcpy(pBuffer, &isSummoned, sizeof(isSummoned));
				pBuffer += sizeof(isSummoned);
				bufferSize += sizeof(isSummoned);

				int level = spawn->Level;
				memcpy(pBuffer, &level, sizeof(level));
				pBuffer += sizeof(level);
				bufferSize += sizeof(level);

				//TODO:possible bug? default to false for now
				// https://github.com/macroquest/macroquest/issues/632
				//bool levitation = (spawn->mPlayerPhysicsClient.Levitate == 2);
				bool levitation = false;
				memcpy(pBuffer, &levitation, sizeof(levitation));
				pBuffer += sizeof(levitation);
				bufferSize += sizeof(levitation);

				bool linkDead = spawn->Linkdead;
				memcpy(pBuffer, &linkDead, sizeof(linkDead));
				pBuffer += sizeof(linkDead);
				bufferSize += sizeof(linkDead);

				float look = spawn->CameraAngle;
				memcpy(pBuffer, &look, sizeof(look));
				pBuffer += sizeof(look);
				bufferSize += sizeof(look);

				int master = spawn->MasterID;
				memcpy(pBuffer, &master, sizeof(master));
				pBuffer += sizeof(master);
				bufferSize += sizeof(master);

				int maxEndurance = spawn->GetMaxEndurance();
				memcpy(pBuffer, &maxEndurance, sizeof(maxEndurance));
				pBuffer += sizeof(maxEndurance);
				bufferSize += sizeof(maxEndurance);

				auto spawnType = GetSpawnType(spawn);

				float maxRange = 0.0;
				if (spawnType != ITEM)
				{
					maxRange = get_melee_range(spawn, pControlledPlayer);
				}
				memcpy(pBuffer, &maxRange, sizeof(maxRange));
				pBuffer += sizeof(maxRange);
				bufferSize += sizeof(maxRange);

				float maxRanageTo = 0.0;
				if (spawnType != ITEM)
				{
					maxRanageTo = get_melee_range(pControlledPlayer, spawn);
				}
				memcpy(pBuffer, &maxRanageTo, sizeof(maxRanageTo));
				pBuffer += sizeof(maxRanageTo);
				bufferSize += sizeof(maxRanageTo);

				bool mount = false;
				if (spawn->Mount)
				{
					mount = true;
				}
				memcpy(pBuffer, &mount, sizeof(mount));
				pBuffer += sizeof(mount);
				bufferSize += sizeof(mount);

				bool moving = (fabs(spawn->SpeedRun) > 0.0f);
				memcpy(pBuffer, &moving, sizeof(moving));
				pBuffer += sizeof(moving);
				bufferSize += sizeof(moving);

				std::string name(spawn->Name);
				int tNameLength = static_cast<int>(name.size());
				//copy the size
				memcpy(pBuffer, &tNameLength, sizeof(tNameLength));
				pBuffer += sizeof(tNameLength);
				bufferSize += sizeof(tNameLength);
				//copy the data
				memcpy(pBuffer, name.c_str(), tNameLength);
				pBuffer += tNameLength;
				bufferSize += tNameLength;

				bool isNamed = IsNamed(spawn);
				memcpy(pBuffer, &isNamed, sizeof(isNamed));
				pBuffer += sizeof(isNamed);
				bufferSize += sizeof(isNamed);

				int pctHPs = static_cast<int>(spawn->HPMax == 0 ? 0 : spawn->HPCurrent * 100 / spawn->HPMax);
				memcpy(pBuffer, &pctHPs, sizeof(pctHPs));
				pBuffer += sizeof(pctHPs);
				bufferSize += sizeof(pctHPs);

				int pctMana = 0;
				if (int maxmana = spawn->GetMaxMana())
				{
					if (maxmana > 0)
					{
						pctMana = spawn->GetCurrentMana() * 100 / maxmana;
					}
				}
				memcpy(pBuffer, &pctMana, sizeof(pctMana));
				pBuffer += sizeof(pctMana);
				bufferSize += sizeof(pctMana);

				int petID = spawn->PetID;
				memcpy(pBuffer, &petID, sizeof(petID));
				pBuffer += sizeof(petID);
				bufferSize += sizeof(petID);

				int playerState = spawn->PlayerState;
				memcpy(pBuffer, &playerState, sizeof(playerState));
				pBuffer += sizeof(playerState);
				bufferSize += sizeof(playerState);

				int raceID = spawn->GetRace();
				memcpy(pBuffer, &raceID, sizeof(raceID));
				pBuffer += sizeof(raceID);
				bufferSize += sizeof(raceID);

				std::string raceName(spawn->GetRaceString());
				int tRaceNameLength = static_cast<int>(raceName.size());
				//copy the size
				memcpy(pBuffer, &tRaceNameLength, sizeof(tRaceNameLength));
				pBuffer += sizeof(tRaceNameLength);
				bufferSize += sizeof(tRaceNameLength);
				//copy the data
				memcpy(pBuffer, raceName.c_str(), tRaceNameLength);
				pBuffer += tRaceNameLength;
				bufferSize += tRaceNameLength;

				bool rolePlaying = (spawn->Anon == 2);
				memcpy(pBuffer, &rolePlaying, sizeof(rolePlaying));
				pBuffer += sizeof(rolePlaying);
				bufferSize += sizeof(rolePlaying);

				bool sitting = (spawn->StandState == STANDSTATE_SIT);
				memcpy(pBuffer, &sitting, sizeof(sitting));
				pBuffer += sizeof(sitting);
				bufferSize += sizeof(sitting);

				bool sneaking = (spawn->Sneak);
				memcpy(pBuffer, &sneaking, sizeof(sneaking));
				pBuffer += sizeof(sneaking);
				bufferSize += sizeof(sneaking);

				bool standing = (spawn->StandState == STANDSTATE_STAND);
				memcpy(pBuffer, &standing, sizeof(standing));
				pBuffer += sizeof(standing);
				bufferSize += sizeof(standing);

				bool stunned = false;
				if (spawn->PlayerState & 0x20)
				{
					stunned = true;
				}
				memcpy(pBuffer, &stunned, sizeof(stunned));
				pBuffer += sizeof(stunned);
				bufferSize += sizeof(stunned);

				std::string suffix(spawn->Suffix);
				int tsuffixLength = static_cast<int>(suffix.size());
				//copy the size
				memcpy(pBuffer, &tsuffixLength, sizeof(tsuffixLength));
				pBuffer += sizeof(tsuffixLength);
				bufferSize += sizeof(tsuffixLength);
				//copy the data
				memcpy(pBuffer, suffix.c_str(), tsuffixLength);
				pBuffer += tsuffixLength;
				bufferSize += tsuffixLength;

				bool targetable = spawn->Targetable;
				memcpy(pBuffer, &targetable, sizeof(targetable));
				pBuffer += sizeof(targetable);
				bufferSize += sizeof(targetable);

				int targetoftargetID = spawn->TargetOfTarget;
				memcpy(pBuffer, &targetoftargetID, sizeof(targetoftargetID));
				pBuffer += sizeof(targetoftargetID);
				bufferSize += sizeof(targetoftargetID);

				bool trader = (spawn->Trader != 0);
				memcpy(pBuffer, &trader, sizeof(trader));
				pBuffer += sizeof(trader);
				bufferSize += sizeof(trader);

				std::string typeDescription(GetTypeDesc(spawnType));
				int ttypeDescriptionLength = static_cast<int>(typeDescription.size());
				//copy the size
				memcpy(pBuffer, &ttypeDescriptionLength, sizeof(ttypeDescriptionLength));
				pBuffer += sizeof(ttypeDescriptionLength);
				bufferSize += sizeof(ttypeDescriptionLength);
				//copy the data
				memcpy(pBuffer, typeDescription.c_str(), ttypeDescriptionLength);
				pBuffer += ttypeDescriptionLength;
				bufferSize += ttypeDescriptionLength;

				bool underwater = (spawn->UnderWater == LiquidType_Water);
				memcpy(pBuffer, &underwater, sizeof(underwater));
				pBuffer += sizeof(underwater);
				bufferSize += sizeof(underwater);

				float x = spawn->X;
				memcpy(pBuffer, &x, sizeof(x));
				pBuffer += sizeof(x);
				bufferSize += sizeof(x);

				float y = spawn->Y;
				memcpy(pBuffer, &y, sizeof(y));
				pBuffer += sizeof(y);
				bufferSize += sizeof(y);

				float z = spawn->Z;
				memcpy(pBuffer, &z, sizeof(z));
				pBuffer += sizeof(z);
				bufferSize += sizeof(z);

				//so distance calculations can be done
				float playerx = pControlledPlayer->X;
				memcpy(pBuffer, &playerx, sizeof(playerx));
				pBuffer += sizeof(playerx);
				bufferSize += sizeof(playerx);

				float playery = pControlledPlayer->Y;
				memcpy(pBuffer, &playery, sizeof(playery));
				pBuffer += sizeof(playery);
				bufferSize += sizeof(playery);

				float playerz = pControlledPlayer->Z;
				memcpy(pBuffer, &playerz, sizeof(playerz));
				pBuffer += sizeof(playerz);
				bufferSize += sizeof(playerz);
				//needed to tell NPC vs PC corpses
				int deity = spawn->Deity;
				memcpy(pBuffer, &deity, sizeof(deity));
				pBuffer += sizeof(deity);
				bufferSize += sizeof(deity);


				//copy over the array
				for (auto i = 0; i < bufferSize; i++) {
					mono_array_set(data, uint8_t, i, buffer[i]);
				}
				void* params[2] =
				{
					data,
					&bufferSize
				};
				mono_runtime_invoke(domainInfo.m_OnSetSpawns, domainInfo.m_classInstance, params, nullptr);

				spawn = spawn->GetNext();
			}
		}


	}


}
#pragma endregion Exposed methods to plugin





