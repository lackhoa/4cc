/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 09.10.2019
 *
 * Primary list for all key codes.
 *
 */

// TOP

#include "4coder_base_types.h"

#include "4coder_base_types.cpp"
#include "4coder_stringf.cpp"
#include "4coder_malloc_allocator.cpp"

#include <stdio.h>

////////////////////////////////

struct Event_Code{
    Event_Code *next;
    String name;
};

struct Event_Code_List{
    Event_Code *first;
    Event_Code *last;
    i1 count;
    
    String code_prefix;
    String name_table;
};

////////////////////////////////

function void
generate_codes(Arena *scratch, Event_Code_List *list, FILE *out){
    String code_prefix = list->code_prefix;
    String name_table = list->name_table;
    
    fprintf(out, "enum{\n");
    i1 counter = 1;
    for (Event_Code *code = list->first;
         code != 0;
         code = code->next){
        fprintf(out, "%.*s_%.*s = %d,\n",
                string_expand(code_prefix), string_expand(code->name), counter);
        counter += 1;
    }
    fprintf(out, "%.*s_COUNT = %d,\n", string_expand(code_prefix), counter);
    fprintf(out, "};\n");
    
    fprintf(out, "global char* %.*s[%.*s_COUNT] = {\n",
            string_expand(name_table), string_expand(code_prefix));
    fprintf(out, "\"None\",\n");
    for (Event_Code *code = list->first;
         code != 0;
         code = code->next){
        fprintf(out, "\"%.*s\",\n", string_expand(code->name));
        counter += 1;
    }
    fprintf(out, "};\n");
}

////////////////////////////////

function Event_Code*
add_code(Arena *arena, Event_Code_List *list, String name){
    Event_Code *code = push_array(arena, Event_Code, 1);
    sll_queue_push(list->first, list->last, code);
    list->count;
    code->name = push_string_copy(arena, name);
    return(code);
}

function Event_Code_List
make_key_list(Arena *arena){
    Event_Code_List list = {};
    list.code_prefix = strlit("Key_Code");
    list.name_table = strlit("key_code_name");
    for (u32 i = 'A'; i <= 'Z'; i += 1){
        u8 c = (u8)i;
        add_code(arena, &list, SCu8(&c, 1));
    }
    for (u32 i = '0'; i <= '9'; i += 1){
        u8 c = (u8)i;
        add_code(arena, &list, SCu8(&c, 1));
    }
    add_code(arena, &list, strlit("Space"));
    add_code(arena, &list, strlit("Tick"));
    add_code(arena, &list, strlit("Minus"));
    add_code(arena, &list, strlit("Equal"));
    add_code(arena, &list, strlit("LeftBracket"));
    add_code(arena, &list, strlit("RightBracket"));
    add_code(arena, &list, strlit("Semicolon"));
    add_code(arena, &list, strlit("Quote"));
    add_code(arena, &list, strlit("Comma"));
    add_code(arena, &list, strlit("Period"));
    add_code(arena, &list, strlit("ForwardSlash"));
    add_code(arena, &list, strlit("BackwardSlash"));
    add_code(arena, &list, strlit("Tab"));
    add_code(arena, &list, strlit("Escape"));
    add_code(arena, &list, strlit("Pause"));
    add_code(arena, &list, strlit("Up"));
    add_code(arena, &list, strlit("Down"));
    add_code(arena, &list, strlit("Left"));
    add_code(arena, &list, strlit("Right"));
    add_code(arena, &list, strlit("Backspace"));
    add_code(arena, &list, strlit("Return"));
    add_code(arena, &list, strlit("Delete"));
    add_code(arena, &list, strlit("Insert"));
    add_code(arena, &list, strlit("Home"));
    add_code(arena, &list, strlit("End"));
    add_code(arena, &list, strlit("PageUp"));
    add_code(arena, &list, strlit("PageDown"));
    add_code(arena, &list, strlit("CapsLock"));
    add_code(arena, &list, strlit("NumLock"));
    add_code(arena, &list, strlit("ScrollLock"));
    add_code(arena, &list, strlit("Menu"));
    add_code(arena, &list, strlit("Shift"));
    add_code(arena, &list, strlit("Control"));
    add_code(arena, &list, strlit("Alt"));
    add_code(arena, &list, strlit("Command"));
    for (u32 i = 1; i <= 24; i += 1){
        add_code(arena, &list, push_u8_stringf(arena, "F%d", i));
    }
    for (u32 i = '0'; i <= '9'; i += 1){
        add_code(arena, &list, push_u8_stringf(arena, "NumPad%c", i));
    }
    add_code(arena, &list, strlit("NumPadStar"));
    add_code(arena, &list, strlit("NumPadPlus"));
    add_code(arena, &list, strlit("NumPadMinus"));
    add_code(arena, &list, strlit("NumPadDot"));
    add_code(arena, &list, strlit("NumPadSlash"));
    for (i1 i = 0; i < 30; i += 1){
        add_code(arena, &list, push_u8_stringf(arena, "Ex%d", i));
    }
    return(list);
}

function Event_Code_List
make_mouse_list(Arena *arena){
    Event_Code_List list = {};
    list.code_prefix = strlit("MouseCode");
    list.name_table = strlit("mouse_code_name");
    add_code(arena, &list, strlit("Left"));
    add_code(arena, &list, strlit("Middle"));
    add_code(arena, &list, strlit("Right"));
    return(list);
}

function Event_Code_List
make_core_list(Arena *arena){
    Event_Code_List list = {};
    list.code_prefix = strlit("CoreCode");
    list.name_table = strlit("core_code_name");
    add_code(arena, &list, strlit("Startup"));
    add_code(arena, &list, strlit("Animate"));
    add_code(arena, &list, strlit("ClickActivateView"));
    add_code(arena, &list, strlit("ClickDeactivateView"));
    add_code(arena, &list, strlit("TryExit"));
    add_code(arena, &list, strlit("FileExternallyModified"));
    add_code(arena, &list, strlit("NewClipboardContents"));
    return(list);
}

////////////////////////////////

int
main(void){
    Arena arena = make_arena_malloc();
    
    Event_Code_List key_list = make_key_list(&arena);
    Event_Code_List mouse_list = make_mouse_list(&arena);
    Event_Code_List core_list = make_core_list(&arena);
    
    String path_to_self = strlit(__FILE__);
    path_to_self = string_remove_last_folder(path_to_self);
    String file_name =
        push_u8_stringf(&arena, "%.*scustom/generated/4coder_event_codes.h",
                        string_expand(path_to_self));
    
    FILE *out = fopen((char*)file_name.str, "wb");
    if (out == 0){
        printf("could not open output file '%s'\n", file_name.str);
        exit(1);
    }
    
    generate_codes(&arena, &key_list, out);
    generate_codes(&arena, &mouse_list, out);
    generate_codes(&arena, &core_list, out);
    
    fclose(out);
    return(0);
}

// BOTTOM
