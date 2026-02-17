#pragma once

#include <mq/Plugin.h>
#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>
#include <cstdint>

// All ImGui wrapper function declarations used by MQ2Mono
// These are used by mono_add_internal_call in MQ2Mono.cpp

// Open/close state helpers
void mono_ImGUI_Begin_OpenFlagSet(MonoString* name, bool open);
boolean mono_ImGUI_Begin_OpenFlagGet(MonoString* name);

// Basic window and widgets
bool mono_ImGUI_Begin(MonoString* name, int flags);
bool mono_ImGUI_Button(MonoString* name);
bool mono_ImGUI_ButtonEx(MonoString* name, float width, float height);
bool mono_ImGUI_InvisibleButton(MonoString* name, float width, float height, int flags);
bool mono_ImGUI_SmallButton(MonoString* name);
void mono_ImGUI_End();
float mono_ImGUI_GetWindowContentRegionMin_X();
float mono_ImGUI_GetWindowContentRegionMin_Y();
float mono_ImGUI_GetWindowContentRegionMax_X();
float mono_ImGUI_GetWindowContentRegionMax_Y();
void mono_ImGUI_SetNextWindowPos(float x, float y, int flags, float xpiv, float ypiv);
float mono_ImGUI_GetWindowPosX();
float mono_ImGUI_GetWindowPosY();
float mono_ImGUI_GetWindowSizeX();
float mono_ImGUI_GetWindowSizeY();
void mono_ImGUI_SetNextWindowFocus();
void mono_ImGUI_TableNextRowEx(int row_flags, float min_row_height);
float mono_ImGUI_GetCursorPosX();
void mono_ImGUI_SetCursorPosX(float x);
float mono_ImGUI_GetCursorPosY();
void mono_ImGUI_SetCursorPosY(float y);
float mono_ImGUI_GetCursorScreenPosX();
float mono_ImGUI_GetCursorScreenPosY();
// Text helpers
void mono_ImGUI_Text(MonoString* text);
void mono_ImGUI_TextUnformatted(MonoString* text);
void mono_ImGUI_TextWrapped(MonoString* text);
void mono_ImGUI_PushTextWrapPos(float wrap_local_pos_x);
void mono_ImGUI_PopTextWrapPos();
void mono_ImGUI_TextColored(float r, float g, float b, float a, MonoString* text);
float mono_ImGUI_CalcTextSizeX(MonoString* input);
MonoArray* mono_ImGUI_CalcTextSize(MonoString* input);
MonoArray* mono_ImGUI_GetItemRectSize();

// Layout helpers
void mono_ImGUI_Separator();
void mono_ImGUI_SameLine();
void mono_ImGUI_SameLineEx(float offset_from_start_x, float spacing);
float mono_ImGUI_GetContentRegionAvailX();
float mono_ImGUI_GetContentRegionAvailY();
void mono_ImGUI_SetNextItemWidth(float width);

// Window control helpers
void mono_ImGUI_SetNextWindowBgAlpha(float alpha);
void mono_ImGUI_SetNextWindowSize(float width, float height);
void mono_ImGUI_SetNextWindowSizeWithCond(float width, float height, int cond);
bool mono_ImGUI_IsWindowHovered();
float mono_ImGUI_GetWindowWidth();
float mono_ImGUI_GetWindowHeight();
bool mono_ImGUI_IsMouseClicked(int button);
void mono_ImGUI_SetNextWindowSizeConstraints(float min_w, float min_h, float max_w, float max_h);

// Widgets
bool mono_ImGUI_Checkbox(MonoString* name, bool defaultValue);
void mono_ImGUI_Checkbox_Clear(MonoString* id);
bool mono_ImGUI_Checkbox_Get(MonoString* id);
bool mono_ImGUI_BeginChild(MonoString* id, float width, float height, int child_flags, int window_flags);
void mono_ImGUI_EndChild();
bool mono_ImGUI_Selectable(MonoString* label, bool selected);
bool mono_ImGUI_Selectable_WithFlags(MonoString* label, bool selected, int flags);

// progress bar
void mono_ImGUI_ProgressBar(float fraction, float height, float width, MonoString* overlay);
// Tabs
bool mono_ImGUI_BeginTabBar(MonoString* name);
void mono_ImGUI_EndTabBar();
bool mono_ImGUI_BeginTabItem(MonoString* label);
void mono_ImGUI_EndTabItem();

// Tables
bool mono_ImGUI_BeginTable(MonoString* id, int columns, int flags, float outer_width,float outer_height);
bool mono_ImGUI_BeginTableSimple(MonoString* id, int columns, int flags);
void mono_ImGUI_EndTable();
void mono_ImGUI_TableSetupColumn(MonoString* label, int flags, float init_width);
void mono_ImGUI_TableSetupColumn_Default(MonoString* label);
void mono_ImGUI_TableHeadersRow();
void mono_ImGUI_TableHeader(MonoString* label);
MonoString* mono_ImGUI_TableGetColumnName(int column);
void mono_ImGUI_TableNextRow();
bool mono_ImGUI_TableNextColumn();
void mono_ImGUI_TableSetColumnIndex(int index);
void mono_ImGUI_TableSetBgColor(int tablebgcolortarget, unsigned int color, int currentcolumn);

//colortable
void mono_ImGUI_ColorPicker_Clear(MonoString* label);
bool mono_ImGUI_ColorPicker4(MonoString* label, int r, int g, int b, int a, int flags);
bool mono_ImGUI_ColorPicker4_Float(MonoString* label, float r, float g, float b, float a, int flags);
MonoArray* mono_ImGUI_ColorPicker_GetRGBA(MonoString* label);
MonoArray* mono_ImGUI_ColorPicker_GetRGBA_Float(MonoString* label);
void mono_ImGUI_PushID(int id);
void mono_ImGUI_PopID();

void mono_ImGUI_PushItemWidth(float width);
void mono_ImGUI_PopItemWidth(float width);


// Style colors
void mono_ImGUI_PushStyleColor(int which, float r, float g, float b, float a);
void mono_ImGUI_PopStyleColor(int count);

// Style variables
void mono_ImGUI_PushStyleVarFloat(int which, float value);
void mono_ImGUI_PushStyleVarVec2(int which, float x, float y);
void mono_ImGUI_PopStyleVar(int count);
float mono_ImGUI_GetStyleVarFloat(int which);
void mono_ImGUI_GetStyleVarVec2(int which, float* x, float* y);

// Combo
bool mono_ImGUI_BeginCombo(MonoString* label, MonoString* preview, int flags);
void mono_ImGUI_EndCombo();

// Alignment helper
bool mono_ImGUI_RightAlignButton(MonoString* name);

// Text input
bool mono_ImGUI_InputText(MonoString* id, MonoString* initial);
bool mono_ImGUI_InputText_Clear(MonoString* id);
bool mono_ImGUI_InputTextMultiline(MonoString* id, MonoString* initial, float width, float height);
MonoString* mono_ImGUI_InputText_Get(MonoString* id);

// Int input
bool mono_ImGUI_InputInt(MonoString* id, int initial, int steps, int fastSteps);
int mono_ImGUI_InputInt_Get(MonoString* id);
bool mono_ImGUI_InputInt_Clear(MonoString* id);

// Sliders
bool mono_ImGUI_SliderInt(MonoString* id, int* value, int minValue, int maxValue);
bool mono_ImGUI_SliderDouble(MonoString* id, double* value, double minValue, double maxValue, MonoString* fmt);

// Context menu / popups
bool mono_ImGUI_BeginPopupContextItem(MonoString* id, int flags);
bool mono_ImGUI_BeginPopupContextWindow(MonoString* id, int flags);
void mono_ImGUI_EndPopup();
bool mono_ImGUI_MenuItem(MonoString* label);

// Tree helpers
bool mono_ImGUI_TreeNode(MonoString* label);
bool mono_ImGUI_TreeNodeEx(MonoString* label, int flags);
void mono_ImGUI_TreePop();
bool mono_ImGUI_CollapsingHeader(MonoString* label, int flags);

// Tooltips and hover detection
bool mono_ImGUI_IsItemHovered();
void mono_ImGUI_BeginTooltip();
void mono_ImGUI_EndTooltip();

// Images and textures
void mono_ImGUI_Image(void* textureId, float width, float height);
void mono_ImGUI_DrawSpellIconByIconIndex(int iconIndex, float size);
void mono_ImGUI_DrawSpellIconBySpellID(int spellId, float size);

// Fonts
void* mono_ImGUI_AddFontFromFileTTF(MonoString* path, float size_pixels, const uint16_t* ranges, int range_count, bool merge_mode);
bool mono_ImGUI_PushFont(MonoString* name,float font_size);
void mono_ImGUI_PopFont();
bool mono_ImGUI_PushEQFont(int fontID, float size);
float mono_ImGUI_Style_GetFontSizeBase();

// Convenience: push Material Design icons font if present
void mono_ImGUI_PushMaterialIconsFont();

// Drawing functions for custom backgrounds
float mono_ImGUI_GetCursorPosY();
float mono_ImGUI_GetCursorScreenPosX();
float mono_ImGUI_GetCursorScreenPosY();
float mono_ImGUI_GetTextLineHeightWithSpacing();
float mono_ImGUI_GetFrameHeight();
void mono_ImGUI_GetWindowDrawList_AddRectFilled(float x1, float y1, float x2, float y2, uint32_t color);
void mono_ImGUI_GetWindowDrawList_AddRect(float p_min_x, float p_min_y, float p_max_x, float p_max_y, unsigned int color, float rounding, int rounding_corners_flags, float thickness);

void mono_ImGUI_GetWindowDrawList_AddText(float x, float y, uint32_t color, MonoString* text);

// Item rect bounds and color helpers
float mono_ImGUI_GetItemRectMinX();
float mono_ImGUI_GetItemRectMinY();
float mono_ImGUI_GetItemRectMaxX();
float mono_ImGUI_GetItemRectMaxY();

MonoArray* mono_ImGUI_GetItemRectMin();
uint32_t mono_ImGUI_GetColorU32(int imguiCol, float alpha_mul);

// Texture creation from raw data (placeholders)
void* mono_CreateTextureFromData(const uint8_t* data, int width, int height, int channels);
void mono_DestroyTexture(void* textureId);
