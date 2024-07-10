/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 01.10.2019
 *
 * Search list helper.
 *
 */

// TOP

////////////////////////////////
// NOTE(allen): Search List Builders

function void
def_search_list_add_path(Arena *arena, List_String *list, String path){
    String path_copy = push_string_copyz(arena, path);
    string_list_push(arena, list, path_copy);
}

function void
def_search_list_add_system_path(Arena *arena, List_String *list, System_Path_Code path){
    String path_string = system_get_path(arena, path);
    string_list_push(arena, list, path_string);
}

////////////////////////////////
// NOTE(allen): Search List Functions

function String
def_search_get_full_path(Arena *arena, List_String *list, String relative)
{
    String result = {};
    
    Temp_Memory temp = begin_temp(arena);
    
    u8 slash = '/';
    
    for (Node_String *node = list->first;
         node != 0;
         node = node->next){
        String full_name = {};
        full_name.size = node->string.size + 1 + relative.size;
        full_name.str = push_array(arena, u8, full_name.size + 1);
        block_copy(full_name.str, node->string.str, node->string.size);
        full_name.str[node->string.size] = slash;
        block_copy(full_name.str + node->string.size + 1, relative.str, relative.size);
        full_name.str[full_name.size] = 0;
        
        File_Attributes attribs = system_quick_file_attributes(arena, full_name);
        if (attribs.last_write_time > 0){
            result = full_name;
            break;
        }
        
        end_temp(temp);
    }
    
    return(result);
}

function FILE*
def_search_fopen(Arena *arena, List_String *list, char *file_name, char *opt){
    Temp_Memory_Block block(arena);
    String full_path = def_search_get_full_path(arena, list, SCu8(file_name));
    FILE *file = 0;
    if (full_path.size > 0){
        file = fopen((char*)full_path.str, opt);
    }
    return(file);
}

// BOTTOM

