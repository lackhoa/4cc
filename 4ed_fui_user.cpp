// ;fui ///////////////////////////////////
// TODO: It's not really about sliders anymore

typedef i32 Fui_Item_Index;
global      Fui_Item_Index global_fui_active_item_index;

inline b32 fui_is_active(Fui_Item_Index index)
{
    return (index == global_fui_active_item_index);
}
// TODO @Cleanup @Moveme
global b8 global_key_states            [KeyCode_COUNT];
global u8 global_game_key_state_changes[KeyCode_COUNT];

inline b32
key_is_down(Key_Code keycode)
{
    return global_key_states[keycode];
}

inline u32
key_state_changes(Key_Code keycode)
{
    return global_game_key_state_changes[keycode];
}

// TODO(kv): This key tracking is inaccurate at least in the case when you alt+tab out of the app
internal void
update_global_key_states(Input_Event *event)
{
    b32 keydown = (event->kind == InputEventKind_KeyStroke);
    b32 keyup   = (event->kind == InputEventKind_KeyRelease);
    if (keydown || keyup)
    {
        Key_Code keycode = event->key.code;
        // NOTE: We have system_get_keyboard_modifiers to track modifier keys already
        if ( !is_modifier_key(keycode) )
        {
            global_key_states            [keycode] = keydown;
            global_game_key_state_changes[keycode]++;
        }
    }
}

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

#define X_FUI_OPTIONS(X) \
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

global Fui_Slider *global_fui_store;

// TODO(kv): @Incomplete, need to search by filename as well.
internal Fui_Slider *
fui_get_slider_by_name(String name)
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
fui_direction_from_key_states(Key_Mod active_mods, Key_Mod wanted_mods)
{
    v4 direction = {};
    if (active_mods == wanted_mods)
    {
#define Down(N)  (global_key_states[KeyCode_##N] != 0)
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

internal FUI_UPDATE_RETURN
fui_update_linear(FUI_UPDATE_PARAMS)
{
    v4 direction = fui_direction_from_key_states(0,0);
    f32 speed = 1.0f;
    // NOTE: Update slider value
    v4 delta = dt * speed * direction;
    return slider->value + delta;
}

internal FUI_UPDATE_RETURN 
fui_update_null(FUI_UPDATE_PARAMS)
{ 
    return slider->value; 
}

internal Fui_Item_Index
fui_get_item_index(String id, Fui_Type type, Fui_Value init_value, Fui_Options options)
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
        String item_id = store[item_index].id;
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
        
        // NOTE(kv): find file|name separator
        u64 separator_index = id.size-1;
        while ( id.str[separator_index] != '|' ) { separator_index--; }
        
        u64 len = separator_index+1;
        slider.name = String{
            .str = id.str + len,
            .len = id.len - len,
        };
        
        arrput(store, slider);
    }
    return item_index;
}

internal Fui_Value
fui_user_main(String id, Fui_Type type, Fui_Value init_value, Fui_Options options)
{
    Fui_Item_Index item_index = fui_get_item_index(id, type, init_value, options);
    
    Fui_Slider *slider = &global_fui_store[item_index];
    Fui_Value result = slider->value;
    return result;
}

// NOTE: We define overloads to avoid having to specify the type as a separate argument 
// as well as receive the appropriate values as output.
#define X(T) \
\
internal T fui_user_type(String id, T init_value_T, Fui_Options options={}) \
{ \
    Fui_Type  type = Fui_Type_##T; \
    Fui_Value init_value = { .T = init_value_T }; \
    Fui_Value value = fui_user_main(id, type, init_value, options); \
    return value.T; \
}
//
X_Fui_Types(X)
//
#undef X

#define fui_user(NAME, ...) \
    fui_user_type(str8lit(__FILE__ "|" #NAME), ##__VA_ARGS__)

#define fslider(NAME, ...) \
    auto NAME = fui_user(NAME, ##__VA_ARGS__)

internal void
fui_post_value(Fui_Item_Index index, v4 value)
{
    global_fui_store[index].value.v4 = value;
}
