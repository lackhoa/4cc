// TODO @Cleanup The editor doesn't need all these so don't put them here?
#pragma once

//#include "4coder_events.h"

#define X_Fui_Types(X) \
X(v1) \
X(v2) \
X(v3) \
X(v4) \
X(i1) \
X(i2) \
X(i3) \
X(i4) \
X(Planar_Bezier) \

enum Fui_Type
{
 Fui_Type_Null,
 
#define X(T)   Fui_Type_##T,
 X_Fui_Types(X)
#undef X
 
 Fui_Type_Count,
};

typedef u32 Fui_Flags;
enum Fui_Flag
{// NOTE: These are total hacks, man!
 Fui_Flag_NULL            = 0x0,
 Fui_Flag_Camera_Aligned  = 0x1,
 Fui_Flag_NOZ             = 0x2,
 Fui_Flag_Clamp_X         = 0x4,
 Fui_Flag_Clamp_Y         = 0x8,
 Fui_Flag_Clamp_Z         = 0x10,
 Fui_Flag_Clamp_01        = 0x20,
 Fui_Flag_Inited          = 1 << 31,
};

struct Fui_Options
{
#define FUI_OPTIONS(X)  \
Fui_Flags flags;        \
v1 delta_scale;         \
 
 FUI_OPTIONS(X);
};

struct Fui_Slider
{
 Fui_Type  type;   // 1
 union
 {
  Fui_Options options;  // 5
  struct { FUI_OPTIONS(X); };
 };
};

#define X(T) \
T \
fval(T init_value_T, Fui_Options options={}, i32 line=__builtin_LINE());
//
X_Fui_Types(X)
//
#undef X

//-
#define fval2(x,y,...)      fval(V2(x,y),     ##__VA_ARGS__)
#define fval3(x,y,z,...)    fval(V3(x,y,z),   ##__VA_ARGS__)
#define fval4(x,y,z,w,...)  fval(V4(x,y,z,w), ##__VA_ARGS__)
#define fvali               fval
//NOTE: "c"=convert, "i"=integer, meaning convert integer to floats
#define fci(...)            i2f6(fval(__VA_ARGS__))
#define fval2i(x,y)         fval(I2(x,y))
#define fval3i(x,y,z)       fval(I3(x,y,z))
#define fval4i(x,y,z,w)     fval(I4(x,y,z,w))
#define fbool(value)        fval(value, (Fui_Options{.flags=Fui_Flag_Clamp_01}))
#define fval01              fbool
//-

force_inline Fui_Options
fopts(u32 flags, v1 delta_scale)
{
 Fui_Options result = {};
 result.flags=flags;
 result.delta_scale = delta_scale;
 return result;
}

force_inline Fui_Options
fopts_add_flags(Fui_Options options, u32 flags)
{
 options.flags |= flags;
 return options;
}

force_inline Fui_Options
fopts_add_delta_scale(Fui_Options options, v1 delta_scale)
{
 if(options.delta_scale == 0.f)
 {
  options.delta_scale = delta_scale;
 }
 return options;
}

force_inline Fui_Options fopts_maybe(u32 flags) { 
 Fui_Options result = {};
 result.flags=flags;
 return result;
}
force_inline Fui_Options fopts_maybe(Fui_Options options) { return options; }
force_inline Fui_Options fopts_maybe() { return {}; }

//~EOF
