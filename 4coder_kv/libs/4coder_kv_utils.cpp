#pragma once

#include "kv.h"
#include "4coder_default_include.cpp"

#define GET_VIEW_AND_BUFFER \
  View_ID   view = get_active_view(app, Access_ReadVisible); \
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible)

inline void kv_print_message(Application_Links *app, char *message) 
{
  print_message(app, SCu8(message));
}

inline void
kv_buffer_replace_range(Application_Links *app, Buffer_ID buffer, i64 min, i64 max, String_Const_u8 replacement)
{
  buffer_replace_range(app, buffer, Ii64(min, max), replacement);
}

inline void kv_buffer_delete_range(Application_Links *app, Buffer_ID buffer, i64 min, i64 max)
{
  buffer_replace_range(app, buffer, Ii64(min, max), string_u8_empty);
}

inline void kv_buffer_delete_pos(Application_Links *app, Buffer_ID buffer, i64 min)
{
  buffer_replace_range(app, buffer, Ii64(min, min+1), string_u8_empty);
}

inline String_Const_u8
push_buffer_dir_name(Application_Links *app, Arena *arena, Buffer_ID buffer)
{
  String_Const_u8 filename = push_buffer_file_name(app, arena, buffer);
  return string_remove_last_folder(filename);
}

internal Token *
kv_token_at_cursor(Application_Links *app, i64 delta=0)
{
  GET_VIEW_AND_BUFFER;
  Token_Array tokens = get_token_array_from_buffer(app, buffer);
  i64 pos = view_correct_cursor(app, view) + delta;
  return token_from_pos(&tokens, pos);
}

internal Token_Iterator_Array
kv_token_it_at_cursor(Application_Links *app, i64 delta=0)
{
  GET_VIEW_AND_BUFFER;
  Token_Array tokens = get_token_array_from_buffer(app, buffer);
  i64 pos = view_correct_cursor(app, view) + delta;
  return token_iterator_pos(0, &tokens, pos);
}

inline void kv_goto_pos(Application_Links *app, View_ID view, i64 pos)
{
  view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

inline void kv_goto_token(Application_Links *app, Token *token)
{
  View_ID view = get_active_view(app, Access_ReadVisible);
  view_set_cursor_and_preferred_x(app, view, seek_pos(token->pos));
}

inline Range_i64 token_range(Token *token)
{
  if (token)
    return Range_i64{token->pos, token->pos + token->size};
  else
    return Range_i64{};
}

inline i64 get_pos_column(Application_Links *app, Buffer_ID buffer, i64 pos)
{
  i64 line = get_line_number_from_pos(app, buffer, pos);
  i64 column = pos - get_line_start_pos(app, buffer, line) + 1;
  return column;
}

inline i64 get_current_column(Application_Links *app)
{
  GET_VIEW_AND_BUFFER;
  i64 column = get_pos_column(app, buffer, view_get_cursor_pos(app, view));
  return column;
}


inline i64 get_current_line(Application_Links *app)
{
  GET_VIEW_AND_BUFFER;
  i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
  return line;
}

inline i64 get_current_char(Application_Links *app)
{
  GET_VIEW_AND_BUFFER;
  i64 pos = view_get_cursor_pos(app, view);
  u8 character = buffer_get_char(app, buffer, pos);
  return character;
}

inline Rect_f32 
get_cursor_rect(Application_Links *app, Text_Layout_ID text_layout_id)
{
  GET_VIEW_AND_BUFFER;
  i64 cursor_pos = view_get_cursor_pos(app, view);
  Rect_f32 result = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
  return result;
}