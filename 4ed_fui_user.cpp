// ;fui ///////////////////////////////////

enum Fui_Slider_Type
{
    Fui_Slider_Type_v1,
    Fui_Slider_Type_v2,
    Fui_Slider_Type_v3,
    Fui_Slider_Type_boolean,
};

struct Fui_Slider
{
    String8 id;
    String8 name;
    Fui_Slider_Type type;
    v4  value;
};

global Fui_Slider *fui_slider_store;

// TODO(kv): @ Incomplete, need to search by filename as well.
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
fui_slider_init_or_get_main(String8 id, Fui_Slider_Type type, void *init_value)
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
        new_slider.value = *(v4 *)init_value;
        
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
    
    return store[slider_index].value;
}

// NOTE: overloads are used to distinguish types, which is cutnpasty so let's use varargs when we have time
inline v3 fui_slider_init_or_get(String8 id, f32 x, f32 y, f32 z)
{
    Fui_Slider_Type type = Fui_Slider_Type_v3;
    v3 init_value = v3{x,y,z};
    v4 value = fui_slider_init_or_get_main(id, type, &init_value);
    return value.xyz;
}

inline v2 fui_slider_init_or_get(String8 id, f32 x, f32 y)
{
    Fui_Slider_Type type = Fui_Slider_Type_v2;
    v2 init_value = v2{x,y};
    v4 value = fui_slider_init_or_get_main(id, type, &init_value);
    return value.xy;
}

inline f32 fui_slider_init_or_get(String8 id, f32 x)
{
    Fui_Slider_Type type = Fui_Slider_Type_v1;
    f32 init_value = x;
    v4 value = fui_slider_init_or_get_main(id, type, &init_value);
    return value.x;
}

#define fslider(NAME, ...) \
    auto NAME = fui_slider_init_or_get(str8lit(__FILE__ "|" #NAME), ##__VA_ARGS__)
