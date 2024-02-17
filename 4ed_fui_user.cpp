// ;fui ///////////////////////////////////
// TODO: It's not really about sliders anymore

global i32 fui_active_slider_index;
global b8  global_fui_key_states[KeyCode_COUNT];

#define X_Fui_Types(X)  \
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
#define FUI_UPDATE_VALUE_RETURN v4
#define FUI_UPDATE_VALUE_PARAMS Fui_Slider *slider, f32 dt
//
typedef FUI_UPDATE_VALUE_RETURN Fui_Update_Value(FUI_UPDATE_VALUE_PARAMS);

#define X_FUI_OPTIONS(X) \
    Range_f32 range; \
    Fui_Update_Value *update; \
    void *userdata;

struct Fui_Options
{
#define X(F) F;
    X_FUI_OPTIONS(X);
#undef X
};

struct Fui_Slider
{
    String8 id;
    String8 name;
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

global Fui_Slider *fui_slider_store;

// TODO(kv): @Incomplete, need to search by filename as well.
internal Fui_Slider *
fui_get_slider_by_name(String8 name)
{
    Fui_Slider *&store = fui_slider_store;
   
    for_i32 ( index, 0, arrlen(fui_slider_store) )
    {
        if ( string_equal(name, store[index].name) )
        return &store[index];
    }
    
    return 0;
}

internal v4
fui__direction_from_key_states()
{
    v4 direction = {};
    //
#define Down(N)  (global_fui_key_states[KeyCode_##N] != 0)
    //
    if (Down(L)) direction.x = +1;
    if (Down(H)) direction.x = -1;
    if (Down(K)) direction.y = +1;
    if (Down(J)) direction.y = -1;
    if (Down(O)) direction.z = +1;
    if (Down(I)) direction.z = -1;
    //
#undef Down
    return direction;
}

internal FUI_UPDATE_VALUE_RETURN
fui_update_value_linear(FUI_UPDATE_VALUE_PARAMS)
{
    v4 direction = fui__direction_from_key_states();
    f32 speed = 1.0f;
    // NOTE: Update slider value
    v4 delta = dt * speed * direction;
    return slider->value + delta;
}

#if 0
internal FUI_EVENT_HANDLER_RETURN
fui_event_handler_linear(FUI_EVENT_HANDLER_PARAMS)
{
    // NOTE: This direction value is persistent, so we can't quite get rid of it.
    v4 &direction = fui_slider_direction;
    
#define Match(CODE) keycode == KeyCode_##CODE
    //
    if (0) {}
    else if (Match(L)) direction.x = keydown ? +1 : 0;
    else if (Match(H)) direction.x = keydown ? -1 : 0;
    else if (Match(K)) direction.y = keydown ? +1 : 0;
    else if (Match(J)) direction.y = keydown ? -1 : 0;
    else if (Match(O)) direction.z = keydown ? +1 : 0;
    else if (Match(I)) direction.z = keydown ? -1 : 0;
    //
#undef Match
}
#endif

internal Fui_Value
fui_slider_user_main(String8 id, Fui_Type type, Fui_Value init_value, Fui_Options options)
{
    Fui_Slider *&store = fui_slider_store;
    if ( arrlen(store) == 0 )
    {
        arrput( store, Fui_Slider{} );
    }
        
    // NOTE(kv): Find match @Slow
    i32 slider_index = 0;
    for (;
         slider_index < arrlen(store); 
         slider_index++)
    {
        String8 item_id = store[slider_index].id;
        if ( id.str == item_id.str )
        {
            break;
        }
    }
   
    if ( slider_index == arrlen(store) )
    {
        Fui_Slider slider = { .id = id, .type = type, .options = options};
        slider.value = init_value;
        
        // NOTE: update function
        if (!slider.update)
            slider.update = fui_update_value_linear;
        
        // NOTE: range
        if (slider.range.min == 0.0f && slider.range.max == 0.0f)
        {
            slider.range = If32(0.0f,  1.0f);
        }
        
        // NOTE(kv): find file|name separator
        u64 separator_index = id.size-1;
        while ( id.str[separator_index] != '|' ) { separator_index--; }
        
        u64 len = separator_index+1;
        slider.name = String8{
            .str = id.str + len,
            .len = id.len - len,
        };
        
        arrput(store, slider);
    }
    
    Fui_Slider *slider = &store[slider_index];
    
    // NOTE: Convert internal to external result (@Slow We could do this in the event handler)
    Range_f32 range = slider->range;
    v4 internal_value = slider->value.v4;
    v4 result;
    result.x = lerp(range.min, internal_value.x, range.max);
    result.y = lerp(range.min, internal_value.y, range.max);
    result.z = lerp(range.min, internal_value.z, range.max);
    result.w = lerp(range.min, internal_value.w, range.max);
    
    return Fui_Value{.v4 = result};
}

// NOTE: We define overloads to avoid having to specify the type as a separate argument 
// TODO: (This might be the wrong decision?),
// as well as receive the appropriate values.
#define X(T) \
\
internal T fui_slider_user(String8 id, T init_value_T, Fui_Options options={}) \
{ \
    Fui_Type  type = Fui_Type_##T; \
    Fui_Value init_value = { .T = init_value_T }; \
    Fui_Value value = fui_slider_user_main(id, type, init_value, options); \
    return value.T; \
}
//
X_Fui_Types(X)
//
#undef X

#define fslider(NAME, ...) \
    auto NAME = fui_slider_user(str8lit(__FILE__ "|" #NAME), ##__VA_ARGS__)
