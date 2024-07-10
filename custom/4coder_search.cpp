/*
4coder_search.cpp - Commands that search accross buffers including word complete,
and list all locations.
*/

// TOP

global const String8 search_buffer_name = str8lit("*search*");

function String8_Array
kv_string_split_wildcards(Arena *arena, String8 string)
{
  String8_Array array = {};
  List_String list = string_split(arena, string, (u8*)" ", 2);
  array.count   = list.node_count;
  array.strings = push_array(arena, String, array.count);
  i64 index = 0;
  for (Node_String *node = list.first;
       node;
       node = node->next)
  {
    kv_assert(index < array.count);
    array.strings[index++] = node->string;
  }
  return(array);
}

internal b32 
string_has_uppercase(String string)
{
    for_u64 (index,0,string.size)
    {
        if ( is_uppercase(string.str[index]) )
   return true;
 }
 return false;
}

// NOTE(kv): We don't handle multiline string! @FuzzyMultiline
function i64
kv_fuzzy_search_forward(App *app, Buffer_ID buffer, i64 pos, String needle)
{
 i64 buffer_size = buffer_get_size(app, buffer);
 i64 result = buffer_size;
 
 Scratch_Block temp(app);
 String8_Array splits = kv_string_split_wildcards(temp, needle);
 if ( !splits.count ) { return result; }
 
 while( pos < buffer_size )
 {
  i64 original_pos = pos;
  String_Match first_match;
  {
   String first_word = splits.strings[0];
   b32 case_sensitive = string_has_uppercase(first_word);
   first_match = buffer_seek_string(app, buffer, first_word, Scan_Forward, pos, case_sensitive);
  }
  if ( !first_match.buffer ) break;
  
  i64 match_start = first_match.range.min;
  i64 line_end    = get_line_end_pos_from_pos(app, buffer, match_start);
  pos = first_match.range.end - 1;
  b32 matched = true;
  for_i64 (index, 1, splits.count)
  {
   String word = splits.strings[index];
   b32 case_sensitive = string_has_uppercase(word);
   String_Match match = buffer_seek_string(app, buffer, word, Scan_Forward, pos, case_sensitive);
   if ( match.buffer )
   {
    if ( match.range.max <= line_end )
    {
     pos = match.range.end - 1;
    }
    else
    {
     // This breaks for @FuzzyMultiline
     pos = get_line_start_pos_from_pos(app, buffer, match.range.start) - 1;
     matched = false;
     break;
    }
   }
   else return result;
  }
  
  if ( matched )
  {
   result = match_start;
   break;
  }
  
  assert_defend(pos > original_pos, return buffer_size;);
 }
 
 return result;
}

internal i64
kv_fuzzy_search_backward(App *app, Buffer_ID buffer, i64 pos, String needle)
{
    i64 result = -1;
    
    Scratch_Block temp(app);
    String8_Array splits = kv_string_split_wildcards(temp, needle);
    if ( !splits.count ) { return result; }
    
    while( pos > -1 )
    {
        i64 original_pos = pos;
        String_Match first_match;
        {
            String last_word = splits.strings[splits.count-1];
            b32 case_sensitive = string_has_uppercase(last_word);
            first_match = buffer_seek_string(app, buffer, last_word, Scan_Backward, pos, case_sensitive);
        }
        if( !first_match.buffer ) break;
        
        i64 match_start = first_match.range.max;
        i64 line_start   = get_line_start_pos_from_pos(app, buffer, match_start);
        pos = first_match.range.start;
        b32 matched = true;
        for (i64 index = splits.count-2;
             index >= 0;
             index--)
        {
            String word = splits.strings[index];
            b32 case_sensitive = string_has_uppercase(word);
            String_Match match = buffer_seek_string(app, buffer, word, Scan_Backward, pos, case_sensitive);
            if ( match.buffer)
            {
                if ( match.range.min >= line_start )
                {
                    pos = match.range.start;
                }
                else
                {
                    pos = get_line_end_pos_from_pos(app, buffer, match.range.start);
                    matched = false;
                    break;
                }
            }
            else
            {
                return result;
            }
        }
        if ( matched )
        {
            result = pos;
            break;
        }
        
        assert_defend(pos < original_pos, return result;);
    }
    
    return result;
}

internal void
print_string_match_list_to_buffer(App *app, Buffer_ID out_buffer_id, String_Match_List matches)
{
    Scratch_Block scratch(app);
    clear_buffer(app, out_buffer_id);
    Buffer_Insertion out = begin_buffer_insertion_at_buffered(app, out_buffer_id, 0, scratch, KB(64));
    buffer_set_setting(app, out_buffer_id, BufferSetting_ReadOnly, true);
    buffer_set_setting(app, out_buffer_id, BufferSetting_RecordsHistory, false);
    
    Temp_Memory buffer_name_restore_point = begin_temp(scratch);
    String current_filename = {};
    Buffer_ID current_buffer = 0;
    
    if (matches.first != 0){
        for (String_Match *node = matches.first;
             node != 0;
             node = node->next){
            if (node->buffer != out_buffer_id){
                if (current_buffer != 0 && current_buffer != node->buffer){
                    insertc(&out, '\n');
                }
                if (current_buffer != node->buffer){
                    end_temp(buffer_name_restore_point);
                    current_buffer = node->buffer;
                    current_filename = push_buffer_filename(app, scratch, current_buffer);
                    if (current_filename.size == 0){
                        current_filename = push_buffer_unique_name(app, scratch, current_buffer);
                    }
                }
                
                Buffer_Cursor cursor = buffer_compute_cursor(app, current_buffer, seek_pos(node->range.first));
                Temp_Memory line_temp = begin_temp(scratch);
                String full_line_str = push_buffer_line(app, scratch, current_buffer, cursor.line);
                String line_str = string_skip_chop_whitespace(full_line_str);
                insertf(&out, "%.*s:%d:%d: %.*s\n",
                        string_expand(current_filename), cursor.line, cursor.col,
                        string_expand(line_str));
                end_temp(line_temp);
            }
        }
    }
    else{
        insertf(&out, "no matches");
    }
    
    end_buffer_insertion(&out);
}

internal void
kv_filter_match_list(App *app, String_Match_List *matches, Buffer_ID out_buffer)
{
    string_match_list_filter_remove_buffer(matches, out_buffer);
    string_match_list_filter_remove_buffer_predicate(app, matches, buffer_has_name_with_star);
    string_match_list_filter_remove_buffer_predicate(app, matches, buffer_is_skm);
}

internal void
print_all_matches_all_buffers(App *app, String8_Array match_patterns, String_Match_Flag must_have_flags, String_Match_Flag must_not_have_flags, Buffer_ID out_buffer)
{
    Scratch_Block scratch(app);
    String_Match_List matches = find_all_matches_all_buffers(app, scratch, match_patterns, must_have_flags, must_not_have_flags);
    kv_filter_match_list(app, &matches, out_buffer);
    print_string_match_list_to_buffer(app, out_buffer, matches);
}

internal void
print_all_matches_all_buffers(App *app, String8 pattern, String_Match_Flag must_have_flags, String_Match_Flag must_not_have_flags, Buffer_ID out_buffer_id)
{
    String8_Array array = {&pattern, 1};
    print_all_matches_all_buffers(app, array, must_have_flags, must_not_have_flags, out_buffer_id);
}

internal String
query_user_list_needle(App *app, Arena *arena)
{
    u8 *space = push_array(arena, u8, KB(1));
    return(get_query_string(app, "List Locations For: ", space, KB(1)));
}

internal String_Array
user_list_definition_array(App *app, Arena *arena, String base_needle)
{
    String_Array result = {};
    if (base_needle.size > 0)
    {
        result.count = 12;
        result.vals = push_array(arena, String, result.count);
        i1 i = 0;
        result.vals[i++] = (push_stringfz(arena, "struct %.*s{"  , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "struct %.*s\n{", string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "struct %.*s\r\n{", string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "struct %.*s {" , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "union %.*s{"   , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "union %.*s\n{" , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "union %.*s\r\n{" , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "union %.*s {"  , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "enum %.*s{"    , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "enum %.*s\n{"  , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "enum %.*s\r\n{"  , string_expand(base_needle)));
        result.vals[i++] = (push_stringfz(arena, "enum %.*s {"   , string_expand(base_needle)));
        Assert(i == result.count);
    }
    return(result);
}

internal String_Array
query_user_list_definition_needle(App *app, Arena *arena){
    u8 *space = push_array(arena, u8, KB(1));
    String base_needle = get_query_string(app, "List Definitions For: ", space, KB(1));
    return(user_list_definition_array(app, arena, base_needle));
}

internal void
list_all_locations__generic(App *app, String8_Array needle, List_All_Locations_Flag flags)
{
    if (needle.count > 0)
    {
        String_Match_Flag must_have_flags = 0;
        String_Match_Flag must_not_have_flags = 0;
        if (HasFlag(flags, ListAllLocationsFlag_CaseSensitive))
        {
            AddFlag(must_have_flags, StringMatch_CaseSensitive);
        }
        if (!HasFlag(flags, ListAllLocationsFlag_MatchSubstring))
        {
            AddFlag(must_not_have_flags, StringMatch_LeftSideSloppy);
            AddFlag(must_not_have_flags, StringMatch_RightSideSloppy);
        }
       
        Buffer_ID search_buffer = maybe_create_buffer_and_clear_by_name(app, search_buffer_name, global_bottom_view);
        print_all_matches_all_buffers(app, needle, must_have_flags, must_not_have_flags, search_buffer);
        lock_jump_buffer(app, search_buffer_name);
    }
}

internal void
list_all_locations__generic(App *app, String needle, List_All_Locations_Flag flags)
{
    if (needle.size != 0){
        String_Array array = {&needle, 1};
        list_all_locations__generic(app, array, flags);
    }
}

internal void
list_all_locations__generic_query(App *app, List_All_Locations_Flag flags){
    Scratch_Block scratch(app);
    u8 *space = push_array(scratch, u8, KB(1));
    String needle = get_query_string(app, "List Locations For: ", space, KB(1));
    list_all_locations__generic(app, needle, flags);
}

internal void
list_all_locations__generic_identifier(App *app, List_All_Locations_Flag flags)
{
    Scratch_Block scratch(app);
    String needle = push_token_or_word_under_active_cursor(app, scratch);
    list_all_locations__generic(app, needle, flags);
}

internal void
list_all_locations__generic_view_range(App *app, List_All_Locations_Flag flags){
    Scratch_Block scratch(app);
    String needle = push_view_range_string(app, scratch);
    list_all_locations__generic(app, needle, flags);
}

CUSTOM_COMMAND_SIG(list_all_locations)
CUSTOM_DOC("Queries the user for a string and lists all exact case-sensitive matches found in all open buffers.")
{
    list_all_locations__generic_query(app, ListAllLocationsFlag_CaseSensitive);
}

CUSTOM_COMMAND_SIG(list_all_substring_locations)
CUSTOM_DOC("Queries the user for a string and lists all case-sensitive substring matches found in all open buffers.")
{
    list_all_locations__generic_query(app, ListAllLocationsFlag_CaseSensitive|ListAllLocationsFlag_MatchSubstring);
}

CUSTOM_COMMAND_SIG(list_all_locations_case_insensitive)
CUSTOM_DOC("Queries the user for a string and lists all exact case-insensitive matches found in all open buffers.")
{
    list_all_locations__generic_query(app, 0);
}

CUSTOM_COMMAND_SIG(list_all_substring_locations_case_insensitive)
CUSTOM_DOC("Queries the user for a string and lists all case-insensitive substring matches found in all open buffers.")
{
    list_all_locations__generic_query(app, ListAllLocationsFlag_MatchSubstring);
}

internal void 
list_all_locations_of_identifier(App *app)
{
    list_all_locations__generic_identifier(app, ListAllLocationsFlag_CaseSensitive);
}

internal Range_i64
get_word_complete_needle_range(App *app, Buffer_ID buffer, i64 pos)
{
    Range_i64 needle_range = {};
    needle_range.max = pos;
    needle_range.min = scan(app, boundary_alnum_underscore_utf8, buffer, Scan_Backward, pos);
    i64 e = scan(app, boundary_alnum_underscore_utf8, buffer, Scan_Forward, needle_range.min);
    if (pos > e){
        needle_range = Ii64(pos);
    }
    return(needle_range);
}

internal void
string_match_list_enclose_all(App *app, String_Match_List list,
                              Enclose_Function *enclose){
    for (String_Match *node = list.first;
         node != 0;
         node = node->next){
        node->range = enclose(app, node->buffer, node->range);
    }
}

global String_Match_Flag complete_must = (StringMatch_CaseSensitive|
                                          StringMatch_RightSideSloppy);
global String_Match_Flag complete_must_not = StringMatch_LeftSideSloppy;

internal String_Match_List
get_complete_list_raw(App *app, Arena *arena, Buffer_ID buffer,
                      Range_i64 needle_range, String needle){
    local_persist Character_Predicate *pred =
        &character_predicate_alnum_underscore_utf8;
    
    String_Match_List result = {};
    i64 size = buffer_get_size(app, buffer);
    if (range_size(needle_range) > 0){
        String_Match_List up = buffer_find_all_matches(app, arena, buffer, 0,
                                                       Ii64(0, needle_range.min),
                                                       needle, pred, Scan_Backward, false);
        String_Match_List down = buffer_find_all_matches(app, arena, buffer, 0,
                                                         Ii64(needle_range.max, size),
                                                         needle, pred, Scan_Forward, false);
        string_match_list_filter_flags(&up, complete_must, complete_must_not);
        string_match_list_filter_flags(&down, complete_must, complete_must_not);
        result = string_match_list_merge_nearest(&up, &down, needle_range);
    }
    else{
        result = buffer_find_all_matches(app, arena, buffer, 0,
                                         Ii64(0, size), needle, pred, Scan_Forward, false);
        string_match_list_filter_flags(&result, complete_must, complete_must_not);
    }
    
    string_match_list_enclose_all(app, result,
                                  right_enclose_alnum_underscore_utf8);
    return(result);
}

function void
word_complete_list_extend_from_raw(App *app, Arena *arena, String_Match_List *matches, List_String *list, Table_Data_u64 *used_table){
    ProfileScope(app, "word complete list extend from raw");
    Scratch_Block scratch(app);
    for (String_Match *node = matches->first;
         node != 0;
         node = node->next){
        String s = push_buffer_range(app, scratch, node->buffer, node->range);
        Table_Lookup lookup = table_lookup(used_table, s);
        if (!lookup.found_match){
            String data = push_data_copy(arena, s);
            table_insert(used_table, data, 1);
            string_list_push(arena, list, data);
        }
    }
}

function void
word_complete_iter_init__inner(Buffer_ID buffer, String needle, Range_i64 range, Word_Complete_Iterator *iter){
    App *app = iter->app;
    Arena *arena = iter->arena;
    
    Base_Allocator *allocator = get_base_allocator_system();
    if (iter->already_used_table.allocator != 0){
        end_temp(iter->arena_restore);
        table_clear(&iter->already_used_table);
    }
    
    block_zero_struct(iter);
    iter->app = app;
    iter->arena = arena;
    
    iter->arena_restore = begin_temp(arena);
    iter->needle = push_string_copyz(arena, needle);
    iter->first_buffer = buffer;
    iter->current_buffer = buffer;
    
    Scratch_Block scratch(app, arena);
    String_Match_List list = get_complete_list_raw(app, scratch, buffer, range, needle);
    
    iter->already_used_table = make_table_Data_u64(allocator, 100);
    word_complete_list_extend_from_raw(app, arena, &list, &iter->list, &iter->already_used_table);
    
    iter->scan_all_buffers = true;
}

function void
word_complete_iter_init(Buffer_ID buffer, Range_i64 range, Word_Complete_Iterator *iter){
    if (iter->app != 0 && iter->arena != 0){
        App *app = iter->app;
        Arena *arena = iter->arena;
        Scratch_Block scratch(app, arena);
        String needle = push_buffer_range(app, scratch, buffer, range);
        word_complete_iter_init__inner(buffer, needle, range, iter); 
    }
}

function void
word_complete_iter_init(Buffer_ID first_buffer, String needle, Word_Complete_Iterator *iter){
    if (iter->app != 0 && iter->arena != 0){
        word_complete_iter_init__inner(first_buffer, needle, Ii64(), iter);
    }
}

function void
word_complete_iter_init(String needle, Word_Complete_Iterator *iter){
    if (iter->app != 0 && iter->arena != 0){
        App *app = iter->app;
        Buffer_ID first_buffer = get_buffer_next(app, 0, Access_Read);
        word_complete_iter_init__inner(first_buffer, needle, Ii64(), iter);
    }
}

function void
word_complete_iter_stop_on_this_buffer(Word_Complete_Iterator *iter){
    iter->scan_all_buffers = false;
}

function void
word_complete_iter_next(Word_Complete_Iterator *it){
    for (;;){
        if (it->node == 0){
            it->node = it->list.first;
        }
        else{
            it->node = it->node->next;
        }
        
        if (it->node != 0){
            break;
        }
        
        if (!it->scan_all_buffers){
            break;
        }
        
        App *app = it->app;
        Buffer_ID next = get_buffer_next_looped(app, it->current_buffer, Access_Read);
        if (next == it->first_buffer){
            break;
        }
        
        it->node = it->list.last;
        it->current_buffer = next;
        Scratch_Block scratch(app);
        String_Match_List list = get_complete_list_raw(app, scratch,
                                                       next, Ii64(), it->needle);
        word_complete_list_extend_from_raw(app, it->arena, &list,
                                           &it->list, &it->already_used_table);
    }
}

function String
word_complete_iter_read(Word_Complete_Iterator *it){
    String result = {};
    if (it->node == 0){
        result = it->needle;
    }
    else{
        result = it->node->string;
    }
    return(result);
}


function b32
word_complete_iter_is_at_base_slot(Word_Complete_Iterator *it){
    return(it->node == 0);
}

function Word_Complete_Iterator*
word_complete_get_shared_iter(App *app){
    local_persist Arena completion_arena = {};
    local_persist Word_Complete_Iterator it = {};
    local_persist b32 first_call = true;
    if (first_call){
        first_call = false;
        completion_arena = make_arena_system();
    }
    it.app = app;
    it.arena = &completion_arena;
    return(&it);
}

CUSTOM_COMMAND_SIG(word_complete)
CUSTOM_DOC("Iteratively tries completing the word to the left of the cursor with other words in open buffers that have the same prefix string.")
{
    ProfileScope(app, "word complete");
    
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0){
        Managed_Scope scope = view_get_managed_scope(app, view);
        
        b32 first_completion = false;
        Rewrite_Type *rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
        if (*rewrite != Rewrite_WordComplete){
            first_completion = true;
        }
        
        set_next_rewrite(app, view, Rewrite_WordComplete);
        
        Word_Complete_Iterator *it = word_complete_get_shared_iter(app);
        local_persist b32 initialized = false;
        local_persist Range_i64 range = {};
        
        if (first_completion || !initialized){
            ProfileBlock(app, "word complete state init");
            initialized = false;
            i64 pos = view_get_cursor_pos(app, view);
            Range_i64 needle_range = get_word_complete_needle_range(app, buffer, pos);
            if (range_size(needle_range) > 0){
                initialized = true;
                range = needle_range;
                word_complete_iter_init(buffer, needle_range, it);
            }
        }
        
        if (initialized){
            ProfileBlock(app, "word complete apply");
            
            word_complete_iter_next(it);
            String str = word_complete_iter_read(it);
            
            buffer_replace_range(app, buffer, range, str);
            
            range.max = range.min + str.size;
            view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
        }
    }
}

function Word_Complete_Menu
make_word_complete_menu(Render_Caller_Function *prev_render_caller, Word_Complete_Iterator *it){
    Word_Complete_Menu menu = {};
    menu.prev_render_caller = prev_render_caller;
    menu.it = it;
    return(menu);
}

function void
word_complete_menu_next(Word_Complete_Menu *menu){
    i1 count = 0;
    for (u32 i = 0; i < ArrayCount(menu->options); i += 1){
        word_complete_iter_next(menu->it);
        if (word_complete_iter_is_at_base_slot(menu->it)){
            break;
        }
        else{
            menu->options[i] = word_complete_iter_read(menu->it);
            count += 1;
        }
    }
    menu->count = count;
}

function void
word_complete_menu_render(App *app, Frame_Info frame_info, View_ID view){
    Managed_Scope scope = view_get_managed_scope(app, view);
    Word_Complete_Menu **menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
    Word_Complete_Menu *menu = *menu_ptr;
    
    if (menu != 0){
        menu->prev_render_caller(app, frame_info, view);
        
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        Face_ID face = get_face_id(app, buffer);
        
        Scratch_Block scratch(app);
        
        Fancy_Block block = {};
        for (i1 i = 0; i < menu->count; i += 1){
            if (menu->options[i].size > 0){
                Fancy_Line *line = push_fancy_line(scratch, &block, face);
                push_fancy_stringf(scratch, line, fcolor_id(defcolor_pop1), "F%d:", i + 1);
                push_fancy_string(scratch, line, fcolor_id(defcolor_text_default), menu->options[i]);
            }
        }
        
        Rect_f32 region = view_get_buffer_region(app, view);
        
        Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
        Buffer_Point buffer_point = scroll.position;
        i64 pos = view_get_cursor_pos(app, view);
        Vec2_f32 cursor_p = view_relative_xy_of_pos(app, view, buffer_point.line_number, pos);
        cursor_p -= buffer_point.pixel_shift;
        cursor_p += region.p0;
        
        Face_Metrics metrics = get_face_metrics(app, face);
        f32 x_padding = metrics.normal_advance;
        f32 x_half_padding = x_padding*0.5f;
        
        draw_drop_down(app, face, &block, cursor_p, region, x_padding, x_half_padding,
                       fcolor_id(defcolor_margin_hover), fcolor_id(defcolor_back));
    }
}

function Edit
get_word_complete_from_user_drop_down(App *app){
    View_ID view = get_this_ctx_view(app, Access_Always);
    View_Context ctx = view_current_context(app, view);
    Render_Caller_Function *prev_render_caller = ctx.render_caller;
    
    Edit result = {};
    
    Word_Complete_Iterator *it = word_complete_get_shared_iter(app);
    
    i64 pos = view_get_cursor_pos(app, view);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_word_complete_needle_range(app, buffer, pos);
    if (range_size(range) != 0){
        word_complete_iter_init(buffer, range, it);
        Word_Complete_Menu menu = make_word_complete_menu(prev_render_caller, it);
        word_complete_menu_next(&menu);
        
        ctx.render_caller = word_complete_menu_render;
        View_Context_Block ctx_block(app, view, &ctx);
        
        Managed_Scope scope = view_get_managed_scope(app, view);
        Word_Complete_Menu **menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
        *menu_ptr = &menu;
        
        b32 keep_looping_menu = true;
        for (;keep_looping_menu;){
            User_Input in = get_next_input(app, EventPropertyGroup_Any,
                                           EventProperty_Escape);
            if (in.abort){
                break;
            }
            
            b32 handled = true;
            switch (in.event.kind){
                case InputEventKind_TextInsert:
                {
                    write_text_input(app);
                    pos = view_get_cursor_pos(app, view);
                    range = get_word_complete_needle_range(app, buffer, pos);
                    if (range_size(range) == 0){
                        keep_looping_menu = false;
                    }
                    else{
                        word_complete_iter_init(buffer, range, it);
                        menu = make_word_complete_menu(prev_render_caller, it);
                        word_complete_menu_next(&menu);
                        if (menu.count == 0){
                            keep_looping_menu = false;
                        }
                    }
                }break;
                
                case InputEventKind_KeyStroke:
                {
                    switch (in.event.key.code){
                        case Key_Code_Return:
                        {
                            result.text = menu.options[0];
                            result.range = range;
                            keep_looping_menu = false;
                        }break;
                        
                        case Key_Code_Tab:
                        {
                            word_complete_menu_next(&menu);
                        }break;
                        
                        case Key_Code_F1:
                        case Key_Code_F2:
                        case Key_Code_F3:
                        case Key_Code_F4:
                        case Key_Code_F5:
                        case Key_Code_F6:
                        case Key_Code_F7:
                        case Key_Code_F8:
                        {
                            i1 index = (in.event.key.code - Key_Code_F1);
                            result.text = menu.options[index];
                            result.range = range;
                            keep_looping_menu = false;
                        }break;
                        
                        case Key_Code_Backspace:
                        {
                            backspace_char(app);
                            pos = view_get_cursor_pos(app, view);
                            range = get_word_complete_needle_range(app, buffer, pos);
                            if (range_size(range) == 0){
                                keep_looping_menu = false;
                            }
                            else{
                                word_complete_iter_init(buffer, range, it);
                                menu = make_word_complete_menu(prev_render_caller, it);
                                word_complete_menu_next(&menu);
                                if (menu.count == 0){
                                    keep_looping_menu = false;
                                }
                            }
                        }break;
                        
                        default:
                        {
                            leave_current_input_unhandled(app);
                        }break;
                    }
                }break;
                
                case InputEventKind_MouseButton:
                {
                    leave_current_input_unhandled(app);
                    keep_looping_menu = false;
                }break;
                
                default:
                {
                    handled = false;
                }break;
            }
            
            if (!handled){
                leave_current_input_unhandled(app);
            }
        }
        
        scope = view_get_managed_scope(app, view);
        menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
        *menu_ptr = 0;
    }
    
    return(result);
}

CUSTOM_COMMAND_SIG(word_complete_drop_down)
CUSTOM_DOC("Word complete with drop down menu.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0){
        Edit edit = get_word_complete_from_user_drop_down(app);
        if (edit.text.size > 0){
            buffer_replace_range(app, buffer, edit.range, edit.text);
            view_set_cursor_and_preferred_x(app, view, seek_pos(edit.range.min + edit.text.size));
        }
    }
}

// BOTTOM

