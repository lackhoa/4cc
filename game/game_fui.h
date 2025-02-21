//-
typedef u32 Slider_Flags;
enum
{
 Slider_NULL           = 0,
 //Slider_Camera_Aligned = bit_1,
 
 // NOTE(kv) Are these really flags?
 //Slider_Vertex         = bit_2,
 //Slider_Vector         = bit_3,
 //Slider_NOZ            = bit_4,  // NOTE(kv) NOZ are implied vectors... not sure what to feel about that.
 
 Slider_Clamp_X        = bit_5,
 Slider_Clamp_Y        = bit_6,
 Slider_Clamp_Z        = bit_7,
 Slider_Clamp_01       = bit_8,
};

struct Fui_Options
{
#define FUI_OPTIONS(X) \
Slider_Flags flags;    \
v1 delta_scale;        \
 
 FUI_OPTIONS(X);
};

typedef i32 Type_Index;

// NOTE(kv) Really should be called "controller" instead,
// since multiple sliders is usually what we want... oh well!
struct Slider
{
 // TODO(kv) @Memory Ideally there would only be a single type index.
 Type_Info *type;
 
 Location location;
 i32 index;  //NOTE(kv) Redundant, but I don't care rn
 union{
  Fui_Options options;
  struct { FUI_OPTIONS(X); };
 };
 void *value;
};
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
fopts(u32 flags)
{
 Fui_Options result = {};
 result.flags=flags;
 return result;
}
myinline Fui_Options fopts(Fui_Options options) { return options; }
myinline Fui_Options fopts() { return {}; }

global Fui_Options f20th = Fui_Options{0, 0.05f};
global Fui_Options f10th = Fui_Options{0, 0.1f};
global Fui_Options f10s  = Fui_Options{0, 10.f};

//-
// NOTE(kv) @Slow We still have double-indirection,
// first we gotta find where the slider is. Then read off the value pointer.
#define ReadSlider(TYPE, INDEX) \
(*(TYPE *)global_sliders[INDEX].value)

// NOTE(kv) Pretty clever use of the assignment operator
// NOTE(kv) You can put a static slider within a staic slider, pretty great! ;>
#define ReadSliderRuntime(TYPE, INDEX, VALUE) \
(*(TYPE *)global_sliders[INDEX].value = VALUE)
//-
//~EOF
