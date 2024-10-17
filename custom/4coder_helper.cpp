/*
 * Miscellaneous helpers for common operations.
 * NOTE(kv): It's when you just give up on trying to organize things and accept that no,
 * there really is definite order to things, at the very least it's not immediately obvious.
 * And when you pull your hair out trying to finish the goddamn project after 12 months,
 * "immediately obvious" is the bare minimum you can afford.
 */

// TOP

#define GET_VIEW_AND_BUFFER \
View_ID   view = get_active_view(app, Access_ReadVisible); \
Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible)

#define HISTORY_GROUP_SCOPE \
  History_Group history_group = history_group_begin(app, buffer); \
  defer( history_group_end(history_group) );

////////////////////////////////

#define scope_attachment(app,S,I,T) ((T*)managed_scope_get_attachment((app), (S), (I), sizeof(T)))
#define set_custom_hook(app,ID,F) set_custom_hook_func((app),(ID),(Void_Func*)(F))

////////////////////////////////

function i32
get_command_id(Custom_Command_Function *func)
{
    i32 result = -1;
    for (i32 i = 0; i < ArrayCountSigned(fcoder_metacmd_table); i += 1)
    {
        if (func == fcoder_metacmd_table[i].proc)
        {
            result = i;
            break;
        }
    }
    return(result);
}

function Command_Metadata*
get_command_metadata(Custom_Command_Function *func){
    Command_Metadata *result = 0;
    i32 id = get_command_id(func);
    if (id >= 0){
        result = &fcoder_metacmd_table[id];
    }
    return(result);
}

function Command_Metadata*
get_command_metadata_from_name(String name){
    Command_Metadata *result = 0;
    Command_Metadata *candidate = fcoder_metacmd_table;
    for (i32 i = 0; i < ArrayCountSigned(fcoder_metacmd_table); i += 1, candidate += 1){
        if (string_match(SCu8(candidate->name, candidate->name_len), name)){
            result = candidate;
            break;
        }
    }
    return(result);
}

////////////////////////////////

function Token_Array
get_token_array_from_buffer(App *app, Buffer_ID buffer){
    Token_Array result = {};
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
    if (lex_task_ptr != 0){
        async_task_wait(app, &global_async_system, *lex_task_ptr);
    }
    Token_Array *ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
    if (ptr != 0){
        result = *ptr;
    }
 return(result);
}

function Token *
kv_token_at_cursor(App *app, i64 delta=0)
{
 GET_VIEW_AND_BUFFER;
 Token_Array tokens = get_token_array_from_buffer(app, buffer);
 i64 pos = view_get_cursor_pos(app, view) + delta;
 return token_from_pos(&tokens, pos);
}

function Token_Iterator_Array
get_token_it_at_pos(App *app, Buffer_ID buffer, i64 pos){
 Token_Array tokens = get_token_array_from_buffer(app, buffer);
 return tkarr_at_pos(0, &tokens, pos);
}
function Token_Iterator_Array
get_token_it_at_cursor(App *app, i64 delta=0){
 GET_VIEW_AND_BUFFER;
 i64 pos = view_get_cursor_pos(app, view) + delta;
 return get_token_it_at_pos(app, buffer, pos);
}

function i64 get_line_start_pos(App *app, Buffer_ID buffer, i64 line_number);

function Token_Iterator_Array
get_token_it_for_line(App *app, Buffer_ID buffer, i64 line_number)
{
 Token_Array tokens = get_token_array_from_buffer(app, buffer);
 i64 pos = get_line_start_pos(app, buffer, line_number);
 Token_Iterator_Array result = tkarr_at_pos(0, &tokens, pos);
 return result;
}

////////////////////////////////

function Buffer_Seek
seek_location(ID_Line_Column_Jump_Location location){
    return(seek_line_col(location.line, location.column));
}

function Buffer_Seek
seek_location(ID_Pos_Jump_Location location){
    return(seek_pos(location.pos));
}

function Buffer_Seek
seek_location(Name_Line_Column_Location location){
    return(seek_line_col(location.line, location.column));
}

function Buffer_Seek
seek_jump(Parsed_Jump jump){
    return(seek_location(jump.location));
}

////////////////////////////////

View_Context_Block::View_Context_Block(App *a, View_ID v,
                                       View_Context *ctx){
    this->app = a;
    this->view = v;
    view_push_context(a, v, ctx);
}

View_Context_Block::~View_Context_Block(){
    view_pop_context(this->app, this->view);
}

////////////////////////////////

function Character_Predicate
character_predicate_from_function(Character_Predicate_Function *func){
    Character_Predicate predicate = {};
    i32 byte_index = 0;
    for (u32 i = 0; i <= 255;){
        b8 v[8];
        for (i32 bit_index = 0; bit_index < 8; i += 1, bit_index += 1){
            v[bit_index] = func((u8)i);
        }
        predicate.b[byte_index] = ((v[0] << 0) |
                                   (v[1] << 1) |
                                   (v[2] << 2) |
                                   (v[3] << 3) |
                                   (v[4] << 4) |
                                   (v[5] << 5) |
                                   (v[6] << 6) |
                                   (v[7] << 7));
        byte_index += 1;
    }
    return(predicate);
}

function Character_Predicate
character_predicate_from_character(u8 character){
    Character_Predicate predicate = {};
    predicate.b[character/8] = (1 << (character%8));
    return(predicate);
}

#define character_predicate_check(p, c) (((p).b[(c)/8] & (1 << ((c)%8))) != 0)

function Character_Predicate
character_predicate_or(Character_Predicate *a, Character_Predicate *b){
    Character_Predicate p = {};
    for (u32 i = 0; i < ArrayCount(p.b); i += 1){
        p.b[i] = a->b[i] | b->b[i];
    }
    return(p);
}

function Character_Predicate
character_predicate_and(Character_Predicate *a, Character_Predicate *b){
    Character_Predicate p = {};
    for (u32 i = 0; i < ArrayCount(p.b); i += 1){
        p.b[i] = a->b[i] & b->b[i];
    }
    return(p);
}

function Character_Predicate
character_predicate_not(Character_Predicate *a){
    Character_Predicate p = {};
    for (u32 i = 0; i < ArrayCount(p.b); i += 1){
        p.b[i] = ~(a->b[i]);
    }
    return(p);
}

function i64
buffer_seek_character_class_change__inner(App *app, Buffer_ID buffer, Character_Predicate *positive, Character_Predicate *negative, Scan_Direction direction, i64 start_pos){
    i64 pos = start_pos;
    switch (direction){
        case Scan_Backward:
        {
            String_Match m1 = buffer_seek_character_class(app, buffer, negative, direction, pos);
            String_Match m2 = buffer_seek_character_class(app, buffer, positive, direction, m1.range.min);
            pos = m2.range.min;
            if (m1.buffer == buffer && m2.buffer == buffer){
                pos += 1;
            }
        }break;
        case Scan_Forward:
        {
            pos -= 1;
            String_Match m1 = buffer_seek_character_class(app, buffer, positive, direction, pos);
            String_Match m2 = buffer_seek_character_class(app, buffer, negative, direction, m1.range.min);
            pos = m2.range.min;
        }break;
    }
    return(pos);
}

function i64
buffer_seek_character_class_change_1_0(App *app, Buffer_ID buffer, Character_Predicate *predicate, Scan_Direction direction, i64 start_pos){
    Character_Predicate negative = character_predicate_not(predicate);
    return(buffer_seek_character_class_change__inner(app, buffer, predicate, &negative, direction, start_pos));
}

function i64
buffer_seek_character_class_change_0_1(App *app, Buffer_ID buffer, Character_Predicate *predicate, Scan_Direction direction, i64 start_pos){
 Character_Predicate negative = character_predicate_not(predicate);
 return(buffer_seek_character_class_change__inner(app, buffer, &negative, predicate, direction, start_pos));
}

////////////////////////////////

function i64
view_pos_from_xy(App *app, View_ID view, Vec2_f32 p)
{
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Rect_f32 region = view_get_buffer_region(app, view);
    f32 width = rect_width(region);
    Face_ID face_id = get_face_id(app, buffer);
    Buffer_Scroll scroll_vars = view_get_buffer_scroll(app, view);
    i64 line = scroll_vars.position.line_number;
    p = (p - region.p0) + scroll_vars.position.pixel_shift;
    return(buffer_pos_at_relative_xy(app, buffer, width, face_id, line, p));
}

function Buffer_Point
view_move_buffer_point(App *app, View_ID view, Buffer_Point buffer_point, Vec2_f32 delta){
    delta += buffer_point.pixel_shift;
    Line_Shift_Vertical shift = view_line_shift_y(app, view, buffer_point.line_number, delta.y);
    buffer_point.line_number = shift.line;
    buffer_point.pixel_shift = V2(delta.x, delta.y - shift.y_delta);
    return(buffer_point);
}

function void
view_zero_scroll(App *app, View_ID view){
    Buffer_Scroll scroll = {};
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

function Vec2_f32
view_relative_xy_of_pos(App *app, View_ID view, i64 base_line, i64 pos){
    Rect_f32 rect = view_relative_box_of_pos(app, view, base_line, pos);
    return(rect_center(rect));
}

function void
view_set_cursor_and_preferred_x(App *app, View_ID view, Buffer_Seek seek){
    view_set_cursor(app, view, seek);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek);
    Vec2_f32 p = view_relative_xy_of_pos(app, view, cursor.line, cursor.pos);
    view_set_preferred_x(app, view, p.x);
}

inline void kv_goto_pos(App *app, View_ID view, i64 pos)
{
  view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

inline void kv_goto_token(App *app, Token *token)
{
  View_ID view = get_active_view(app, Access_ReadVisible);
  view_set_cursor_and_preferred_x(app, view, seek_pos(token->pos));
}

function i64
view_set_pos_by_character_delta(App *app, View_ID view, i64 pos, i64 character_delta){
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    i64 character = view_relative_character_from_pos(app, view, cursor.line, cursor.pos);
    i64 new_pos = view_pos_from_relative_character(app, view, cursor.line, character + character_delta);
    return(new_pos);
}

function i64
view_set_cursor_by_character_delta(App *app, View_ID view, i64 character_delta){
 i64 pos = view_get_cursor_pos(app, view);
 i64 new_pos = view_set_pos_by_character_delta(app, view, pos, character_delta);
 view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
 return(new_pos);
}

function i64
view_correct_cursor(App *app, View_ID view){
    i64 pos = view_get_cursor_pos(app, view);
    i64 new_pos = view_set_pos_by_character_delta(app, view, pos, 0);
    view_set_cursor(app, view, seek_pos(new_pos));
    return(new_pos);
}

function i64
view_correct_mark(App *app, View_ID view){
    i64 pos = view_get_mark_pos(app, view);
    i64 new_pos = view_set_pos_by_character_delta(app, view, pos, 0);
    view_set_mark(app, view, seek_pos(new_pos));
    return(new_pos);
}

function Vec2_f32
buffer_point_difference(App *app, Buffer_ID buffer, f32 width, Face_ID face_id,
                        Buffer_Point a, Buffer_Point b){
    f32 y_difference = buffer_line_y_difference(app, buffer, width, face_id, a.line_number, b.line_number);
    Vec2_f32 result = a.pixel_shift - b.pixel_shift;
    result.y += y_difference;
    return(result);
}

function Vec2_f32
view_point_difference(App *app, View_ID view, Buffer_Point a, Buffer_Point b){
    f32 y_difference = view_line_y_difference(app, view, a.line_number, b.line_number);
    Vec2_f32 result = a.pixel_shift - b.pixel_shift;
    result.y += y_difference;
    return(result);
}

////////////////////////////////

function Range_i64
buffer_range(App *app, Buffer_ID buffer)
{
    Range_i64 range = {};
    range.min = 0;
    range.end = buffer_get_size(app, buffer);
    return(range);
}

function i64
buffer_side(App *app, Buffer_ID buffer, Side side){
 return(range_side(buffer_range(app, buffer), side));
}

// NOTE(kv): this was written by Allen, but idk why it doesn't increment the range max
function Range_i64
get_view_range(App *app, View_ID view){
 return(Ii64(view_get_cursor_pos(app, view), view_get_mark_pos(app, view)));
}

function void
set_view_range(App *app, View_ID view, Range_i64 range){
    i64 c = view_get_cursor_pos(app, view);
    i64 m = view_get_mark_pos(app, view);
    if (c < m){
        view_set_cursor_and_preferred_x(app, view, seek_pos(range.min));
        view_set_mark(app, view, seek_pos(range.max));
    }
    else{
        view_set_mark(app, view, seek_pos(range.min));
        view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
    }
}

function b32
is_valid_line(App *app, Buffer_ID buffer_id, i64 line){
    i64 max_line = buffer_get_line_count(app, buffer_id);
    return(1 <= line && line <= max_line);
}

function b32
is_valid_line_range(App *app, Buffer_ID buffer_id, Range_i64 range){
    i64 max_line = buffer_get_line_count(app, buffer_id);
    return(1 <= range.first && range.first <= range.end && range.end <= max_line);
}

function i64
get_line_number_from_pos(App *app, Buffer_ID buffer, i64 pos){
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
    return(cursor.line);
}

function i64
buffer_get_character_legal_pos_from_pos(App *app, Buffer_ID buffer, f32 width, Face_ID face, i64 pos){
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
    i64 character = buffer_relative_character_from_pos(app, buffer, width, face, cursor.line, pos);
    return(buffer_pos_from_relative_character(app, buffer, width, face, cursor.line, character));
}

function i64
view_get_character_legal_pos_from_pos(App *app, View_ID view, i64 pos){
 Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
 i64 character = view_relative_character_from_pos(app, view, cursor.line, pos);
 return(view_pos_from_relative_character(app, view, cursor.line, character));
}

function Buffer_Cursor
get_line_side(App *app, Buffer_ID buffer, i64 line_number, Side side){
 i64 character_index = (side == Side_Min)?(1):(-1);
 return(buffer_compute_cursor(app, buffer, seek_line_col(line_number, character_index)));
}
function i64
get_line_side_pos(App *app, Buffer_ID buffer, i64 line_number, Side side){
 i64 pos = -1;
 Buffer_Cursor cursor = get_line_side(app, buffer, line_number, side);
 if (cursor.line != 0){
  pos = cursor.pos;
 }
 return(pos);
}

function Buffer_Cursor
get_line_start(App *app, Buffer_ID buffer, i64 line_number){
 return(get_line_side(app, buffer, line_number, Side_Min));
}
function i64
get_line_start_pos(App *app, Buffer_ID buffer, i64 line_number)
{
 return(get_line_side_pos(app, buffer, line_number, Side_Min));
}


// NOTE(allen): The position returned has the index of the terminating LF.
// not one past the newline character.
function Buffer_Cursor
get_line_end(App *app, Buffer_ID buffer, i64 line_number){
    return(get_line_side(app, buffer, line_number, Side_Max));
}
function i64
get_line_end_pos(App *app, Buffer_ID buffer, i64 line_number){
    return(get_line_side_pos(app, buffer, line_number, Side_Max));
}

// NOTE(allen): The range returned does not include the terminating LF or CRLF
function Range_Cursor
get_line_range(App *app, Buffer_ID buffer, i64 line_number){
    b32 success = false;
    Range_Cursor result = {};
    result.start = get_line_start(app, buffer, line_number);
    if (result.start.line != 0){
        result.end = get_line_end(app, buffer, line_number);
        if (result.end.line != 0){
            success = true;
        }
    }
    if (!success){
        block_zero_struct(&result);
    }
    return(result);
}

// NOTE(allen): The range returned does not include the terminating LF or CRLF
function Range_i64
get_line_pos_range(App *app, Buffer_ID buffer, i64 line_number){
    Range_Cursor range = get_line_range(app, buffer, line_number);
    Range_i64 result = {};
    if (range.start.line != 0 && range.end.line != 0){
        result = Ii64(range.start.pos, range.end.pos);
    }
    return(result);
}

function Range_i64
make_range_from_cursors(Range_Cursor range){
 return(Ii64(range.start.pos, range.end.pos));
}

function i64
get_line_side_pos_from_pos(App *app, Buffer_ID buffer, i64 pos, Side side){
    i64 line_number = get_line_number_from_pos(app, buffer, pos);
    return(get_line_side_pos(app, buffer, line_number, side));
}
function i64
get_line_start_pos_from_pos(App *app, Buffer_ID buffer, i64 pos){
    return(get_line_side_pos_from_pos(app, buffer, pos, Side_Min));
}
function i64
get_line_end_pos_from_pos(App *app, Buffer_ID buffer, i64 pos){
 return(get_line_side_pos_from_pos(app, buffer, pos, Side_Max));
}

function Token*
get_first_token_from_line(App *app, Buffer_ID buffer, Token_Array tokens, i64 line){
 i64 line_start = get_line_start_pos(app, buffer, line);
 return(token_from_pos(&tokens, line_start));
}


////////////////////////////////

function i64
scan_any_boundary(App *app, Boundary_Function *func, Buffer_ID buffer, Scan_Direction direction, i64 pos)
{
    i64 a = func(app, buffer, Side_Min, direction, pos);
    i64 b = func(app, buffer, Side_Max, direction, pos);
    i64 result = 0;
    if (direction == Scan_Forward){
        result = Min(a, b);
    }
    else{
        result = Max(a, b);
    }
    return(result);
}

function i64
scan(App *app, Boundary_Function *func, Buffer_ID buffer, Scan_Direction direction, i64 pos)
{
    Side side = (direction == Scan_Forward) ? (Side_Max) : (Side_Min);
    return (func(app, buffer, side, direction, pos));
}

function i64
scan_list(App *app, Boundary_Function_List funcs, Buffer_ID buffer, Scan_Direction direction, i64 start_pos)
{
    i64 result = 0;
    if (direction == Scan_Forward){
        i64 size = buffer_get_size(app, buffer);
        result = size + 1;
        for (Boundary_Function_Node *node = funcs.first;
             node != 0;
             node = node->next){
            i64 pos = scan(app, node->func, buffer, direction, start_pos);
            result = Min(result, pos);
        }
    }
    else{
        result = -1;
        for (Boundary_Function_Node *node = funcs.first;
             node != 0;
             node = node->next){
            i64 pos = scan(app, node->func, buffer, direction, start_pos);
            result = Max(result, pos);
        }
    }
    return(result);
}

function void
push_boundary(Arena *arena, Boundary_Function_List *list, Boundary_Function *func){
    Boundary_Function_Node *node = push_array(arena, Boundary_Function_Node, 1);
    sll_queue_push(list->first, list->last, node);
    list->count += 1;
    node->func = func;
}

function Boundary_Function_List
push_boundary_list__innerv(Arena *arena, va_list args){
    Boundary_Function_List list = {};
    for (;;){
        Boundary_Function *func = va_arg(args, Boundary_Function*);
        if (func == 0){
            break;
        }
        push_boundary(arena, &list, func);
    }
    return(list);
}
function Boundary_Function_List
push_boundary_list__inner(Arena *arena, ...){
    va_list args;
    va_start(args, arena);
    Boundary_Function_List result = push_boundary_list__innerv(arena, args);
    va_end(args);
    return(result);
}
#define push_boundary_list(a,...) push_boundary_list__inner((a), __VA_ARGS__, 0)

function i64
boundary_predicate(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos, Character_Predicate *predicate)
{
    i64 result = 0;
    switch (side){
        case Side_Min:
        {
            result = buffer_seek_character_class_change_0_1(app, buffer, predicate, direction, pos);
        }break;
        case Side_Max:
        {
            result = buffer_seek_character_class_change_1_0(app, buffer, predicate, direction, pos);
        }break;
    }
    return(result);
}

function i64
boundary_non_whitespace(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_non_whitespace));
}

function i64
boundary_base10(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_base10));
}

function i64
boundary_base10_colon(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    function Character_Predicate predicate = {};
    function b32 first_call = true;
    if (first_call){
        first_call = false;
        Character_Predicate colon = character_predicate_from_character((u8)':');
        predicate = character_predicate_or(&character_predicate_base10, &colon);
    }
    return(boundary_predicate(app, buffer, side, direction, pos, &predicate));
}

function i64
boundary_base16(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_base16));
}

function i64
boundary_alnum(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_alnum));
}

function i64
boundary_alnum_unicode(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_alnum_utf8));
}

function i64
boundary_alnum_underscore(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_alnum_underscore));
}

function i64
boundary_alnum_underscore_utf8(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_alnum_underscore_utf8));
}

function i64
boundary_alnum_camel(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    i64 an_pos = boundary_alnum(app, buffer, side, direction, pos);
    String_Match m = buffer_seek_character_class(app, buffer, &character_predicate_uppercase, direction, pos);
    i64 cap_pos = m.range.min;
    if (side == Side_Max)
    {
        i64 an_left_pos = boundary_alnum(app, buffer, flip_side(side), flip_direction(direction), an_pos);
        if (cap_pos == an_left_pos)
        {
            m = buffer_seek_character_class(app, buffer, &character_predicate_uppercase, direction, cap_pos);
            cap_pos = m.range.min;
        }
    }
    if (direction == Scan_Backward) { return Max(an_pos, cap_pos); }
    else                            { return Min(an_pos, cap_pos); }
}

function i64
boundary_inside_quotes(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    function Character_Predicate predicate = {};
    function b32 first_call = true;
    if (first_call){
        first_call = false;
        predicate = character_predicate_from_character((u8)'"');
        predicate = character_predicate_not(&predicate);
    }
    return(boundary_predicate(app, buffer, side, direction, pos, &predicate));
}

function i64
boundary_token(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    i64 result = boundary_non_whitespace(app, buffer, side, direction, pos);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if (tokens.tokens != 0){
        switch (direction){
            case Scan_Forward:
            {
                i64 buffer_size = buffer_get_size(app, buffer);
                result = buffer_size;
                if (tokens.count > 0){
                    Token_Iterator_Array it = tkarr_at_pos(0, &tokens, pos);
                    Token *token = tkarr_read(&it);
                    if (token->kind == TokenBaseKind_Whitespace){
                        tkarr_inc_non_whitespace(&it);
                        token = tkarr_read(&it);
                    }
                    if (token != 0){
                        if (side == Side_Max){
                            result = token->pos + token->size;
                        }
                        else{
                            if (token->pos <= pos){
                                tkarr_inc_non_whitespace(&it);
                                token = tkarr_read(&it);
                            }
                            if (token != 0){
                                result = token->pos;
                            }
                        }
                    }
                }
            }break;
            
            case Scan_Backward:
            {
                result = 0;
                if (tokens.count > 0){
                    Token_Iterator_Array it = tkarr_at_pos(0, &tokens, pos);
                    Token *token = tkarr_read(&it);
                    if (token->kind == TokenBaseKind_Whitespace){
                        tkarr_dec_non_whitespace(&it);
                        token = tkarr_read(&it);
                    }
                    if (token != 0){
                        if (side == Side_Min){
                            if (token->pos >= pos){
                                tkarr_dec_non_whitespace(&it);
                                token = tkarr_read(&it);
                            }
                            result = token->pos;
                        }
                        else{
                            if (token->pos + token->size >= pos){
                                tkarr_dec_non_whitespace(&it);
                                token = tkarr_read(&it);
                            }
                            result = token->pos + token->size;
                        }
                    }
                }
   }break;
  }
 }
 return(result);
}

function i64
boundary_line(App *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
 i64 line_number = get_line_number_from_pos(app, buffer, pos);
 i64 new_pos = get_line_side_pos(app, buffer, line_number, side);
 if (direction == Scan_Backward && new_pos >= pos){
  if (line_number > 1){
   new_pos = get_line_side_pos(app, buffer, line_number - 1, side);
  }
  else{
   new_pos = 0;
  }
 }
 else if (direction == Scan_Forward && new_pos <= pos){
  new_pos = get_line_side_pos(app, buffer, line_number + 1, side);
  if (new_pos <= pos){
   new_pos = (i32)buffer_get_size(app, buffer);
  }
 }
 return(new_pos);
}

////////////////////////////////

// TODO(allen): these need a little more rewrite
function void
seek_string_forward(App *app, Buffer_ID buffer, i64 pos, i64 end, String8 needle, i64 *result)
{
 if (end == 0){
  end = (i32)buffer_get_size(app, buffer);
 }
 String_Match match = {};
 match.range.first = pos;
 for (;;)
 {
  match = buffer_seek_string(app, buffer, needle, Scan_Forward, (i32)match.range.first, true);
  if ( HasFlag(match.flags, StringMatch_CaseSensitive) ) break;
  if (match.buffer != buffer || match.range.first >= end) break;
 }
 if (match.range.first < end && match.buffer == buffer){
  *result = match.range.first;
 }
 else{
  *result = buffer_get_size(app, buffer);
 }
}

function void
seek_string_backward(App *app, Buffer_ID buffer, i64 pos, i64 min, String8 needle, i64 *result)
{
    String_Match match = {};
    match.range.first = pos;
    for (;;)
    {
        match = buffer_seek_string(app, buffer, needle, Scan_Backward, match.range.first, true);
        if ( HasFlag(match.flags, StringMatch_CaseSensitive) ) break;
        if (match.buffer != buffer || match.range.first < min) break;
        
    }
    if (match.range.first >= min && match.buffer == buffer){
        *result = match.range.first;
    }
    else{
        *result = -1;
    }
}

function void
seek_string_insensitive_forward(App *app, Buffer_ID buffer, i64 pos, i64 end, String8 needle, i64 *result)
{
    if (end == 0){
        end = (i32)buffer_get_size(app, buffer);
    }
    String_Match match = buffer_seek_string(app, buffer, needle, Scan_Forward, pos, false);
    if (match.range.first < end && match.buffer == buffer){
        *result = match.range.first;
    }
    else{
        *result = buffer_get_size(app, buffer);
    }
}

function void
seek_string_insensitive_backward(App *app, Buffer_ID buffer, i64 pos, i64 min, String needle, i64 *result)
{
    String_Match match = buffer_seek_string(app, buffer, needle, Scan_Backward, pos, false);
    if (match.range.first >= min && match.buffer == buffer){
        *result = match.range.first;
    }
    else{
        *result = -1;
    }
}

function void
seek_string(App *app, Buffer_ID buffer_id, i64 pos, i64 end, i64 min, String8 str, i64 *result, Buffer_Seek_String_Flags flags)
{
    switch (flags & 3)
    {
        case 0:
        {
            seek_string_forward(app, buffer_id, pos, end, str, result);
        }break;
        
        case BufferSeekString_Backward:
        {
            seek_string_backward(app, buffer_id, pos, min, str, result);
        }break;
        
        case BufferSeekString_CaseInsensitive:
        {
            seek_string_insensitive_forward(app, buffer_id, pos, end, str, result);
        }break;
        
        case BufferSeekString_Backward|BufferSeekString_CaseInsensitive:
        {
            seek_string_insensitive_backward(app, buffer_id, pos, min, str, result);
        }break;
    }
}

////////////////////////////////

function Range_i64
get_line_range_from_pos_range(App *app, Buffer_ID buffer, Range_i64 pos_range){
    Range_i64 line_range = {};
    line_range.first = get_line_number_from_pos(app, buffer, pos_range.first);
    if (line_range.first != 0){
        line_range.end = get_line_number_from_pos(app, buffer, pos_range.opl);
        if (line_range.end == 0){
            line_range.first = 0;
        }
    }
    return(line_range);
}

// NOTE(allen): The end of the returned range does not include the terminating newline character of
// the last line.
function Range_i64
get_pos_range_from_line_range(App *app, Buffer_ID buffer, Range_i64 line_range){
    Range_i64 pos_range = {};
    if (is_valid_line_range(app, buffer, line_range)){
        pos_range.first = get_line_start_pos(app, buffer, line_range.first);
        pos_range.opl = get_line_end_pos(app, buffer, line_range.end);
    }
    return(pos_range);
}

function Range_i64
enclose_boundary(App *app, Buffer_ID buffer, Range_i64 range,
                 Boundary_Function *func){
    i64 new_min       = func(app, buffer, Side_Min, Scan_Backward, range.min + 1);
    i64 new_min_check = func(app, buffer, Side_Max, Scan_Backward, range.min + 1);
    if (new_min_check <= new_min && new_min < range.min){
        range.min = new_min;
    }
    i64 new_max       = func(app, buffer, Side_Max, Scan_Forward, range.max - 1);
    i64 new_max_check = func(app, buffer, Side_Min, Scan_Forward, range.max);
    if (new_max_check >= new_max && new_max > range.max){
        range.max = new_max;
    }
    return(range);
}

function Range_i64
left_enclose_boundary(App *app, Buffer_ID buffer, Range_i64 range,
                      Boundary_Function *func){
    i64 new_min       = func(app, buffer, Side_Min, Scan_Backward, range.min + 1);
    i64 new_min_check = func(app, buffer, Side_Max, Scan_Backward, range.min + 1);
    if (new_min_check <= new_min && new_min < range.min){
        range.min = new_min;
    }
    return(range);
}

function Range_i64
right_enclose_boundary(App *app, Buffer_ID buffer, Range_i64 range,
                       Boundary_Function *func){
    i64 new_max       = func(app, buffer, Side_Max, Scan_Forward, range.max - 1);
    i64 new_max_check = func(app, buffer, Side_Min, Scan_Forward, range.max - 1);
    if (new_max_check >= new_max && new_max > range.max){
        range.max = new_max;
    }
    return(range);
}

function Range_i64
enclose_non_whitespace(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_non_whitespace));
}
function Range_i64
enclose_pos_non_whitespace(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_non_whitespace));
}

function Range_i64
enclose_tokens(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_token));
}
function Range_i64
enclose_pos_tokens(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_token));
}

function Range_i64
enclose_base10(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_base10));
}
function Range_i64
enclose_pos_base10(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_base10));
}

function Range_i64
enclose_base16(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_base16));
}
function Range_i64
enclose_pos_base16(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_base16));
}

function Range_i64
enclose_base10_colon(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_base10_colon));
}
function Range_i64
enclose_pos_base10_colon(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_base10_colon));
}

function Range_i64
enclose_alnum(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_alnum));
}
function Range_i64
enclose_pos_alnum(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_alnum));
}

function Range_i64
enclose_alnum_unicode(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_alnum_unicode));
}
function Range_i64
enclose_pos_alnum_unicode(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_alnum_unicode));
}

function Range_i64
enclose_alnum_underscore(App *app, Buffer_ID buffer, Range_i64 range){
    return(enclose_boundary(app, buffer, range, boundary_alnum_underscore));
}
function Range_i64
enclose_pos_alnum_underscore(App *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_alnum_underscore));
}
function Range_i64
right_enclose_alnum_underscore(App *app, Buffer_ID buffer,
                               Range_i64 range){
 return(right_enclose_boundary(app, buffer, range, boundary_alnum_underscore));
}

function Range_i64
enclose_alnum_underscore_utf8(App *app, Buffer_ID buffer, Range_i64 range){
 return(enclose_boundary(app, buffer, range, boundary_alnum_underscore_utf8));
}
function Range_i64
enclose_pos_alnum_underscore_utf8(App *app, Buffer_ID buffer, i64 pos){
 return(enclose_boundary(app, buffer, Ii64(pos), boundary_alnum_underscore_utf8));
}
function Range_i64
right_enclose_alnum_underscore_utf8(App *app, Buffer_ID buffer,
                                    Range_i64 range){
 return(right_enclose_boundary(app, buffer, range, boundary_alnum_underscore_utf8));
}

function Range_i64
enclose_pos_inside_quotes(App *app, Buffer_ID buffer, i64 pos){
 return(enclose_boundary(app, buffer, Ii64(pos), boundary_inside_quotes));
}
function Range_i64
enclose_whole_lines(App *app, Buffer_ID buffer, Range_i64 range){
 return(enclose_boundary(app, buffer, range, boundary_line));
}
function Range_i64
enclose_pos_whole_lines(App *app, Buffer_ID buffer, i64 pos){
 return(enclose_boundary(app, buffer, Ii64(pos), boundary_line));
}
function void
view_set_cursor_pos(App *app, View_ID view, i64 pos){
 view_set_cursor(app, view, seek_pos(pos));
}

////////////////////////////////

function String
push_buffer_range(App *app, Arena *arena, Buffer_ID buffer, Range_i64 range){
 String result = {};
 i64 length = range_size(range);
 if(length > 0){
  Temp_Memory restore_point = begin_temp(arena);
  u8 *memory = push_array(arena, u8, length);
  if(buffer_read_range(app, buffer, range, memory)){
   result = SCu8(memory, length);
  }else{
   end_temp(restore_point);
  }
 }
 return(result);
}
function Range_i64
get_selected_range(App *app){
 GET_VIEW_AND_BUFFER;
 i64 curpos = view_get_cursor_pos(app, view);
 i64 markpos = view_get_mark_pos(app, view);
 Range_i64 result = Ii64(curpos, markpos);
 result.max += 1;
 return result;
}
function String8
get_selected_string(App *app, Arena *arena){
 GET_VIEW_AND_BUFFER;
 Range_i64 range = get_selected_range(app);
 return push_buffer_range(app, arena, buffer, range);
}
inline b32
token_equal_string(App *app, Buffer_ID buffer, Token *token, String string){
 Scratch_Block scratch(app);
 String8 token_string = push_token_lexeme(app, scratch, buffer, token);
 return string_match(token_string, string);
}
inline b32
token_equal_cstring(App *app, Buffer_ID buffer, Token *token, char *cstring){
 return token_equal_string(app, buffer, token, SCu8(cstring));
}

function String
push_buffer_line(App *app, Arena *arena, Buffer_ID buffer, i64 line_number)
{
 // NOTE(allen): 4coder flaw
 // The system for dealing with CRLF vs LF is too sloppy. There is no way to
 // avoid returning the CR from this function by adjusting the more
 // fundamental position getter functions without risking breaking some of
 // the users of those functions. It seems okay to just chop the CR
 // off here - but it's clearly sloppy. Oh well - we're in duct tape mode
 // these days anyways.
 String string = push_buffer_range(app, arena, buffer, get_line_pos_range(app, buffer, line_number));
 for (;string.size > 0 && string.str[string.size - 1] == '\r';) {
  string.size -= 1;
 }
 return(string);
}

function String
push_whole_buffer(App *app, Arena *arena, Buffer_ID buffer){
    return(push_buffer_range(app, arena, buffer, buffer_range(app, buffer)));
}

function String
push_view_range_string(App *app, Arena *arena, View_ID view){
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    return(push_buffer_range(app, arena, buffer, get_view_range(app, view)));
}

function String
push_view_range_string(App *app, Arena *arena){
    View_ID view = get_active_view(app, Access_Always);
    return(push_view_range_string(app, arena, view));
}

function String
push_enclose_range_at_pos(App *app, Arena *arena, Buffer_ID buffer, i64 pos, Enclose_Function *enclose){
    Range_i64 range = enclose(app, buffer, Ii64(pos));
    return(push_buffer_range(app, arena, buffer, range));
}

////////////////////////////////

function String
token_it_lexeme(App *app, Arena *arena, Token_Iterator_Array *it){
    String result = {};
    Token *token = tkarr_read(it);
    if (token != 0){
        result = push_token_lexeme(app, arena, (Buffer_ID)it->user_id, token);
    }
    return(result);
}

function b32
token_it_check_and_get_lexeme(App *app, Arena *arena, Token_Iterator_Array *it, Token_Base_Kind kind, String *lexeme_out){
    Token *token = tkarr_read(it);
    b32 result = {};
    if (token != 0 && token->kind == kind){
        result = true;
        *lexeme_out = push_token_lexeme(app, arena, (Buffer_ID)it->user_id, token);
    }
    return(result);
}

function String
token_it_lexeme(App *app, Arena *arena, Token_Iterator_List *it){
    String result = {};
    Token *token = token_it_read(it);
    if (token != 0){
        result = push_token_lexeme(app, arena, (Buffer_ID)it->user_id, token);
    }
    return(result);
}

function b32
token_it_check_and_get_lexeme(App *app, Arena *arena, Token_Iterator_List *it, Token_Base_Kind kind, String *lexeme_out){
    Token *token = token_it_read(it);
    b32 result = {};
    if (token != 0 && token->kind == kind){
        result = true;
        *lexeme_out = push_token_lexeme(app, arena, (Buffer_ID)it->user_id, token);
    }
    return(result);
}

////////////////////////////////

function b32
buffer_has_name_with_star(App *app, Buffer_ID buffer)
{
    Scratch_Block scratch(app);
    String str = push_buffer_unique_name(app, scratch, buffer);
    return(str.size > 0 && str.str[0] == '*');
}

function b32
buffer_is_skm(App *app, Buffer_ID buffer)
{
    Scratch_Block scratch(app);
    String str = push_buffer_unique_name(app, scratch, buffer);
    return( string_ends_with(str, str8lit(".skm")) );
}

function u8
buffer_get_char(App *app, Buffer_ID buffer_id, i64 pos)
{
    i64 buffer_size = buffer_get_size(app, buffer_id);
    u8 result = 0;
    if (0 <= pos && pos < buffer_size)
    {
        buffer_read_range(app, buffer_id, Ii64(pos, pos + 1), &result);
    }
    return(result);
}

function b32
line_is_valid_and_blank(App *app, Buffer_ID buffer, i64 line_number){
    b32 result = false;
    if (is_valid_line(app, buffer, line_number)){
        Scratch_Block scratch(app);
        String line = push_buffer_line(app, scratch, buffer, line_number);
        result = true;
        for (u64 i = 0; i < line.size; i += 1){
            if (!char_is_whitespace(line.str[i])){
                result = false;
                break;
            }
        }
    }
    return(result);
}

////////////////////////////////

function i64
get_pos_past_lead_whitespace_from_line_number(App *app, Buffer_ID buffer, i64 line_number){
    Scratch_Block scratch(app);
    Range_i64 line_range = get_line_pos_range(app, buffer, line_number);
    String line = push_buffer_range(app, scratch, buffer, line_range);
    i64 result = line_range.end;
    for (u64 i = 0; i < line.size; i += 1){
        if (!char_is_whitespace(line.str[i])){
            result = line_range.start + i;
            break;
        }
    }
    return(result);
}

function i64
get_pos_past_lead_whitespace(App *app, Buffer_ID buffer, i64 pos){
    i64 line_number = get_line_number_from_pos(app, buffer, pos);
    i64 result = get_pos_past_lead_whitespace_from_line_number(app, buffer, line_number);
    result = clamp_min(pos, result);
    return(result);
}

function void
move_past_lead_whitespace(App *app, View_ID view, Buffer_ID buffer){
    i64 pos = view_get_cursor_pos(app, view);
    i64 new_pos = get_pos_past_lead_whitespace(app, buffer, pos);
    view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
}

function void
move_past_lead_whitespace(App *app, View_ID view){
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    move_past_lead_whitespace(app, view, buffer);
}

function b32
line_is_blank(App *app, Buffer_ID buffer, i64 line_number){
    Scratch_Block scratch(app);
    String line = push_buffer_line(app, scratch, buffer, line_number);
    b32 is_blank = true;
    for (u64 i = 0; i < line.size; i += 1){
        if (!char_is_whitespace(line.str[i])){
            is_blank = false;
            break;
        }
    }
    return(is_blank);
}

function i64
get_line_number_of__whitespace_status_line(App *app, Buffer_ID buffer, Scan_Direction direction, i64 line_number_start, b32 get_blank_line){
    i64 line_count = buffer_get_line_count(app, buffer);
    i64 line_number = line_number_start + direction;
    for (;1 <= line_number && line_number <= line_count; line_number += direction){
        b32 is_blank = line_is_blank(app, buffer, line_number);
        if (is_blank == get_blank_line){
            break;
        }
    }
    line_number = clamp_between(1, line_number, line_count);
    return(line_number);
}

function i64
get_line_number_of_non_blank_line(App *app, Buffer_ID buffer, Scan_Direction direction, i64 line_number_start){
    return(get_line_number_of__whitespace_status_line(app, buffer, direction, line_number_start, false));
}

function i64
get_line_number_of_blank_line(App *app, Buffer_ID buffer, Scan_Direction direction, i64 line_number_start){
    return(get_line_number_of__whitespace_status_line(app, buffer, direction, line_number_start, true));
}

function i64
get_pos_of_blank_line(App *app, Buffer_ID buffer, Scan_Direction direction, i64 pos_start){
    i64 line_number_start = get_line_number_from_pos(app, buffer, pos_start);
    i64 blank_line = get_line_number_of_blank_line(app, buffer, direction, line_number_start);
    i64 pos = get_line_start_pos(app, buffer, blank_line);
    return(pos);
}

function i64
get_line_number_of_blank_line_grouped(App *app, Buffer_ID buffer, Scan_Direction direction, i64 line_number_start){
    i64 line_number = line_number_start;
    if (line_is_blank(app, buffer, line_number)){
        line_number = get_line_number_of_non_blank_line(app, buffer, direction, line_number);
    }
    line_number = get_line_number_of_blank_line(app, buffer, direction, line_number);
    return(line_number);
}

function i64
get_pos_of_blank_line_grouped(App *app, Buffer_ID buffer, Scan_Direction direction, i64 pos_start){
    i64 line_number_start = get_line_number_from_pos(app, buffer, pos_start);
    i64 blank_line = get_line_number_of_blank_line_grouped(app, buffer, direction, line_number_start);
    i64 pos = get_line_start_pos(app, buffer, blank_line);
    return(pos);
}

function Indent_Info
get_indent_info_range(App *app, Buffer_ID buffer, Range_i64 range, i32 tab_width)
{
 Scratch_Block scratch(app);
 String s = push_buffer_range(app, scratch, buffer, range);
 
 Indent_Info info = {};
 info.first_char_pos = range.end;
 info.is_blank = true;
 info.all_space = true;
 
 for (u64 i = 0; i < s.size; i += 1)
 {
  u8 c = s.str[i];
  if ( !char_is_whitespace(c) )
  {
   info.is_blank = false;
   info.all_space = false;
   info.first_char_pos = range.start + (i64)i;
   break;
  }
  if (c == ' ')
  {
   info.indent_pos += 1;
  }
  else
  {
   info.all_space = false;
  }
  if (c == '\t')
  {
   info.indent_pos += tab_width;
  }
 }
 
 return(info);
}

function Indent_Info
get_indent_info_line_number_and_start(App *app, Buffer_ID buffer, i64 line_number, i64 line_start, i32 tab_width)
{
 i64 end = get_line_side_pos(app, buffer, line_number, Side_Max);
 return(get_indent_info_range(app, buffer, Ii64(line_start, end), tab_width));
}

////////////////////////////////

function History_Group
history_group_begin(App *app, Buffer_ID buffer){
    History_Group group = {};
    group.app = app;
    group.buffer = buffer;
    group.first = buffer_history_get_current_state_index(app, buffer);
    group.first += 1;
    return(group);
}

function void
history_group_end(History_Group group){
    History_Record_Index last = buffer_history_get_current_state_index(group.app, group.buffer);
    if (group.first < last){
  buffer_history_merge_record_range(group.app, group.buffer, group.first, last, RecordMergeFlag_StateInRange_MoveStateForward);
 }
}

////////////////////////////////

function void
replace_in_range(App *app, Buffer_ID buffer, Range_i64 range, String needle, String string)
{
 if (needle.len > 0)
 {
  // TODO(allen): rewrite
  History_Group group = history_group_begin(app, buffer);
  i64 pos = range.min - 1;
  i64 new_pos = 0;
  seek_string_forward(app, buffer, pos, range.end, needle, &new_pos);
  i64 shift = replace_range_shift(needle.size, string.size);
  for (; new_pos + (i64)needle.size <= range.end;)
  {
   Range_i64 needle_range = Ii64(new_pos, new_pos + (i32)needle.size);
   buffer_replace_range(app, buffer, needle_range, string);
   range.end += shift;
   pos = new_pos + (i32)string.size - 1;
   seek_string_forward(app, buffer, pos, range.end, needle, &new_pos);
  }
  history_group_end(group);
 }
}

function Range_i64
swap_lines(App *app, Buffer_ID buffer, i64 line_1, i64 line_2){
    Range_i64 result = {};
    i64 line_count = buffer_get_line_count(app, buffer);
    if (1 <= line_1 && line_2 <= line_count){
        Range_i64 range_1 = get_line_pos_range(app, buffer, line_1);
        Range_i64 range_2 = get_line_pos_range(app, buffer, line_2);
        
        Scratch_Block scratch(app);
        
        String text_1 = push_buffer_range(app, scratch, buffer, range_1);
        String text_2 = push_buffer_range(app, scratch, buffer, range_2);
        
        History_Group group = history_group_begin(app, buffer);
        buffer_replace_range(app, buffer, range_2, text_1);
        buffer_replace_range(app, buffer, range_1, text_2);
        history_group_end(group);
        
        i64 shift = replace_range_shift(range_1, text_2.size);
        result.min = range_1.min;
        result.max = range_2.min + shift;
    }
    return(result);
}

function i64
move_line(App *app, Buffer_ID buffer, i64 line_number, Scan_Direction direction){
    i64 line_1 = 0;
    i64 line_2 = 0;
    if (direction == Scan_Forward){
        line_1 = line_number;
        line_2 = line_number + 1;
    }
    else{
        line_1 = line_number - 1;
        line_2 = line_number;
    }
    Range_i64 line_starts = swap_lines(app, buffer, line_1, line_2);
    i64 result = 0;
    if (line_starts.min < line_starts.max){
        if (direction == Scan_Forward){
            result = line_starts.max;
        }
        else{
            result = line_starts.min;
        }
    }
 else{
  result = get_line_side_pos(app, buffer, line_number, Side_Min);
 }
 return(result);
}

function void
clear_buffer(App *app, Buffer_ID buffer) {
 buffer_replace_range(app, buffer, buffer_range(app, buffer), strlit(""));
}

////////////////////////////////

function String_Match_List
find_all_matches_all_buffers(App *app, Arena *arena, String_Array match_patterns, String_Match_Flag must_have_flags, String_Match_Flag must_not_have_flags)
{
 String_Match_List all_matches = {};
 for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
      buffer != 0;
      buffer = get_buffer_next(app, buffer, Access_Always)){
  String_Match_List buffer_matches = {};
  for (i32 i = 0; i < match_patterns.count; i += 1){
   Range_i64 range = buffer_range(app, buffer);
   String_Match_List pattern_matches = buffer_find_all_matches(app, arena, buffer, i, range, match_patterns.vals[i],
                                                               &character_predicate_alnum_underscore_utf8, Scan_Forward, false);
   string_match_list_filter_flags(&pattern_matches, must_have_flags, must_not_have_flags);
   if (pattern_matches.count > 0)
   {
    if (buffer_matches.count == 0)
    {
     buffer_matches = pattern_matches;
    }
    else
    {
     buffer_matches = string_match_list_merge_front_to_back(&buffer_matches, &pattern_matches);
    }
   }
  }
  all_matches = string_match_list_join(&all_matches, &buffer_matches);
 }
 return(all_matches);
}

function String_Match_List
find_all_matches_all_buffers(App *app, Arena *arena, String pattern, String_Match_Flag must_have_flags, String_Match_Flag must_not_have_flags){
    String_Array array = {&pattern, 1};
    return(find_all_matches_all_buffers(app, arena, array, must_have_flags, must_not_have_flags));
}

////////////////////////////////

function b32
is_modified(User_Input *input){
    return(is_modified(&input->event));
}

function String
to_writable(User_Input *in){
    return(to_writable(&in->event));
}

function b32
input_has_modifier(User_Input *in, Key_Code key_code)
{
    b32 result = false;
    Input_Modifier_Set *mods = get_modifiers(&in->event);
    if (mods != 0)
    {
        result = set_has_modifier(mods, key_code);
    }
    return(result);
}

function b32
match_key_code(User_Input *in, Key_Code key_code){
    return(match_key_code(&in->event, key_code));
}

function b32
match_core_code(User_Input *in, Core_Code core_code){
    return(match_core_code(&in->event, core_code));
}

function String
backspace_utf8(String string){
    if (string.size > 0){
        u64 i = string.size - 1;
        for (; i > 0; --i){
            if (string.str[i] <= 0x7F || string.str[i] >= 0xC0){
                break;
            }
        }
        string.size = i;
    }
    return(string);
}

////////////////////////////////

Query_Bar_Group::Query_Bar_Group(App *app){
    this->app = app;
    this->view = get_active_view(app, Access_Always);
}

Query_Bar_Group::Query_Bar_Group(App *app, View_ID view){
    this->app = app;
    this->view = view;
}

Query_Bar_Group::~Query_Bar_Group(){
 clear_all_query_bars(this->app, this->view);
}

function b32
query_user_general(App *app, Query_Bar *bar, b32 force_number, String init_string)
{
    if (start_query_bar(app, bar, 0) == 0){
        return(false);
    }
    
    if (init_string.size > 0){
        String_u8 string = Su8(bar->string.str, bar->string.size, bar->string_capacity);
        string_concat(&string, init_string);
        bar->string.size = string.string.size;
    }
    
    b32 success = true;
    for (;;){
        User_Input in = get_next_input(app, EventPropertyGroup_Any,
                                       EventProperty_Escape|EventProperty_MouseButton);
        if (in.abort){
            success = false;
            break;
        }
        
        Scratch_Block scratch(app);
        b32 good_insert = false;
        String insert_string = to_writable(&in);
        if (insert_string.str != 0 && insert_string.size > 0){
            insert_string = string_replace(scratch, insert_string,
                                           strlit("\n"),
                                           strlit(""));
            insert_string = string_replace(scratch, insert_string,
                                           strlit("\t"),
                                           strlit(""));
            if (force_number){
                if (string_is_integer(insert_string, 10)){
                    good_insert = true;
                }
            }
            else{
                good_insert = true;
            }
        }
        
        if (in.event.kind == InputEventKind_KeyStroke &&
            (in.event.key.code == Key_Code_Return || in.event.key.code == Key_Code_Tab)){
            break;
        }
        else if (in.event.kind == InputEventKind_KeyStroke &&
                 in.event.key.code == Key_Code_Backspace){
            bar->string = backspace_utf8(bar->string);
        }
        else if (good_insert){
            String_u8 string = Su8(bar->string.str, bar->string.size, bar->string_capacity);
            string_concat(&string, insert_string);
            bar->string.size = string.string.size;
        }
        else{
            // NOTE(allen): is the user trying to execute another command?
            View_ID view = get_this_ctx_view(app, Access_Always);
            View_Context ctx = view_current_context(app, view);
            Mapping *mapping = ctx.mapping;
            Command_Map *map = mapping_get_map(mapping, ctx.map_id);
            Command_Binding binding = map_get_binding_recursive(mapping, map, &in.event);
            if (binding.custom != 0){
                Command_Metadata *metadata = get_command_metadata(binding.custom);
                if (metadata != 0){
                    if (metadata->is_ui){
                        view_enqueue_command_function(app, view, binding.custom);
                        break;
                    }
                }
                binding.custom(app);
            }
            else{
                leave_current_input_unhandled(app);
            }
        }
    }
    
    return(success);
}

function b32
query_user_string(App *app, Query_Bar *bar){
    return(query_user_general(app, bar, false, empty_string));
}

function b32
query_user_number(App *app, Query_Bar *bar){
    return(query_user_general(app, bar, true, empty_string));
}

function b32
query_user_number(App *app, Query_Bar *bar, i32 x){
    Scratch_Block scratch(app);
    String string = push_stringfz(scratch, "%d", x);
    return(query_user_general(app, bar, true, string));
}

////////////////////////////////

function Buffer_Identifier
buffer_identifier(char *str, i32 len){
    Buffer_Identifier identifier;
    identifier.name = str;
    identifier.name_len = len;
    identifier.id = 0;
    return(identifier);
}

function Buffer_Identifier
buffer_identifier(String str){
    return(buffer_identifier((char*)str.str, (i32)str.size));
}

function Buffer_Identifier
buffer_identifier(Buffer_ID id){
    Buffer_Identifier identifier;
    identifier.name = 0;
    identifier.name_len = 0;
    identifier.id = id;
    return(identifier);
}

function Buffer_ID
buffer_identifier_to_id(App *app, Buffer_Identifier identifier){
    Buffer_ID id = 0;
    if (identifier.id != 0){
        id = identifier.id;
    }
    else{
        String name = SCu8(identifier.name, identifier.name_len);
        id = get_buffer_by_name(app, name, Access_Always);
        if (id == 0){
            id = get_buffer_by_filename(app, name, Access_Always);
        }
    }
    return(id);
}

function Buffer_ID
buffer_identifier_to_id_create_out_buffer(App *app, Buffer_Identifier buffer_id){
    Buffer_ID result = 0;
    if (buffer_id.name != 0 && buffer_id.name_len > 0){
        String buffer_name = SCu8(buffer_id.name, buffer_id.name_len);
        Buffer_ID buffer_attach_id = get_buffer_by_name(app, buffer_name, Access_Always);
        if (buffer_attach_id != 0){
            result = buffer_attach_id;
        }
        else{
            buffer_attach_id = create_buffer(app, buffer_name, BufferCreate_AlwaysNew|BufferCreate_NeverAttachToFile);
            if (buffer_attach_id != 0){
                buffer_set_setting(app, buffer_attach_id, BufferSetting_ReadOnly, true);
                buffer_set_setting(app, buffer_attach_id, BufferSetting_Unimportant, true);
                result = buffer_attach_id;
            }
        }
    }
 else{
  result = buffer_id.id;
 }
 return(result);
}

////////////////////////////////

function void
place_begin_and_end_on_own_lines(App *app, char *begin, char *end){
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    Range_i64 range = get_view_range(app, view);
    Range_i64 lines = get_line_range_from_pos_range(app, buffer, range);
    range = get_pos_range_from_line_range(app, buffer, lines);
    
    Scratch_Block scratch(app);
    
    b32 min_line_blank = line_is_valid_and_blank(app, buffer, lines.min);
    b32 max_line_blank = line_is_valid_and_blank(app, buffer, lines.max);
    
    if ((lines.min < lines.max) || (!min_line_blank)){
        String begin_str = {};
        String end_str = {};
        
        i64 min_adjustment = 0;
        i64 max_adjustment = 0;
        
        if (min_line_blank){
            begin_str = push_stringfz(scratch, "\n%s", begin);
            min_adjustment += 1;
        }
        else{
            begin_str = push_stringfz(scratch, "%s\n", begin);
        }
        if (max_line_blank){
            end_str = push_stringfz(scratch, "%s\n", end);
        }
        else{
            end_str = push_stringfz(scratch, "\n%s", end);
            max_adjustment += 1;
        }
        
        max_adjustment += begin_str.size;
        Range_i64 new_pos = Ii64(range.min + min_adjustment, range.max + max_adjustment);
        
        History_Group group = history_group_begin(app, buffer);
        buffer_replace_range(app, buffer, Ii64(range.min), begin_str);
        buffer_replace_range(app, buffer, Ii64(range.max + begin_str.size), end_str);
        history_group_end(group);
        
        set_view_range(app, view, new_pos);
    }
    else{
        String str = push_stringfz(scratch, "%s\n\n%s", begin, end);
        buffer_replace_range(app, buffer, range, str);
        i64 center_pos = range.min + cstring_length(begin) + 1;
        view_set_cursor_and_preferred_x(app, view, seek_pos(center_pos));
        view_set_mark(app, view, seek_pos(center_pos));
    }
}

////////////////////////////////

function Face_ID
get_view_face_id(App *app, View_ID view){
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    return(get_face_id(app, buffer));
}

function Face_Metrics
get_view_face_metrics(App *app, View_ID view){
    Face_ID face = get_view_face_id(app, view);
    return(get_face_metrics(app, face));
}

function f32
get_view_line_height(App *app, View_ID view){
    Face_Metrics metrics = get_view_face_metrics(app, view);
    return(metrics.line_height);
}

function View_ID
get_first_view_with_buffer(App *app, Buffer_ID buffer_id)
{
    View_ID result = {};
    if (buffer_id != 0){
        for (View_ID test = get_view_next(app, 0, Access_Always);
             test != 0;
             test = get_view_next(app, test, Access_Always)){
            Buffer_ID test_buffer = view_get_buffer(app, test, Access_Always);
            if (test_buffer == buffer_id){
                result = test;
                break;
   }
  }
 }
 return(result);
}

function b32
open_editing_file(App *app, Buffer_ID *buffer_out, String8 filename, b32 background, b32 never_new)
{
 b32 result = false;
 Buffer_ID buffer = get_buffer_by_name(app, filename, Access_ReadVisible);
 b32 exists = buffer_exists(app, buffer);
 if (!exists)
 {
  Buffer_Create_Flag flags = 0;
  if (background) flags |= BufferCreate_Background;
  if (never_new)  flags |= BufferCreate_NeverNew;
  
  buffer = create_buffer(app, filename, flags);
  exists = buffer_exists(app, buffer);
 }
 if (exists)
 {
  result = true;
  if (buffer_out != 0)
   *buffer_out = buffer;
 }
 return(result);
}

function b32
view_open_file(App *app, View_ID view, String8 filename, b32 never_new) {
 b32 result = false;
 if (view != 0) {
  Buffer_ID buffer = 0;
  if (open_editing_file(app, &buffer, filename, false, never_new))
  {
   view_set_buffer(app, view, buffer, 0);
   result = true;
  }
 }
 return(result);
}

function void
view_disable_highlight_range(App *app, View_ID view){
    Managed_Scope scope = view_get_managed_scope(app, view);
    Managed_Object *highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
    if (*highlight != 0){
        managed_object_free(app, *highlight);
    }
    managed_scope_attachment_erase(app, scope, view_highlight_range);
    managed_scope_attachment_erase(app, scope, view_highlight_buffer);
}

function void
view_set_highlight_range(App *app, View_ID view, Range_i64 range){
    view_disable_highlight_range(app, view);
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Managed_Object *highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
    *highlight = alloc_buffer_markers_on_buffer(app, buffer, 2, &scope);
    Marker markers[2] = {};
    markers[0].pos = range.min;
    markers[1].pos = range.max;
    managed_object_store_data(app, *highlight, 0, 2, markers);
    Buffer_ID *highlight_buffer = scope_attachment(app, scope, view_highlight_buffer, Buffer_ID);
    *highlight_buffer = buffer;
}

function void
view_look_at_region(App *app, View_ID view, i64 major_pos, i64 minor_pos){
    Range_i64 range = Ii64(major_pos, minor_pos);
    
    Buffer_Cursor top = view_compute_cursor(app, view, seek_pos(range.min));
    if (top.line > 0){
        Buffer_Cursor bottom = view_compute_cursor(app, view, seek_pos(range.max));
        if (bottom.line > 0){
            Rect_f32 region = view_get_buffer_region(app, view);
            f32 view_height = rect_height(region);
            f32 skirt_height = view_height*.1f;
            Range_f32 acceptable_y = If32(skirt_height, view_height*.9f);
            
            f32 target_height = view_line_y_difference(app, view, bottom.line + 1, top.line);
            
            f32 line_height = get_view_line_height(app, view);
            if (target_height + 2*line_height > view_height){
                i64 major_line = bottom.line;
                if (range.min == major_pos){
                    major_line = top.line;
                }
                
                Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
                scroll.target.line_number = major_line;
                scroll.target.pixel_shift.y = -skirt_height;
                view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
            }
            else{
                Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
                Vec2_f32 top_p = view_relative_xy_of_pos(app, view, scroll.position.line_number, range.min);
                top_p -= scroll.position.pixel_shift;
                if (top_p.y < acceptable_y.min){
                    scroll.target.line_number = top.line;
                    scroll.target.pixel_shift.y = -skirt_height;
                    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
                }
                else{
                    Vec2_f32 bot_p = view_relative_xy_of_pos(app, view, scroll.position.line_number, range.max);
                    bot_p -= scroll.position.pixel_shift;
                    if (bot_p.y > acceptable_y.max){
                        scroll.target.line_number = bottom.line;
                        scroll.target.pixel_shift.y = skirt_height - view_height;
                        view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
                    }
                }
            }
        }
    }
}

function void
view_look_at_region(App *app, View_ID view, Range_i64 range){
    view_look_at_region(app, view, range.min, range.max);
}

////////////////////////////////

function Buffer_ID
get_buffer_next_looped(App *app, Buffer_ID buffer, Access_Flag access){
    buffer = get_buffer_next(app, buffer, access);
 if (buffer == 0){
  buffer = get_buffer_next(app, 0, access);
 }
 return(buffer);
}

////////////////////////////////

function b32
view_is_passive(App *app, View_ID view_id)
{
 Managed_Scope scope = view_get_managed_scope(app, view_id);
 b32 *is_passive = scope_attachment(app, scope, view_is_passive_loc, b32);
 b32 result = false;
 if (is_passive != 0){
  result = *is_passive;
 }
 return(result);
}

function View_ID
open_view(App *app, View_ID view_location, View_Split_Position position)
{
 View_ID result = 0;
 if ( view_exists(app, view_location) )
 {
  Panel_ID panel_id = view_get_panel(app, view_location);
  if (panel_id != 0)
  {
   Dimension split = ((position == ViewSplit_Left ||
                       position == ViewSplit_Right) ?
                      Dimension_X :
                      Dimension_Y);
   Side side = ((position == ViewSplit_Left || 
                 position == ViewSplit_Top) ?
                Side_Min :
                Side_Max);
   if ( panel_split(app, panel_id, split) )
   {
    Panel_ID new_panel_id = panel_get_child(app, panel_id, side);
    result = panel_get_view(app, new_panel_id, Access_Always);
   }
  }
 }
 return(result);
}

function View_ID
get_prev_view_looped_all_panels(App *app, View_ID view_id, Access_Flag access){
    view_id = get_view_prev(app, view_id, access);
    if (view_id == 0){
        view_id = get_view_prev(app, 0, access);
    }
    return(view_id);
}

////////////////////////////////

function Buffer_Kill_Result
try_buffer_kill(App *app, Buffer_ID buffer, View_ID gui_view_id, Buffer_Kill_Flag flags){
    Buffer_Kill_Result result = buffer_kill(app, buffer, flags);
    if (result == BufferKillResult_Dirty){
        if (do_buffer_kill_user_check(app, buffer, gui_view_id)){
            result = buffer_kill(app, buffer, BufferKill_AlwaysKill);
        }
    }
    return(result);
}

////////////////////////////////

function String
get_query_string(App *app, char *query_str, u8 *string_space, i32 space_size){
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    bar.prompt = SCu8((u8*)query_str);
    bar.string = SCu8(string_space, (u64)0);
    bar.string_capacity = space_size;
    if (!query_user_string(app, &bar)){
        bar.string.size = 0;
    }
    return(bar.string);
}

function Token*
get_token_from_pos(App *app, Token_Array *array, u64 pos){
    Token *result = 0;
    if (array->count > 0){
        i64 index = token_index_from_pos(array, pos);
        result = array->tokens + index;
    }
    return(result);
}

function Token*
get_token_from_pos(App *app, Buffer_ID buffer, u64 pos){
 Token_Array array = get_token_array_from_buffer(app, buffer);
 return(get_token_from_pos(app, &array, pos));
}

typedef b32 KV_Character_Predicate(char c);

function Range_i64
get_surrounding_characters(App *app, KV_Character_Predicate predicate)
{
 GET_VIEW_AND_BUFFER;
 i64 curpos = view_get_cursor_pos(app, view);
 
 i64 buffer_size = buffer_get_size(app, buffer);
 i64 min = curpos;
 i64 max = curpos;
 //
 for (i64 pos=curpos; pos < buffer_size; pos++)
 {
  u8 character = buffer_get_char(app, buffer, pos);
  if (predicate(character))
   max = pos+1;
  else
   break;
 }
 for (i64 pos=curpos; pos >= 0; pos--)
 {
  u8 character = buffer_get_char(app, buffer, pos);
  if (predicate(character))
   min = pos;
  else
   break;
 }
 
 if (max > min) 
  return Range_i64{min, max};
 else
  return {};
}

function String
push_token_or_word_under_pos(App *app, Arena *arena, Buffer_ID buffer, u64 pos)
{
 String result = {};
 Token *token = get_token_from_pos(app, buffer, pos);
 Range_i64 range;
 if (token != 0 && 
     token->size > 0 && 
     token->kind != TokenBaseKind_Whitespace &&
     token->kind != TokenBaseKind_Comment)
 {
  range = Ii64(token);
 }
 else
 {
  range = get_surrounding_characters(app, character_is_alnum);
 }
 
 result = push_buffer_range(app, arena, buffer, range);
 
 return(result);
}

function String8
push_token_or_word_under_active_cursor(App *app, Arena *arena)
{
 View_ID view = get_active_view(app, Access_Always);
 Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
 i64 pos = view_get_cursor_pos(app, view);
 String result = push_token_or_word_under_pos(app, arena, buffer, pos);
 return result;
}

////////////////////////////////

function b32
file_exists(Arena *scratch, String8 filename)
{
    File_Attributes attributes = system_quick_file_attributes(scratch, filename);
    return(attributes.last_write_time > 0);
}

function b32
file_exists_and_is_file(Arena *scratch, String8 filename)
{
    File_Attributes attributes = system_quick_file_attributes(scratch, filename);
    return(attributes.last_write_time > 0 && !HasFlag(attributes.flags, FileAttribute_IsDirectory));
}

function b32
file_exists_and_is_folder(Arena *scratch, String8 filename)
{
 File_Attributes attributes = system_quick_file_attributes(scratch, filename);
 return(attributes.last_write_time > 0 && HasFlag(attributes.flags, FileAttribute_IsDirectory));
}

function Stringz
search_up_path(Arena *arena, String start_path, String filename){
 Stringz result = {};
 String path = start_path;
 for (;path.size > 0;)
 {
  char last_char = string_get_character(path, path.size - 1);
  if (character_is_slash(last_char))
  {
   path = string_chop(path, 1);
  }
  Temp_Memory temp = begin_temp(arena);
  Stringz full_path = push_stringfz(arena, "%.*s/%.*s",
                                    string_expand(path),
                                    string_expand(filename));
  if (file_exists(arena, full_path)){
   result = full_path;
   break;
  }else{
   end_temp(temp);
   path = path_dirname(path);
  }
 }
 return(result);
}

function File_Name_Data
read_entire_file_search_up_path(Arena *arena, String path, String filename)
{
 File_Name_Data result = {};
 Stringz full_path = search_up_path(arena, path, filename);
 if (full_path.size > 0){
  String data = read_entire_file(arena, full_path);
  if(data.str){
   result.data = data;
   result.name = full_path;
  }
 }
 return(result);
}

function void
sort_pairs_by_key__quick(Sort_Pair_i32 *pairs, i32 first, i32 one_past_last){
    i32 dif = one_past_last - first;
    if (dif >= 2){
        i32 pivot = one_past_last - 1;
        Sort_Pair_i32 pivot_pair = pairs[pivot];
        i32 j = first;
        b32 interleave = false;
        for (i32 i = first; i < pivot; i += 1){
            Sort_Pair_i32 pair = pairs[i];
            if (pair.key < pivot_pair.key){
                pairs[i] = pairs[j];
                pairs[j] = pair;
                j += 1;
            }
            else if (pair.key == pivot_pair.key){
                if (interleave){
                    pairs[i] = pairs[j];
                    pairs[j] = pair;
                    j += 1;
                }
                interleave = !interleave;
            }
        }
        pairs[pivot] = pairs[j];
        pairs[j] = pivot_pair;
        sort_pairs_by_key__quick(pairs, first, j);
        sort_pairs_by_key__quick(pairs, j + 1, one_past_last);
    }
}

function void
sort_pairs_by_key(Sort_Pair_i32 *pairs, i32 count){
 sort_pairs_by_key__quick(pairs, 0, count);
}

function Range_i32_Array
get_ranges_of_duplicate_keys(Arena *arena, i32 *keys, i32 stride, i32 count)
{
    Range_i32_Array result = {};
    result.ranges = push_array(arena, Range_i32, count);
    u8 *ptr = (u8*)keys;
    i32 start_i = 0;
    for (i32 i = 1; i <= count; i += 1){
        b32 is_end = false;
        if (i == count){
            is_end = true;
        }
        else if (*(i32*)(ptr + i*stride) != *(i32*)(ptr + start_i*stride)){
            is_end = true;
  }
  if (is_end){
   Range_i32 *new_range = &result.ranges[result.count++];
   new_range->first = start_i;
   new_range->opl = i;
   start_i = i;
  }
 }
 pop_array(arena, Range_i32, count - result.count);
 return(result);
}

function void
no_mark_snap_to_cursor(App *app, Managed_Scope view_scope){
 b32 *snap_to_cursor = scope_attachment(app, view_scope, view_snap_mark_to_cursor, b32);
 *snap_to_cursor = false;
}

function void
no_mark_snap_to_cursor(App *app, View_ID view_id){
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    no_mark_snap_to_cursor(app, scope);
}

function void
no_mark_snap_to_cursor_if_shift(App *app, View_ID view_id)
{
    Scratch_Block scratch(app);
    Input_Modifier_Set mods = system_get_keyboard_modifiers(scratch);
    if (set_has_modifier(&mods, Key_Code_Shift))
    {
        no_mark_snap_to_cursor(app, view_id);
    }
}

function b32
view_has_highlighted_range(App *app, View_ID view){
    b32 result = false;
    if (fcoder_mode == FCoderMode_NotepadLike){
        i64 pos = view_get_cursor_pos(app, view);
        i64 mark = view_get_mark_pos(app, view);
        result = (pos != mark);
    }
    return(result);
}

function b32
if_view_has_highlighted_range_delete_range(App *app, View_ID view_id){
    b32 result = false;
    if (view_has_highlighted_range(app, view_id)){
        Range_i64 range = get_view_range(app, view_id);
        Buffer_ID buffer = view_get_buffer(app, view_id, Access_ReadWriteVisible);
        buffer_replace_range(app, buffer, range, strlit(""));
        result = true;
    }
    return(result);
}

function void
begin_notepad_mode(App *app){
    fcoder_mode = FCoderMode_NotepadLike;
    for (View_ID view = get_view_next(app, 0, Access_Always);
         view != 0;
         view = get_view_next(app, view, Access_Always)){
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
    }
}

////////////////////////////////

function void
seek_pos_of_textual_line(App *app, Side side){
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 new_pos = get_line_side_pos_from_pos(app, buffer, pos, side);
    view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
    no_mark_snap_to_cursor_if_shift(app, view);
}

function void
seek_pos_of_visual_line(App *app, Side side)
{
 View_ID view = get_active_view(app, Access_ReadVisible);
 i64 pos = view_get_cursor_pos(app, view);
 Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
 v2 p = view_relative_xy_of_pos(app, view, cursor.line, pos);
 p.x = (side == Side_Min)?(0.f):(max_f32);
 i64 new_pos = view_pos_at_relative_xy(app, view, cursor.line, p);
 view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
 no_mark_snap_to_cursor_if_shift(app, view);
}

CUSTOM_COMMAND_SIG(seek_beginning_of_textual_line)
CUSTOM_DOC("Seeks the cursor to the beginning of the line across all text.")
{
    seek_pos_of_textual_line(app, Side_Min);
}

CUSTOM_COMMAND_SIG(seek_end_of_textual_line)
CUSTOM_DOC("Seeks the cursor to the end of the line across all text.")
{
    seek_pos_of_textual_line(app, Side_Max);
}

function void 
seek_beginning_of_line(App *app)
{
    seek_pos_of_visual_line(app, Side_Min);
}

function void 
seek_end_of_line(App *app)
{
    seek_pos_of_visual_line(app, Side_Max);
}

CUSTOM_COMMAND_SIG(goto_beginning_of_file)
CUSTOM_DOC("Sets the cursor to the beginning of the file.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    view_set_cursor_and_preferred_x(app, view, seek_pos(0));
    no_mark_snap_to_cursor_if_shift(app, view);
}

CUSTOM_COMMAND_SIG(goto_end_of_file)
CUSTOM_DOC("Sets the cursor to the end of the file.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view, Access_ReadVisible);
    i32 size = (i32)buffer_get_size(app, buffer_id);
    view_set_cursor_and_preferred_x(app, view, seek_pos(size));
    no_mark_snap_to_cursor_if_shift(app, view);
}

////////////////////////////////

function b32
view_set_split(App *app, View_ID view, View_Split_Kind kind, f32 t){
    b32 result = false;
    if (view != 0){
        Panel_ID panel_id = view_get_panel(app, view);
        if (panel_id != 0){
            Panel_ID parent_panel_id = panel_get_parent(app, panel_id);
            if (parent_panel_id != 0){
                Panel_ID min_child_id = panel_get_child(app, parent_panel_id, Side_Min);
                if (min_child_id != 0){
                    b32 panel_is_min = (min_child_id == panel_id);
                    Panel_Split_Kind panel_kind = ((kind == ViewSplitKind_Ratio)?
                                                   (panel_is_min?PanelSplitKind_Ratio_Min:PanelSplitKind_Ratio_Max):
                                                   (panel_is_min?PanelSplitKind_FixedPixels_Min:PanelSplitKind_FixedPixels_Max));
                    result = panel_set_split(app, parent_panel_id, panel_kind, t);
                }
            }
        }
    }
    return(result);
}

function b32
view_set_split_proportion(App *app, View_ID view, f32 t){
 return(view_set_split(app, view, ViewSplitKind_Ratio, t));
}

function b32
view_set_split_pixel_size(App *app, View_ID view, i32 t){
 return(view_set_split(app, view, ViewSplitKind_FixedPixels, (f32)t));
}

////////////////////////////////

function Record_Info
get_single_record(App *app, Buffer_ID buffer_id, History_Record_Index index){
    Record_Info record = buffer_history_get_record_info(app, buffer_id, index);
    if (record.error == RecordError_NoError && record.kind == RecordKind_Group){
        record = buffer_history_get_group_sub_record(app, buffer_id, index, record.group_count - 1);
    }
    return(record);
}

////////////////////////////////

function Nest_Delimiter_Kind
get_nest_delimiter_kind(Token_Base_Kind kind, Find_Nest_Flag flags){
    Nest_Delimiter_Kind result = NestDelim_None;
    switch (kind){
        case TokenBaseKind_ScopeOpen:
        {
            if (HasFlag(flags, FindNest_Scope)){
                result = NestDelim_Open;
            }
        }break;
        case TokenBaseKind_ScopeClose:
        {
            if (HasFlag(flags, FindNest_Scope)){
                result = NestDelim_Close;
            }
        }break;
        case TokenBaseKind_ParenOpen:
        {
            if (HasFlag(flags, FindNest_Paren)){
                result = NestDelim_Open;
            }
        }break;
        case TokenBaseKind_ParenClose:
        {
            if (HasFlag(flags, FindNest_Paren)){
                result = NestDelim_Close;
            }
        }break;
    }
    return(result);
}

function b32
find_nest_side(App *app, Buffer_ID buffer, i64 pos,
               Find_Nest_Flag flags, Scan_Direction scan, Nest_Delimiter_Kind delim,
               Range_i64 *out){
    b32 result = false;
    
    b32 balanced = HasFlag(flags, FindNest_Balanced);
    if (balanced){
        if ((delim == NestDelim_Open && scan == Scan_Forward) ||
            (delim == NestDelim_Close && scan == Scan_Backward)){
            balanced = false;
        }
    }
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Token_Array *tokens = scope_attachment(app, scope, attachment_tokens, Token_Array);
    if (tokens != 0 && tokens->count > 0){
        Token_Iterator_Array it = tkarr_at_pos(0, tokens, pos);
        i32 level = 0;
        for (;;){
            Token *token = tkarr_read(&it);
            Nest_Delimiter_Kind token_delim = get_nest_delimiter_kind(token->kind, flags);
            
            if (level == 0 && token_delim == delim){
                *out = Ii64_size(token->pos, token->size);
                result = true;
                break;
            }
            
            if (balanced && token_delim != NestDelim_None){
                level += (token_delim == delim)?-1:1;
            }
            
            b32 good = false;
            if (scan == Scan_Forward){
                good = tkarr_inc(&it) != 0;
            }
            else{
                good = tkarr_dec(&it) != 0;
            }
            if (!good){
                break;
            }
        }
    }
    
    return(result);
}

function b32
find_nest_side(App *app, Buffer_ID buffer, i64 pos,
               Find_Nest_Flag flags, Scan_Direction scan, Nest_Delimiter_Kind delim,
               i64 *out){
    Range_i64 range = {};
    b32 result = find_nest_side(app, buffer, pos, flags, scan, delim, &range);
    if (result){
        if (HasFlag(flags, FindNest_EndOfToken)){
            *out = range.end;
        }
        else{
            *out = range.start;
        }
    }
    return(result);
}

function b32
find_surrounding_nest(App *app, Buffer_ID buffer, i64 pos,
                      Find_Nest_Flag flags, Range_i64 *out){
    b32 result = false;
    Range_i64 range = {};
    if (find_nest_side(app, buffer, pos - 1, flags|FindNest_Balanced,
                       Scan_Backward, NestDelim_Open, &range.start) &&
        find_nest_side(app, buffer, pos, flags|FindNest_Balanced|FindNest_EndOfToken,
                       Scan_Forward, NestDelim_Close, &range.end)){
        *out = range;
        result = true;
    }
    return(result);
}

function void
select_scope(App *app, View_ID view, Range_i64 range){
    view_set_cursor_and_preferred_x(app, view, seek_pos(range.first));
    view_set_mark(app, view, seek_pos(range.end));
    view_look_at_region(app, view, range.first, range.end);
    no_mark_snap_to_cursor(app, view);
}

////////////////////////////////

function Line_Ending_Kind
guess_line_ending_kind_from_buffer(App *app, Buffer_ID buffer){
    u64 size = buffer_get_size(app, buffer);
    size = clamp_max(size, KB(8));
    Scratch_Block scratch(app);
    String string = push_buffer_range(app, scratch, buffer, Ii64(0, size));
    return(string_guess_line_ending_kind(string));
}

////////////////////////////////

// TODO(allen): REWRITE THIS EXACTLY HOW YOU WANT IT --- start ---

function Child_Process_Set_Target_Flags
flags_system_command(Command_Line_Interface_Flag flags){
    Child_Process_Set_Target_Flags set_buffer_flags = 0;
    if (!HasFlag(flags, CLI_OverlapWithConflict)){
        set_buffer_flags |= ChildProcessSet_FailIfBufferAlreadyAttachedToAProcess;
    }
    if (HasFlag(flags, CLI_CursorAtEnd)){
        set_buffer_flags |= ChildProcessSet_CursorAtEnd;
    }
    return(set_buffer_flags);
}

function b32
set_buffer_system_command(App *app, Child_Process_ID process, Buffer_ID buffer, Command_Line_Interface_Flag flags){
    b32 result = false;
    Child_Process_Set_Target_Flags set_buffer_flags = flags_system_command(flags);
    if (child_process_set_target_buffer(app, process, buffer, set_buffer_flags)){
        clear_buffer(app, buffer);
        if (HasFlag(flags, CLI_SendEndSignal)){
            buffer_send_end_signal(app, buffer);
            
            Buffer_Hook_Function *begin_buffer = (Buffer_Hook_Function*)get_custom_hook(app, HookID_BeginBuffer);
            if (begin_buffer != 0){
                begin_buffer(app, buffer);
            }
        }
  result = true;
 }
 return(result);
}

function b32
exec_system_command(App *app, View_ID view, Buffer_Identifier buffer_id,
                    String path, String command, Command_Line_Interface_Flag flags)
{
 b32 result = false;
 Child_Process_ID child_process_id = create_child_process(app, path, command);
 if (child_process_id != 0)
 {
  result = true;
  Buffer_ID buffer_attach_id = buffer_identifier_to_id_create_out_buffer(app, buffer_id);
  if (buffer_attach_id != 0)
  {
   if ( set_buffer_system_command(app, child_process_id, buffer_attach_id, flags) )
   {
    if (view != 0)
    {
     view_set_buffer(app, view, buffer_attach_id, 0);
     view_set_cursor(app, view, seek_pos(0));
    }
   }
  }
 }
 return(result);
}

// TODO(allen): --- end ---

////////////////////////////////

// NOTE(allen): Layout Invalidate

function void
clear_all_layouts(App *app){
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_Always)){
        buffer_clear_layout_cache(app, buffer);
    }
}

inline i64 get_pos_column(App *app, Buffer_ID buffer, i64 pos)
{
  i64 line = get_line_number_from_pos(app, buffer, pos);
  i64 column = pos - get_line_start_pos(app, buffer, line) + 1;
  return column;
}

inline i64 get_current_column(App *app)
{
 GET_VIEW_AND_BUFFER;
 i64 column = get_pos_column(app, buffer, view_get_cursor_pos(app, view));
 return column;
}

inline i64 get_current_char(App *app)
{
  GET_VIEW_AND_BUFFER;
  i64 pos = view_get_cursor_pos(app, view);
  u8 character = buffer_get_char(app, buffer, pos);
  return character;
}

inline Rect_f32 
get_cursor_rect(App *app, Text_Layout_ID text_layout_id)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    Rect_f32 result = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
    return result;
}

function b32
view_is_active(App *app, View_ID view)
{
    View_ID active_view = get_active_view(app, Access_Always);
    return (active_view == view);
}

function b32
kv_find_nest_side_paren(App *app, Token_Array *tokens, i64 pos,
                        Scan_Direction scan, Nest_Delimiter_Kind delim,
                        i64 *out)
{
  if (!(tokens && tokens->count)) return false;

  Range_i64 range = {};
  b32 nest_found = false;
  {// b32 result = find_nest_side(app, buffer, pos, flags, scan, delim, &range);
    Token_Iterator_Array it = tkarr_at_pos(0, tokens, pos);
    i32 level = 0;
    for (;;)
    {
      Token *token = tkarr_read(&it);
      Nest_Delimiter_Kind token_delim_kind = NestDelim_None;
      //
      if (token->kind == TokenBaseKind_ParenOpen)
      {
        token_delim_kind = NestDelim_Open;
      }
      else if (token->kind == TokenBaseKind_ParenClose)
      {
        token_delim_kind = NestDelim_Close;
      }
            
      if (level == 0 && token_delim_kind == delim)
      {
        range = Ii64_size(token->pos, token->size);
        nest_found = true;
        break;
      }
      else
      {
        if (token_delim_kind != NestDelim_None)
        {
          if (token_delim_kind == delim) { level--; }
          else                           { level++; }
        }
            
        if (scan == Scan_Forward)
        {
          if (!tkarr_inc(&it))
          {
            break;
          }
        }
        else
        {
          if (!tkarr_dec(&it))
          {
            break;
          }
        }
      }
    }
  }
  if (nest_found)
  {
    if (delim == NestDelim_Close) { *out = range.end;   }
    else                          { *out = range.start; }
 }
 return(nest_found);
}

inline void animate_next_frame(App *app)
{
 animate_in_n_milliseconds(app, 0);
}

function void
app_printer_function(void *userdata, char *format, va_list args)
{
 auto app = cast(App *)userdata;
 Scratch_Block scratch(app);
 String message = push_stringfv(scratch, format, args, false);
 print_message(app, message);
}

inline Printer
make_printer_app(App *app)
{
 Printer result = {
  .type           = Printer_Type_Generic,
  .userdata       = app,
  .print_function = app_printer_function,
 };
 return result;
}

//-