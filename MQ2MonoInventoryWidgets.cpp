#include "MQ2MonoInventoryWidgets.h"
#include "MQ2MonoShared.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mq/imgui/Widgets.h>
#include <mono/metadata/object.h>

#include <string>

namespace
{
	constexpr float kTileRounding = 6.0f;
	constexpr float kIconSize = 36.0f;

	CTextureAnimation* GetItemIconAnimation()
	{
		static CTextureAnimation* s_itemIcons = nullptr;
		if (!s_itemIcons && pSidlMgr)
		{
			if (CTextureAnimation* temp = pSidlMgr->FindAnimation("A_DragItem"))
			{
				s_itemIcons = new CTextureAnimation(*temp);
			}
		}

		return s_itemIcons;
	}

	void DrawItemIcon(ImDrawList* drawList, int iconIndex, const ImVec2& pos)
	{
		if (iconIndex <= 0)
			return;

		CTextureAnimation* itemIcons = GetItemIconAnimation();
		if (!itemIcons)
			return;

		const int iconFrame = iconIndex - 500;
		if (iconFrame < 0)
			return;

		itemIcons->SetCurCell(iconFrame);
		mq::imgui::DrawTextureAnimation(drawList, itemIcons, eqlib::CXPoint((int)pos.x, (int)pos.y), eqlib::CXSize((int)kIconSize, (int)kIconSize));
	}
}

bool mono_ImGUI_InventorySlotTile(MonoString* id, MonoString* label, int iconIndex, float width, float height, bool selected)
{
	if (!id || !label)
		return false;

	char* idText = mono_string_to_utf8(id);
	char* labelText = mono_string_to_utf8(label);
	std::string tileId(idText ? idText : "");
	std::string tileLabel(labelText ? labelText : "");
	if (idText) mono_free(idText);
	if (labelText) mono_free(labelText);

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (!window || window->SkipItems)
		return false;

	const ImVec2 start = ImGui::GetCursorScreenPos();
	const ImVec2 size(width, height);
	const ImRect bb(start, ImVec2(start.x + width, start.y + height));

	ImGui::PushID(tileId.c_str());
	const bool pressed = ImGui::InvisibleButton("##inventory_slot_tile", size);
	const bool hovered = ImGui::IsItemHovered();
	ImGui::PopID();

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 bgColor = selected
		? IM_COL32(68, 88, 108, 255)
		: hovered ? IM_COL32(52, 58, 66, 255) : IM_COL32(36, 40, 46, 255);
	const ImU32 borderColor = selected ? IM_COL32(220, 192, 120, 255) : IM_COL32(90, 96, 108, 255);
	const ImU32 emptyColor = IM_COL32(150, 154, 160, 255);

	drawList->AddRectFilled(bb.Min, bb.Max, bgColor, kTileRounding);
	drawList->AddRect(bb.Min, bb.Max, borderColor, kTileRounding, 0, selected ? 2.0f : 1.0f);

	if (iconIndex > 0)
	{
		const ImVec2 iconPos(bb.Min.x + ((width - kIconSize) * 0.5f), bb.Min.y + ((height - kIconSize) * 0.5f));
		DrawItemIcon(drawList, iconIndex, iconPos);
	}
	else
	{
		const char* emptyText = "[ ]";
		const ImVec2 emptySize = ImGui::CalcTextSize(emptyText);
		drawList->AddText(
			ImVec2(bb.Min.x + ((width - emptySize.x) * 0.5f), bb.Min.y + ((height - emptySize.y) * 0.5f)),
			emptyColor,
			emptyText);
	}

	return pressed;
}
