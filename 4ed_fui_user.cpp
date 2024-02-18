// ;fui ///////////////////////////////////
// TODO: It's not really about sliders anymore

typedef i32 Fui_Item_Index;
global      Fui_Item_Index global_fui_active_item_index;

inline b32 fui_is_active(Fui_Item_Index index)
{
    return (index == global_fui_active_item_index);
}
//
global b8 global_fui_key_states[KeyCode_COUNT];

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
#define FUI_UPDATE_RETURN v4
#define FUI_UPDATE_PARAMS Fui_Slider *slider, f32 dt
//
typedef FUI_UPDATE_RETURN Fui_Update_Value(FUI_UPDATE_PARAMS);

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

global Fui_Slider *global_fui_store;

// TODO(kv): @Incomplete, need to search by filename as well.
internal Fui_Slider *
fui_get_slider_by_name(String8 name)
{
    Fui_Slider *&store = global_fui_store;
   
    for_i32 ( index, 0, arrlen(store) )
    {
        if ( string_equal(name, store[index].name) )
        return &store[index];
    }
    
    return 0;
}

internal v4
fui_direction_from_key_states()
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

internal FUI_UPDATE_RETURN
fui_update_linear(FUI_UPDATE_PARAMS)
{
    v4 direction = fui_direction_from_key_states();
    f32 speed = 1.0f;
    // NOTE: Update slider value
    v4 delta = dt * speed * direction;
    return slider->value + delta;
}

internal FUI_UPDATE_RETURN 
fui_update_null(FUI_UPDATE_PARAMS)
{ return slider->value; }

internal Fui_Item_Index
fui_get_item_index(String8 id, Fui_Type type, Fui_Value init_value, Fui_Options options)
{
    Fui_Slider *&store = global_fui_store;
    if ( arrlen(store) == 0 )
    {
        arrsetcap(store, 32);
        arrput( store, Fui_Slider{} );
    }
    
    // NOTE(kv): Find matching item index
    i32 item_index = 0;
    for (;
         item_index < arrlen(store); 
         item_index++)
    {
        String8 item_id = store[item_index].id;
        if ( id.str == item_id.str )
            break;
    }
   
    if ( item_index == arrlen(store) )
    {
        Fui_Slider slider = { .id = id, .type = type, .options = options};
        slider.value = init_value;
        
        // NOTE: Update function
        if (!slider.update)
            slider.update = fui_update_linear;
        
        // NOTE: Range
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
    return item_index;
}

internal Fui_Value
fui_user_main(String8 id, Fui_Type type, Fui_Value init_value, 
              Fui_Options options, Fui_Item_Index *index_out)
{
    Fui_Item_Index item_index = fui_get_item_index(id, type, init_value, options);
    if (index_out) *index_out = item_index;
    
    Fui_Slider *slider = &global_fui_store[item_index];
    
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
internal T fui_user(String8 id, T init_value_T, Fui_Options options={}, Fui_Item_Index *index=0) \
{ \
    Fui_Type  type = Fui_Type_##T; \
    Fui_Value init_value = { .T = init_value_T }; \
    Fui_Value value = fui_user_main(id, type, init_value, options, index); \
    return value.T; \
}
//
X_Fui_Types(X)
//
#undef X

#define fslider(NAME, ...) \
    auto NAME = fui_user(str8lit(__FILE__ "|" #NAME), ##__VA_ARGS__)

internal void 
fui_post_value(Fui_Item_Index index, v4 value)
{
    global_fui_store[index].value.v4 = value;
}
