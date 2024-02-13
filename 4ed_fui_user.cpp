// ;fui ///////////////////////////////////

#define X_Fui_Slider_Types(X)  \
    X(v1) \
    X(v2) \
    X(v3) \
    X(v4) \

enum Fui_Slider_Type
{
#define X(T)   Fui_Slider_Type_##T,
    //
    X_Fui_Slider_Types(X)
    //
#undef X
};

union Fui_Slider_Value
{
#define X(T)   T T;
    // 
    X_Fui_Slider_Types(X);
    // 
#undef X
};

struct Fui_Slider
{
    String8 id;
    String8 name;
    Fui_Slider_Type  type;
    Fui_Slider_Value value;
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

internal Fui_Slider_Value
fui_slider_init_or_get_main(String8 id, Fui_Slider_Type type, Fui_Slider_Value init_value)
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
    
    return store[slider_index].value;
}

// NOTE: Overloads are used to pass types to "fui_slider_init_or_get", based on argument type,
// as well as receive the appropriate values.
/*
inline v3 fui_slider_init_or_get(String8 id, v3 init_value)
{
    Fui_Slider_Type type = Fui_Slider_Type_v3;
    Fui_Slider_Value value = fui_slider_init_or_get_main(id, type, &init_value);
    return value.v3_value;
}

inline v2 fui_slider_init_or_get(String8 id, v2 init_value)
{
    Fui_Slider_Type type = Fui_Slider_Type_v2;
    Fui_Slider_Value value = fui_slider_init_or_get_main(id, type, &init_value);
    return value.v2_value;
}

inline v1 fui_slider_init_or_get(String8 id, v1 init_value)
{
    Fui_Slider_Type type = Fui_Slider_Type_v1;
    Fui_Slider_Value value = fui_slider_init_or_get_main(id, type, &init_value);
    return value.v1_value;
}

inline bool fui_slider_init_or_get(String8 id, bool init_value)
{
    Fui_Slider_Type type = Fui_Slider_Type_bool;
    Fui_Slider_Value value = fui_slider_init_or_get_main(id, type, &init_value);
    return (bool)value;
}
*/

#define X(T) \
\
internal T fui_slider_init_or_get(String8 id, T init_value_T) \
{ \
    Fui_Slider_Type  type = Fui_Slider_Type_##T; \
    Fui_Slider_Value init_value = { .T = init_value_T }; \
    Fui_Slider_Value value = fui_slider_init_or_get_main(id, type, init_value); \
    return value.T; \
}
//
X_Fui_Slider_Types(X)
//
#undef X

#define fslider(NAME, ...) \
    auto NAME = fui_slider_init_or_get(str8lit(__FILE__ "|" #NAME), ##__VA_ARGS__)
