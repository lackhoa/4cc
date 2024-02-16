// ;fui ///////////////////////////////////
// TODO: It's not really about sliders anymore

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

#define FUI_UPDATE_VALUE_RETURN void
#define FUI_UPDATE_VALUE_PARAMS Fui_Slider *slider, f32 dt, v4 direction
//
typedef FUI_UPDATE_VALUE_RETURN Fui_Update_Value_Sig(FUI_UPDATE_VALUE_PARAMS);

struct Fui_Slider
{
    String8 id;
    String8 name;
    Fui_Type  type;
    Fui_Value value;
    Fui_Update_Value_Sig *update_value_function;
    void *userdata;
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

internal FUI_UPDATE_VALUE_RETURN
fui_update_value_linear(FUI_UPDATE_VALUE_PARAMS)
{
    f32 speed = 1.0f;
    slider->value.v4 += dt * speed * direction;
}

struct Fui_Options
{
    Range_f32 range;
    Fui_Update_Value_Sig *update_value_function;
    void *userdata;
};

internal Fui_Value
fui_slider_user_main(String8 id, Fui_Type type, Fui_Value init_value, Fui_Options options)
{
    Fui_Slider *&store = fui_slider_store;

    if ( arrlen(store) == 0 )
    {
        arrput( store, Fui_Slider{} );
    }
        
    // NOTE(kv): find match
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
    
    if (slider_index == arrlen(store))
    {
        Fui_Slider new_slider = { .id = id, .type = type, };
        new_slider.value = init_value;
        
        // NOTE(kv): find file|name separator
        u64 separator_index = id.size-1;
        while ( id.str[separator_index] != '|' ) { separator_index--; }
        
        u64 len = separator_index+1;
        new_slider.name = String8{
            .str = id.str + len,
            .len = id.len - len,
        };
        
        arrput(store, new_slider);
    }
   
    Fui_Slider *slider = &store[slider_index];
    
    // NOTE: update function
    slider->update_value_function = options.update_value_function;
    if (!slider->update_value_function)
        slider->update_value_function = fui_update_value_linear;
    
    // NOTE: External result
    Range_f32 range = options.range;
    if (range.min == 0.0f && 
        range.max == 0.0f)
    {
        range.min = 0.0f;
        range.max = 1.0f;
    }
   
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
