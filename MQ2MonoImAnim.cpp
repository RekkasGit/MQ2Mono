#include "MQ2MonoImAnim.h"
#include "MQ2MonoShared.h"
#include <imgui/imanim/im_anim.h>
#include <imgui/imgui.h>
#include <mono/metadata/object.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <cmath>
#include <cstring>

// Storage for active iam_clip builders (keyed by clip_id)
static std::unordered_map<unsigned int, iam_clip> g_monoImAnimClips;
// Storage for active iam_path builders (keyed by path_id)
static std::unordered_map<unsigned int, iam_path> g_monoImAnimPaths;
// Storage for active gradients (keyed by gradient_id)
static std::unordered_map<unsigned int, iam_gradient> g_monoImAnimGradients;

struct MonoClipCallbackRef
{
	uint32_t delegateHandle = 0;
};

struct MonoMarkerCallbackRef
{
	uint32_t delegateHandle = 0;
};

static std::vector<MonoClipCallbackRef*> g_monoImAnimClipCallbackRefs;
static std::vector<MonoMarkerCallbackRef*> g_monoImAnimMarkerCallbackRefs;
static std::array<uint32_t, 16> g_customEaseHandles = {};
static iam_context* g_monoImAnimContext = nullptr;

struct ResolvedFloatContext { MonoObject* delegate = nullptr; };
struct ResolvedIntContext { MonoObject* delegate = nullptr; };
struct ResolvedVec2Context { MonoObject* delegate = nullptr; };
struct ResolvedVec4Context { MonoObject* delegate = nullptr; };
struct ResolvedColorContext { MonoObject* delegate = nullptr; };

static inline void EnsureMonoImAnimContext()
{
	// ImAnim keeps state in a global "current context". Exposing thin wrappers to
	// C# is fine, but only if MQ2Mono guarantees those wrappers always run against
	// a valid context that outlives the managed callers.
	if (!g_monoImAnimContext)
	{
		g_monoImAnimContext = iam_context_create();
		iam_set_lazy_init(true);
	}

	iam_context_set_current(g_monoImAnimContext);
}

void mono_ImAnim_RuntimeInit()
{
	EnsureMonoImAnimContext();
}

void mono_ImAnim_RuntimeShutdown()
{
	if (!g_monoImAnimContext)
		return;

	// Reset wrapper-owned builder caches before destroying the underlying runtime
	// context they depend on.
	g_monoImAnimClips.clear();
	g_monoImAnimPaths.clear();
	g_monoImAnimGradients.clear();

	iam_context_destroy(g_monoImAnimContext);
	g_monoImAnimContext = nullptr;
}

static inline iam_ease_desc MakeEaseDesc(int type, float p0, float p1, float p2, float p3)
{
	iam_ease_desc ez;
	ez.type = type;
	ez.p0 = p0;
	ez.p1 = p1;
	ez.p2 = p2;
	ez.p3 = p3;
	return ez;
}

static inline MonoArray* MakeFloatArrayN(int count)
{
	MonoDomain* currentDomain = mono_domain_get();
	MonoClass* floatClass = mono_get_single_class();
	return mono_array_new(currentDomain, floatClass, count);
}

static inline MonoArray* MakeFloatArray2(float x, float y)
{
	MonoArray* arr = MakeFloatArrayN(2);
	mono_array_set(arr, float, 0, x);
	mono_array_set(arr, float, 1, y);
	return arr;
}

static inline MonoArray* MakeFloatArray4(float x, float y, float z, float w)
{
	MonoArray* arr = MakeFloatArrayN(4);
	mono_array_set(arr, float, 0, x);
	mono_array_set(arr, float, 1, y);
	mono_array_set(arr, float, 2, z);
	mono_array_set(arr, float, 3, w);
	return arr;
}

static inline MonoArray* MakeFloatArray5(float a, float b, float c, float d, float e)
{
	MonoArray* arr = MakeFloatArrayN(5);
	mono_array_set(arr, float, 0, a);
	mono_array_set(arr, float, 1, b);
	mono_array_set(arr, float, 2, c);
	mono_array_set(arr, float, 3, d);
	mono_array_set(arr, float, 4, e);
	return arr;
}

static inline MonoArray* MakeIntArray2(int a, int b)
{
	MonoDomain* currentDomain = mono_domain_get();
	MonoClass* intClass = mono_get_int32_class();
	MonoArray* arr = mono_array_new(currentDomain, intClass, 2);
	mono_array_set(arr, int, 0, a);
	mono_array_set(arr, int, 1, b);
	return arr;
}

static inline std::string MonoStringToStdString(MonoString* text)
{
	if (!text)
		return {};
	char* str = mono_string_to_utf8(text);
	std::string out(str ? str : "");
	if (str)
		mono_free(str);
	return out;
}

static inline float UnboxSingle(MonoObject* obj)
{
	if (!obj)
		return 0.0f;
	return *static_cast<float*>(mono_object_unbox(obj));
}

static inline int UnboxInt32(MonoObject* obj)
{
	if (!obj)
		return 0;
	return *static_cast<int*>(mono_object_unbox(obj));
}

static inline bool UnboxBool(MonoObject* obj)
{
	if (!obj)
		return false;
	return *static_cast<mono_bool*>(mono_object_unbox(obj)) != 0;
}

static inline MonoObject* InvokeDelegate(MonoObject* delegate, void** params = nullptr)
{
	if (!delegate)
		return nullptr;
	MonoObject* exc = nullptr;
	return mono_runtime_delegate_invoke(delegate, params, &exc);
}

static inline ImVec2 ReadVec2FromMonoArray(MonoObject* obj)
{
	if (!obj)
		return ImVec2(0, 0);
	MonoArray* arr = reinterpret_cast<MonoArray*>(obj);
	if (mono_array_length(arr) < 2)
		return ImVec2(0, 0);
	return ImVec2(mono_array_get(arr, float, 0), mono_array_get(arr, float, 1));
}

static inline ImVec4 ReadVec4FromMonoArray(MonoObject* obj)
{
	if (!obj)
		return ImVec4(0, 0, 0, 0);
	MonoArray* arr = reinterpret_cast<MonoArray*>(obj);
	if (mono_array_length(arr) < 4)
		return ImVec4(0, 0, 0, 0);
	return ImVec4(
		mono_array_get(arr, float, 0),
		mono_array_get(arr, float, 1),
		mono_array_get(arr, float, 2),
		mono_array_get(arr, float, 3));
}

static inline iam_drag_feedback MakeEmptyDragFeedback()
{
	iam_drag_feedback feedback{};
	feedback.position = ImVec2(0, 0);
	feedback.offset = ImVec2(0, 0);
	feedback.velocity = ImVec2(0, 0);
	feedback.is_dragging = false;
	feedback.is_snapping = false;
	feedback.snap_progress = 0.0f;
	return feedback;
}

static inline MonoArray* MakeDragFeedbackArray(const iam_drag_feedback& feedback)
{
	MonoArray* arr = MakeFloatArrayN(9);
	mono_array_set(arr, float, 0, feedback.position.x);
	mono_array_set(arr, float, 1, feedback.position.y);
	mono_array_set(arr, float, 2, feedback.offset.x);
	mono_array_set(arr, float, 3, feedback.offset.y);
	mono_array_set(arr, float, 4, feedback.velocity.x);
	mono_array_set(arr, float, 5, feedback.velocity.y);
	mono_array_set(arr, float, 6, feedback.is_dragging ? 1.0f : 0.0f);
	mono_array_set(arr, float, 7, feedback.is_snapping ? 1.0f : 0.0f);
	mono_array_set(arr, float, 8, feedback.snap_progress);
	return arr;
}

static inline MonoArray* EncodeGradient(const iam_gradient& gradient)
{
	const int stopCount = gradient.stop_count();
	MonoArray* arr = MakeFloatArrayN(1 + stopCount * 5);
	mono_array_set(arr, float, 0, static_cast<float>(stopCount));
	for (int i = 0; i < stopCount; ++i)
	{
		const int base = 1 + i * 5;
		mono_array_set(arr, float, base + 0, gradient.positions[i]);
		mono_array_set(arr, float, base + 1, gradient.colors[i].x);
		mono_array_set(arr, float, base + 2, gradient.colors[i].y);
		mono_array_set(arr, float, base + 3, gradient.colors[i].z);
		mono_array_set(arr, float, base + 4, gradient.colors[i].w);
	}
	return arr;
}

static constexpr int kImGuiStyleDataCount = 85 + ImGuiCol_COUNT * 4;

static inline void SerializeStyle(const ImGuiStyle& style, float* out)
{
	int i = 0;
	out[i++] = style.FontSizeBase;
	out[i++] = style.FontScaleMain;
	out[i++] = style.FontScaleDpi;
	out[i++] = style.Alpha;
	out[i++] = style.DisabledAlpha;
	out[i++] = style.WindowPadding.x; out[i++] = style.WindowPadding.y;
	out[i++] = style.WindowRounding;
	out[i++] = style.WindowBorderSize;
	out[i++] = style.WindowBorderHoverPadding;
	out[i++] = style.WindowMinSize.x; out[i++] = style.WindowMinSize.y;
	out[i++] = style.WindowTitleAlign.x; out[i++] = style.WindowTitleAlign.y;
	out[i++] = static_cast<float>(style.WindowMenuButtonPosition);
	out[i++] = style.ChildRounding;
	out[i++] = style.ChildBorderSize;
	out[i++] = style.PopupRounding;
	out[i++] = style.PopupBorderSize;
	out[i++] = style.FramePadding.x; out[i++] = style.FramePadding.y;
	out[i++] = style.FrameRounding;
	out[i++] = style.FrameBorderSize;
	out[i++] = style.ItemSpacing.x; out[i++] = style.ItemSpacing.y;
	out[i++] = style.ItemInnerSpacing.x; out[i++] = style.ItemInnerSpacing.y;
	out[i++] = style.CellPadding.x; out[i++] = style.CellPadding.y;
	out[i++] = style.TouchExtraPadding.x; out[i++] = style.TouchExtraPadding.y;
	out[i++] = style.IndentSpacing;
	out[i++] = style.ColumnsMinSpacing;
	out[i++] = style.ScrollbarSize;
	out[i++] = style.ScrollbarRounding;
	out[i++] = style.ScrollbarPadding;
	out[i++] = style.GrabMinSize;
	out[i++] = style.GrabRounding;
	out[i++] = style.LayoutAlign;
	out[i++] = style.LogSliderDeadzone;
	out[i++] = style.ImageBorderSize;
	out[i++] = style.TabRounding;
	out[i++] = style.TabBorderSize;
	out[i++] = style.TabMinWidthBase;
	out[i++] = style.TabMinWidthShrink;
	out[i++] = style.TabCloseButtonMinWidthSelected;
	out[i++] = style.TabCloseButtonMinWidthUnselected;
	out[i++] = style.TabBarBorderSize;
	out[i++] = style.TabBarOverlineSize;
	out[i++] = style.TableAngledHeadersAngle;
	out[i++] = style.TableAngledHeadersTextAlign.x; out[i++] = style.TableAngledHeadersTextAlign.y;
	out[i++] = static_cast<float>(style.TreeLinesFlags);
	out[i++] = style.TreeLinesSize;
	out[i++] = style.TreeLinesRounding;
	out[i++] = style.DragDropTargetRounding;
	out[i++] = style.DragDropTargetBorderSize;
	out[i++] = style.DragDropTargetPadding;
	out[i++] = static_cast<float>(style.ColorButtonPosition);
	out[i++] = style.ButtonTextAlign.x; out[i++] = style.ButtonTextAlign.y;
	out[i++] = style.SelectableTextAlign.x; out[i++] = style.SelectableTextAlign.y;
	out[i++] = style.SeparatorTextBorderSize;
	out[i++] = style.SeparatorTextAlign.x; out[i++] = style.SeparatorTextAlign.y;
	out[i++] = style.SeparatorTextPadding.x; out[i++] = style.SeparatorTextPadding.y;
	out[i++] = style.DisplayWindowPadding.x; out[i++] = style.DisplayWindowPadding.y;
	out[i++] = style.DisplaySafeAreaPadding.x; out[i++] = style.DisplaySafeAreaPadding.y;
	out[i++] = style.DockingNodeHasCloseButton ? 1.0f : 0.0f;
	out[i++] = style.DockingSeparatorSize;
	out[i++] = style.MouseCursorScale;
	out[i++] = style.AntiAliasedLines ? 1.0f : 0.0f;
	out[i++] = style.AntiAliasedLinesUseTex ? 1.0f : 0.0f;
	out[i++] = style.AntiAliasedFill ? 1.0f : 0.0f;
	out[i++] = style.CurveTessellationTol;
	out[i++] = style.CircleTessellationMaxError;
	for (int c = 0; c < ImGuiCol_COUNT; ++c)
	{
		out[i++] = style.Colors[c].x;
		out[i++] = style.Colors[c].y;
		out[i++] = style.Colors[c].z;
		out[i++] = style.Colors[c].w;
	}
	out[i++] = style.HoverStationaryDelay;
	out[i++] = style.HoverDelayShort;
	out[i++] = style.HoverDelayNormal;
	out[i++] = static_cast<float>(style.HoverFlagsForTooltipMouse);
	out[i++] = static_cast<float>(style.HoverFlagsForTooltipNav);
}

static inline bool DeserializeStyle(MonoArray* styleArray, ImGuiStyle& style)
{
	if (!styleArray || mono_array_length(styleArray) < kImGuiStyleDataCount)
		return false;
	auto next = [styleArray](int& i) { return mono_array_get(styleArray, float, i++); };
	int i = 0;
	style.FontSizeBase = next(i);
	style.FontScaleMain = next(i);
	style.FontScaleDpi = next(i);
	style.Alpha = next(i);
	style.DisabledAlpha = next(i);
	style.WindowPadding = ImVec2(next(i), next(i));
	style.WindowRounding = next(i);
	style.WindowBorderSize = next(i);
	style.WindowBorderHoverPadding = next(i);
	style.WindowMinSize = ImVec2(next(i), next(i));
	style.WindowTitleAlign = ImVec2(next(i), next(i));
	style.WindowMenuButtonPosition = static_cast<ImGuiDir>(static_cast<int>(std::lround(next(i))));
	style.ChildRounding = next(i);
	style.ChildBorderSize = next(i);
	style.PopupRounding = next(i);
	style.PopupBorderSize = next(i);
	style.FramePadding = ImVec2(next(i), next(i));
	style.FrameRounding = next(i);
	style.FrameBorderSize = next(i);
	style.ItemSpacing = ImVec2(next(i), next(i));
	style.ItemInnerSpacing = ImVec2(next(i), next(i));
	style.CellPadding = ImVec2(next(i), next(i));
	style.TouchExtraPadding = ImVec2(next(i), next(i));
	style.IndentSpacing = next(i);
	style.ColumnsMinSpacing = next(i);
	style.ScrollbarSize = next(i);
	style.ScrollbarRounding = next(i);
	style.ScrollbarPadding = next(i);
	style.GrabMinSize = next(i);
	style.GrabRounding = next(i);
	style.LayoutAlign = next(i);
	style.LogSliderDeadzone = next(i);
	style.ImageBorderSize = next(i);
	style.TabRounding = next(i);
	style.TabBorderSize = next(i);
	style.TabMinWidthBase = next(i);
	style.TabMinWidthShrink = next(i);
	style.TabCloseButtonMinWidthSelected = next(i);
	style.TabCloseButtonMinWidthUnselected = next(i);
	style.TabBarBorderSize = next(i);
	style.TabBarOverlineSize = next(i);
	style.TableAngledHeadersAngle = next(i);
	style.TableAngledHeadersTextAlign = ImVec2(next(i), next(i));
	style.TreeLinesFlags = static_cast<ImGuiTreeNodeFlags>(static_cast<int>(std::lround(next(i))));
	style.TreeLinesSize = next(i);
	style.TreeLinesRounding = next(i);
	style.DragDropTargetRounding = next(i);
	style.DragDropTargetBorderSize = next(i);
	style.DragDropTargetPadding = next(i);
	style.ColorButtonPosition = static_cast<ImGuiDir>(static_cast<int>(std::lround(next(i))));
	style.ButtonTextAlign = ImVec2(next(i), next(i));
	style.SelectableTextAlign = ImVec2(next(i), next(i));
	style.SeparatorTextBorderSize = next(i);
	style.SeparatorTextAlign = ImVec2(next(i), next(i));
	style.SeparatorTextPadding = ImVec2(next(i), next(i));
	style.DisplayWindowPadding = ImVec2(next(i), next(i));
	style.DisplaySafeAreaPadding = ImVec2(next(i), next(i));
	style.DockingNodeHasCloseButton = next(i) != 0.0f;
	style.DockingSeparatorSize = next(i);
	style.MouseCursorScale = next(i);
	style.AntiAliasedLines = next(i) != 0.0f;
	style.AntiAliasedLinesUseTex = next(i) != 0.0f;
	style.AntiAliasedFill = next(i) != 0.0f;
	style.CurveTessellationTol = next(i);
	style.CircleTessellationMaxError = next(i);
	for (int c = 0; c < ImGuiCol_COUNT; ++c)
		style.Colors[c] = ImVec4(next(i), next(i), next(i), next(i));
	style.HoverStationaryDelay = next(i);
	style.HoverDelayShort = next(i);
	style.HoverDelayNormal = next(i);
	style.HoverFlagsForTooltipMouse = static_cast<ImGuiHoveredFlags>(static_cast<int>(std::lround(next(i))));
	style.HoverFlagsForTooltipNav = static_cast<ImGuiHoveredFlags>(static_cast<int>(std::lround(next(i))));
	return true;
}

static inline MonoArray* EncodeStyle(const ImGuiStyle& style)
{
	MonoArray* arr = MakeFloatArrayN(kImGuiStyleDataCount);
	std::vector<float> data(kImGuiStyleDataCount);
	SerializeStyle(style, data.data());
	for (int i = 0; i < kImGuiStyleDataCount; ++i)
		mono_array_set(arr, float, i, data[i]);
	return arr;
}

template <int Slot>
static float CustomEaseThunk(float t)
{
	uint32_t handle = g_customEaseHandles[Slot];
	if (!handle)
		return t;
	MonoObject* target = mono_gchandle_get_target(handle);
	if (!target)
		return t;
	void* args[1] = { &t };
	return UnboxSingle(InvokeDelegate(target, args));
}

static iam_ease_fn GetCustomEaseThunk(int slot)
{
	switch (slot)
	{
	case 0: return &CustomEaseThunk<0>;
	case 1: return &CustomEaseThunk<1>;
	case 2: return &CustomEaseThunk<2>;
	case 3: return &CustomEaseThunk<3>;
	case 4: return &CustomEaseThunk<4>;
	case 5: return &CustomEaseThunk<5>;
	case 6: return &CustomEaseThunk<6>;
	case 7: return &CustomEaseThunk<7>;
	case 8: return &CustomEaseThunk<8>;
	case 9: return &CustomEaseThunk<9>;
	case 10: return &CustomEaseThunk<10>;
	case 11: return &CustomEaseThunk<11>;
	case 12: return &CustomEaseThunk<12>;
	case 13: return &CustomEaseThunk<13>;
	case 14: return &CustomEaseThunk<14>;
	case 15: return &CustomEaseThunk<15>;
	default: return nullptr;
	}
}

static float ResolvedFloatThunk(void* user)
{
	auto* ctx = static_cast<ResolvedFloatContext*>(user);
	return UnboxSingle(InvokeDelegate(ctx ? ctx->delegate : nullptr));
}

static int ResolvedIntThunk(void* user)
{
	auto* ctx = static_cast<ResolvedIntContext*>(user);
	return UnboxInt32(InvokeDelegate(ctx ? ctx->delegate : nullptr));
}

static ImVec2 ResolvedVec2Thunk(void* user)
{
	auto* ctx = static_cast<ResolvedVec2Context*>(user);
	return ReadVec2FromMonoArray(InvokeDelegate(ctx ? ctx->delegate : nullptr));
}

static ImVec4 ResolvedVec4Thunk(void* user)
{
	auto* ctx = static_cast<ResolvedVec4Context*>(user);
	return ReadVec4FromMonoArray(InvokeDelegate(ctx ? ctx->delegate : nullptr));
}

static ImVec4 ResolvedColorThunk(void* user)
{
	auto* ctx = static_cast<ResolvedColorContext*>(user);
	return ReadVec4FromMonoArray(InvokeDelegate(ctx ? ctx->delegate : nullptr));
}

static void MonoClipCallbackThunk(ImGuiID inst_id, void* user_data)
{
	auto* cb = static_cast<MonoClipCallbackRef*>(user_data);
	if (!cb || !cb->delegateHandle)
		return;
	MonoObject* target = mono_gchandle_get_target(cb->delegateHandle);
	if (!target)
		return;
	uint32_t inst = inst_id;
	void* args[1] = { &inst };
	InvokeDelegate(target, args);
}

static void MonoMarkerCallbackThunk(ImGuiID inst_id, ImGuiID marker_id, float marker_time, void* user_data)
{
	auto* cb = static_cast<MonoMarkerCallbackRef*>(user_data);
	if (!cb || !cb->delegateHandle)
		return;
	MonoObject* target = mono_gchandle_get_target(cb->delegateHandle);
	if (!target)
		return;
	uint32_t inst = inst_id;
	uint32_t marker = marker_id;
	void* args[3] = { &inst, &marker, &marker_time };
	InvokeDelegate(target, args);
}

static MonoClipCallbackRef* CreateClipCallbackRef(MonoObject* delegate)
{
	if (!delegate)
		return nullptr;
	auto* ref = new MonoClipCallbackRef();
	ref->delegateHandle = mono_gchandle_new(delegate, false);
	g_monoImAnimClipCallbackRefs.push_back(ref);
	return ref;
}

static MonoMarkerCallbackRef* CreateMarkerCallbackRef(MonoObject* delegate)
{
	if (!delegate)
		return nullptr;
	auto* ref = new MonoMarkerCallbackRef();
	ref->delegateHandle = mono_gchandle_new(delegate, false);
	g_monoImAnimMarkerCallbackRefs.push_back(ref);
	return ref;
}

// ============================================================================
// Frame / Global Management
// ============================================================================

void mono_ImAnim_UpdateBeginFrame()
{
	EnsureMonoImAnimContext();
	iam_update_begin_frame();
}

void mono_ImAnim_GC(unsigned int max_age_frames)
{
	EnsureMonoImAnimContext();
	iam_gc(max_age_frames);
}

void mono_ImAnim_PoolClear()
{
	EnsureMonoImAnimContext();
	iam_pool_clear();
}

void mono_ImAnim_Reserve(int cap_float, int cap_vec2, int cap_vec4, int cap_int, int cap_color)
{
	EnsureMonoImAnimContext();
	iam_reserve(cap_float, cap_vec2, cap_vec4, cap_int, cap_color);
}

void mono_ImAnim_SetEaseLutSamples(int count)
{
	EnsureMonoImAnimContext();
	iam_set_ease_lut_samples(count);
}

void mono_ImAnim_SetGlobalTimeScale(float scale)
{
	EnsureMonoImAnimContext();
	iam_set_global_time_scale(scale);
}

float mono_ImAnim_GetGlobalTimeScale()
{
	EnsureMonoImAnimContext();
	return iam_get_global_time_scale();
}

void mono_ImAnim_SetLazyInit(bool enable)
{
	EnsureMonoImAnimContext();
	iam_set_lazy_init(enable);
}

bool mono_ImAnim_IsLazyInitEnabled()
{
	EnsureMonoImAnimContext();
	return iam_is_lazy_init_enabled();
}

void mono_ImAnim_RegisterCustomEase(int slot, MonoObject* delegate)
{
	EnsureMonoImAnimContext();
	if (slot < 0 || slot >= static_cast<int>(g_customEaseHandles.size()))
		return;

	if (g_customEaseHandles[slot] != 0)
	{
		mono_gchandle_free(g_customEaseHandles[slot]);
		g_customEaseHandles[slot] = 0;
	}

	if (!delegate)
	{
		iam_register_custom_ease(slot, nullptr);
		return;
	}

	g_customEaseHandles[slot] = mono_gchandle_new(delegate, false);
	iam_register_custom_ease(slot, GetCustomEaseThunk(slot));
}

MonoObject* mono_ImAnim_GetCustomEase(int slot)
{
	EnsureMonoImAnimContext();
	if (slot < 0 || slot >= static_cast<int>(g_customEaseHandles.size()))
		return nullptr;
	if (g_customEaseHandles[slot] == 0)
		return nullptr;
	return mono_gchandle_get_target(g_customEaseHandles[slot]);
}

uint64_t mono_ImAnim_ContextCreate()
{
	return reinterpret_cast<uint64_t>(iam_context_create());
}

void mono_ImAnim_ContextDestroy(uint64_t ctx_ptr)
{
	iam_context_destroy(reinterpret_cast<iam_context*>(ctx_ptr));
}

uint64_t mono_ImAnim_ContextSetCurrent(uint64_t ctx_ptr)
{
	return reinterpret_cast<uint64_t>(iam_context_set_current(reinterpret_cast<iam_context*>(ctx_ptr)));
}

void mono_ImAnim_ContextSetUserData(uint64_t ctx_ptr, uint64_t user_data)
{
	iam_context_set_user_data(reinterpret_cast<iam_context*>(ctx_ptr), reinterpret_cast<void*>(user_data));
}

uint64_t mono_ImAnim_ContextGetCurrent()
{
	return reinterpret_cast<uint64_t>(iam_context_get_current());
}

uint64_t mono_ImAnim_ContextGetUserData()
{
	return reinterpret_cast<uint64_t>(iam_context_get_user_data());
}

uint64_t mono_ImAnim_ContextGetDefaultContext()
{
	return reinterpret_cast<uint64_t>(iam_context_get_default_context());
}

void mono_ImAnim_ProfilerEnable(bool enable)
{
	iam_profiler_enable(enable);
}

bool mono_ImAnim_ProfilerIsEnabled()
{
	return iam_profiler_is_enabled();
}

void mono_ImAnim_ProfilerBeginFrame()
{
	iam_profiler_begin_frame();
}

void mono_ImAnim_ProfilerEndFrame()
{
	iam_profiler_end_frame();
}

void mono_ImAnim_ProfilerBegin(MonoString* name)
{
	std::string section = MonoStringToStdString(name);
	iam_profiler_begin(section.c_str());
}

void mono_ImAnim_ProfilerEnd()
{
	iam_profiler_end();
}

MonoArray* mono_ImAnim_DragBegin(unsigned int id, float pos_x, float pos_y)
{
	return MakeDragFeedbackArray(iam_drag_begin(id, ImVec2(pos_x, pos_y)));
}

MonoArray* mono_ImAnim_DragUpdate(unsigned int id, float pos_x, float pos_y, float dt)
{
	return MakeDragFeedbackArray(iam_drag_update(id, ImVec2(pos_x, pos_y), dt));
}

MonoArray* mono_ImAnim_DragRelease(unsigned int id, float pos_x, float pos_y, MonoArray* snap_points_xy, float snap_grid_x, float snap_grid_y, float snap_duration, float overshoot, int ease_type, float dt)
{
	iam_drag_opts opts;
	opts.snap_grid = ImVec2(snap_grid_x, snap_grid_y);
	opts.snap_duration = snap_duration;
	opts.overshoot = overshoot;
	opts.ease_type = ease_type;

	std::vector<ImVec2> points;
	if (snap_points_xy)
	{
		uintptr_t len = mono_array_length(snap_points_xy);
		for (uintptr_t i = 0; i + 1 < len; i += 2)
			points.emplace_back(mono_array_get(snap_points_xy, float, i), mono_array_get(snap_points_xy, float, i + 1));
		if (!points.empty())
		{
			opts.snap_points = points.data();
			opts.snap_points_count = static_cast<int>(points.size());
		}
	}

	return MakeDragFeedbackArray(iam_drag_release(id, ImVec2(pos_x, pos_y), opts, dt));
}

void mono_ImAnim_DragCancel(unsigned int id)
{
	iam_drag_cancel(id);
}

// ============================================================================
// Easing Evaluation
// ============================================================================

float mono_ImAnim_EvalPreset(int type, float t)
{
	EnsureMonoImAnimContext();
	return iam_eval_preset(type, t);
}

// ============================================================================
// Core Tween API
// ============================================================================

float mono_ImAnim_TweenFloat(unsigned int id, unsigned int channel_id, float target, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, float init_value)
{
	EnsureMonoImAnimContext();
	return iam_tween_float(id, channel_id, target, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt, init_value);
}

MonoArray* mono_ImAnim_TweenVec2(unsigned int id, unsigned int channel_id, float target_x, float target_y, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, float init_x, float init_y)
{
	EnsureMonoImAnimContext();
	ImVec2 result = iam_tween_vec2(id, channel_id, ImVec2(target_x, target_y), dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt, ImVec2(init_x, init_y));
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_TweenVec4(unsigned int id, unsigned int channel_id, float target_x, float target_y, float target_z, float target_w, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, float init_x, float init_y, float init_z, float init_w)
{
	EnsureMonoImAnimContext();
	ImVec4 result = iam_tween_vec4(id, channel_id, ImVec4(target_x, target_y, target_z, target_w), dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt, ImVec4(init_x, init_y, init_z, init_w));
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

int mono_ImAnim_TweenInt(unsigned int id, unsigned int channel_id, int target, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, int init_value)
{
	EnsureMonoImAnimContext();
	return iam_tween_int(id, channel_id, target, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt, init_value);
}

MonoArray* mono_ImAnim_TweenColor(unsigned int id, unsigned int channel_id, float target_r, float target_g, float target_b, float target_a, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, float dt, float init_r, float init_g, float init_b, float init_a)
{
	EnsureMonoImAnimContext();
	ImVec4 result = iam_tween_color(id, channel_id, ImVec4(target_r, target_g, target_b, target_a), dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, color_space, dt, ImVec4(init_r, init_g, init_b, init_a));
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

// ============================================================================
// Relative Tween API
// ============================================================================

float mono_ImAnim_TweenFloatRel(unsigned int id, unsigned int channel_id, float percent, float px_bias, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int anchor_space, int axis, float dt)
{
	return iam_tween_float_rel(id, channel_id, percent, px_bias, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, anchor_space, axis, dt);
}

MonoArray* mono_ImAnim_TweenVec2Rel(unsigned int id, unsigned int channel_id, float pct_x, float pct_y, float bias_x, float bias_y, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int anchor_space, float dt)
{
	ImVec2 result = iam_tween_vec2_rel(id, channel_id, ImVec2(pct_x, pct_y), ImVec2(bias_x, bias_y), dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, anchor_space, dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_TweenVec4Rel(unsigned int id, unsigned int channel_id, float pct_x, float pct_y, float pct_z, float pct_w, float bias_x, float bias_y, float bias_z, float bias_w, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int anchor_space, float dt)
{
	ImVec4 result = iam_tween_vec4_rel(id, channel_id, ImVec4(pct_x, pct_y, pct_z, pct_w), ImVec4(bias_x, bias_y, bias_z, bias_w), dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, anchor_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_TweenColorRel(unsigned int id, unsigned int channel_id, float pct_r, float pct_g, float pct_b, float pct_a, float bias_r, float bias_g, float bias_b, float bias_a, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, int anchor_space, float dt)
{
	ImVec4 result = iam_tween_color_rel(id, channel_id, ImVec4(pct_r, pct_g, pct_b, pct_a), ImVec4(bias_r, bias_g, bias_b, bias_a), dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, color_space, anchor_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

float mono_ImAnim_TweenFloatResolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt)
{
	ResolvedFloatContext ctx{ resolver };
	return iam_tween_float_resolved(id, channel_id, &ResolvedFloatThunk, &ctx, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt);
}

MonoArray* mono_ImAnim_TweenVec2Resolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt)
{
	ResolvedVec2Context ctx{ resolver };
	ImVec2 result = iam_tween_vec2_resolved(id, channel_id, &ResolvedVec2Thunk, &ctx, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_TweenVec4Resolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt)
{
	ResolvedVec4Context ctx{ resolver };
	ImVec4 result = iam_tween_vec4_resolved(id, channel_id, &ResolvedVec4Thunk, &ctx, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_TweenColorResolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, float dt)
{
	ResolvedColorContext ctx{ resolver };
	ImVec4 result = iam_tween_color_resolved(id, channel_id, &ResolvedColorThunk, &ctx, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, color_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

int mono_ImAnim_TweenIntResolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt)
{
	ResolvedIntContext ctx{ resolver };
	return iam_tween_int_resolved(id, channel_id, &ResolvedIntThunk, &ctx, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt);
}

// ============================================================================
// Per-axis Tween API
// ============================================================================

MonoArray* mono_ImAnim_TweenVec2PerAxis(unsigned int id, unsigned int channel_id, float target_x, float target_y, float dur, int ease_x_type, float ease_x_p0, float ease_x_p1, float ease_x_p2, float ease_x_p3, int ease_y_type, float ease_y_p0, float ease_y_p1, float ease_y_p2, float ease_y_p3, int policy, float dt)
{
	iam_ease_per_axis ez;
	ez.x = MakeEaseDesc(ease_x_type, ease_x_p0, ease_x_p1, ease_x_p2, ease_x_p3);
	ez.y = MakeEaseDesc(ease_y_type, ease_y_p0, ease_y_p1, ease_y_p2, ease_y_p3);
	ImVec2 result = iam_tween_vec2_per_axis(id, channel_id, ImVec2(target_x, target_y), dur, ez, policy, dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_TweenVec4PerAxis(unsigned int id, unsigned int channel_id, float target_x, float target_y, float target_z, float target_w, float dur, int ease_x_type, float ease_x_p0, float ease_x_p1, float ease_x_p2, float ease_x_p3, int ease_y_type, float ease_y_p0, float ease_y_p1, float ease_y_p2, float ease_y_p3, int ease_z_type, float ease_z_p0, float ease_z_p1, float ease_z_p2, float ease_z_p3, int ease_w_type, float ease_w_p0, float ease_w_p1, float ease_w_p2, float ease_w_p3, int policy, float dt)
{
	iam_ease_per_axis ez;
	ez.x = MakeEaseDesc(ease_x_type, ease_x_p0, ease_x_p1, ease_x_p2, ease_x_p3);
	ez.y = MakeEaseDesc(ease_y_type, ease_y_p0, ease_y_p1, ease_y_p2, ease_y_p3);
	ez.z = MakeEaseDesc(ease_z_type, ease_z_p0, ease_z_p1, ease_z_p2, ease_z_p3);
	ez.w = MakeEaseDesc(ease_w_type, ease_w_p0, ease_w_p1, ease_w_p2, ease_w_p3);
	ImVec4 result = iam_tween_vec4_per_axis(id, channel_id, ImVec4(target_x, target_y, target_z, target_w), dur, ez, policy, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_TweenColorPerAxis(unsigned int id, unsigned int channel_id, float target_r, float target_g, float target_b, float target_a, float dur, int ease_r_type, float ease_r_p0, float ease_r_p1, float ease_r_p2, float ease_r_p3, int ease_g_type, float ease_g_p0, float ease_g_p1, float ease_g_p2, float ease_g_p3, int ease_b_type, float ease_b_p0, float ease_b_p1, float ease_b_p2, float ease_b_p3, int ease_a_type, float ease_a_p0, float ease_a_p1, float ease_a_p2, float ease_a_p3, int policy, int color_space, float dt)
{
	iam_ease_per_axis ez;
	ez.x = MakeEaseDesc(ease_r_type, ease_r_p0, ease_r_p1, ease_r_p2, ease_r_p3);
	ez.y = MakeEaseDesc(ease_g_type, ease_g_p0, ease_g_p1, ease_g_p2, ease_g_p3);
	ez.z = MakeEaseDesc(ease_b_type, ease_b_p0, ease_b_p1, ease_b_p2, ease_b_p3);
	ez.w = MakeEaseDesc(ease_a_type, ease_a_p0, ease_a_p1, ease_a_p2, ease_a_p3);
	ImVec4 result = iam_tween_color_per_axis(id, channel_id, ImVec4(target_r, target_g, target_b, target_a), dur, ez, policy, color_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

// ============================================================================
// Rebase API
// ============================================================================

void mono_ImAnim_RebaseFloat(unsigned int id, unsigned int channel_id, float new_target, float dt)
{
	iam_rebase_float(id, channel_id, new_target, dt);
}

void mono_ImAnim_RebaseVec2(unsigned int id, unsigned int channel_id, float new_x, float new_y, float dt)
{
	iam_rebase_vec2(id, channel_id, ImVec2(new_x, new_y), dt);
}

void mono_ImAnim_RebaseVec4(unsigned int id, unsigned int channel_id, float new_x, float new_y, float new_z, float new_w, float dt)
{
	iam_rebase_vec4(id, channel_id, ImVec4(new_x, new_y, new_z, new_w), dt);
}

void mono_ImAnim_RebaseColor(unsigned int id, unsigned int channel_id, float new_r, float new_g, float new_b, float new_a, float dt)
{
	iam_rebase_color(id, channel_id, ImVec4(new_r, new_g, new_b, new_a), dt);
}

void mono_ImAnim_RebaseInt(unsigned int id, unsigned int channel_id, int new_target, float dt)
{
	iam_rebase_int(id, channel_id, new_target, dt);
}

// ============================================================================
// Color Blending
// ============================================================================

MonoArray* mono_ImAnim_GetBlendedColor(float a_r, float a_g, float a_b, float a_a, float b_r, float b_g, float b_b, float b_a, float t, int color_space)
{
	ImVec4 result = iam_get_blended_color(ImVec4(a_r, a_g, a_b, a_a), ImVec4(b_r, b_g, b_b, b_a), t, color_space);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

// ============================================================================
// Oscillators
// ============================================================================

float mono_ImAnim_Oscillate(unsigned int id, float amplitude, float frequency, int wave_type, float phase, float dt)
{
	return iam_oscillate(id, amplitude, frequency, wave_type, phase, dt);
}

int mono_ImAnim_OscillateInt(unsigned int id, int amplitude, float frequency, int wave_type, float phase, float dt)
{
	return iam_oscillate_int(id, amplitude, frequency, wave_type, phase, dt);
}

MonoArray* mono_ImAnim_OscillateVec2(unsigned int id, float amp_x, float amp_y, float freq_x, float freq_y, int wave_type, float phase_x, float phase_y, float dt)
{
	ImVec2 result = iam_oscillate_vec2(id, ImVec2(amp_x, amp_y), ImVec2(freq_x, freq_y), wave_type, ImVec2(phase_x, phase_y), dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_OscillateVec4(unsigned int id, float amp_x, float amp_y, float amp_z, float amp_w, float freq_x, float freq_y, float freq_z, float freq_w, int wave_type, float phase_x, float phase_y, float phase_z, float phase_w, float dt)
{
	ImVec4 result = iam_oscillate_vec4(id, ImVec4(amp_x, amp_y, amp_z, amp_w), ImVec4(freq_x, freq_y, freq_z, freq_w), wave_type, ImVec4(phase_x, phase_y, phase_z, phase_w), dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_OscillateColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float frequency, int wave_type, float phase, int color_space, float dt)
{
	ImVec4 result = iam_oscillate_color(id, ImVec4(base_r, base_g, base_b, base_a), ImVec4(amp_r, amp_g, amp_b, amp_a), frequency, wave_type, phase, color_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

// ============================================================================
// Shake / Wiggle
// ============================================================================

float mono_ImAnim_Shake(unsigned int id, float intensity, float frequency, float decay_time, float dt)
{
	return iam_shake(id, intensity, frequency, decay_time, dt);
}

int mono_ImAnim_ShakeInt(unsigned int id, int intensity, float frequency, float decay_time, float dt)
{
	return iam_shake_int(id, intensity, frequency, decay_time, dt);
}

MonoArray* mono_ImAnim_ShakeVec2(unsigned int id, float int_x, float int_y, float frequency, float decay_time, float dt)
{
	ImVec2 result = iam_shake_vec2(id, ImVec2(int_x, int_y), frequency, decay_time, dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_ShakeVec4(unsigned int id, float int_x, float int_y, float int_z, float int_w, float frequency, float decay_time, float dt)
{
	ImVec4 result = iam_shake_vec4(id, ImVec4(int_x, int_y, int_z, int_w), frequency, decay_time, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_ShakeColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float int_r, float int_g, float int_b, float int_a, float frequency, float decay_time, int color_space, float dt)
{
	ImVec4 result = iam_shake_color(id, ImVec4(base_r, base_g, base_b, base_a), ImVec4(int_r, int_g, int_b, int_a), frequency, decay_time, color_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

float mono_ImAnim_Wiggle(unsigned int id, float amplitude, float frequency, float dt)
{
	return iam_wiggle(id, amplitude, frequency, dt);
}

int mono_ImAnim_WiggleInt(unsigned int id, int amplitude, float frequency, float dt)
{
	return iam_wiggle_int(id, amplitude, frequency, dt);
}

MonoArray* mono_ImAnim_WiggleVec2(unsigned int id, float amp_x, float amp_y, float frequency, float dt)
{
	ImVec2 result = iam_wiggle_vec2(id, ImVec2(amp_x, amp_y), frequency, dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_WiggleVec4(unsigned int id, float amp_x, float amp_y, float amp_z, float amp_w, float frequency, float dt)
{
	ImVec4 result = iam_wiggle_vec4(id, ImVec4(amp_x, amp_y, amp_z, amp_w), frequency, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_WiggleColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float frequency, int color_space, float dt)
{
	ImVec4 result = iam_wiggle_color(id, ImVec4(base_r, base_g, base_b, base_a), ImVec4(amp_r, amp_g, amp_b, amp_a), frequency, color_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

void mono_ImAnim_TriggerShake(unsigned int id)
{
	iam_trigger_shake(id);
}

// ============================================================================
// Scroll Animation
// ============================================================================

void mono_ImAnim_ScrollToY(float target_y, float duration, int ease_type, float p0, float p1, float p2, float p3)
{
	iam_scroll_to_y(target_y, duration, MakeEaseDesc(ease_type, p0, p1, p2, p3));
}

void mono_ImAnim_ScrollToX(float target_x, float duration, int ease_type, float p0, float p1, float p2, float p3)
{
	iam_scroll_to_x(target_x, duration, MakeEaseDesc(ease_type, p0, p1, p2, p3));
}

void mono_ImAnim_ScrollToTop(float duration, int ease_type, float p0, float p1, float p2, float p3)
{
	iam_scroll_to_top(duration, MakeEaseDesc(ease_type, p0, p1, p2, p3));
}

void mono_ImAnim_ScrollToBottom(float duration, int ease_type, float p0, float p1, float p2, float p3)
{
	iam_scroll_to_bottom(duration, MakeEaseDesc(ease_type, p0, p1, p2, p3));
}

// ============================================================================
// Anchor Size
// ============================================================================

MonoArray* mono_ImAnim_AnchorSize(int space)
{
	ImVec2 result = iam_anchor_size(space);
	return MakeFloatArray2(result.x, result.y);
}

// ============================================================================
// Motion Path - Stateless Evaluation
// ============================================================================

MonoArray* mono_ImAnim_BezierQuadratic(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float t)
{
	ImVec2 result = iam_bezier_quadratic(ImVec2(p0_x, p0_y), ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), t);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_BezierCubic(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t)
{
	ImVec2 result = iam_bezier_cubic(ImVec2(p0_x, p0_y), ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), t);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_CatmullRom(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t, float tension)
{
	ImVec2 result = iam_catmull_rom(ImVec2(p0_x, p0_y), ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), t, tension);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_BezierQuadraticDeriv(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float t)
{
	ImVec2 result = iam_bezier_quadratic_deriv(ImVec2(p0_x, p0_y), ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), t);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_BezierCubicDeriv(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t)
{
	ImVec2 result = iam_bezier_cubic_deriv(ImVec2(p0_x, p0_y), ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), t);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_CatmullRomDeriv(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t, float tension)
{
	ImVec2 result = iam_catmull_rom_deriv(ImVec2(p0_x, p0_y), ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), ImVec2(p3_x, p3_y), t, tension);
	return MakeFloatArray2(result.x, result.y);
}

// ============================================================================
// Motion Path - Builder
// ============================================================================

void mono_ImAnim_PathBegin(unsigned int path_id, float start_x, float start_y)
{
	g_monoImAnimPaths.insert_or_assign(path_id, iam_path::begin(path_id, ImVec2(start_x, start_y)));
}

void mono_ImAnim_PathLineTo(unsigned int path_id, float end_x, float end_y)
{
	auto it = g_monoImAnimPaths.find(path_id);
	if (it != g_monoImAnimPaths.end())
		it->second.line_to(ImVec2(end_x, end_y));
}

void mono_ImAnim_PathQuadraticTo(unsigned int path_id, float ctrl_x, float ctrl_y, float end_x, float end_y)
{
	auto it = g_monoImAnimPaths.find(path_id);
	if (it != g_monoImAnimPaths.end())
		it->second.quadratic_to(ImVec2(ctrl_x, ctrl_y), ImVec2(end_x, end_y));
}

void mono_ImAnim_PathCubicTo(unsigned int path_id, float ctrl1_x, float ctrl1_y, float ctrl2_x, float ctrl2_y, float end_x, float end_y)
{
	auto it = g_monoImAnimPaths.find(path_id);
	if (it != g_monoImAnimPaths.end())
		it->second.cubic_to(ImVec2(ctrl1_x, ctrl1_y), ImVec2(ctrl2_x, ctrl2_y), ImVec2(end_x, end_y));
}

void mono_ImAnim_PathCatmullTo(unsigned int path_id, float end_x, float end_y, float tension)
{
	auto it = g_monoImAnimPaths.find(path_id);
	if (it != g_monoImAnimPaths.end())
		it->second.catmull_to(ImVec2(end_x, end_y), tension);
}

void mono_ImAnim_PathClose(unsigned int path_id)
{
	auto it = g_monoImAnimPaths.find(path_id);
	if (it != g_monoImAnimPaths.end())
		it->second.close();
}

void mono_ImAnim_PathEnd(unsigned int path_id)
{
	auto it = g_monoImAnimPaths.find(path_id);
	if (it != g_monoImAnimPaths.end())
	{
		it->second.end();
		g_monoImAnimPaths.erase(it);
	}
}

// ============================================================================
// Motion Path - Queries
// ============================================================================

bool mono_ImAnim_PathExists(unsigned int path_id)
{
	return iam_path_exists(path_id);
}

float mono_ImAnim_PathLength(unsigned int path_id)
{
	return iam_path_length(path_id);
}

MonoArray* mono_ImAnim_PathEvaluate(unsigned int path_id, float t)
{
	ImVec2 result = iam_path_evaluate(path_id, t);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_PathTangent(unsigned int path_id, float t)
{
	ImVec2 result = iam_path_tangent(path_id, t);
	return MakeFloatArray2(result.x, result.y);
}

float mono_ImAnim_PathAngle(unsigned int path_id, float t)
{
	return iam_path_angle(path_id, t);
}

// ============================================================================
// Motion Path - Tweens
// ============================================================================

MonoArray* mono_ImAnim_TweenPath(unsigned int id, unsigned int channel_id, unsigned int path_id, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt)
{
	ImVec2 result = iam_tween_path(id, channel_id, path_id, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt);
	return MakeFloatArray2(result.x, result.y);
}

float mono_ImAnim_TweenPathAngle(unsigned int id, unsigned int channel_id, unsigned int path_id, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt)
{
	return iam_tween_path_angle(id, channel_id, path_id, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, dt);
}

// ============================================================================
// Motion Path - Arc Length
// ============================================================================

void mono_ImAnim_PathBuildArcLUT(unsigned int path_id, int subdivisions)
{
	iam_path_build_arc_lut(path_id, subdivisions);
}

bool mono_ImAnim_PathHasArcLUT(unsigned int path_id)
{
	return iam_path_has_arc_lut(path_id);
}

float mono_ImAnim_PathDistanceToT(unsigned int path_id, float distance)
{
	return iam_path_distance_to_t(path_id, distance);
}

MonoArray* mono_ImAnim_PathEvaluateAtDistance(unsigned int path_id, float distance)
{
	ImVec2 result = iam_path_evaluate_at_distance(path_id, distance);
	return MakeFloatArray2(result.x, result.y);
}

float mono_ImAnim_PathAngleAtDistance(unsigned int path_id, float distance)
{
	return iam_path_angle_at_distance(path_id, distance);
}

MonoArray* mono_ImAnim_PathTangentAtDistance(unsigned int path_id, float distance)
{
	ImVec2 result = iam_path_tangent_at_distance(path_id, distance);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_PathMorph(unsigned int path_a, unsigned int path_b, float t, float blend, int samples, bool match_endpoints, bool use_arc_length)
{
	iam_morph_opts opts;
	opts.samples = samples;
	opts.match_endpoints = match_endpoints;
	opts.use_arc_length = use_arc_length;
	ImVec2 result = iam_path_morph(path_a, path_b, t, blend, opts);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_PathMorphTangent(unsigned int path_a, unsigned int path_b, float t, float blend, int samples, bool match_endpoints, bool use_arc_length)
{
	iam_morph_opts opts;
	opts.samples = samples;
	opts.match_endpoints = match_endpoints;
	opts.use_arc_length = use_arc_length;
	ImVec2 result = iam_path_morph_tangent(path_a, path_b, t, blend, opts);
	return MakeFloatArray2(result.x, result.y);
}

float mono_ImAnim_PathMorphAngle(unsigned int path_a, unsigned int path_b, float t, float blend, int samples, bool match_endpoints, bool use_arc_length)
{
	iam_morph_opts opts;
	opts.samples = samples;
	opts.match_endpoints = match_endpoints;
	opts.use_arc_length = use_arc_length;
	return iam_path_morph_angle(path_a, path_b, t, blend, opts);
}

MonoArray* mono_ImAnim_TweenPathMorph(unsigned int id, unsigned int channel_id, unsigned int path_a, unsigned int path_b, float target_blend, float dur, int path_ease_type, float path_p0, float path_p1, float path_p2, float path_p3, int morph_ease_type, float morph_p0, float morph_p1, float morph_p2, float morph_p3, int policy, float dt, int samples, bool match_endpoints, bool use_arc_length)
{
	iam_morph_opts opts;
	opts.samples = samples;
	opts.match_endpoints = match_endpoints;
	opts.use_arc_length = use_arc_length;
	ImVec2 result = iam_tween_path_morph(id, channel_id, path_a, path_b, target_blend, dur,
		MakeEaseDesc(path_ease_type, path_p0, path_p1, path_p2, path_p3),
		MakeEaseDesc(morph_ease_type, morph_p0, morph_p1, morph_p2, morph_p3),
		policy, dt, opts);
	return MakeFloatArray2(result.x, result.y);
}

float mono_ImAnim_GetMorphBlend(unsigned int id, unsigned int channel_id)
{
	return iam_get_morph_blend(id, channel_id);
}

void mono_ImAnim_TextPath(unsigned int path_id, MonoString* text, float origin_x, float origin_y, float offset, float letter_spacing, int align, bool flip_y, uint32_t color, float font_scale)
{
	std::string txt = MonoStringToStdString(text);
	iam_text_path_opts opts;
	opts.origin = ImVec2(origin_x, origin_y);
	opts.offset = offset;
	opts.letter_spacing = letter_spacing;
	opts.align = align;
	opts.flip_y = flip_y;
	opts.color = color;
	opts.font_scale = font_scale;
	iam_text_path(path_id, txt.c_str(), opts);
}

void mono_ImAnim_TextPathAnimated(unsigned int path_id, MonoString* text, float progress, float origin_x, float origin_y, float offset, float letter_spacing, int align, bool flip_y, uint32_t color, float font_scale)
{
	std::string txt = MonoStringToStdString(text);
	iam_text_path_opts opts;
	opts.origin = ImVec2(origin_x, origin_y);
	opts.offset = offset;
	opts.letter_spacing = letter_spacing;
	opts.align = align;
	opts.flip_y = flip_y;
	opts.color = color;
	opts.font_scale = font_scale;
	iam_text_path_animated(path_id, txt.c_str(), progress, opts);
}

float mono_ImAnim_TextPathWidth(MonoString* text, float origin_x, float origin_y, float offset, float letter_spacing, int align, bool flip_y, uint32_t color, float font_scale)
{
	std::string txt = MonoStringToStdString(text);
	iam_text_path_opts opts;
	opts.origin = ImVec2(origin_x, origin_y);
	opts.offset = offset;
	opts.letter_spacing = letter_spacing;
	opts.align = align;
	opts.flip_y = flip_y;
	opts.color = color;
	opts.font_scale = font_scale;
	return iam_text_path_width(txt.c_str(), opts);
}

MonoArray* mono_ImAnim_TransformQuad(float center_x, float center_y, float angle_rad, float translation_x, float translation_y, float q0_x, float q0_y, float q1_x, float q1_y, float q2_x, float q2_y, float q3_x, float q3_y)
{
	ImVec2 quad[4] = { ImVec2(q0_x, q0_y), ImVec2(q1_x, q1_y), ImVec2(q2_x, q2_y), ImVec2(q3_x, q3_y) };
	iam_transform_quad(quad, ImVec2(center_x, center_y), angle_rad, ImVec2(translation_x, translation_y));
	MonoArray* arr = MakeFloatArrayN(8);
	for (int i = 0; i < 4; ++i)
	{
		mono_array_set(arr, float, i * 2 + 0, quad[i].x);
		mono_array_set(arr, float, i * 2 + 1, quad[i].y);
	}
	return arr;
}

MonoArray* mono_ImAnim_MakeGlyphQuad(float pos_x, float pos_y, float angle_rad, float glyph_width, float glyph_height, float baseline_offset)
{
	ImVec2 quad[4];
	iam_make_glyph_quad(quad, ImVec2(pos_x, pos_y), angle_rad, glyph_width, glyph_height, baseline_offset);
	MonoArray* arr = MakeFloatArrayN(8);
	for (int i = 0; i < 4; ++i)
	{
		mono_array_set(arr, float, i * 2 + 0, quad[i].x);
		mono_array_set(arr, float, i * 2 + 1, quad[i].y);
	}
	return arr;
}

// ============================================================================
// Text Stagger
// ============================================================================

void mono_ImAnim_TextStagger(unsigned int id, MonoString* text, float progress, float pos_x, float pos_y, int effect, float char_delay, float char_duration, float effect_intensity, int ease_type, float ease_p0, float ease_p1, float ease_p2, float ease_p3, uint32_t color, float font_scale, float letter_spacing)
{
	if (!text) return;
	char* str = mono_string_to_utf8(text);
	std::string txt(str ? str : "");
	if (str) mono_free(str);

	iam_text_stagger_opts opts;
	opts.pos = ImVec2(pos_x, pos_y);
	opts.effect = effect;
	opts.char_delay = char_delay;
	opts.char_duration = char_duration;
	opts.effect_intensity = effect_intensity;
	opts.ease = MakeEaseDesc(ease_type, ease_p0, ease_p1, ease_p2, ease_p3);
	opts.color = color;
	opts.font_scale = font_scale;
	opts.letter_spacing = letter_spacing;

	iam_text_stagger(id, txt.c_str(), progress, opts);
}

float mono_ImAnim_TextStaggerWidth(MonoString* text, float letter_spacing, float font_scale)
{
	if (!text) return 0.0f;
	char* str = mono_string_to_utf8(text);
	std::string txt(str ? str : "");
	if (str) mono_free(str);

	iam_text_stagger_opts opts;
	opts.letter_spacing = letter_spacing;
	opts.font_scale = font_scale;
	return iam_text_stagger_width(txt.c_str(), opts);
}

float mono_ImAnim_TextStaggerDuration(MonoString* text, float char_delay, float char_duration, float letter_spacing, float font_scale)
{
	if (!text) return 0.0f;
	char* str = mono_string_to_utf8(text);
	std::string txt(str ? str : "");
	if (str) mono_free(str);

	iam_text_stagger_opts opts;
	opts.char_delay = char_delay;
	opts.char_duration = char_duration;
	opts.letter_spacing = letter_spacing;
	opts.font_scale = font_scale;
	return iam_text_stagger_duration(txt.c_str(), opts);
}

// ============================================================================
// Noise
// ============================================================================

float mono_ImAnim_Noise2D(float x, float y, int type, int octaves, float persistence, float lacunarity, int seed)
{
	iam_noise_opts opts;
	opts.type = type;
	opts.octaves = octaves;
	opts.persistence = persistence;
	opts.lacunarity = lacunarity;
	opts.seed = seed;
	return iam_noise_2d(x, y, opts);
}

float mono_ImAnim_Noise3D(float x, float y, float z, int type, int octaves, float persistence, float lacunarity, int seed)
{
	iam_noise_opts opts;
	opts.type = type;
	opts.octaves = octaves;
	opts.persistence = persistence;
	opts.lacunarity = lacunarity;
	opts.seed = seed;
	return iam_noise_3d(x, y, z, opts);
}

float mono_ImAnim_NoiseChannelFloat(unsigned int id, float frequency, float amplitude, int type, int octaves, float persistence, float lacunarity, int seed, float dt)
{
	iam_noise_opts opts;
	opts.type = type;
	opts.octaves = octaves;
	opts.persistence = persistence;
	opts.lacunarity = lacunarity;
	opts.seed = seed;
	return iam_noise_channel_float(id, frequency, amplitude, opts, dt);
}

MonoArray* mono_ImAnim_NoiseChannelVec2(unsigned int id, float freq_x, float freq_y, float amp_x, float amp_y, int type, int octaves, float persistence, float lacunarity, int seed, float dt)
{
	iam_noise_opts opts;
	opts.type = type;
	opts.octaves = octaves;
	opts.persistence = persistence;
	opts.lacunarity = lacunarity;
	opts.seed = seed;
	ImVec2 result = iam_noise_channel_vec2(id, ImVec2(freq_x, freq_y), ImVec2(amp_x, amp_y), opts, dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_NoiseChannelVec4(unsigned int id, float freq_x, float freq_y, float freq_z, float freq_w, float amp_x, float amp_y, float amp_z, float amp_w, int type, int octaves, float persistence, float lacunarity, int seed, float dt)
{
	iam_noise_opts opts;
	opts.type = type;
	opts.octaves = octaves;
	opts.persistence = persistence;
	opts.lacunarity = lacunarity;
	opts.seed = seed;
	ImVec4 result = iam_noise_channel_vec4(id, ImVec4(freq_x, freq_y, freq_z, freq_w), ImVec4(amp_x, amp_y, amp_z, amp_w), opts, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_NoiseChannelColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float frequency, int type, int octaves, float persistence, float lacunarity, int seed, int color_space, float dt)
{
	iam_noise_opts opts;
	opts.type = type;
	opts.octaves = octaves;
	opts.persistence = persistence;
	opts.lacunarity = lacunarity;
	opts.seed = seed;
	ImVec4 result = iam_noise_channel_color(id, ImVec4(base_r, base_g, base_b, base_a), ImVec4(amp_r, amp_g, amp_b, amp_a), frequency, opts, color_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

float mono_ImAnim_SmoothNoiseFloat(unsigned int id, float amplitude, float speed, float dt)
{
	return iam_smooth_noise_float(id, amplitude, speed, dt);
}

MonoArray* mono_ImAnim_SmoothNoiseVec2(unsigned int id, float amp_x, float amp_y, float speed, float dt)
{
	ImVec2 result = iam_smooth_noise_vec2(id, ImVec2(amp_x, amp_y), speed, dt);
	return MakeFloatArray2(result.x, result.y);
}

MonoArray* mono_ImAnim_SmoothNoiseVec4(unsigned int id, float amp_x, float amp_y, float amp_z, float amp_w, float speed, float dt)
{
	ImVec4 result = iam_smooth_noise_vec4(id, ImVec4(amp_x, amp_y, amp_z, amp_w), speed, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

MonoArray* mono_ImAnim_SmoothNoiseColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float speed, int color_space, float dt)
{
	ImVec4 result = iam_smooth_noise_color(id, ImVec4(base_r, base_g, base_b, base_a), ImVec4(amp_r, amp_g, amp_b, amp_a), speed, color_space, dt);
	return MakeFloatArray4(result.x, result.y, result.z, result.w);
}

// ============================================================================
// Style Interpolation
// ============================================================================

void mono_ImAnim_StyleRegister(unsigned int style_id, MonoArray* style_data)
{
	ImGuiStyle style = ImGui::GetStyle();
	if (!DeserializeStyle(style_data, style))
		return;
	iam_style_register(style_id, style);
}

void mono_ImAnim_StyleRegisterCurrent(unsigned int style_id)
{
	iam_style_register_current(style_id);
}

void mono_ImAnim_StyleBlend(unsigned int style_a, unsigned int style_b, float t, int color_space)
{
	iam_style_blend(style_a, style_b, t, color_space);
}

void mono_ImAnim_StyleTween(unsigned int id, unsigned int target_style, float duration, int ease_type, float p0, float p1, float p2, float p3, int color_space, float dt)
{
	iam_style_tween(id, target_style, duration, MakeEaseDesc(ease_type, p0, p1, p2, p3), color_space, dt);
}

MonoArray* mono_ImAnim_StyleBlendTo(unsigned int style_a, unsigned int style_b, float t, int color_space)
{
	ImGuiStyle style = ImGui::GetStyle();
	iam_style_blend_to(style_a, style_b, t, &style, color_space);
	return EncodeStyle(style);
}

bool mono_ImAnim_StyleExists(unsigned int style_id)
{
	return iam_style_exists(style_id);
}

void mono_ImAnim_StyleUnregister(unsigned int style_id)
{
	iam_style_unregister(style_id);
}

void mono_ImAnim_GradientBegin(unsigned int gradient_id)
{
	g_monoImAnimGradients.insert_or_assign(gradient_id, iam_gradient());
}

void mono_ImAnim_GradientClear(unsigned int gradient_id)
{
	g_monoImAnimGradients.insert_or_assign(gradient_id, iam_gradient());
}

void mono_ImAnim_GradientAddStop(unsigned int gradient_id, float position, float r, float g, float b, float a)
{
	auto [it, inserted] = g_monoImAnimGradients.try_emplace(gradient_id);
	it->second.add(position, ImVec4(r, g, b, a));
}

bool mono_ImAnim_GradientExists(unsigned int gradient_id)
{
	return g_monoImAnimGradients.find(gradient_id) != g_monoImAnimGradients.end();
}

void mono_ImAnim_GradientDestroy(unsigned int gradient_id)
{
	g_monoImAnimGradients.erase(gradient_id);
}

MonoArray* mono_ImAnim_GradientSample(unsigned int gradient_id, float t, int color_space)
{
	auto it = g_monoImAnimGradients.find(gradient_id);
	if (it == g_monoImAnimGradients.end())
		return MakeFloatArray4(0, 0, 0, 0);
	ImVec4 color = it->second.sample(t, color_space);
	return MakeFloatArray4(color.x, color.y, color.z, color.w);
}

MonoArray* mono_ImAnim_GradientGetData(unsigned int gradient_id)
{
	auto it = g_monoImAnimGradients.find(gradient_id);
	if (it == g_monoImAnimGradients.end())
		return MakeFloatArrayN(1);
	return EncodeGradient(it->second);
}

void mono_ImAnim_GradientLerp(unsigned int gradient_a, unsigned int gradient_b, float t, int color_space, unsigned int out_gradient_id)
{
	auto itA = g_monoImAnimGradients.find(gradient_a);
	auto itB = g_monoImAnimGradients.find(gradient_b);
	if (itA == g_monoImAnimGradients.end() || itB == g_monoImAnimGradients.end())
		return;
	g_monoImAnimGradients.insert_or_assign(out_gradient_id, iam_gradient_lerp(itA->second, itB->second, t, color_space));
}

MonoArray* mono_ImAnim_TweenGradient(unsigned int id, unsigned int channel_id, unsigned int target_gradient_id, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, float dt)
{
	auto it = g_monoImAnimGradients.find(target_gradient_id);
	if (it == g_monoImAnimGradients.end())
		return MakeFloatArrayN(1);
	iam_gradient result = iam_tween_gradient(id, channel_id, it->second, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, color_space, dt);
	return EncodeGradient(result);
}

// ============================================================================
// Transform Interpolation
// ============================================================================

MonoArray* mono_ImAnim_TransformLerp(float a_pos_x, float a_pos_y, float a_scale_x, float a_scale_y, float a_rot, float b_pos_x, float b_pos_y, float b_scale_x, float b_scale_y, float b_rot, float t, int rotation_mode)
{
	iam_transform a(ImVec2(a_pos_x, a_pos_y), a_rot, ImVec2(a_scale_x, a_scale_y));
	iam_transform b(ImVec2(b_pos_x, b_pos_y), b_rot, ImVec2(b_scale_x, b_scale_y));
	iam_transform result = iam_transform_lerp(a, b, t, rotation_mode);
	MonoDomain* currentDomain = mono_domain_get();
	MonoClass* floatClass = mono_get_single_class();
	MonoArray* arr = mono_array_new(currentDomain, floatClass, 5);
	mono_array_set(arr, float, 0, result.position.x);
	mono_array_set(arr, float, 1, result.position.y);
	mono_array_set(arr, float, 2, result.scale.x);
	mono_array_set(arr, float, 3, result.scale.y);
	mono_array_set(arr, float, 4, result.rotation);
	return arr;
}

MonoArray* mono_ImAnim_TweenTransform(unsigned int id, unsigned int channel_id, float target_pos_x, float target_pos_y, float target_scale_x, float target_scale_y, float target_rot, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int rotation_mode, float dt)
{
	iam_transform target(ImVec2(target_pos_x, target_pos_y), target_rot, ImVec2(target_scale_x, target_scale_y));
	iam_transform result = iam_tween_transform(id, channel_id, target, dur, MakeEaseDesc(ease_type, p0, p1, p2, p3), policy, rotation_mode, dt);
	MonoDomain* currentDomain = mono_domain_get();
	MonoClass* floatClass = mono_get_single_class();
	MonoArray* arr = mono_array_new(currentDomain, floatClass, 5);
	mono_array_set(arr, float, 0, result.position.x);
	mono_array_set(arr, float, 1, result.position.y);
	mono_array_set(arr, float, 2, result.scale.x);
	mono_array_set(arr, float, 3, result.scale.y);
	mono_array_set(arr, float, 4, result.rotation);
	return arr;
}

MonoArray* mono_ImAnim_TransformFromMatrix(float m00, float m01, float m10, float m11, float tx, float ty)
{
	iam_transform result = iam_transform_from_matrix(m00, m01, m10, m11, tx, ty);
	MonoDomain* currentDomain = mono_domain_get();
	MonoClass* floatClass = mono_get_single_class();
	MonoArray* arr = mono_array_new(currentDomain, floatClass, 5);
	mono_array_set(arr, float, 0, result.position.x);
	mono_array_set(arr, float, 1, result.position.y);
	mono_array_set(arr, float, 2, result.scale.x);
	mono_array_set(arr, float, 3, result.scale.y);
	mono_array_set(arr, float, 4, result.rotation);
	return arr;
}

MonoArray* mono_ImAnim_TransformToMatrix(float pos_x, float pos_y, float scale_x, float scale_y, float rotation)
{
	iam_transform t(ImVec2(pos_x, pos_y), rotation, ImVec2(scale_x, scale_y));
	float mat[6];
	iam_transform_to_matrix(t, mat);
	MonoDomain* currentDomain = mono_domain_get();
	MonoClass* floatClass = mono_get_single_class();
	MonoArray* arr = mono_array_new(currentDomain, floatClass, 6);
	mono_array_set(arr, float, 0, mat[0]);
	mono_array_set(arr, float, 1, mat[1]);
	mono_array_set(arr, float, 2, mat[2]);
	mono_array_set(arr, float, 3, mat[3]);
	mono_array_set(arr, float, 4, mat[4]);
	mono_array_set(arr, float, 5, mat[5]);
	return arr;
}

// ============================================================================
// Clip System - Lifecycle
// ============================================================================

void mono_ImAnim_ClipInit(int initial_clip_cap, int initial_inst_cap)
{
	iam_clip_init(initial_clip_cap, initial_inst_cap);
}

void mono_ImAnim_ClipShutdown()
{
	iam_clip_shutdown();
}

void mono_ImAnim_ClipUpdate(float dt)
{
	iam_clip_update(dt);
}

void mono_ImAnim_ClipGC(unsigned int max_age_frames)
{
	iam_clip_gc(max_age_frames);
}

// ============================================================================
// Clip System - Playback
// ============================================================================

void mono_ImAnim_Play(unsigned int clip_id, unsigned int instance_id)
{
	iam_play(clip_id, instance_id);
}

void mono_ImAnim_PlayStagger(unsigned int clip_id, unsigned int instance_id, int index)
{
	iam_play_stagger(clip_id, instance_id, index);
}

float mono_ImAnim_StaggerDelay(unsigned int clip_id, int index)
{
	return iam_stagger_delay(clip_id, index);
}

float mono_ImAnim_ClipDuration(unsigned int clip_id)
{
	return iam_clip_duration(clip_id);
}

bool mono_ImAnim_ClipExists(unsigned int clip_id)
{
	return iam_clip_exists(clip_id);
}

unsigned int mono_ImAnim_GetInstance(unsigned int instance_id)
{
	iam_instance inst = iam_get_instance(instance_id);
	return inst.valid() ? inst.id() : 0;
}

int mono_ImAnim_ClipSave(unsigned int clip_id, MonoString* path)
{
	std::string savePath = MonoStringToStdString(path);
	return static_cast<int>(iam_clip_save(clip_id, savePath.c_str()));
}

MonoArray* mono_ImAnim_ClipLoad(MonoString* path)
{
	std::string loadPath = MonoStringToStdString(path);
	unsigned int clipId = 0;
	iam_result result = iam_clip_load(loadPath.c_str(), &clipId);
	return MakeIntArray2(static_cast<int>(result), static_cast<int>(clipId));
}

// ============================================================================
// Clip System - Instance Queries & Control
// ============================================================================

void mono_ImAnim_InstancePause(unsigned int instance_id)
{
	iam_instance(instance_id).pause();
}

void mono_ImAnim_InstanceResume(unsigned int instance_id)
{
	iam_instance(instance_id).resume();
}

void mono_ImAnim_InstanceStop(unsigned int instance_id)
{
	iam_instance(instance_id).stop();
}

void mono_ImAnim_InstanceDestroy(unsigned int instance_id)
{
	iam_instance(instance_id).destroy();
}

void mono_ImAnim_InstanceSeek(unsigned int instance_id, float time)
{
	iam_instance(instance_id).seek(time);
}

void mono_ImAnim_InstanceSetTimeScale(unsigned int instance_id, float scale)
{
	iam_instance(instance_id).set_time_scale(scale);
}

void mono_ImAnim_InstanceSetWeight(unsigned int instance_id, float weight)
{
	iam_instance(instance_id).set_weight(weight);
}

void mono_ImAnim_InstanceThen(unsigned int instance_id, unsigned int next_clip_id)
{
	iam_instance(instance_id).then(next_clip_id);
}

void mono_ImAnim_InstanceThenDelay(unsigned int instance_id, float delay)
{
	iam_instance(instance_id).then_delay(delay);
}

float mono_ImAnim_InstanceTime(unsigned int instance_id)
{
	return iam_instance(instance_id).time();
}

float mono_ImAnim_InstanceDuration(unsigned int instance_id)
{
	return iam_instance(instance_id).duration();
}

bool mono_ImAnim_InstanceIsPlaying(unsigned int instance_id)
{
	return iam_instance(instance_id).is_playing();
}

bool mono_ImAnim_InstanceIsPaused(unsigned int instance_id)
{
	return iam_instance(instance_id).is_paused();
}

bool mono_ImAnim_InstanceValid(unsigned int instance_id)
{
	return iam_instance(instance_id).valid();
}

float mono_ImAnim_InstanceGetFloat(unsigned int instance_id, unsigned int channel)
{
	float out = 0.0f;
	iam_instance(instance_id).get_float(channel, &out);
	return out;
}

MonoArray* mono_ImAnim_InstanceGetVec2(unsigned int instance_id, unsigned int channel)
{
	ImVec2 out(0, 0);
	iam_instance(instance_id).get_vec2(channel, &out);
	return MakeFloatArray2(out.x, out.y);
}

MonoArray* mono_ImAnim_InstanceGetVec4(unsigned int instance_id, unsigned int channel)
{
	ImVec4 out(0, 0, 0, 0);
	iam_instance(instance_id).get_vec4(channel, &out);
	return MakeFloatArray4(out.x, out.y, out.z, out.w);
}

int mono_ImAnim_InstanceGetInt(unsigned int instance_id, unsigned int channel)
{
	int out = 0;
	iam_instance(instance_id).get_int(channel, &out);
	return out;
}

MonoArray* mono_ImAnim_InstanceGetColor(unsigned int instance_id, unsigned int channel, int color_space)
{
	ImVec4 out(0, 0, 0, 0);
	iam_instance(instance_id).get_color(channel, &out, color_space);
	return MakeFloatArray4(out.x, out.y, out.z, out.w);
}

// ============================================================================
// Clip System - Layering
// ============================================================================

void mono_ImAnim_LayerBegin(unsigned int instance_id)
{
	iam_layer_begin(instance_id);
}

void mono_ImAnim_LayerAdd(unsigned int instance_id, float weight)
{
	iam_layer_add(iam_instance(instance_id), weight);
}

void mono_ImAnim_LayerEnd(unsigned int instance_id)
{
	iam_layer_end(instance_id);
}

float mono_ImAnim_GetBlendedFloat(unsigned int instance_id, unsigned int channel)
{
	float out = 0.0f;
	iam_get_blended_float(instance_id, channel, &out);
	return out;
}

MonoArray* mono_ImAnim_GetBlendedVec2(unsigned int instance_id, unsigned int channel)
{
	ImVec2 out(0, 0);
	iam_get_blended_vec2(instance_id, channel, &out);
	return MakeFloatArray2(out.x, out.y);
}

MonoArray* mono_ImAnim_GetBlendedVec4(unsigned int instance_id, unsigned int channel)
{
	ImVec4 out(0, 0, 0, 0);
	iam_get_blended_vec4(instance_id, channel, &out);
	return MakeFloatArray4(out.x, out.y, out.z, out.w);
}

int mono_ImAnim_GetBlendedInt(unsigned int instance_id, unsigned int channel)
{
	int out = 0;
	iam_get_blended_int(instance_id, channel, &out);
	return out;
}

// ============================================================================
// Clip Builder
// ============================================================================

void mono_ImAnim_ClipBegin(unsigned int clip_id)
{
	g_monoImAnimClips.insert_or_assign(clip_id, iam_clip::begin(clip_id));
}

void mono_ImAnim_ClipKeyFloat(unsigned int clip_id, unsigned int channel, float time, float value, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_float(channel, time, value, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyVec2(unsigned int clip_id, unsigned int channel, float time, float val_x, float val_y, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_vec2(channel, time, ImVec2(val_x, val_y), ease_type, pBezier);
}

void mono_ImAnim_ClipKeyVec4(unsigned int clip_id, unsigned int channel, float time, float val_x, float val_y, float val_z, float val_w, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_vec4(channel, time, ImVec4(val_x, val_y, val_z, val_w), ease_type, pBezier);
}

void mono_ImAnim_ClipKeyInt(unsigned int clip_id, unsigned int channel, float time, int value, int ease_type)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	it->second.key_int(channel, time, value, ease_type);
}

void mono_ImAnim_ClipKeyColor(unsigned int clip_id, unsigned int channel, float time, float val_r, float val_g, float val_b, float val_a, int color_space, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_color(channel, time, ImVec4(val_r, val_g, val_b, val_a), color_space, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyFloatVar(unsigned int clip_id, unsigned int channel, float time, float value, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	iam_variation_float var = { var_mode, var_amount, min_clamp, max_clamp, seed, nullptr, nullptr };
	it->second.key_float_var(channel, time, value, var, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyVec2Var(unsigned int clip_id, unsigned int channel, float time, float value_x, float value_y, int var_mode, float amount_x, float amount_y, float min_x, float min_y, float max_x, float max_y, unsigned int seed, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	iam_variation_vec2 var = { var_mode, ImVec2(amount_x, amount_y), ImVec2(min_x, min_y), ImVec2(max_x, max_y), seed, nullptr, nullptr, {}, {} };
	it->second.key_vec2_var(channel, time, ImVec2(value_x, value_y), var, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyVec4Var(unsigned int clip_id, unsigned int channel, float time, float value_x, float value_y, float value_z, float value_w, int var_mode, float amount_x, float amount_y, float amount_z, float amount_w, float min_x, float min_y, float min_z, float min_w, float max_x, float max_y, float max_z, float max_w, unsigned int seed, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	iam_variation_vec4 var = { var_mode, ImVec4(amount_x, amount_y, amount_z, amount_w), ImVec4(min_x, min_y, min_z, min_w), ImVec4(max_x, max_y, max_z, max_w), seed, nullptr, nullptr, {}, {}, {}, {} };
	it->second.key_vec4_var(channel, time, ImVec4(value_x, value_y, value_z, value_w), var, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyIntVar(unsigned int clip_id, unsigned int channel, float time, int value, int var_mode, int var_amount, int min_clamp, int max_clamp, unsigned int seed, int ease_type)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	iam_variation_int var = { var_mode, var_amount, min_clamp, max_clamp, seed, nullptr, nullptr };
	it->second.key_int_var(channel, time, value, var, ease_type);
}

void mono_ImAnim_ClipKeyColorVar(unsigned int clip_id, unsigned int channel, float time, float value_r, float value_g, float value_b, float value_a, int var_mode, float amount_r, float amount_g, float amount_b, float amount_a, float min_r, float min_g, float min_b, float min_a, float max_r, float max_g, float max_b, float max_a, int var_color_space, unsigned int seed, int color_space, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	iam_variation_color var = { var_mode, ImVec4(amount_r, amount_g, amount_b, amount_a), ImVec4(min_r, min_g, min_b, min_a), ImVec4(max_r, max_g, max_b, max_a), var_color_space, seed, nullptr, nullptr, {}, {}, {}, {} };
	it->second.key_color_var(channel, time, ImVec4(value_r, value_g, value_b, value_a), var, color_space, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyFloatSpring(unsigned int clip_id, unsigned int channel, float time, float target, float mass, float stiffness, float damping, float initial_velocity)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	iam_spring_params spring{ mass, stiffness, damping, initial_velocity };
	it->second.key_float_spring(channel, time, target, spring);
}

void mono_ImAnim_ClipKeyFloatRel(unsigned int clip_id, unsigned int channel, float time, float percent, float px_bias, int anchor_space, int axis, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_float_rel(channel, time, percent, px_bias, anchor_space, axis, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyVec2Rel(unsigned int clip_id, unsigned int channel, float time, float percent_x, float percent_y, float bias_x, float bias_y, int anchor_space, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_vec2_rel(channel, time, ImVec2(percent_x, percent_y), ImVec2(bias_x, bias_y), anchor_space, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyVec4Rel(unsigned int clip_id, unsigned int channel, float time, float percent_x, float percent_y, float percent_z, float percent_w, float bias_x, float bias_y, float bias_z, float bias_w, int anchor_space, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_vec4_rel(channel, time, ImVec4(percent_x, percent_y, percent_z, percent_w), ImVec4(bias_x, bias_y, bias_z, bias_w), anchor_space, ease_type, pBezier);
}

void mono_ImAnim_ClipKeyColorRel(unsigned int clip_id, unsigned int channel, float time, float percent_r, float percent_g, float percent_b, float percent_a, float bias_r, float bias_g, float bias_b, float bias_a, int color_space, int anchor_space, int ease_type, float b0, float b1, float b2, float b3)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	float bezier[4] = { b0, b1, b2, b3 };
	float* pBezier = (ease_type == iam_ease_cubic_bezier) ? bezier : nullptr;
	it->second.key_color_rel(channel, time, ImVec4(percent_r, percent_g, percent_b, percent_a), ImVec4(bias_r, bias_g, bias_b, bias_a), color_space, anchor_space, ease_type, pBezier);
}

void mono_ImAnim_ClipSeqBegin(unsigned int clip_id)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
		it->second.seq_begin();
}

void mono_ImAnim_ClipSeqEnd(unsigned int clip_id)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
		it->second.seq_end();
}

void mono_ImAnim_ClipParBegin(unsigned int clip_id)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
		it->second.par_begin();
}

void mono_ImAnim_ClipParEnd(unsigned int clip_id)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
		it->second.par_end();
}

void mono_ImAnim_ClipMarker(unsigned int clip_id, float time, unsigned int marker_id, MonoObject* delegate)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end() || !delegate) return;
	auto* ref = CreateMarkerCallbackRef(delegate);
	it->second.marker(time, marker_id, &MonoMarkerCallbackThunk, ref);
}

void mono_ImAnim_ClipSetLoop(unsigned int clip_id, bool loop, int direction, int loop_count)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
		it->second.set_loop(loop, direction, loop_count);
}

void mono_ImAnim_ClipSetDelay(unsigned int clip_id, float delay_seconds)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
		it->second.set_delay(delay_seconds);
}

void mono_ImAnim_ClipSetStagger(unsigned int clip_id, int count, float each_delay, float from_center_bias)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
		it->second.set_stagger(count, each_delay, from_center_bias);
}

void mono_ImAnim_ClipSetDurationVar(unsigned int clip_id, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	iam_variation_float var = { var_mode, var_amount, min_clamp, max_clamp, seed, nullptr, nullptr };
	it->second.set_duration_var(var);
}

void mono_ImAnim_ClipSetDelayVar(unsigned int clip_id, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	iam_variation_float var = { var_mode, var_amount, min_clamp, max_clamp, seed, nullptr, nullptr };
	it->second.set_delay_var(var);
}

void mono_ImAnim_ClipSetTimescaleVar(unsigned int clip_id, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end()) return;
	iam_variation_float var = { var_mode, var_amount, min_clamp, max_clamp, seed, nullptr, nullptr };
	it->second.set_timescale_var(var);
}

void mono_ImAnim_ClipOnBegin(unsigned int clip_id, MonoObject* delegate)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end() || !delegate) return;
	auto* ref = CreateClipCallbackRef(delegate);
	it->second.on_begin(&MonoClipCallbackThunk, ref);
}

void mono_ImAnim_ClipOnUpdate(unsigned int clip_id, MonoObject* delegate)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end() || !delegate) return;
	auto* ref = CreateClipCallbackRef(delegate);
	it->second.on_update(&MonoClipCallbackThunk, ref);
}

void mono_ImAnim_ClipOnComplete(unsigned int clip_id, MonoObject* delegate)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it == g_monoImAnimClips.end() || !delegate) return;
	auto* ref = CreateClipCallbackRef(delegate);
	it->second.on_complete(&MonoClipCallbackThunk, ref);
}

void mono_ImAnim_ClipEnd(unsigned int clip_id)
{
	auto it = g_monoImAnimClips.find(clip_id);
	if (it != g_monoImAnimClips.end())
	{
		it->second.end();
		g_monoImAnimClips.erase(it);
	}
}

// ============================================================================
// Debug / Inspector
// ============================================================================

void mono_ImAnim_ShowUnifiedInspector()
{
	iam_show_unified_inspector();
}

void mono_ImAnim_ShowDebugTimeline(unsigned int instance_id)
{
	iam_show_debug_timeline(instance_id);
}

void mono_ImAnim_DemoWindow()
{
	ImAnimDemoWindow(nullptr);
}

void mono_ImAnim_DocWindow()
{
	ImAnimDocWindow(nullptr);
}

void mono_ImAnim_UsecaseWindow()
{
	ImAnimUsecaseWindow(nullptr);
}
