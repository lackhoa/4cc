//-
typedef u32 Slider_Flags;
enum Slider_Flag
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
Slider_Flags flags;     \
v1 delta_scale;         \
 
 FUI_OPTIONS(X);
};

struct Slider
{
 Basic_Type type;
 u32 pos;
 u32 size;
 u32 index;  //NOTE(kv) because I don't care
 union{
  Fui_Options options;  // 8
  struct { FUI_OPTIONS(X); };
 };
};

//-
#if 0
#define fval
#define fbool
#endif
//-

myinline Fui_Options
fopts(u32 flags, v1 delta_scale)
{
 Fui_Options result = {};
 result.flags=flags;
 result.delta_scale = delta_scale;
 return result;
}

myinline Fui_Options
fopts_add_flags(Fui_Options options, u32 flags){
 options.flags |= flags;
 return options;
}

myinline Fui_Options
fopts_add_delta_scale(Fui_Options options, v1 delta_scale){
 if(options.delta_scale == 0.f){
  options.delta_scale = delta_scale;
 }
 return options;
}

myinline Fui_Options
fopts(u32 flags){
 Fui_Options result = {};
 result.flags=flags;
 return result;
}
myinline Fui_Options fopts(Fui_Options options) { return options; }
myinline Fui_Options fopts() { return {}; }

//-The Slow Path
struct Slow_Line_Map_Entry{
 String file;
 i32 linum;
 Slider *slider;
};

struct Slow_Line_Map{
 i32 cap;
 i32 count;
 struct Slow_Line_Map_Entry *map;
};
global Slow_Line_Map slow_line_map;

global Fui_Options f20th = Fui_Options{0, 0.05f};
global Fui_Options f10th = Fui_Options{0, 0.1f};
global Fui_Options f10s  = Fui_Options{0, 10.f};

//~ Statically generated sliders
#include "generated/sliders0.gen.h"

#if 0
global Slider global_sliders[];
#endif
global Slider global_sliders[FUI_SLIDER_COUNT] = {
#include "generated/slider_info.gen.h"
};
global void  *global_slider_values[FUI_SLIDER_COUNT];

function void
create_sliders(Arena *arena)
{
#include "generated/slider_values.gen.h"
}

//~
#define ReadSlider(type, index) \
*(type *)(global_slider_values[index])

//~EOF
