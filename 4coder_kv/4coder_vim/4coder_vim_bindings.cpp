#pragma once

#include "4coder_vim.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-null-pointer-arithmetic"
#pragma clang diagnostic ignored "-Wnull-pointer-subtraction"

// TODO(BYP): Might want meta-data on these, for now I prefer the simplicity
function b32 vim_map_set_binding(u32 mode, u32 sub_mode, void *func, u64 key)
{
    b32 no_conflict = true;
    if ((mode & bitmask_3) == 0)
    {
        // set everything
        mode |= bitmask_3;
        foreach(s,VIM_SUBMODE_COUNT)
        {
            no_conflict = vim_map_set_binding(mode, s, func, key) && no_conflict;
        }
    }
    else 
    {
        for (i1 mode_index=0; mode_index < 3; mode_index++) {
            if (mode & (1 << mode_index)) {
                Table_u64_u64 *table = vim_maps + mode_index + sub_mode*VIM_MODE_COUNT;
                no_conflict = !table_erase(table, key);
                table_insert(table, key, PtrAsInt(func));
            }
        }
    }
    return no_conflict;
}

#pragma clang diagnostic pop

inline b32 VimBind(u32 mode, Custom_Command_Function *custom, Vim_Sub_Mode sub_mode, u64 key)
{
    return vim_map_set_binding(mode, sub_mode, (void *)custom, key);
}
inline b32 VimBind(u32 mode, Custom_Command_Function *custom, u64 key)
{
    return vim_map_set_binding(mode, SUB_None, (void *)custom, key);
}
