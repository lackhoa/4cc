#define fui_is_active_return b32
#define fui_is_active_params void
//
#define fui_push_active_slider_value_return String
#define fui_push_active_slider_value_params Arena *arena
//
#define fui_at_slider_p_return i64
#define fui_at_slider_p_params App *app, Buffer_ID buffer, Token_Iterator_Array *it_out
//
#define fui_handle_slider_return b32
#define fui_handle_slider_params App *app, Buffer_ID buffer, String filename, i1 line_number
