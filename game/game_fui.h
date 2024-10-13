// TODO @Cleanup The editor doesn't need all these so don't put them here?
#pragma once

typedef u32 Sliders;
enum Slider
{// NOTE: These are total hacks, man!
 Slider_NULL           = 0,
 Slider_Camera_Aligned = bit_1,
 Slider_Vertex         = bit_2,
 Slider_Vector         = bit_3,
 Slider_NOZ            = bit_4,
 Slider_Clamp_X        = bit_5,
 Slider_Clamp_Y        = bit_6,
 Slider_Clamp_Z        = bit_7,
 Slider_Clamp_01       = bit_8,
};

struct Fui_Options
{
#define FUI_OPTIONS(X)  \
Sliders flags;        \
v1 delta_scale;         \
 
 FUI_OPTIONS(X);
};

struct Fui_Slider{
 Basic_Type  type;   // 1
 union{
  Fui_Options options;  // 5
  struct { FUI_OPTIONS(X); };
 };
};

xfunction void *
fast_fval_inner(Basic_Type type, void *init_value,
                i32 linum, Fui_Options options);
xfunction void *
slow_fval_inner(Basic_Type type, void *init_value,
                char *file_c, i32 linum, Fui_Options options);

template<class T>
function T
fast_fval(T init_value, Fui_Options options={}, i32 line=__builtin_LINE())
{
 Basic_Type type = basic_type_from_pointer((T *)0);
 void *value = fast_fval_inner(type, cast(void*)(&init_value), line, options);
 return *(cast(T *)value);
}

template<class T>
function T
slow_fval(T init_value, Fui_Options options={},
          const char *file=__builtin_FILE(), i32 line=__builtin_LINE())
{
 Basic_Type type = basic_type_from_pointer((T *)0);
 void *value = slow_fval_inner(type, cast(void*)(&init_value), file, line, options);
 return *(cast(T *)value);
}

// NOTE: The "fval" macro switches between "fast" and "slow" version
#define fval slow_fval

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
#define fbool(value)        fval(value, (Fui_Options{.flags=Slider_Clamp_01}))
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
fopts_add_flags(Fui_Options options, u32 flags){
 options.flags |= flags;
 return options;
}

force_inline Fui_Options
fopts_add_delta_scale(Fui_Options options, v1 delta_scale){
 if(options.delta_scale == 0.f) {
  options.delta_scale = delta_scale;
 }
 return options;
}

force_inline Fui_Options
fopts(u32 flags){
 Fui_Options result = {};
 result.flags=flags;
 return result;
}
force_inline Fui_Options fopts(Fui_Options options) { return options; }
force_inline Fui_Options fopts() { return {}; }

//~EOF
