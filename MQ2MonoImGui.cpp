#include "MQ2MonoImGui.h"
#include "MQ2MonoShared.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "imgui/misc/cpp/imgui_stdlib.h"
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <mq/imgui/Widgets.h>

// All ImGui wrapper function definitions moved out of MQ2Mono.cpp
// These rely on globals declared in MQ2MonoShared.h and defined in MQ2Mono.cpp

void mono_ImGUI_TableNextRowEx(int row_flags, float min_row_height)
{
	ImGui::TableNextRow((ImGuiTableRowFlags)row_flags, min_row_height);
}
float mono_ImGUI_GetCursorPosX()
{
	return ImGui::GetCursorPosX();
}

void mono_ImGUI_SetCursorPosX(float x)
{
	ImGui::SetCursorPosX(x);
}

float mono_ImGUI_GetCursorPosY()
{
	return ImGui::GetCursorPosY();
}

void mono_ImGUI_SetCursorPosY(float y)
{
	ImGui::SetCursorPosY(y);
}

float mono_ImGUI_GetCursorScreenPosX()
{
	return ImGui::GetCursorScreenPos().x;
}

float mono_ImGUI_GetCursorScreenPosY()
{
	return ImGui::GetCursorScreenPos().y;
}

void mono_ImGUI_Begin_OpenFlagSet(MonoString* name, bool open)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);

	MonoDomain* currentDomain = mono_domain_get();
	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		auto& domainInfo = monoAppDomains[key];
		if (domainInfo.m_IMGUI_OpenWindows.find(str) == domainInfo.m_IMGUI_OpenWindows.end())
		{
			domainInfo.m_IMGUI_OpenWindows[str] = open;
		}
		domainInfo.m_IMGUI_OpenWindows[str] = open;
	}
}

boolean mono_ImGUI_Begin_OpenFlagGet(MonoString* name)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		auto& domainInfo = monoAppDomains[key];
		if (domainInfo.m_IMGUI_OpenWindows.find(str) == domainInfo.m_IMGUI_OpenWindows.end())
		{
			domainInfo.m_IMGUI_OpenWindows[str] = true;
		}
		return domainInfo.m_IMGUI_OpenWindows[str];
	}
	return false;
}

bool mono_ImGUI_Begin(MonoString* name, int flags)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	MonoDomain* currentDomain = mono_domain_get();

	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		auto& domainInfo = monoAppDomains[key];

		domainInfo.m_CurrentWindow = str;
		if (domainInfo.m_IMGUI_OpenWindows.find(str) == domainInfo.m_IMGUI_OpenWindows.end())
		{
			domainInfo.m_IMGUI_OpenWindows[str] = true;
		}

		return ImGui::Begin(str.c_str(), &domainInfo.m_IMGUI_OpenWindows[str], flags);
	}
	return false;
}
bool mono_ImGUI_InvisibleButton(MonoString* name, float width, float height, int flags)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::InvisibleButton(str.c_str(), ImVec2(width, height), flags);
}
bool mono_ImGUI_Button(MonoString* name)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::Button(str.c_str());
}

bool mono_ImGUI_ButtonEx(MonoString* name, float width, float height)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::Button(str.c_str(), ImVec2(width, height));
}

bool mono_ImGUI_SmallButton(MonoString* name)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::SmallButton(str.c_str());
}

void mono_ImGUI_End()
{
	ImGui::End();
}

void mono_ImGUI_Text(MonoString* text)
{
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	mono_free(cppString);
	ImGui::TextUnformatted(str.c_str());
}

void mono_ImGUI_TextUnformatted(MonoString* text)
{
	char* cppString = mono_string_to_utf8(text);
	std::string str(cppString);
	mono_free(cppString);
	ImGui::TextUnformatted(str.c_str());
}

void mono_ImGUI_Separator()
{
	ImGui::Separator();
}

void mono_ImGUI_SameLine()
{
	ImGui::SameLine();
}

void mono_ImGUI_SameLineEx(float offset_from_start_x, float spacing)
{
	ImGui::SameLine(offset_from_start_x, spacing);
}

void mono_ImGUI_Checkbox_Clear(MonoString* id)
{
	char* cppString = mono_string_to_utf8(id);
	std::string str(cppString);
	mono_free(cppString);

	MonoDomain* currentDomain = mono_domain_get();
	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		auto& domainInfo = monoAppDomains[key];
		
		auto it = domainInfo.m_IMGUI_CheckboxValues.find(str);
		if (it != domainInfo.m_IMGUI_CheckboxValues.end())
		{
			domainInfo.m_IMGUI_CheckboxValues.erase(it);
		}
	}
}

bool mono_ImGUI_Checkbox_Get(MonoString* id)
{
	char* cppString = mono_string_to_utf8(id);
	std::string str(cppString);
	mono_free(cppString);
	MonoDomain* currentDomain = mono_domain_get();
	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		auto& domainInfo = monoAppDomains[key];
		
		auto it = domainInfo.m_IMGUI_CheckboxValues.find(str);
		if (it != domainInfo.m_IMGUI_CheckboxValues.end())
		{
			return it->second;
		}
	}
	return false;
}

bool mono_ImGUI_Checkbox(MonoString* name, bool currentValue)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);

	MonoDomain* currentDomain = mono_domain_get();
	if (currentDomain)
	{
		std::string key = monoAppDomainPtrToString[currentDomain];
		auto& domainInfo = monoAppDomains[key];
		
		domainInfo.m_IMGUI_CheckboxValues[str] = currentValue;
		auto it = domainInfo.m_IMGUI_CheckboxValues.find(str);
		
		if (ImGui::Checkbox(str.c_str(), &it->second))
		{
			return true;
		}
		return false;
	}
	return false;
}

bool mono_ImGUI_BeginTabBar(MonoString* name)
{
	char* cppString = mono_string_to_utf8(name);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::BeginTabBar(str.c_str());
}

void mono_ImGUI_EndTabBar()
{
	ImGui::EndTabBar();
}

bool mono_ImGUI_BeginTabItem(MonoString* label)
{
	char* cppString = mono_string_to_utf8(label);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::BeginTabItem(str.c_str());
}

void mono_ImGUI_EndTabItem()
{
	ImGui::EndTabItem();
}

bool mono_ImGUI_BeginTableSimple(MonoString* id, int columns, int flags)
{
	if (!id) return false;
	char* sid = mono_string_to_utf8(id);
	std::string strID(sid);
	mono_free(sid);
	bool result = ImGui::BeginTable(strID.c_str(), columns, flags);
	return result;
}

bool mono_ImGUI_BeginTable(MonoString* id, int columns, int flags, float outer_width,float outer_height)
{
	if (!id) return false;
	char* sid = mono_string_to_utf8(id);
	std::string strID(sid);
	mono_free(sid);
	bool result = ImGui::BeginTable(strID.c_str(), columns, (ImGuiTableFlags)flags, ImVec2(outer_width, outer_height));
	return result;
}

void mono_ImGUI_TableSetColumnIndex(int index)
{
	ImGui::TableSetColumnIndex(index);
}

void mono_ImGUI_EndTable()
{
	ImGui::EndTable();
}
void mono_ImGUI_TableSetupColumn(MonoString* label, int flags, float init_width)
{
	if (!label) return;
	char* slabel = mono_string_to_utf8(label); 
	std::string strLabel(slabel);
	ImGui::TableSetupColumn(strLabel.c_str(), (ImGuiTableColumnFlags)flags, init_width);
	mono_free(slabel);
}
void mono_ImGUI_TableSetupColumn_Default(MonoString* label)
{
	if (!label) return;
	char* slabel = mono_string_to_utf8(label);
	std::string strLabel(slabel);
	mono_free(slabel);
	ImGui::TableSetupColumn(strLabel.c_str());
	
}
bool mono_ImGUI_ColorPicker4(MonoString* label, int r, int g, int b, int a, int flags)
{

	char* labelPtr = mono_string_to_utf8(label);
	std::string labelStr(labelPtr);
	mono_free(labelPtr);

	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;

	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];

	auto it = domainInfo.m_IMGUI_InputColorValues.find(labelStr);
	if (it == domainInfo.m_IMGUI_InputColorValues.end())
	{
		//didn't find it, insert it
		domainInfo.m_IMGUI_InputColorValues[labelStr]={static_cast<float>(r)/255,static_cast<float>(g)/255,static_cast<float>(b)/255,static_cast<float>(a)/255 };
	
	}
	
	return ImGui::ColorPicker4(labelStr.c_str(), it->second.data(), (ImGuiColorEditFlags)flags);

}
bool mono_ImGUI_ColorPicker4_Float(MonoString* label, float r, float g, float b, float a, int flags)
{

	char* labelPtr = mono_string_to_utf8(label);
	std::string labelStr(labelPtr);
	mono_free(labelPtr);

	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;

	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];

	auto it = domainInfo.m_IMGUI_InputColorValues.find(labelStr);
	if (it == domainInfo.m_IMGUI_InputColorValues.end())
	{
		//didn't find it, insert it
		domainInfo.m_IMGUI_InputColorValues[labelStr] = { r,g,b,a };
	}
	return ImGui::ColorPicker4(labelStr.c_str(), it->second.data(), (ImGuiColorEditFlags)flags);

}
void mono_ImGUI_ColorPicker_Clear(MonoString* label)
{
	char* labelPtr = mono_string_to_utf8(label);
	std::string labelStr(labelPtr);
	mono_free(labelPtr);

	MonoDomain* currentDomain = mono_domain_get();

	if (!currentDomain) return;
	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];
	auto it = domainInfo.m_IMGUI_InputColorValues.find(labelPtr);
	if (it != domainInfo.m_IMGUI_InputColorValues.end())
	{
		domainInfo.m_IMGUI_InputColorValues.erase(it);
	}
}
MonoArray* mono_ImGUI_ColorPicker_GetRGBA(MonoString* label)
{
	char* labelPtr = mono_string_to_utf8(label);
	std::string labelStr(labelPtr);
	mono_free(labelPtr);

	MonoDomain* currentDomain = mono_domain_get();

	MonoClass* intClass = mono_get_int32_class();
	int arraySize = 4;

	MonoArray* monoArray = mono_array_new(currentDomain, intClass, arraySize);

	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];

	auto it = domainInfo.m_IMGUI_InputColorValues.find(labelStr);
	if (it != domainInfo.m_IMGUI_InputColorValues.end())
	{
		mono_array_set(monoArray, int, 0, static_cast<int>(it->second[0]*255));
		mono_array_set(monoArray, int, 1, static_cast<int>(it->second[1]*255));
		mono_array_set(monoArray, int, 2, static_cast<int>(it->second[2]*255));
		mono_array_set(monoArray, int, 3, static_cast<int>(it->second[3]*255));
	}
	return monoArray;
}

MonoArray* mono_ImGUI_GetStyleColorVec4(int flag)
{
	ImVec4 result = ImGui::GetStyleColorVec4((ImGuiCol)flag);
	MonoDomain* currentDomain = mono_domain_get();
	MonoClass* floatClass = mono_get_single_class();
	int arraySize = 4;
	MonoArray* monoArray = mono_array_new(currentDomain, floatClass, arraySize);
	mono_array_set(monoArray, float, 0, result.x);
	mono_array_set(monoArray, float, 1, result.y);
	mono_array_set(monoArray, float, 2, result.z);
	mono_array_set(monoArray, float, 3, result.w);
	return monoArray;

}

MonoArray* mono_ImGUI_ColorPicker_GetRGBA_Float(MonoString* label)
{
	char* labelPtr = mono_string_to_utf8(label);
	std::string labelStr(labelPtr);
	mono_free(labelPtr);

	MonoDomain* currentDomain = mono_domain_get();

	MonoClass* floatClass = mono_get_single_class();
	int arraySize = 4;

	MonoArray* monoArray = mono_array_new(currentDomain, floatClass, arraySize);

	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];

	auto it = domainInfo.m_IMGUI_InputColorValues.find(labelStr);
	if (it != domainInfo.m_IMGUI_InputColorValues.end())
	{
		mono_array_set(monoArray, float, 0, it->second[0]);
		mono_array_set(monoArray, float, 1, it->second[1]);
		mono_array_set(monoArray, float, 2, it->second[2]);
		mono_array_set(monoArray, float, 3, it->second[3]);
	}
	return monoArray;
}
void mono_ImGUI_TableHeadersRow()
{
	ImGui::TableHeadersRow();
}
MonoString* mono_ImGUI_TableGetColumnName(int column)
{
	const char * value = ImGui::TableGetColumnName(column);
	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return mono_string_new(mono_get_root_domain(), "");
	return mono_string_new(currentDomain, value);
}
void mono_ImGUI_TableHeader(MonoString* label)
{
	if (!label) return;
	char* slabel = mono_string_to_utf8(label);
	std::string strLabel(slabel);
	mono_free(slabel);
	ImGui::TableHeader(strLabel.c_str());
}
void mono_ImGUI_TableNextRow()
{
	ImGui::TableNextRow();
}
bool mono_ImGUI_TableNextColumn()
{
	return ImGui::TableNextColumn();
}

void mono_ImGUI_TextColored(float r, float g, float b, float a, MonoString* text)
{
	if (!text) return;
	char* stext = mono_string_to_utf8(text);
	ImGui::TextColored(ImVec4(r, g, b, a), "%s", stext);
	mono_free(stext);
}
void mono_ImGUI_PushStyleColor(int which, float r, float g, float b, float a)
{
	ImGui::PushStyleColor((ImGuiCol)which, ImVec4(r, g, b, a));
}
void mono_ImGUI_PopStyleColor(int count)
{
	ImGui::PopStyleColor(count);
}

// Style variables
void mono_ImGUI_PushStyleVarFloat(int which, float value)
{
	ImGui::PushStyleVar((ImGuiStyleVar)which, value);
}

void mono_ImGUI_PushStyleVarVec2(int which, float x, float y)
{
	ImGui::PushStyleVar((ImGuiStyleVar)which, ImVec2(x, y));
}

void mono_ImGUI_PopStyleVar(int count)
{
	ImGui::PopStyleVar(count);
}




float mono_ImGUI_GetStyleVarFloat(int which)
{
	ImGuiStyle& style = ImGui::GetStyle();
	switch ((ImGuiStyleVar)which)
	{
	case ImGuiStyleVar_Alpha: return style.Alpha;
	case ImGuiStyleVar_DisabledAlpha: return style.DisabledAlpha;
	case ImGuiStyleVar_WindowRounding: return style.WindowRounding;
	case ImGuiStyleVar_WindowBorderSize: return style.WindowBorderSize;
	case ImGuiStyleVar_ChildRounding: return style.ChildRounding;
	case ImGuiStyleVar_ChildBorderSize: return style.ChildBorderSize;
	case ImGuiStyleVar_PopupRounding: return style.PopupRounding;
	case ImGuiStyleVar_PopupBorderSize: return style.PopupBorderSize;
	case ImGuiStyleVar_FrameRounding: return style.FrameRounding;
	case ImGuiStyleVar_FrameBorderSize: return style.FrameBorderSize;
	case ImGuiStyleVar_IndentSpacing: return style.IndentSpacing;
	case ImGuiStyleVar_ScrollbarSize: return style.ScrollbarSize;
	case ImGuiStyleVar_ScrollbarRounding: return style.ScrollbarRounding;
	case ImGuiStyleVar_GrabMinSize: return style.GrabMinSize;
	case ImGuiStyleVar_GrabRounding: return style.GrabRounding;
	case ImGuiStyleVar_TabRounding: return style.TabRounding;
	default: return 0.0f;
	}
}

void mono_ImGUI_GetStyleVarVec2(int which, float* x, float* y)
{
	if (!x || !y) return;
	ImGuiStyle& style = ImGui::GetStyle();
	switch ((ImGuiStyleVar)which)
	{
	case ImGuiStyleVar_WindowPadding: *x = style.WindowPadding.x; *y = style.WindowPadding.y; break;
	case ImGuiStyleVar_WindowMinSize: *x = style.WindowMinSize.x; *y = style.WindowMinSize.y; break;
	case ImGuiStyleVar_WindowTitleAlign: *x = style.WindowTitleAlign.x; *y = style.WindowTitleAlign.y; break;
	case ImGuiStyleVar_FramePadding: *x = style.FramePadding.x; *y = style.FramePadding.y; break;
	case ImGuiStyleVar_ItemSpacing: *x = style.ItemSpacing.x; *y = style.ItemSpacing.y; break;
	case ImGuiStyleVar_ItemInnerSpacing: *x = style.ItemInnerSpacing.x; *y = style.ItemInnerSpacing.y; break;
	case ImGuiStyleVar_CellPadding: *x = style.CellPadding.x; *y = style.CellPadding.y; break;
	case ImGuiStyleVar_ButtonTextAlign: *x = style.ButtonTextAlign.x; *y = style.ButtonTextAlign.y; break;
	case ImGuiStyleVar_SelectableTextAlign: *x = style.SelectableTextAlign.x; *y = style.SelectableTextAlign.y; break;
	default: *x = 0.0f; *y = 0.0f; break;
	}
}

void mono_ImGUI_TextWrapped(MonoString* text)
{
	if (!text) return;
	char* stext = mono_string_to_utf8(text);
	ImGui::TextWrapped("%s", stext);
	mono_free(stext);
}

void mono_ImGUI_PushTextWrapPos(float wrap_local_pos_x)
{
	ImGui::PushTextWrapPos(wrap_local_pos_x);
}
void mono_ImGUI_PopTextWrapPos()
{
	ImGui::PopTextWrapPos();
}
void mono_ImGUI_SetNextWindowSizeConstraints(float min_w, float min_h, float max_w, float max_h)
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(min_w, min_h), ImVec2(max_w, max_h));
}

void mono_ImGUI_SetNextWindowBgAlpha(float alpha)
{
	ImGui::SetNextWindowBgAlpha(alpha);
}

void mono_ImGUI_SetNextWindowSize(float width, float height)
{
	ImGui::SetNextWindowSize(ImVec2(width, height));
}

void mono_ImGUI_SetNextWindowSizeWithCond(float width, float height, int cond)
{
	ImGui::SetNextWindowSize(ImVec2(width, height), (ImGuiCond)cond);
}

bool mono_ImGUI_IsWindowHovered()
{
	return ImGui::IsWindowHovered();
}

float mono_ImGUI_GetWindowWidth()
{
	return ImGui::GetWindowWidth();
}

float mono_ImGUI_GetWindowHeight()
{
	return ImGui::GetWindowHeight();
}

bool mono_ImGUI_IsMouseClicked(int button)
{
	return ImGui::IsMouseClicked(button);
}

void mono_ImGUI_PushItemWidth(float width)
{
	ImGui::PushItemWidth(width);
}

void mono_ImGUI_PopItemWidth(float width)
{
	ImGui::PopItemWidth();
}

void mono_ImGUI_PushID(int id)
{
	ImGui::PushID(id);
}

void mono_ImGUI_PopID()
{
	ImGui::PopID();
}

void mono_ImGUI_SetNextWindowFocus()
{
	ImGui::SetNextWindowFocus();
}

void mono_ImGUI_SetNextWindowPos(float x, float y,int flags, float xpiv, float ypiv)
{
	ImGui::SetNextWindowPos(ImVec2(x,y), flags, ImVec2(xpiv, ypiv));
}

float mono_ImGUI_GetWindowPosX()
{
	return ImGui::GetWindowPos().x;
}
float mono_ImGUI_GetWindowPosY()
{
	return ImGui::GetWindowPos().y;
}

float mono_ImGUI_GetWindowSizeX()
{
	return ImGui::GetWindowSize().x;
}
float mono_ImGUI_GetWindowSizeY()
{
	return ImGui::GetWindowSize().y;
}


float mono_ImGUI_GetWindowContentRegionMin_X()
{
	return ImGui::GetWindowContentRegionMin().x;
}
float mono_ImGUI_GetWindowContentRegionMin_Y()
{
	return ImGui::GetWindowContentRegionMin().y;
}
float mono_ImGUI_GetWindowContentRegionMax_X()
{
	return ImGui::GetWindowContentRegionMax().x;
}
float mono_ImGUI_GetWindowContentRegionMax_Y()
{
	return ImGui::GetWindowContentRegionMax().y;
}


bool mono_ImGUI_BeginChild(MonoString* id, float width, float height, int child_flags, int window_flags)
{
	char* cppString = mono_string_to_utf8(id);
	std::string str(cppString);
	mono_free(cppString);
	ImVec2 size(width, height);
	return ImGui::BeginChild(str.c_str(), size, (ImGuiChildFlags)child_flags, (ImGuiWindowFlags)window_flags);
}
bool mono_ImGUI_BeginChildResize(MonoString* id, float width, float height, int child_flags, int window_flags)
{
	char* cppString = mono_string_to_utf8(id);
	std::string str(cppString);
	mono_free(cppString);
	ImVec2 size(width, height);
	return ImGui::BeginChild(str.c_str(), size, (ImGuiChildFlags)child_flags, (ImGuiWindowFlags)window_flags);
}

void mono_ImGUI_EndChild()
{
	ImGui::EndChild();
}

bool mono_ImGUI_Selectable(MonoString* label, bool selected)
{
	char* cppString = mono_string_to_utf8(label);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::Selectable(str.c_str(), selected);
}

bool mono_ImGUI_Selectable_WithFlags(MonoString* label, bool selected,int flags)
{
	char* cppString = mono_string_to_utf8(label);
	std::string str(cppString);
	mono_free(cppString);
	return ImGui::Selectable(str.c_str(), selected,(ImGuiSelectableFlags_)flags);
}

float mono_ImGUI_GetContentRegionAvailX()
{
	return ImGui::GetContentRegionAvail().x;
}

float mono_ImGUI_GetContentRegionAvailY()
{
	return ImGui::GetContentRegionAvail().y;
}

bool mono_ImGUI_BeginCombo(MonoString* label, MonoString* preview, int flags)
{
	char* lC = mono_string_to_utf8(label);
	std::string lStr(lC);
	mono_free(lC);
	char* pC = mono_string_to_utf8(preview);
	std::string pStr(pC ? pC : "");
	if (pC) mono_free(pC);
	return ImGui::BeginCombo(lStr.c_str(), pStr.c_str(), (ImGuiComboFlags)flags);
}

void mono_ImGUI_EndCombo()
{
	ImGui::EndCombo();
}
MonoArray* mono_ImGUI_CalcTextSize(MonoString* input)
{
	MonoClass* singleClass = mono_get_single_class();
	int arraySize = 2;
	MonoDomain* currentDomain = mono_domain_get();
	MonoArray* monoArray = mono_array_new(currentDomain, singleClass, arraySize);

	if (input)
	{
		char* strptr = mono_string_to_utf8(input);
		std::string inputStr(strptr);
		mono_free(strptr);
		ImVec2 textSize = ImGui::CalcTextSize(inputStr.c_str());
		mono_array_set(monoArray, float, 0, textSize.x);
		mono_array_set(monoArray, float, 1, textSize.y);

	}
	return monoArray;
}
float mono_ImGUI_CalcTextSizeX(MonoString* input)
{
	if (!input) return false;
	char* strptr = mono_string_to_utf8(input);
	std::string inputStr(strptr);
	if (strptr) mono_free(strptr);

	ImVec2 textSize = ImGui::CalcTextSize(inputStr.c_str());
	return textSize.x;
}

bool mono_ImGUI_RightAlignButton(MonoString* name)
{
	if (!name) return false;
	char* cpp = mono_string_to_utf8(name);
	std::string label(cpp ? cpp : "");
	if (cpp) mono_free(cpp);

	ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
	float buttonWidth = textSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
	float avail = ImGui::GetContentRegionAvail().x;
	if (avail > buttonWidth)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - buttonWidth));
	}
	return ImGui::Button(label.c_str());
}

bool mono_ImGUI_InputText_Clear(MonoString* id)
{
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC);
	mono_free(idC);
	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;
	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];
	auto it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
	if (it != domainInfo.m_IMGUI_InputTextValues.end())
	{
		domainInfo.m_IMGUI_InputTextValues.erase(it);
		return true;
	}
	return false;
}
bool mono_ImGUI_InputInt_Clear(MonoString* id)
{
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC);
	mono_free(idC);
	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;
	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];
	auto it = domainInfo.m_IMGUI_InputIntValues.find(idStr);
	if (it != domainInfo.m_IMGUI_InputIntValues.end())
	{
		domainInfo.m_IMGUI_InputIntValues.erase(it);
		return true;
	}
	return false;
}
bool mono_ImGUI_InputInt(MonoString* id, int current,int steps, int fastSteps)
{
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC);
	mono_free(idC);
	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;
	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];

	domainInfo.m_IMGUI_InputIntValues[idStr] = current;
	auto it = domainInfo.m_IMGUI_InputIntValues.find(idStr);

	return ImGui::InputInt(idStr.c_str(), &it->second, steps, fastSteps);

}
int mono_ImGUI_InputInt_Get(MonoString* id)
{
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC);
	mono_free(idC);
	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;

	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];
	auto it = domainInfo.m_IMGUI_InputIntValues.find(idStr);
	if (it == domainInfo.m_IMGUI_InputIntValues.end())
	{
		return -1;
	}
	return it->second;
}
bool mono_ImGUI_InputText(MonoString* id, MonoString* currentValue)
{
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC);
	mono_free(idC);

	char* initC = mono_string_to_utf8(currentValue);
	std::string currentStr(initC ? initC : "");
	if (initC) mono_free(initC);

	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;

	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];
	
	auto it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
	if (it != domainInfo.m_IMGUI_InputTextValues.end())
	{
		if (it->second != currentStr)
		{
			//update it
			domainInfo.m_IMGUI_InputTextValues[idStr] = currentStr;
			it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
		}
	}
	else
	{
		//didn't find it, insert
		domainInfo.m_IMGUI_InputTextValues[idStr] = currentStr;
		it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
	}
	if (ImGui::InputText(idStr.c_str(), &it->second))
	{
		return true;
	}
	return false;
}


bool mono_ImGUI_InputTextMultiline(MonoString* id, MonoString* currentValue, float width, float height)
{
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC);
	mono_free(idC);

	char* initC = mono_string_to_utf8(currentValue);
	std::string currentStr(initC ? initC : "");
	if (initC) mono_free(initC);

	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return false;

	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];

	auto it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
	if (it != domainInfo.m_IMGUI_InputTextValues.end())
	{
		if (it->second != currentStr)
		{
			//update it
			domainInfo.m_IMGUI_InputTextValues[idStr] = currentStr;
			it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
		}
	}
	else
	{
		//didn't find it, insert
		domainInfo.m_IMGUI_InputTextValues[idStr] = currentStr;
		it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
	}

	ImVec2 inputSize(width <= 0.0f ? 0.0f : width, height <= 0.0f ? 0.0f : height);

	if (ImGui::InputTextMultiline(idStr.c_str(), &it->second, inputSize))
	{
		return true;
	}
	return false;
}

MonoString* mono_ImGUI_InputText_Get(MonoString* id)
{
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC);
	mono_free(idC);

	MonoDomain* currentDomain = mono_domain_get();
	if (!currentDomain) return mono_string_new(mono_get_root_domain(), "");
	std::string key = monoAppDomainPtrToString[currentDomain];
	auto& domainInfo = monoAppDomains[key];
	auto it = domainInfo.m_IMGUI_InputTextValues.find(idStr);
	if (it == domainInfo.m_IMGUI_InputTextValues.end())
	{
		return mono_string_new(currentDomain, "");
	}
	return mono_string_new(currentDomain, it->second.c_str());
}

void mono_ImGUI_SetNextItemWidth(float width)
{
	ImGui::SetNextItemWidth(width);
}

bool mono_ImGUI_SliderInt(MonoString* id, int* value, int minValue, int maxValue)
{
	if (!id || !value) return false;
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC ? idC : "");
	if (idC) mono_free(idC);

	int v = *value;
	bool changed = ImGui::SliderInt(idStr.c_str(), &v, minValue, maxValue);
	if (changed) { *value = v; }
	return changed;
}

bool mono_ImGUI_SliderDouble(MonoString* id, double* value, double minValue, double maxValue, MonoString* fmt)
{
	if (!id || !value) return false;
	char* idC = mono_string_to_utf8(id);
	std::string idStr(idC ? idC : "");
	if (idC) mono_free(idC);

	std::string fmtStr = "%.3f";
	if (fmt)
	{
		char* f = mono_string_to_utf8(fmt);
		if (f) { fmtStr = f; mono_free(f); }
	}

	double v = *value;
	bool changed = ImGui::SliderScalar(idStr.c_str(), ImGuiDataType_Double, &v, &minValue, &maxValue, fmtStr.c_str());
	if (changed) { *value = v; }
	return changed;
}


void mono_ImGUI_ProgressBar(float fraction,float height, float width, MonoString* overlay)
{
	if (overlay)
	{
		char* pOverlay = mono_string_to_utf8(overlay);
		std::string overlayInfo(pOverlay);
		mono_free(pOverlay);
		ImGui::ProgressBar(fraction, ImVec2(width, height), overlayInfo.c_str());
	}
	else
	{ 
		ImGui::ProgressBar(fraction, ImVec2(width, height));
	}
}

bool mono_ImGUI_BeginPopupContextItem(MonoString* id, int flags)
{
	const char* c_id = nullptr;
	std::string temp;
	if (id)
	{
		char* s = mono_string_to_utf8(id);
		temp = (s ? s : "");
		if (s) mono_free(s);
		c_id = temp.c_str();
	}
	return ImGui::BeginPopupContextItem(c_id, (ImGuiPopupFlags)flags);
}

bool mono_ImGUI_BeginPopupContextWindow(MonoString* id, int flags)
{
	const char* c_id = nullptr;
	std::string temp;
	if (id)
	{
		char* s = mono_string_to_utf8(id);
		temp = (s ? s : "");
		if (s) mono_free(s);
		c_id = temp.c_str();
	}
	return ImGui::BeginPopupContextWindow(c_id, (ImGuiPopupFlags)flags);
}

void mono_ImGUI_EndPopup()
{
	ImGui::EndPopup();
}

bool mono_ImGUI_MenuItem(MonoString* label)
{
	if (!label) return false;
	char* l = mono_string_to_utf8(label);
	std::string s(l ? l : "");
	if (l) mono_free(l);
	return ImGui::MenuItem(s.c_str());
}

bool mono_ImGUI_TreeNode(MonoString* label)
{
	if (!label) return false;
	char* l = mono_string_to_utf8(label);
	std::string s(l ? l : "");
	if (l) mono_free(l);
	return ImGui::TreeNode(s.c_str());
}

bool mono_ImGUI_TreeNodeEx(MonoString* label, int flags)
{
	if (!label) return false;
	char* l = mono_string_to_utf8(label);
	std::string s(l ? l : "");
	if (l) mono_free(l);
	return ImGui::TreeNodeEx(s.c_str(), (ImGuiTreeNodeFlags)flags);
}

void mono_ImGUI_TreePop()
{
	ImGui::TreePop();
}

bool mono_ImGUI_CollapsingHeader(MonoString* label, int flags)
{
	if (!label) return false;
	char* l = mono_string_to_utf8(label);
	std::string s(l ? l : "");
	if (l) mono_free(l);
	return ImGui::CollapsingHeader(s.c_str(), (ImGuiTreeNodeFlags)flags);
}

bool mono_ImGUI_IsItemHovered()
{
	return ImGui::IsItemHovered();
}

void mono_ImGUI_BeginTooltip()
{
	ImGui::BeginTooltip();
}

void mono_ImGUI_EndTooltip()
{
	ImGui::EndTooltip();
}

void mono_ImGUI_Image(void* textureId, float width, float height)
{
	ImGui::Image(textureId, ImVec2(width, height));
}

void* mono_ImGUI_AddFontFromFileTTF(MonoString* path, float size_pixels, const uint16_t* ranges, int range_count, bool merge_mode)
{
	if (!path) return nullptr;
	char* cpath = mono_string_to_utf8(path);
	std::string fpath(cpath ? cpath : "");
	if (cpath) mono_free(cpath);
	if (fpath.empty()) return nullptr;

	ImFontConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.MergeMode = merge_mode;
	cfg.PixelSnapH = true;
	cfg.OversampleH = 1;
	cfg.OversampleV = 1;

	const ImWchar* wranges = nullptr;
	std::vector<ImWchar> converted;
	if (ranges && range_count > 0)
	{
		converted.reserve(range_count);
		for (int i = 0; i < range_count; ++i)
		{
			converted.push_back((ImWchar)ranges[i]);
		}
		wranges = converted.data();
	}

	ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(fpath.c_str(), size_pixels, &cfg, wranges);
	return (void*)font;
}

float mono_ImGUI_Style_GetFontSizeBase()
{
	ImGuiStyle& style = ImGui::GetStyle();

	return style.FontSizeBase;
}


bool mono_ImGUI_PushFont(MonoString* name, float font_size)
{
	char* fontname = mono_string_to_utf8(name);
	std::string nameSTR(fontname);
	mono_free(fontname);
	
	
	if (nameSTR.length()==0 && font_size >0)
	{
		//empty string, just pass null
		ImGui::PushFont(NULL, font_size);
		return true;
	}
	ImFont* selectedFont = nullptr;
	ImGuiIO& io = ImGui::GetIO();
	
	for (ImFont* font : io.Fonts->Fonts)
	{
		std::string fontName(font->GetDebugName());
		if (fontName == nameSTR)
		{
			selectedFont = font;
			break;
		}
	}
	if (selectedFont)
	{
		ImGui::PushFont(selectedFont);
		return true;
	}
	return false;
}

bool mono_ImGUI_PushEQFont(int fontID, float size)
{
	ImFont* font =mq::imgui::GetEQImFont(fontID);
	if (font == NULL)
	{
		return false;
	}
	if (size ==0)
	{
		 ImGui::PushFont(font);
		 return true;
	}
	else
	{ 
		ImGui::PushFont(font, size);
		return true;
	}

}

void mono_ImGUI_PopFont()
{
	ImGui::PopFont();
}

void mono_ImGUI_PushMaterialIconsFont()
{
	static ImFont* s_mdFont = nullptr;
	if (!s_mdFont)
	{
		ImGuiIO& io = ImGui::GetIO();
		const ImWchar kTestGlyph = (ImWchar)0xE616; // ICON_MD_EVENT_NOTE as probe
		for (int i = 0; i < io.Fonts->Fonts.Size; ++i)
		{
			ImFont* f = io.Fonts->Fonts[i];
			if (f && f->GetFontBaked(ImGui::GetFontSize())->FindGlyphNoFallback(kTestGlyph))
			{
				s_mdFont = f;
				break;
			}
		}
	}
	if (s_mdFont)
	{
		ImGui::PushFont(s_mdFont);
	}
	// else: no-op if not found
}
static CTextureAnimation* s_pTASpellIcons = nullptr;

void mono_ImGUI_DrawSpellIconByIconIndex(int iconIndex, float size)
{

	if (!pSidlMgr)
		return;

	if (!s_pTASpellIcons)
	{
		if (CTextureAnimation* temp = pSidlMgr->FindAnimation("A_SpellGems"))
		{
			s_pTASpellIcons = new CTextureAnimation(*temp);
		}
	}

	if (!s_pTASpellIcons)
		return;

	if (iconIndex < 0)
		return;

	s_pTASpellIcons->SetCurCell(iconIndex);
	mq::imgui::DrawTextureAnimation(s_pTASpellIcons, eqlib::CXSize((int)size, (int)size));
}

void mono_ImGUI_DrawSpellIconBySpellID(int spellId, float size)
{
	if (spellId <= 0)
		return;

	eqlib::EQ_Spell* pSpell = GetSpellByID(spellId);
	if (!pSpell)
		return;

	int iconIndex = pSpell->SpellIcon;
	mono_ImGUI_DrawSpellIconByIconIndex(iconIndex, size);
}





float mono_ImGUI_GetTextLineHeightWithSpacing()
{
    return ImGui::GetTextLineHeightWithSpacing();
}

float mono_ImGUI_GetFrameHeight()
{
    return ImGui::GetFrameHeight();
}

void mono_ImGUI_TableSetBgColor(int tablebgcolortarget, unsigned int color, int currentcolumn)
{
	ImGui::TableSetBgColor((ImGuiTableBgTarget)tablebgcolortarget, (ImU32) color, currentcolumn);
}

MonoArray* mono_ImGUI_CalcItemSize(float height, float width, float default_width, float default_height)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImVec2 size = ImGui::CalcItemSize(ImVec2(width, height), ImGui::GetContentRegionAvail().x, g.FontSize + style.FramePadding.y * 2.0f);
	MonoClass* singleClass = mono_get_single_class();
	int arraySize = 2;
	MonoDomain* currentDomain = mono_domain_get();
	MonoArray* monoArray = mono_array_new(currentDomain, singleClass, arraySize);
	mono_array_set(monoArray, float, 0, size.x);
	mono_array_set(monoArray, float, 1, size.y);
	return monoArray;
}
 
void mono_ImGUI_Internal_ItemSize(float x, float y,float text_baseline_y)
{
	ImGui::ItemSize(ImVec2(x, y), text_baseline_y);
}
bool mono_ImGUI_Internal_ItemAdd(float x, float y, float x2, float y2)
{
	const ImRect bb(x,y,x2,y2);
	return ImGui::ItemAdd(bb, 0);
}
void mono_ImGUI_Internal_CalcItemSize(float x, float y, float default_w,float* r_x, float* r_y)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImVec2 size = ImGui::CalcItemSize(ImVec2(x, y), default_w, g.FontSize + style.FramePadding.y * 2.0f);

	*r_x = size.x;
	*r_y = size.y;

}
void mono_ImGUI_ProgressBarGradient(float progress, float height, float width, unsigned int color_start, unsigned int color_end)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return;
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = ImGui::CalcItemSize(ImVec2(width, height), ImGui::GetContentRegionAvail().x, g.FontSize + style.FramePadding.y * 2.0f);

	// Define the bounding box
	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImGui::ItemSize(size);
	if (!ImGui::ItemAdd(bb, 0)) return;

	// Render background
	window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), style.FrameRounding);

	// Render progress gradient
	float progress_x = bb.Min.x + size.x * progress;
	if (progress > 0.0f) {
		window->DrawList->AddRectFilledMultiColor(
			bb.Min,
			ImVec2(progress_x, bb.Max.y),
			color_start,   // Top-Left (Red)
			color_end,   // Top-Right (Green)
			color_end,   // Bottom-Right
			color_start    // Bottom-Left
		);
	}
}

void mono_ImGUI_GetWindowDrawList_AddLine(float x, float y, float x_end,float y_end, unsigned int color,float thickness)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddLine(ImVec2(x, y), ImVec2(x_end, y_end), color, thickness);
	}
}

void mono_ImGUI_GetWindowDrawList_AddRectFilledMultiColor(float p_min_x, float p_min_y, float p_max_x, float p_max_y, unsigned int col_upr_left, unsigned int col_upr_right, unsigned int col_bot_right, unsigned int col_bot_left)
{
	//std::string debugOutout = std::format("mono_ImGUI_GetWindowDrawList_AddRectFilledMultiColorcalled with x {:.1f}: max_x is {:.1f}", p_min_x, p_max_x);
	//WriteChatColor(debugOutout.c_str());
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddRectFilledMultiColor(ImVec2(p_min_x, p_min_y),
			ImVec2(p_max_x, p_max_y),
			col_upr_left,   // Top-Left (Red)
			col_upr_right,   // Top-Right (Green)
			col_bot_right,   // Bottom-Right
			col_bot_left    // Bottom-Left);
		);
	}
}
void mono_ImGUI_GetWindowDrawList_AddQuad(float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, unsigned int color, float thickness)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddQuad(ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), ImVec2(p4_x, p4_y), color, thickness);
	}
}
void mono_ImGUI_GetWindowDrawList_AddQuadFilled(float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, unsigned int color)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddQuadFilled(ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), ImVec2(p4_x, p4_y), color);
	}
}
void mono_ImGUI_GetWindowDrawList_AddTriangle(float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, unsigned int color, float thickness)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddTriangle(ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), color,thickness);
	}
}
void mono_ImGUI_GetWindowDrawList_AddTriangleFilled(float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, unsigned int color)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddTriangleFilled(ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), color);
	}
}
void mono_ImGUI_GetWindowDrawList_AddCircle(float center_x, float center_y, float radius, unsigned int color, int num_segments, float thickness)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddCircle(ImVec2(center_x,center_y),radius,color,num_segments,thickness);
	}
}
void mono_ImGUI_GetWindowDrawList_AddCircleFilled(float center_x, float center_y, float radius, unsigned int color, int num_segments)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddCircle(ImVec2(center_x, center_y), radius, color, num_segments);
	}
}
void mono_ImGUI_GetWindowDrawList_AddNgon(float center_x, float center_y, float radius, unsigned int color, int num_segments, float thickness)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddNgon(ImVec2(center_x, center_y), radius, color, num_segments, thickness);
	}
}
void mono_ImGUI_GetWindowDrawList_AddNgonFilled(float center_x, float center_y, float radius, unsigned int col, int num_segments)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddNgon(ImVec2(center_x, center_y), radius, col, num_segments);
	}
}
void mono_ImGUI_GetWindowDrawList_AddEllipse(float center_x, float center_y, float radius_x, float radius_y, unsigned int col, float rot, int num_segments, float thickness)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddEllipse(ImVec2(center_x, center_y),ImVec2(radius_x,radius_y),col,rot,num_segments,thickness);
	}

}
void mono_ImGUI_GetWindowDrawList_AddEllipseFilled(float center_x, float center_y, float radius_x, float radius_y, unsigned int col, float rot, int num_segments)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddEllipse(ImVec2(center_x, center_y), ImVec2(radius_x, radius_y), col, rot, num_segments);
	}
}
void mono_ImGUI_GetWindowDrawList_AddBezierCubic(float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, unsigned int col, float thickness, int num_segments)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddBezierCubic(ImVec2(p1_x,p1_y), ImVec2(p2_x, p2_y),ImVec2(p3_x, p3_y),ImVec2(p4_x, p4_y),col,thickness,num_segments);
	}

}// Cubic Bezier (4 control points)
void mono_ImGUI_GetWindowDrawList_AddBezierQuadratic(float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, unsigned int col, float thickness, int num_segments)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (drawList)
	{
		drawList->AddBezierQuadratic(ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y),col,thickness,num_segments);
	}
}// Quadratic Bezier (3 control points)


void mono_ImGUI_GetWindowDrawList_AddRectFilled(float x1, float y1, float x2, float y2, uint32_t color, float rounding, int rounding_corners_flags)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList)
    {
        drawList->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), color,rounding,rounding_corners_flags);
    }
}
void mono_ImGUI_GetWindowDrawList_AddRect(float p_min_x,float p_min_y, float p_max_x, float p_max_y, unsigned int color, float rounding, int rounding_corners_flags, float thickness)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	if (draw_list)
	{
		draw_list->AddRect(ImVec2(p_min_x,p_min_y), ImVec2(p_max_x,p_max_y), color, rounding, rounding_corners_flags, thickness);
	}
}

void mono_ImGUI_GetWindowDrawList_AddText(float x, float y, uint32_t color, MonoString* text)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList && text)
    {
        char* ctext = mono_string_to_utf8(text);
		std::string input(ctext);
		mono_free(ctext);
		drawList->AddText(ImVec2(x, y), color, input.c_str());
	}
}

MonoArray* mono_ImGUI_GetItemRectSize()
{
	ImVec2 itemSize = ImGui::GetItemRectSize();
	MonoClass* singleClass = mono_get_single_class();
	int arraySize = 2;
	MonoDomain* currentDomain = mono_domain_get();
	MonoArray* monoArray = mono_array_new(currentDomain, singleClass, arraySize);

	mono_array_set(monoArray, float, 0,itemSize.x);
	mono_array_set(monoArray, float, 1,itemSize.y);

	return monoArray;
}

MonoArray* mono_ImGUI_GetItemRectMin()
{
	ImVec2 itemSize = ImGui::GetItemRectMin();
	MonoClass* singleClass = mono_get_single_class();
	int arraySize = 2;
	MonoDomain* currentDomain = mono_domain_get();
	MonoArray* monoArray = mono_array_new(currentDomain, singleClass, arraySize);

	mono_array_set(monoArray, float, 0, itemSize.x);
	mono_array_set(monoArray, float, 1, itemSize.y);

	return monoArray;
}
float mono_ImGUI_GetItemRectMinX()
{
    return ImGui::GetItemRectMin().x;
}

float mono_ImGUI_GetItemRectMinY()
{
    return ImGui::GetItemRectMin().y;
}

float mono_ImGUI_GetItemRectMaxX()
{
    return ImGui::GetItemRectMax().x;
}

float mono_ImGUI_GetItemRectMaxY()
{
    return ImGui::GetItemRectMax().y;
}

uint32_t mono_ImGUI_GetColorU32(int imguiCol, float alpha_mul)
{
    return ImGui::GetColorU32((ImGuiCol)imguiCol, alpha_mul);
}

void* mono_CreateTextureFromData(const uint8_t* data, int width, int height, int channels)
{
	return nullptr;
}

void mono_DestroyTexture(void* textureId)
{
	// Placeholder for texture cleanup
}
