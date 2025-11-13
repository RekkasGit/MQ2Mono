#include "MQ2MonoImGui.h"
#include "MQ2MonoShared.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mq/imgui/Widgets.h>
#include "imgui/misc/cpp/imgui_stdlib.h"
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>

// All ImGui wrapper function definitions moved out of MQ2Mono.cpp
// These rely on globals declared in MQ2MonoShared.h and defined in MQ2Mono.cpp

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
void mono_ImGUI_TableHeadersRow()
{
	ImGui::TableHeadersRow();
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

void mono_ImGUI_PushID(int id)
{
	ImGui::PushID(id);
}

void mono_ImGUI_PopID()
{
	ImGui::PopID();
}

bool mono_ImGUI_BeginChild(MonoString* id, float width, float height, int child_flags, int window_flags)
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
	}
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
	}
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
	if (it == domainInfo.m_IMGUI_InputTextValues.end())
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
	if (it == domainInfo.m_IMGUI_InputTextValues.end())
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

void mono_ImGUI_PushFont(void* font)
{
	if (!font) return;
	ImGui::PushFont(reinterpret_cast<ImFont*>(font));
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
			if (f && f->FindGlyphNoFallback(kTestGlyph))
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

// Drawing functions for custom backgrounds
float mono_ImGUI_GetCursorPosY()
{
	return ImGui::GetCursorPosY();
}

float mono_ImGUI_GetCursorScreenPosX()
{
	return ImGui::GetCursorScreenPos().x;
}

float mono_ImGUI_GetCursorScreenPosY()
{
	return ImGui::GetCursorScreenPos().y;
}

float mono_ImGUI_GetTextLineHeightWithSpacing()
{
    return ImGui::GetTextLineHeightWithSpacing();
}

float mono_ImGUI_GetFrameHeight()
{
    return ImGui::GetFrameHeight();
}



void mono_ImGUI_GetWindowDrawList_AddRectFilled(float x1, float y1, float x2, float y2, uint32_t color)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList)
    {
        drawList->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), color);
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
