#pragma once

#include <mq/Plugin.h>

struct _MonoString;
typedef _MonoString MonoString;

bool mono_ImGUI_InventorySlotTile(MonoString* id, MonoString* label, int iconIndex, float width, float height, bool selected);
