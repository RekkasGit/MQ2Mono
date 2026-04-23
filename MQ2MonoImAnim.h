#pragma once

#include <mq/Plugin.h>
#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>
#include <cstdint>

// ============================================================================
// MQ2Mono ImAnim wrappers
// Exposes ImAnim animation APIs to C# via Mono internal calls.
// Namespace on C# side: MonoCore.E3ImAnim
// ============================================================================

// ----------------------------------------------------------------------------
// Frame / Global Management
// ----------------------------------------------------------------------------
void mono_ImAnim_UpdateBeginFrame();
void mono_ImAnim_GC(unsigned int max_age_frames);
void mono_ImAnim_PoolClear();
void mono_ImAnim_Reserve(int cap_float, int cap_vec2, int cap_vec4, int cap_int, int cap_color);
void mono_ImAnim_SetEaseLutSamples(int count);
void mono_ImAnim_SetGlobalTimeScale(float scale);
float mono_ImAnim_GetGlobalTimeScale();
void mono_ImAnim_SetLazyInit(bool enable);
bool mono_ImAnim_IsLazyInitEnabled();
void mono_ImAnim_RegisterCustomEase(int slot, MonoObject* delegate);
MonoObject* mono_ImAnim_GetCustomEase(int slot);

// ----------------------------------------------------------------------------
// Context / Profiler
// ----------------------------------------------------------------------------
uint64_t mono_ImAnim_ContextCreate();
void mono_ImAnim_ContextDestroy(uint64_t ctx_ptr);
uint64_t mono_ImAnim_ContextSetCurrent(uint64_t ctx_ptr);
void mono_ImAnim_ContextSetUserData(uint64_t ctx_ptr, uint64_t user_data);
uint64_t mono_ImAnim_ContextGetCurrent();
uint64_t mono_ImAnim_ContextGetUserData();
uint64_t mono_ImAnim_ContextGetDefaultContext();
void mono_ImAnim_ProfilerEnable(bool enable);
bool mono_ImAnim_ProfilerIsEnabled();
void mono_ImAnim_ProfilerBeginFrame();
void mono_ImAnim_ProfilerEndFrame();
void mono_ImAnim_ProfilerBegin(MonoString* name);
void mono_ImAnim_ProfilerEnd();

// ----------------------------------------------------------------------------
// Drag Feedback
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_DragBegin(unsigned int id, float pos_x, float pos_y);
MonoArray* mono_ImAnim_DragUpdate(unsigned int id, float pos_x, float pos_y, float dt);
MonoArray* mono_ImAnim_DragRelease(unsigned int id, float pos_x, float pos_y, MonoArray* snap_points_xy, float snap_grid_x, float snap_grid_y, float snap_duration, float overshoot, int ease_type, float dt);
void mono_ImAnim_DragCancel(unsigned int id);

// ----------------------------------------------------------------------------
// Easing Evaluation
// ----------------------------------------------------------------------------
float mono_ImAnim_EvalPreset(int type, float t);

// ----------------------------------------------------------------------------
// Core Tween API
// ----------------------------------------------------------------------------
float mono_ImAnim_TweenFloat(unsigned int id, unsigned int channel_id, float target, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, float init_value);
MonoArray* mono_ImAnim_TweenVec2(unsigned int id, unsigned int channel_id, float target_x, float target_y, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, float init_x, float init_y);
MonoArray* mono_ImAnim_TweenVec4(unsigned int id, unsigned int channel_id, float target_x, float target_y, float target_z, float target_w, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, float init_x, float init_y, float init_z, float init_w);
int mono_ImAnim_TweenInt(unsigned int id, unsigned int channel_id, int target, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt, int init_value);
MonoArray* mono_ImAnim_TweenColor(unsigned int id, unsigned int channel_id, float target_r, float target_g, float target_b, float target_a, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, float dt, float init_r, float init_g, float init_b, float init_a);

// ----------------------------------------------------------------------------
// Relative Tween API (resize-safe)
// ----------------------------------------------------------------------------
float mono_ImAnim_TweenFloatRel(unsigned int id, unsigned int channel_id, float percent, float px_bias, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int anchor_space, int axis, float dt);
MonoArray* mono_ImAnim_TweenVec2Rel(unsigned int id, unsigned int channel_id, float pct_x, float pct_y, float bias_x, float bias_y, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int anchor_space, float dt);
MonoArray* mono_ImAnim_TweenVec4Rel(unsigned int id, unsigned int channel_id, float pct_x, float pct_y, float pct_z, float pct_w, float bias_x, float bias_y, float bias_z, float bias_w, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int anchor_space, float dt);
MonoArray* mono_ImAnim_TweenColorRel(unsigned int id, unsigned int channel_id, float pct_r, float pct_g, float pct_b, float pct_a, float bias_r, float bias_g, float bias_b, float bias_a, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, int anchor_space, float dt);

// ----------------------------------------------------------------------------
// Resolved Tween API
// ----------------------------------------------------------------------------
float mono_ImAnim_TweenFloatResolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt);
MonoArray* mono_ImAnim_TweenVec2Resolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt);
MonoArray* mono_ImAnim_TweenVec4Resolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt);
MonoArray* mono_ImAnim_TweenColorResolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, float dt);
int mono_ImAnim_TweenIntResolved(unsigned int id, unsigned int channel_id, MonoObject* resolver, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt);

// ----------------------------------------------------------------------------
// Per-axis Tween API
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_TweenVec2PerAxis(unsigned int id, unsigned int channel_id, float target_x, float target_y, float dur, int ease_x_type, float ease_x_p0, float ease_x_p1, float ease_x_p2, float ease_x_p3, int ease_y_type, float ease_y_p0, float ease_y_p1, float ease_y_p2, float ease_y_p3, int policy, float dt);
MonoArray* mono_ImAnim_TweenVec4PerAxis(unsigned int id, unsigned int channel_id, float target_x, float target_y, float target_z, float target_w, float dur, int ease_x_type, float ease_x_p0, float ease_x_p1, float ease_x_p2, float ease_x_p3, int ease_y_type, float ease_y_p0, float ease_y_p1, float ease_y_p2, float ease_y_p3, int ease_z_type, float ease_z_p0, float ease_z_p1, float ease_z_p2, float ease_z_p3, int ease_w_type, float ease_w_p0, float ease_w_p1, float ease_w_p2, float ease_w_p3, int policy, float dt);
MonoArray* mono_ImAnim_TweenColorPerAxis(unsigned int id, unsigned int channel_id, float target_r, float target_g, float target_b, float target_a, float dur, int ease_r_type, float ease_r_p0, float ease_r_p1, float ease_r_p2, float ease_r_p3, int ease_g_type, float ease_g_p0, float ease_g_p1, float ease_g_p2, float ease_g_p3, int ease_b_type, float ease_b_p0, float ease_b_p1, float ease_b_p2, float ease_b_p3, int ease_a_type, float ease_a_p0, float ease_a_p1, float ease_a_p2, float ease_a_p3, int policy, int color_space, float dt);

// ----------------------------------------------------------------------------
// Rebase API
// ----------------------------------------------------------------------------
void mono_ImAnim_RebaseFloat(unsigned int id, unsigned int channel_id, float new_target, float dt);
void mono_ImAnim_RebaseVec2(unsigned int id, unsigned int channel_id, float new_x, float new_y, float dt);
void mono_ImAnim_RebaseVec4(unsigned int id, unsigned int channel_id, float new_x, float new_y, float new_z, float new_w, float dt);
void mono_ImAnim_RebaseColor(unsigned int id, unsigned int channel_id, float new_r, float new_g, float new_b, float new_a, float dt);
void mono_ImAnim_RebaseInt(unsigned int id, unsigned int channel_id, int new_target, float dt);

// ----------------------------------------------------------------------------
// Color Blending
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_GetBlendedColor(float a_r, float a_g, float a_b, float a_a, float b_r, float b_g, float b_b, float b_a, float t, int color_space);

// ----------------------------------------------------------------------------
// Oscillators
// ----------------------------------------------------------------------------
float mono_ImAnim_Oscillate(unsigned int id, float amplitude, float frequency, int wave_type, float phase, float dt);
int mono_ImAnim_OscillateInt(unsigned int id, int amplitude, float frequency, int wave_type, float phase, float dt);
MonoArray* mono_ImAnim_OscillateVec2(unsigned int id, float amp_x, float amp_y, float freq_x, float freq_y, int wave_type, float phase_x, float phase_y, float dt);
MonoArray* mono_ImAnim_OscillateVec4(unsigned int id, float amp_x, float amp_y, float amp_z, float amp_w, float freq_x, float freq_y, float freq_z, float freq_w, int wave_type, float phase_x, float phase_y, float phase_z, float phase_w, float dt);
MonoArray* mono_ImAnim_OscillateColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float frequency, int wave_type, float phase, int color_space, float dt);

// ----------------------------------------------------------------------------
// Shake / Wiggle
// ----------------------------------------------------------------------------
float mono_ImAnim_Shake(unsigned int id, float intensity, float frequency, float decay_time, float dt);
int mono_ImAnim_ShakeInt(unsigned int id, int intensity, float frequency, float decay_time, float dt);
MonoArray* mono_ImAnim_ShakeVec2(unsigned int id, float int_x, float int_y, float frequency, float decay_time, float dt);
MonoArray* mono_ImAnim_ShakeVec4(unsigned int id, float int_x, float int_y, float int_z, float int_w, float frequency, float decay_time, float dt);
MonoArray* mono_ImAnim_ShakeColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float int_r, float int_g, float int_b, float int_a, float frequency, float decay_time, int color_space, float dt);
float mono_ImAnim_Wiggle(unsigned int id, float amplitude, float frequency, float dt);
int mono_ImAnim_WiggleInt(unsigned int id, int amplitude, float frequency, float dt);
MonoArray* mono_ImAnim_WiggleVec2(unsigned int id, float amp_x, float amp_y, float frequency, float dt);
MonoArray* mono_ImAnim_WiggleVec4(unsigned int id, float amp_x, float amp_y, float amp_z, float amp_w, float frequency, float dt);
MonoArray* mono_ImAnim_WiggleColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float frequency, int color_space, float dt);
void mono_ImAnim_TriggerShake(unsigned int id);

// ----------------------------------------------------------------------------
// Scroll Animation
// ----------------------------------------------------------------------------
void mono_ImAnim_ScrollToY(float target_y, float duration, int ease_type, float p0, float p1, float p2, float p3);
void mono_ImAnim_ScrollToX(float target_x, float duration, int ease_type, float p0, float p1, float p2, float p3);
void mono_ImAnim_ScrollToTop(float duration, int ease_type, float p0, float p1, float p2, float p3);
void mono_ImAnim_ScrollToBottom(float duration, int ease_type, float p0, float p1, float p2, float p3);

// ----------------------------------------------------------------------------
// Anchor Size
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_AnchorSize(int space);

// ----------------------------------------------------------------------------
// Motion Path - Stateless Evaluation
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_BezierQuadratic(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float t);
MonoArray* mono_ImAnim_BezierCubic(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t);
MonoArray* mono_ImAnim_CatmullRom(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t, float tension);
MonoArray* mono_ImAnim_BezierQuadraticDeriv(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float t);
MonoArray* mono_ImAnim_BezierCubicDeriv(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t);
MonoArray* mono_ImAnim_CatmullRomDeriv(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float t, float tension);

// ----------------------------------------------------------------------------
// Motion Path - Builder (fluent API wrapped via ID map)
// ----------------------------------------------------------------------------
void mono_ImAnim_PathBegin(unsigned int path_id, float start_x, float start_y);
void mono_ImAnim_PathLineTo(unsigned int path_id, float end_x, float end_y);
void mono_ImAnim_PathQuadraticTo(unsigned int path_id, float ctrl_x, float ctrl_y, float end_x, float end_y);
void mono_ImAnim_PathCubicTo(unsigned int path_id, float ctrl1_x, float ctrl1_y, float ctrl2_x, float ctrl2_y, float end_x, float end_y);
void mono_ImAnim_PathCatmullTo(unsigned int path_id, float end_x, float end_y, float tension);
void mono_ImAnim_PathClose(unsigned int path_id);
void mono_ImAnim_PathEnd(unsigned int path_id);

// ----------------------------------------------------------------------------
// Motion Path - Queries
// ----------------------------------------------------------------------------
bool mono_ImAnim_PathExists(unsigned int path_id);
float mono_ImAnim_PathLength(unsigned int path_id);
MonoArray* mono_ImAnim_PathEvaluate(unsigned int path_id, float t);
MonoArray* mono_ImAnim_PathTangent(unsigned int path_id, float t);
float mono_ImAnim_PathAngle(unsigned int path_id, float t);

// ----------------------------------------------------------------------------
// Motion Path - Tweens
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_TweenPath(unsigned int id, unsigned int channel_id, unsigned int path_id, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt);
float mono_ImAnim_TweenPathAngle(unsigned int id, unsigned int channel_id, unsigned int path_id, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, float dt);

// ----------------------------------------------------------------------------
// Motion Path - Arc Length
// ----------------------------------------------------------------------------
void mono_ImAnim_PathBuildArcLUT(unsigned int path_id, int subdivisions);
bool mono_ImAnim_PathHasArcLUT(unsigned int path_id);
float mono_ImAnim_PathDistanceToT(unsigned int path_id, float distance);
MonoArray* mono_ImAnim_PathEvaluateAtDistance(unsigned int path_id, float distance);
float mono_ImAnim_PathAngleAtDistance(unsigned int path_id, float distance);
MonoArray* mono_ImAnim_PathTangentAtDistance(unsigned int path_id, float distance);

// ----------------------------------------------------------------------------
// Motion Path - Morphing / Text
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_PathMorph(unsigned int path_a, unsigned int path_b, float t, float blend, int samples, bool match_endpoints, bool use_arc_length);
MonoArray* mono_ImAnim_PathMorphTangent(unsigned int path_a, unsigned int path_b, float t, float blend, int samples, bool match_endpoints, bool use_arc_length);
float mono_ImAnim_PathMorphAngle(unsigned int path_a, unsigned int path_b, float t, float blend, int samples, bool match_endpoints, bool use_arc_length);
MonoArray* mono_ImAnim_TweenPathMorph(unsigned int id, unsigned int channel_id, unsigned int path_a, unsigned int path_b, float target_blend, float dur, int path_ease_type, float path_p0, float path_p1, float path_p2, float path_p3, int morph_ease_type, float morph_p0, float morph_p1, float morph_p2, float morph_p3, int policy, float dt, int samples, bool match_endpoints, bool use_arc_length);
float mono_ImAnim_GetMorphBlend(unsigned int id, unsigned int channel_id);
void mono_ImAnim_TextPath(unsigned int path_id, MonoString* text, float origin_x, float origin_y, float offset, float letter_spacing, int align, bool flip_y, uint32_t color, float font_scale);
void mono_ImAnim_TextPathAnimated(unsigned int path_id, MonoString* text, float progress, float origin_x, float origin_y, float offset, float letter_spacing, int align, bool flip_y, uint32_t color, float font_scale);
float mono_ImAnim_TextPathWidth(MonoString* text, float origin_x, float origin_y, float offset, float letter_spacing, int align, bool flip_y, uint32_t color, float font_scale);
MonoArray* mono_ImAnim_TransformQuad(float center_x, float center_y, float angle_rad, float translation_x, float translation_y, float q0_x, float q0_y, float q1_x, float q1_y, float q2_x, float q2_y, float q3_x, float q3_y);
MonoArray* mono_ImAnim_MakeGlyphQuad(float pos_x, float pos_y, float angle_rad, float glyph_width, float glyph_height, float baseline_offset);

// ----------------------------------------------------------------------------
// Text Stagger
// ----------------------------------------------------------------------------
void mono_ImAnim_TextStagger(unsigned int id, MonoString* text, float progress, float pos_x, float pos_y, int effect, float char_delay, float char_duration, float effect_intensity, int ease_type, float ease_p0, float ease_p1, float ease_p2, float ease_p3, uint32_t color, float font_scale, float letter_spacing);
float mono_ImAnim_TextStaggerWidth(MonoString* text, float letter_spacing, float font_scale);
float mono_ImAnim_TextStaggerDuration(MonoString* text, float char_delay, float char_duration, float letter_spacing, float font_scale);

// ----------------------------------------------------------------------------
// Noise
// ----------------------------------------------------------------------------
float mono_ImAnim_Noise2D(float x, float y, int type, int octaves, float persistence, float lacunarity, int seed);
float mono_ImAnim_Noise3D(float x, float y, float z, int type, int octaves, float persistence, float lacunarity, int seed);

float mono_ImAnim_NoiseChannelFloat(unsigned int id, float frequency, float amplitude, int type, int octaves, float persistence, float lacunarity, int seed, float dt);
MonoArray* mono_ImAnim_NoiseChannelVec2(unsigned int id, float freq_x, float freq_y, float amp_x, float amp_y, int type, int octaves, float persistence, float lacunarity, int seed, float dt);
MonoArray* mono_ImAnim_NoiseChannelVec4(unsigned int id, float freq_x, float freq_y, float freq_z, float freq_w, float amp_x, float amp_y, float amp_z, float amp_w, int type, int octaves, float persistence, float lacunarity, int seed, float dt);
MonoArray* mono_ImAnim_NoiseChannelColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float frequency, int type, int octaves, float persistence, float lacunarity, int seed, int color_space, float dt);

float mono_ImAnim_SmoothNoiseFloat(unsigned int id, float amplitude, float speed, float dt);
MonoArray* mono_ImAnim_SmoothNoiseVec2(unsigned int id, float amp_x, float amp_y, float speed, float dt);
MonoArray* mono_ImAnim_SmoothNoiseVec4(unsigned int id, float amp_x, float amp_y, float amp_z, float amp_w, float speed, float dt);
MonoArray* mono_ImAnim_SmoothNoiseColor(unsigned int id, float base_r, float base_g, float base_b, float base_a, float amp_r, float amp_g, float amp_b, float amp_a, float speed, int color_space, float dt);

// ----------------------------------------------------------------------------
// Style Interpolation
// ----------------------------------------------------------------------------
void mono_ImAnim_StyleRegister(unsigned int style_id, MonoArray* style_data);
void mono_ImAnim_StyleRegisterCurrent(unsigned int style_id);
void mono_ImAnim_StyleBlend(unsigned int style_a, unsigned int style_b, float t, int color_space);
void mono_ImAnim_StyleTween(unsigned int id, unsigned int target_style, float duration, int ease_type, float p0, float p1, float p2, float p3, int color_space, float dt);
MonoArray* mono_ImAnim_StyleBlendTo(unsigned int style_a, unsigned int style_b, float t, int color_space);
bool mono_ImAnim_StyleExists(unsigned int style_id);
void mono_ImAnim_StyleUnregister(unsigned int style_id);

// ----------------------------------------------------------------------------
// Gradients
// ----------------------------------------------------------------------------
void mono_ImAnim_GradientBegin(unsigned int gradient_id);
void mono_ImAnim_GradientClear(unsigned int gradient_id);
void mono_ImAnim_GradientAddStop(unsigned int gradient_id, float position, float r, float g, float b, float a);
bool mono_ImAnim_GradientExists(unsigned int gradient_id);
void mono_ImAnim_GradientDestroy(unsigned int gradient_id);
MonoArray* mono_ImAnim_GradientSample(unsigned int gradient_id, float t, int color_space);
MonoArray* mono_ImAnim_GradientGetData(unsigned int gradient_id);
void mono_ImAnim_GradientLerp(unsigned int gradient_a, unsigned int gradient_b, float t, int color_space, unsigned int out_gradient_id);
MonoArray* mono_ImAnim_TweenGradient(unsigned int id, unsigned int channel_id, unsigned int target_gradient_id, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int color_space, float dt);

// ----------------------------------------------------------------------------
// Transform Interpolation
// ----------------------------------------------------------------------------
MonoArray* mono_ImAnim_TransformLerp(float a_pos_x, float a_pos_y, float a_scale_x, float a_scale_y, float a_rot, float b_pos_x, float b_pos_y, float b_scale_x, float b_scale_y, float b_rot, float t, int rotation_mode);
MonoArray* mono_ImAnim_TweenTransform(unsigned int id, unsigned int channel_id, float target_pos_x, float target_pos_y, float target_scale_x, float target_scale_y, float target_rot, float dur, int ease_type, float p0, float p1, float p2, float p3, int policy, int rotation_mode, float dt);
MonoArray* mono_ImAnim_TransformFromMatrix(float m00, float m01, float m10, float m11, float tx, float ty);
MonoArray* mono_ImAnim_TransformToMatrix(float pos_x, float pos_y, float scale_x, float scale_y, float rotation);

// ----------------------------------------------------------------------------
// Clip System - Lifecycle
// ----------------------------------------------------------------------------
void mono_ImAnim_ClipInit(int initial_clip_cap, int initial_inst_cap);
void mono_ImAnim_ClipShutdown();
void mono_ImAnim_ClipUpdate(float dt);
void mono_ImAnim_ClipGC(unsigned int max_age_frames);

// ----------------------------------------------------------------------------
// Clip System - Playback
// ----------------------------------------------------------------------------
void mono_ImAnim_Play(unsigned int clip_id, unsigned int instance_id);
void mono_ImAnim_PlayStagger(unsigned int clip_id, unsigned int instance_id, int index);
unsigned int mono_ImAnim_GetInstance(unsigned int instance_id);
float mono_ImAnim_StaggerDelay(unsigned int clip_id, int index);
float mono_ImAnim_ClipDuration(unsigned int clip_id);
bool mono_ImAnim_ClipExists(unsigned int clip_id);
int mono_ImAnim_ClipSave(unsigned int clip_id, MonoString* path);
MonoArray* mono_ImAnim_ClipLoad(MonoString* path);

// ----------------------------------------------------------------------------
// Clip System - Instance Queries & Control
// ----------------------------------------------------------------------------
void mono_ImAnim_InstancePause(unsigned int instance_id);
void mono_ImAnim_InstanceResume(unsigned int instance_id);
void mono_ImAnim_InstanceStop(unsigned int instance_id);
void mono_ImAnim_InstanceDestroy(unsigned int instance_id);
void mono_ImAnim_InstanceSeek(unsigned int instance_id, float time);
void mono_ImAnim_InstanceSetTimeScale(unsigned int instance_id, float scale);
void mono_ImAnim_InstanceSetWeight(unsigned int instance_id, float weight);
void mono_ImAnim_InstanceThen(unsigned int instance_id, unsigned int next_clip_id);
void mono_ImAnim_InstanceThenDelay(unsigned int instance_id, float delay);
float mono_ImAnim_InstanceTime(unsigned int instance_id);
float mono_ImAnim_InstanceDuration(unsigned int instance_id);
bool mono_ImAnim_InstanceIsPlaying(unsigned int instance_id);
bool mono_ImAnim_InstanceIsPaused(unsigned int instance_id);
bool mono_ImAnim_InstanceValid(unsigned int instance_id);

float mono_ImAnim_InstanceGetFloat(unsigned int instance_id, unsigned int channel);
MonoArray* mono_ImAnim_InstanceGetVec2(unsigned int instance_id, unsigned int channel);
MonoArray* mono_ImAnim_InstanceGetVec4(unsigned int instance_id, unsigned int channel);
int mono_ImAnim_InstanceGetInt(unsigned int instance_id, unsigned int channel);
MonoArray* mono_ImAnim_InstanceGetColor(unsigned int instance_id, unsigned int channel, int color_space);

// ----------------------------------------------------------------------------
// Clip System - Layering
// ----------------------------------------------------------------------------
void mono_ImAnim_LayerBegin(unsigned int instance_id);
void mono_ImAnim_LayerAdd(unsigned int instance_id, float weight);
void mono_ImAnim_LayerEnd(unsigned int instance_id);
float mono_ImAnim_GetBlendedFloat(unsigned int instance_id, unsigned int channel);
MonoArray* mono_ImAnim_GetBlendedVec2(unsigned int instance_id, unsigned int channel);
MonoArray* mono_ImAnim_GetBlendedVec4(unsigned int instance_id, unsigned int channel);
int mono_ImAnim_GetBlendedInt(unsigned int instance_id, unsigned int channel);

// ----------------------------------------------------------------------------
// Clip Builder (fluent API wrapped via ID map)
// ----------------------------------------------------------------------------
void mono_ImAnim_ClipBegin(unsigned int clip_id);
void mono_ImAnim_ClipKeyFloat(unsigned int clip_id, unsigned int channel, float time, float value, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyVec2(unsigned int clip_id, unsigned int channel, float time, float val_x, float val_y, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyVec4(unsigned int clip_id, unsigned int channel, float time, float val_x, float val_y, float val_z, float val_w, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyInt(unsigned int clip_id, unsigned int channel, float time, int value, int ease_type);
void mono_ImAnim_ClipKeyColor(unsigned int clip_id, unsigned int channel, float time, float val_r, float val_g, float val_b, float val_a, int color_space, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyFloatVar(unsigned int clip_id, unsigned int channel, float time, float value, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyVec2Var(unsigned int clip_id, unsigned int channel, float time, float value_x, float value_y, int var_mode, float amount_x, float amount_y, float min_x, float min_y, float max_x, float max_y, unsigned int seed, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyVec4Var(unsigned int clip_id, unsigned int channel, float time, float value_x, float value_y, float value_z, float value_w, int var_mode, float amount_x, float amount_y, float amount_z, float amount_w, float min_x, float min_y, float min_z, float min_w, float max_x, float max_y, float max_z, float max_w, unsigned int seed, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyIntVar(unsigned int clip_id, unsigned int channel, float time, int value, int var_mode, int var_amount, int min_clamp, int max_clamp, unsigned int seed, int ease_type);
void mono_ImAnim_ClipKeyColorVar(unsigned int clip_id, unsigned int channel, float time, float value_r, float value_g, float value_b, float value_a, int var_mode, float amount_r, float amount_g, float amount_b, float amount_a, float min_r, float min_g, float min_b, float min_a, float max_r, float max_g, float max_b, float max_a, int var_color_space, unsigned int seed, int color_space, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyFloatSpring(unsigned int clip_id, unsigned int channel, float time, float target, float mass, float stiffness, float damping, float initial_velocity);
void mono_ImAnim_ClipKeyFloatRel(unsigned int clip_id, unsigned int channel, float time, float percent, float px_bias, int anchor_space, int axis, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyVec2Rel(unsigned int clip_id, unsigned int channel, float time, float percent_x, float percent_y, float bias_x, float bias_y, int anchor_space, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyVec4Rel(unsigned int clip_id, unsigned int channel, float time, float percent_x, float percent_y, float percent_z, float percent_w, float bias_x, float bias_y, float bias_z, float bias_w, int anchor_space, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipKeyColorRel(unsigned int clip_id, unsigned int channel, float time, float percent_r, float percent_g, float percent_b, float percent_a, float bias_r, float bias_g, float bias_b, float bias_a, int color_space, int anchor_space, int ease_type, float b0, float b1, float b2, float b3);
void mono_ImAnim_ClipSeqBegin(unsigned int clip_id);
void mono_ImAnim_ClipSeqEnd(unsigned int clip_id);
void mono_ImAnim_ClipParBegin(unsigned int clip_id);
void mono_ImAnim_ClipParEnd(unsigned int clip_id);
void mono_ImAnim_ClipMarker(unsigned int clip_id, float time, unsigned int marker_id, MonoObject* delegate);
void mono_ImAnim_ClipSetLoop(unsigned int clip_id, bool loop, int direction, int loop_count);
void mono_ImAnim_ClipSetDelay(unsigned int clip_id, float delay_seconds);
void mono_ImAnim_ClipSetStagger(unsigned int clip_id, int count, float each_delay, float from_center_bias);
void mono_ImAnim_ClipSetDurationVar(unsigned int clip_id, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed);
void mono_ImAnim_ClipSetDelayVar(unsigned int clip_id, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed);
void mono_ImAnim_ClipSetTimescaleVar(unsigned int clip_id, int var_mode, float var_amount, float min_clamp, float max_clamp, unsigned int seed);
void mono_ImAnim_ClipOnBegin(unsigned int clip_id, MonoObject* delegate);
void mono_ImAnim_ClipOnUpdate(unsigned int clip_id, MonoObject* delegate);
void mono_ImAnim_ClipOnComplete(unsigned int clip_id, MonoObject* delegate);
void mono_ImAnim_ClipEnd(unsigned int clip_id);

// ----------------------------------------------------------------------------
// Debug / Inspector
// ----------------------------------------------------------------------------
void mono_ImAnim_ShowUnifiedInspector();
void mono_ImAnim_ShowDebugTimeline(unsigned int instance_id);
void mono_ImAnim_DemoWindow();
void mono_ImAnim_DocWindow();
void mono_ImAnim_UsecaseWindow();
