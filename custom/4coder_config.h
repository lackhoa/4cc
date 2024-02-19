/*
4coder_config.h - Configuration structs.
*/

// TOP

#if !defined(FCODER_CONFIG_H)
#define FCODER_CONFIG_H

////////////////////////////////
// NOTE(allen): Config Parser Types

struct Error_Location
{
    i32 line_number;
    i32 column_number;
};

struct Config_Error
{
    Config_Error *next;
    Config_Error *prev;
    String8 filename;
    u8 *pos;
    String8 text;
};

struct Config_Error_List
{
    Config_Error *first;
    Config_Error *last;
    i32 count;
};

struct Config_Parser
{
    Token *token;
    Token *opl;
    
    String8 filename;
    String8 data;
    
    Arena *arena;
    
    Config_Error_List errors;
};

struct Config_LValue
{
    String8 identifier;
    i32 index;
};

typedef i32 Config_RValue_Type;
enum
{
    Config_RValue_Type_Null,
    
    Config_RValue_Type_LValue,
    Config_RValue_Type_Boolean,
    Config_RValue_Type_Integer,
    Config_RValue_Type_String,
    Config_RValue_Type_Compound,
    
    Config_RValue_Type_COUNT,
};

struct Config_Compound
{
    struct Config_Compound_Element *first;
    struct Config_Compound_Element *last;
    i32 count;
};

struct Config_RValue
{
    Config_RValue_Type type;
    union
    {
        Config_LValue *lvalue;
        b32 boolean;
        i32 integer;
        u32 uinteger;
        String8 string;
        char character;
        Config_Compound *compound;
    };
};

struct Config_Integer
{
    b32 is_signed;
    union
    {
        i32 integer;
        u32 uinteger;
    };
};

typedef i32 Config_Layout_Type;
enum
{
    ConfigLayoutType_Unset,
    ConfigLayoutType_Identifier,
    ConfigLayoutType_Integer,
    ConfigLayoutType_COUNT,
};
struct Config_Layout
{
    Config_Layout_Type type;
    u8 *pos;
    union
    {
        String8 identifier;
        i32 integer;
    };
};

struct Config_Compound_Element
{
    Config_Compound_Element *next;
    Config_Compound_Element *prev;
    
    Config_Layout l;
    Config_RValue *r;
};

struct Config_Assignment
{
    Config_Assignment *next;
    Config_Assignment *prev;
    
    u8 *pos;
    Config_LValue *l;
    Config_RValue *r;
    
    b32 visited;
};

struct Config
{
    i32 *version;
    Config_Assignment *first;
    Config_Assignment *last;
    i32 count;
    
    Config_Error_List errors;
    
    String8 filename;
    String8 data;
};

////////////////////////////////
// NOTE(allen): Config Iteration

typedef i32 Iteration_Step_Result;
enum
{
    Iteration_Good = 0,
    Iteration_Skip = 1,
    Iteration_Quit = 2,
};

struct Config_Get_Result
{
    b32 success;
    Config_RValue_Type type;
    u8 *pos;
    union
    {
        b32 boolean;
        i32 integer;
        u32 uinteger;
        String_Const_u8 string;
        char character;
        Config_Compound *compound;
    };
};

struct Config_Iteration_Step_Result
{
    Iteration_Step_Result step;
    Config_Get_Result get;
};

struct Config_Get_Result_Node
{
    Config_Get_Result_Node *next;
    Config_Get_Result_Node *prev;
    Config_Get_Result result;
};

struct Config_Get_Result_List
{
    Config_Get_Result_Node *first;
    Config_Get_Result_Node *last;
    i32 count;
};

////////////////////////////////
// NOTE(allen): Config Search List

function void def_search_normal_load_list(Arena *arena, String8List *list);

function String8 def_search_normal_full_path(Arena *arena, String8 relative);
function FILE *def_search_normal_fopen(Arena *arena, char *file_name, char *opt);

////////////////////////////////
// NOTE(allen): Config Parser Functions

function Config_Parser def_config_parser_init(Arena *arena, String8 file_name, String8 data, Token_Array array);

function void def_config_parser_inc(Config_Parser *ctx);
function u8*  def_config_parser_get_pos(Config_Parser *ctx);

function b32 def_config_parser_recognize_base_kind(Config_Parser *ctx, Token_Base_Kind kind);
function b32 def_config_parser_recognize_cpp_kind(Config_Parser *ctx, Token_Cpp_Kind kind);
function b32 def_config_parser_recognize_boolean(Config_Parser *ctx);
function b32 def_config_parser_recognize_text(Config_Parser *ctx, String_Const_u8 text);

function b32 def_config_parser_match_cpp_kind(Config_Parser *ctx, Token_Cpp_Kind kind);
function b32 def_config_parser_match_text(Config_Parser *ctx, String_Const_u8 text);

function String_Const_u8 def_config_parser_get_lexeme(Config_Parser *ctx);
function Config_Integer  def_config_parser_get_int(Config_Parser *ctx);
function b32             def_config_parser_get_boolean(Config_Parser *ctx);

function void def_config_parser_recover(Config_Parser *ctx);

function Config*                  def_config_parser_top       (Config_Parser *ctx);
function i32*                     def_config_parser_version   (Config_Parser *ctx);
function Config_Assignment*       def_config_parser_assignment(Config_Parser *ctx);
function Config_LValue*           def_config_parser_lvalue    (Config_Parser *ctx);
function Config_RValue*           def_config_parser_rvalue    (Config_Parser *ctx);
function Config_Compound*         def_config_parser_compound  (Config_Parser *ctx);
function Config_Compound_Element* def_config_parser_element   (Config_Parser *ctx);


////////////////////////////////
// NOTE(allen): Dump Config to Variables

function Variable_Handle def_fill_var_from_config(App *app, Variable_Handle parent, String_ID key, Config *config);

////////////////////////////////
// NOTE(allen): Config Variables Read

function Variable_Handle def_get_config_var(String_ID key);
function void            def_set_config_var(String_ID key, String_ID val);

function b32  def_get_config_b32(String_ID key);
function void def_set_config_b32(String_ID key, b32 val);

function String8 def_get_config_string(Arena *arena, String_ID key);
function void    def_set_config_string(String_ID key, String8 val);

function u64  def_get_config_u64(App *app, String_ID key);
function void def_set_config_u64(App *app, String_ID key, u64 val);

#endif

// BOTTOM