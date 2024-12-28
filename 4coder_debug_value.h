#pragma once

//TODO: clean this garbage pile up, please!
function void
DEBUG_VALUE_inner(char *scope, char *name, rect2 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name =name;
 entry.value=value.v4_value;
 entry.color=color;
 DEBUG_send_entry(entry);
}
function void
DEBUG_VALUE_inner(char *scope, char *name, i1 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name =name;
 entry.value.x=(f32)value;
 entry.color=color;
 DEBUG_send_entry(entry);
}
function void
DEBUG_VALUE_inner(char *scope, char *name, u32 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.x=(f32)value;
 entry.color=color;
 DEBUG_send_entry(entry);
}
function void
DEBUG_VALUE_inner(char *scope, char *name, i2 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.xy=V2(value);
 entry.color=color;
 DEBUG_send_entry(entry);
}
function void
DEBUG_VALUE_inner(char *scope, char *name, v1 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.x=value;
 entry.color=color;
 DEBUG_send_entry(entry);
}
function void
DEBUG_VALUE_inner(char *scope, char *name, v2 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.xy=value;
 entry.color=color;
 DEBUG_send_entry(entry);
}
function void
DEBUG_VALUE_inner(char *scope, char *name, v3 v, argb color=0)
{
 v4 value = cast_V4(v);
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value=value;
 entry.color=color;
 DEBUG_send_entry(entry);
}
function void
DEBUG_VALUE_inner(char *scope, char *name, v4 v, argb color=0){
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value=v;
 entry.color=color;
 DEBUG_send_entry(entry);
}
//-