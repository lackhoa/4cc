/* date = January 29th 2021 8:01 pm */

#ifndef FCODER_FLEURY_POWER_MODE_H
#define FCODER_FLEURY_POWER_MODE_H

#include "4coder_fleury_ubiquitous.h"

//~ NOTE(rjf): Power Mode API

struct Particle
{
    f32 x;
    f32 y;
    f32 velocity_x;
    f32 velocity_y;
    f32 decay_rate;
    ARGB_Color color;
    f32 alpha;
    f32 roundness;
    f32 scale;
    String string;
    u8 chrs[1];
};

function void F4_PowerMode_SetAllow(b32 allowed);
function b32 F4_PowerMode_IsEnabled(void);
function f32 F4_PowerMode_ScreenShake(void);
function f32 F4_PowerMode_ActiveCharactersPerMinute(void);
function void F4_PowerMode_CharacterPressed(void);
function Particle *
F4_PowerMode_Particle(f32 x, f32 y, f32 velocity_x, f32 velocity_y, f32 decay_rate, ARGB_Color color,
                      f32 roundness, f32 scale, String str);
function Vec2_f32 F4_PowerMode_CameraOffsetFromView(App *app, View_ID view);
function void F4_PowerMode_Spawn(App *app, View_ID view, u8 character);
function void F4_PowerMode_Tick(App *app, Frame_Info frame_info);
function void F4_PowerMode_RenderBuffer(App *app, View_ID view, Face_ID face, Frame_Info frame_info);
function void F4_PowerMode_RenderWholeScreen(App *app, Frame_Info frame_info);

#endif // FCODER_FLEURY_POWER_MODE_H
