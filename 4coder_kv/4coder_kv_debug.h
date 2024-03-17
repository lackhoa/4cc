#pragma once

struct Debug_Entry
{
 char *scope;
 char *name;
 v4    value;
 argb  color;
};

#define DEBUG_send_entry_return void
#define DEBUG_send_entry_params Debug_Entry entry
ED_API_FUNCTION(DEBUG_send_entry);

force_inline void
DEBUG_VALUE_inner(char *scope, char *name, rect2 value)
{
 Debug_Entry entry = {.scope=scope, .name=name, .value=value.v4_value};
 DEBUG_send_entry(entry);
}

force_inline void
DEBUG_VALUE_inner(char *scope, char *name, i32 value, argb color=0)
{
 Debug_Entry entry = {.scope=scope, .name=name, .value=v4{.x=(f32)value}, .color=color};
 DEBUG_send_entry(entry);
}

force_inline void
DEBUG_VALUE_inner(char *scope, char *name, v1 value)
{
 Debug_Entry entry = {.scope=scope, .name=name, .value=v4{.x=value}};
 DEBUG_send_entry(entry);
}

force_inline void
DEBUG_VALUE_inner(char *scope, char *name, v2 value)
{
 Debug_Entry entry = {.scope=scope, .name=name, .value=v4{.x=value.x, .y=value.y}};
 DEBUG_send_entry(entry);
}

force_inline void
DEBUG_VALUE_inner(char *scope, char *name, v3 v, argb color=0)
{
 v4 value = cast_vec4(v);
 Debug_Entry entry = {.scope=scope, .name=name, .value=value, .color=color};
 DEBUG_send_entry(entry);
}

force_inline void
DEBUG_VALUE_inner(char *scope, char *name, v4 v, argb color=0)
{
 Debug_Entry entry = {.scope=scope, .name=name, .value=v, .color=color};
 DEBUG_send_entry(entry);
}

//~ EOF
