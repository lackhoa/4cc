#pragma once

#include "4coder_events.h"

typedef i32 Fui_Item_Index;

#define X_Fui_Types(X) \
    X(v1) \
    X(v2) \
    X(v3) \
    X(v4) \

enum Fui_Type
{
#define X(T)   Fui_Type_##T,
    //
    X_Fui_Types(X)
    //
#undef X
};

union Fui_Value
{
#define X(T)   T T;
    // 
    X_Fui_Types(X);
    // 
#undef X
    
    operator union v4() {return this->v4;}
};

struct Fui_Slider;

// todo @Rename should be "fui_delta" instead
#define FUI_UPDATE_RETURN v4
#define FUI_UPDATE_PARAMS Fui_Slider *slider, f32 dt
//
typedef FUI_UPDATE_RETURN Fui_Update_Value(FUI_UPDATE_PARAMS);

#define X_FUI_OPTIONS(X)

struct Fui_Options
{
#define X(F) F;
    X_FUI_OPTIONS(X);
#undef X
};

struct Fui_Slider
{
    String id;
    String name;
    Fui_Type  type;
    Fui_Value value;
    union
    {
        Fui_Options options;
        struct
        {
#define X(F) F;
    X_FUI_OPTIONS(X);
#undef X
        };
    };
    
};

// TODO: @Cleanup v3 only?
internal v4
fui_direction_from_key_states(Key_Mod active_mods, b8 *key_states, Key_Mod wanted_mods)
{
    v4 direction = {};
    if (active_mods == wanted_mods)
    {
#define Down(N)  (key_states[KeyCode_##N] != 0)
        //
        if (Down(L)) direction.x = +1;
        if (Down(H)) direction.x = -1;
        if (Down(K)) direction.y = +1;
        if (Down(J)) direction.y = -1;
        if (Down(O)) direction.z = +1;
        if (Down(I)) direction.z = -1;
        //
#undef Down
    }
    return direction;
}

#define fui_user_main_return Fui_Value
#define fui_user_main_params String id, Fui_Type type, Fui_Value init_value, Fui_Options options

#define fui_user(NAME, ...) \
    fui_user_type(str8lit(__FILE__ "|" #NAME), ##__VA_ARGS__)

#define fslider(NAME, ...) \
    auto NAME = fui_user(NAME, ##__VA_ARGS__)
