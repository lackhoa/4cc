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
//