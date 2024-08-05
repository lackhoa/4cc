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

//~ EOF
