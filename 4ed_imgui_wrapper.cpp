//
// IMPORTANT(kv) This is an experiment to wrap imgui on the editor side.
// but turned out it may just be stupid and we don't need to do shutdown at all?

api(ed) function b32
im_begin(char* name, b32 *p_open, ImGuiWindowFlags flags)
{
 return ImGui::Begin(name, (bool *)p_open, flags);
}
api(ed) function void
im_end()
{
 ImGui::End();
}

api(ed) function void
im_textv(char *fmt, va_list args)
{
 ImGui::TextV(fmt, args);
}
api(ed) function void
im_image(ImTextureID user_texture_id, v2 image_size, v2 uv0, v2 uv1, v4 tint_col, v4 border_col)
{
 ImGui::Image(user_texture_id, image_size, uv0, uv1, tint_col, border_col);
}
/*api(ed) function v2
im_get_cursor_screen_pos()
{
}
api(ed) function ImDrawList *
get_window_draw_list()
{
 
}
api(ed) function ImGuiCol
get_color_u32()
{
}*/
//