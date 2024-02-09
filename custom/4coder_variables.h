/*
4coder_variables.h - Variables system
*/

// TOP

#if !defined(FCODER_VARIABLES_H)
#define FCODER_VARIABLES_H

////////////////////////////////
// NOTE(allen): Types

typedef u64 String_ID;

struct Variable
{
    Variable *parent;
    Variable *next;
    String_ID key;
    String_ID val;
    Variable *first;
    Variable *last;
};

struct Variable_Handle
{
    Variable *ptr;
};

////////////////////////////////
// NOTE(allen): Functions

function String_ID    vars_save_string(String8 string);
#define var_save_strlit(S) vars_save_string( string_u8_litexpr(S) )
//
#define vars_save_string_lit var_save_strlit
//
function String8         vars_read_string(Arena *arena, String_ID id);

function Variable_Handle vars_get_root(void);
function Variable_Handle vars_get_nil(void);
function b32             vars_is_nil(Variable_Handle var);
function b32             vars_is_nil(Variable *var);
function b32             vars_match(Variable_Handle a, Variable_Handle b);

function Variable_Handle vars_first_child(Variable_Handle var);
function Variable_Handle vars_next_sibling(Variable_Handle var);
function Variable_Handle vars_parent(Variable_Handle var);

#define Vars_Children(it, par) Variable_Handle it = vars_first_child(par); !vars_is_nil(it); it = vars_next_sibling(it)

function Variable_Handle vars_read_key(Variable_Handle var, String_ID key);
function String_ID       vars_key_id_from_var(Variable_Handle var);
function String8         vars_key_from_var(Arena *arena, Variable_Handle var);

function String_ID       vars_string_id_from_var(Variable_Handle var);
function String8         vars_string_from_var(Arena *arena, Variable_Handle var);
function b32             vars_b32_from_var(Variable_Handle var);
function u64             vars_u64_from_var(Application_Links *app, Variable_Handle var);

function void            vars_set(Variable_Handle var, String_ID string);
function void            vars_erase(Variable_Handle var, String_ID key);
function Variable_Handle vars_new_variable(Variable_Handle var, String_ID key);
function Variable_Handle vars_new_variable(Variable_Handle var, String_ID key, String_ID string);
function void            vars_clear_keys(Variable_Handle var);

function void            vars_print_indented(Application_Links *app, Variable_Handle var, i32 indent);
function void            vars_print(Application_Links *app, Variable_Handle var);

#endif //4CODER_VARIABLES_H

// BOTTOM
