#pragma once

#include "4ed_fui_user.h"

global Fui_Slider *global_fui_store;

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

fui_user_main_return
fui_user_main(fui_user_main_params)
{
    Fui_Item_Index item_index = fui_get_item_index(id, type, init_value, options);
    
    Fui_Slider *slider = &global_fui_store[item_index];
    Fui_Value result = slider->value;
    return result;
}
