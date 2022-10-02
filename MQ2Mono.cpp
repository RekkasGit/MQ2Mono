// MQ2Mono.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

#include <mq/Plugin.h>

//mono includes so all the mono_ methods and objects compile
#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>
#include <map>
#include <unordered_map>
PreSetup("MQ2Mono");
PLUGIN_VERSION(0.1);

/**
 * Avoid Globals if at all possible, since they persist throughout your program.
 * But if you must have them, here is the place to put them.
 */
 bool ShowMQ2MonoWindow = true;
 std::string monoDir;
 std::string monoRuntimeDir;
 bool initialized = false;

 //define methods exposde to the plugin to be executed
 void mono_Echo(MonoString* string);
 MonoString* mono_ParseTLO(MonoString* string);
 void mono_DoCommand(MonoString* string);
 void mono_Delay(int milliseconds);
 //IMGUI calls
 bool mono_ImGUI_Begin(MonoString* name, int flags);
 bool mono_ImGUI_Button(MonoString* name);
 void mono_ImGUI_End();
 boolean mono_ImGUI_Begin_OpenFlagGet(MonoString* name);
 void mono_ImGUI_Begin_OpenFlagSet(MonoString* name,bool open);
 //temp methods for now
 bool InitAppDomain(std::string appDomainName);
 bool UnloadAppDomain(std::string appDomainName, bool updateCollections);
 void UnloadAllAppDomains();

 struct monoAppDomainInfo
 {
	 std::string m_appDomainName;
	 //app domain we have created for e3
	 MonoDomain* m_appDomain=nullptr;
	 //core.dll information so we can bind to it
	 MonoAssembly* m_csharpAssembly = nullptr;
	 MonoImage* m_coreAssemblyImage = nullptr;
	 MonoClass* m_classInfo = nullptr;
	 MonoObject* m_classInstance = nullptr;
	 //methods that we call in C# if they are available
	 MonoMethod* m_OnPulseMethod = nullptr;
	 MonoMethod* m_OnWriteChatColor = nullptr;
	 MonoMethod* m_OnIncomingChat = nullptr;
	 MonoMethod* m_OnInit = nullptr;
	 MonoMethod* m_OnUpdateImGui = nullptr;
	 MonoMethod* m_OnStop = nullptr;
	 std::map<std::string, bool> m_IMGUI_OpenWindows;
	 std::map<std::string, bool> m_IMGUI_CheckboxValues;
	 std::map<std::string, bool> m_IMGUI_RadioButtonValues;
	 std::string m_CurrentWindow;
	 bool m_IMGUI_Open = true;
	 int m_delayTime = 0;//amount of time in milliseonds that was set by C#
	 std::chrono::steady_clock::time_point m_delayTimer = std::chrono::steady_clock::now(); //the time this was issued + m_delayTime
 };

 std::map<std::string, monoAppDomainInfo> monoAppDomains;
 std::map<MonoDomain*, std::string> monoAppDomainPtrToString;
 //used to keep a revolving list of who is valid to process. 
 std::deque<std::string> appDomainProcessQueue;


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
	//32vs64bit? necessary? or just have that version of the download folder? currently using 32bit
	/*char* options[1];
		options[0] = (char*)malloc(50 * sizeof(char));
		strcpy(options[0], "--arch=32");
		mono_jit_parse_options(1, (char**)options);
		*/

	//setup mono macro directory + runtime directory
	monoDir = std::filesystem::path(gPathMQRoot).u8string();
	#if !defined(_M_AMD64)
	monoRuntimeDir = monoDir + "\\resources\\mono\\32bit";
	#elif
	monoRuntimeDir = monoDir + "\\resources\\mono\\64bit";
	#endif

	monoDir = monoDir + "\\Mono";


	

	mono_set_dirs((monoRuntimeDir + "\\lib").c_str(), (monoRuntimeDir + "\\etc").c_str());
	_rootDomain = mono_jit_init("Mono_Domain");
	mono_domain_set(_rootDomain, false);

	//Namespace.Class::Method + a Function pointer with the actual definition
	//the namespace/class binding too is hard coded to namespace: MonoCore
	//Class: Core
	mono_add_internal_call("MonoCore.Core::mq_Echo", &mono_Echo);
	mono_add_internal_call("MonoCore.Core::mq_ParseTLO", &mono_ParseTLO);
	mono_add_internal_call("MonoCore.Core::mq_DoCommand", &mono_DoCommand);
	mono_add_internal_call("MonoCore.Core::mq_Delay", &mono_Delay);

	//I'm GUI stuff
	mono_add_internal_call("MonoCore.Core::imgui_Begin", &mono_ImGUI_Begin);
	mono_add_internal_call("MonoCore.Core::imgui_Button", &mono_ImGUI_Button);
	mono_add_internal_call("MonoCore.Core::imgui_End", &mono_ImGUI_End);
	mono_add_internal_call("MonoCore.Core::imgui_Begin_OpenFlagSet", &mono_ImGUI_Begin_OpenFlagSet);
	mono_add_internal_call("MonoCore.Core::imgui_Begin_OpenFlagGet", &mono_ImGUI_Begin_OpenFlagGet);

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

		if (updateCollections)
		{
			monoAppDomains.erase(appDomainName);
			monoAppDomainPtrToString.erase(domainToUnload);
			//remove from the process queue
			int count = 0;
			int processCount = appDomainProcessQueue.size();

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
		int processCount = appDomainProcessQueue.size();

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
	MonoMethod* OnUpdateImGui;
	std::map<std::string, bool> IMGUI_OpenWindows;

	//everything below needs to be moved out to a per application run
	mono_domain_set(appDomain, false);


	std::string fileName = "Core.dll";
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
	std::filesystem::copy(assemblypath, shadowDirectory, std::filesystem::copy_options::overwrite_existing);

	
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
	WriteChatf("\arMQ2Mono\au::\at Command issued.");
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
	

}
PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("MQ2Mono::Initializing version %f", MQ2Version);
	if (!initialized)
	{
		InitMono();

	}
	AddCommand("/mono", MonoCommand, 0, 1, 1);
	// Examples:
	// AddCommand("/mycommand", MyCommand);
	// AddXMLFile("MQUI_MyXMLFile.xml");
	// AddMQ2Data("mytlo", MyTLOData);



}
/**
 * @fn ShutdownPlugin
 *
 * This is called once when the plugin has been asked to shutdown.  The plugin has
 * not actually shut down until this completes.
 */
PLUGIN_API void ShutdownPlugin()
{
	
	DebugSpewAlways("MQ2Mono::Shutting down");
	// Examples:
	// RemoveCommand("/mycommand");
	// RemoveXMLFile("MQUI_MyXMLFile.xml");
	// RemoveMQ2Data("mytlo");
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

	if (appDomainProcessQueue.size() < 1) return;

	std::chrono::steady_clock::time_point proccessingTimer = std::chrono::steady_clock::now(); //the time this was issued + m_delayTime



	int gameState = GetGameState();

	// If we have left the world and have apps running, unload them all
	if (gameState == GAMESTATE_CHARSELECT || gameState == GAMESTATE_PRECHARSELECT)
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
					mono_domain_set(i->second.m_appDomain, false);
					mono_runtime_invoke(i->second.m_OnPulseMethod, i->second.m_classInstance, nullptr, nullptr);
				}
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
	if (monoAppDomains.size() == 0) return;

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
	for (auto i : monoAppDomains)
	{
		//Call the main method in this code
		if (i.second.m_appDomain && i.second.m_OnIncomingChat)
		{
			mono_domain_set(i.second.m_appDomain, false);
			MonoString* monoLine = mono_string_new(i.second.m_appDomain, Line);
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
	// DebugSpewAlways("MQ2Mono::OnUnloadPlugin(%s)", Name);
	/// <summary>
	/// for future me
	/// https://github.com/mono/mono/issues/13557
	/// In general, to reload assemblies you should be running the assembly in a new appdomain 
	/// (this will affect how you organize your application) and then unloading the domain to unload the old assemblies. 
	/// Anything else is not supported by Mono and you may have unpredictable results.
	/// </summary>
	/// 
	UnloadAllAppDomains();
	mono_jit_cleanup(mono_get_root_domain());
	
}
#pragma region

//TODO: change all m_ variables to a collection of them
static void mono_ImGUI_Begin_OpenFlagSet(MonoString* name, bool open)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);

	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto & domainInfo = monoAppDomains[key];
		if (domainInfo.m_IMGUI_OpenWindows.find(str) == domainInfo.m_IMGUI_OpenWindows.end())
		{
			//key doesn't exist, add it
			domainInfo.m_IMGUI_OpenWindows[str] = true;
		}
		domainInfo.m_IMGUI_OpenWindows[str] = open;
		//put updates back
	
	}

	

}
static boolean mono_ImGUI_Begin_OpenFlagGet(MonoString* name)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];
		if (domainInfo.m_IMGUI_OpenWindows.find(str) == domainInfo.m_IMGUI_OpenWindows.end())
		{
			//key doesn't exist, add it
			domainInfo.m_IMGUI_OpenWindows[str] = true;
		}
		return domainInfo.m_IMGUI_OpenWindows[str];
	}
	return false;

}
//define methods exposde to the plugin to be executed
static bool mono_ImGUI_Begin(MonoString* name, int flags)
{

	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		//pointer to the value in the map
		auto& domainInfo = monoAppDomains[key];

		domainInfo.m_CurrentWindow = str;
		if (domainInfo.m_IMGUI_OpenWindows.find(str) == domainInfo.m_IMGUI_OpenWindows.end())
		{
			//key doesn't exist, add it
			domainInfo.m_IMGUI_OpenWindows[str] = true;
		}

		return ImGui::Begin(str.c_str(), &domainInfo.m_IMGUI_OpenWindows[str], flags);
	}
	return false;
}


static bool mono_ImGUI_Button(MonoString* name)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::Button(str.c_str());
}

static void mono_ImGUI_End()
{
	ImGui::End();
}

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
	WriteChatf("%s", str.c_str());
	mono_free(cppString);
}
static void mono_DoCommand(MonoString* text)
{
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	mono_free(cppString);
	HideDoCommand(pLocalPlayer, str.c_str(), false);

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

#pragma endregion Exposed methods to plugin





