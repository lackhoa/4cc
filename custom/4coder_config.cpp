/*
4coder_config.cpp - Parsing *.4coder files.
*/

// TOP

////////////////////////////////
// NOTE(allen): Config Search List

function void
def_search_normal_load_list(Arena *arena, String8List *list){
    Variable_Handle prj_var = vars_read_key(vars_get_root(), vars_intern_lit("prj_config"));
    String prj_dir = prj_path_from_project(arena, prj_var);
    if (prj_dir.size > 0){
        string_list_push(arena, list, prj_dir);
    }
    def_search_list_add_system_path(arena, list, SystemPath_UserDirectory);
    def_search_list_add_system_path(arena, list, SystemPath_BinaryDirectory);
}

function String8
def_search_normal_full_path(Arena *arena, String8 relative){
    String8List list = {};
    def_search_normal_load_list(arena, &list);
    String8 result = def_search_get_full_path(arena, &list, relative);
    return(result);
}

function FILE*
def_search_normal_fopen(Arena *arena, char *filename, char *opt){
    Temp_Memory_Block block(arena);
    String8List list = {};
    def_search_normal_load_list(arena, &list);
 FILE *file = def_search_fopen(arena, &list, filename, opt);
 return(file);
}

////////////////////////////////
// NOTE(allen): Extension List

function String_Array
parse_extension_line_to_extension_list(App *app, Arena *arena, String8 str)
{
 ProfileScope(app, "parse extension line to extension list");
 i1 count = 0;
 for (u64 i = 0; i < str.size; i += 1){
  if (str.str[i] == '.'){
   count += 1;
  }
 }
 
 String_Array array = {};
 array.count = count;
 array.strings = push_array(arena, String, count);
 
 str = string_skip(str, string_find_first(str, '.') + 1);
 for (i1 i = 0; i < count; i += 1) {
  u64 next_period = string_find_first(str, '.');
  String extension = string_prefix(str, next_period);
  str = string_skip(str, next_period + 1);
  array.strings[i] = push_string_copyz(arena, extension);
 }
 
 return(array);
}

////////////////////////////////
// NOTE(allen): Token Array

function Token_Array
token_array_from_text(App *app, Arena *arena, String8 data)
{
    ProfileScope(app, "token array from text");
    Token_List list = lex_full_input_cpp(arena, data);
    return (token_array_from_list(arena, &list));
}

////////////////////////////////
// NOTE(allen): Errors

function Error_Location
get_error_location(App *app, u8 *base, u8 *pos)
{
    ProfileScope(app, "get error location");
    Error_Location location = {};
    location.line_number = 1;
    location.column_number = 1;
    for (u8 *ptr = base;
         ptr < pos;
         ptr += 1){
        if (*ptr == '\n'){
            location.line_number += 1;
            location.column_number = 1;
        }
        else{
            location.column_number += 1;
        }
    }
    return(location);
}

function String8
config_stringize_errors(App *app, Arena *arena, Config *parsed)
{
    ProfileScope(app, "stringize errors");
    String8 result = {};
    if (parsed->errors.first != 0)
    {
        List_String list = {};
        for (Config_Error *error = parsed->errors.first;
             error != 0;
             error = error->next)
        {
            Error_Location location = get_error_location(app, parsed->data.str, error->pos);
            string_list_pushf(arena, &list, "%.*s:%d:%d: %.*s\n",
                              string_expand(error->filename), location.line_number, location.column_number, string_expand(error->text));
  }
  result = string_list_flatten(arena, list);
 }
 return(result);
}

////////////////////////////////
// NOTE(allen): Parser

function void
config_parser__skip_whitespace(Config_Parser *p)
{
 while ((p->token->kind != TokenBaseKind_EOF) &&
        (p->token != p->opl) &&
        (p->token->kind == TokenBaseKind_Comment ||
         p->token->kind == TokenBaseKind_Whitespace))
 {
  p->token++;
 }
 
 if (p->token == p->opl)
 {
  Token *eof = push_struct(p->arena, Token);
  *eof = {
   .kind    =TokenBaseKind_EOF,
   .sub_kind=TokenCppKind_EOF,
  };
  p->token = eof;
 }
}

function void
config_parser_inc(Config_Parser *p)
{
 if (p->token->kind != TokenBaseKind_EOF)
 {
  p->token += 1;
  config_parser__skip_whitespace(p);
 }
}

function Config_Parser
config_parser_init(Arena *arena, String8 filename, String8 data, Token_Array array)
{
 Config_Parser p = {};
 p.token    = &array.tokens[0];
 p.opl      = array.tokens + array.count;
 p.filename = filename;
 p.data     = data;
 p.arena    = arena;
 config_parser__skip_whitespace(&p);
 return(p);
}

function u8*
config_parser_get_pos(Config_Parser *p)
{
    return(p->data.str + p->token->pos);
}

// @Cleanup @Remove
function b32
config_parser_recognize_cpp_kind(Config_Parser *p, Token_Cpp_Kind kind)
{
    b32 result = (p->token->sub_kind == kind);
    return(result);
}

function b32
config_parser_recognize_boolean(Config_Parser *p)
{
    Token *token = p->token;
    return (token->sub_kind == TokenCppKind_LiteralTrue ||
            token->sub_kind == TokenCppKind_LiteralFalse);
}

function String8
config_parser_get_lexeme(Config_Parser *p)
{
    String8 lexeme = {};
    Token *token = p->token;
    if (token < p->opl)
    {
        lexeme = SCu8(p->data.str + token->pos, token->size);
    }
    return(lexeme);
}

function b32
config_parser_recognize_text(Config_Parser *p, String8 text)
{
    String8 lexeme = config_parser_get_lexeme(p);
    return(lexeme.str != 0 && string_match(lexeme, text));
}

function b32
config_parser_eat_cpp_kind(Config_Parser *p, Token_Cpp_Kind kind)
{
    b32 result = (p->token->sub_kind == kind);
    if (result)
    {
        config_parser_inc(p);
    }
    return(result);
}

function b32
config_parser_eat_text(Config_Parser *p, String8 text)
{
    b32 result = config_parser_recognize_text(p, text);
    if (result)
    {
        config_parser_inc(p);
    }
    return(result);
}

function Config_Integer
config_parser_get_int(Config_Parser *p)
{
    Config_Integer config_integer = {};
    String8 str = config_parser_get_lexeme(p);
    if ( string_match(string_prefix(str, 2), string_u8_litexpr("0x")) )
    {
        config_integer.is_signed = false;
        config_integer.uinteger = (u32)(string_to_u64(string_skip(str, 2), 16));
    }
    else
    {
        b32 is_negative = (string_get_character(str, 0) == '-');
        if (is_negative)
        {
            str = string_skip(str, 1);
        }
        config_integer.is_signed = true;
        config_integer.integer = (i1)(string_to_u64(str, 10));
        if (is_negative)
        {
            config_integer.integer *= -1;
        }
    }
    return(config_integer);
}

function b32
config_parser_get_boolean(Config_Parser *p)
{
    String str = config_parser_get_lexeme(p);
    return(string_match(str, string_u8_litexpr("true")));
}

function Config_Error*
config_push_error(Arena *arena, Config_Error_List *list, String8 filename, u8 *pos, char *error_text)
{
    Config_Error *error = push_array(arena, Config_Error, 1);
    zdll_push_back(list->first, list->last, error);
    list->count += 1;
    error->filename = filename;
    error->pos = pos;
    error->text = push_string_copyz(arena, SCu8(error_text));
    return(error);
}

function Config_Error*
config_push_error(Arena *arena, Config *config, u8 *pos, char *error_text)
{
    return(config_push_error(arena, &config->errors, config->filename, pos, error_text));
}

function void
config_parser_push_error(Config_Parser *p, u8 *pos, char *error_text)
{
    config_push_error(p->arena, &p->errors, p->filename, pos, error_text);
}

function void
config_parser_push_error_here(Config_Parser *p, char *error_text)
{
    config_parser_push_error(p, config_parser_get_pos(p), error_text);
}

function void
config_parser_recover(Config_Parser *p)
{
    for (;;)
    {
        if (p->token->sub_kind == TokenCppKind_Semicolon ||
            p->token->sub_kind == TokenCppKind_EOF)
        {
            break;
        }
        config_parser_inc(p);
    }
}

function i1 *
config_parser_version(Config_Parser *p)
{
    macro_require(config_parser_eat_text(p, str8_lit("version")));
    
    if (!config_parser_eat_cpp_kind(p, TokenCppKind_ParenOp))
    {
        config_parser_push_error_here(p, "expected token '(' for version specifier: 'version(#)'");
        config_parser_recover(p);
        return(0);
    }
    
    if (p->token->kind != TokenBaseKind_LiteralInteger)
    {
        config_parser_push_error_here(p, "expected an integer constant for version specifier: 'version(#)'");
        config_parser_recover(p);
        return(0);
    }
    
    Config_Integer value = config_parser_get_int(p);
    config_parser_inc(p);
    
    if (!config_parser_eat_cpp_kind(p, TokenCppKind_ParenCl)){
        config_parser_push_error_here(p, "expected token ')' for version specifier: 'version(#)'");
        config_parser_recover(p);
        return(0);
    }
    
    if (!config_parser_eat_cpp_kind(p, TokenCppKind_Semicolon)){
        config_parser_push_error_here(p, "expected token ';' for version specifier: 'version(#)'");
        config_parser_recover(p);
        return(0);
    }
    
    i1 *ptr = push_array(p->arena, i1, 1);
    *ptr = value.integer;
    return(ptr);
}

function Config_LValue*
config_parser_lvalue(Config_Parser *p)
{
    macro_require(config_parser_recognize_cpp_kind(p, TokenCppKind_Identifier));
    String8 identifier = config_parser_get_lexeme(p);
    config_parser_inc(p);
    
    i1 index = 0;
    if (config_parser_eat_cpp_kind(p, TokenCppKind_BrackOp)){
        macro_require(p->token->kind == TokenBaseKind_LiteralInteger);
        Config_Integer value = config_parser_get_int(p);
        index = value.integer;
        config_parser_inc(p);
        macro_require(config_parser_eat_cpp_kind(p, TokenCppKind_BrackCl));
    }
    
    Config_LValue *lvalue = push_array_zero(p->arena, Config_LValue, 1);
    lvalue->identifier = identifier;
    lvalue->index = index;
    return(lvalue);
}

internal Config_RValue * config_parser_rvalue(Config_Parser *p);

internal Config_Compound_Element *
config_parser_element(Config_Parser *p)
{
    Config_Layout layout = {};
    layout.pos = config_parser_get_pos(p);
    if ( config_parser_eat_cpp_kind(p, TokenCppKind_Dot) )
    {
        if ( config_parser_recognize_cpp_kind(p, TokenCppKind_Identifier) )
        {
            layout.type = ConfigLayoutType_Identifier;
            layout.identifier = config_parser_get_lexeme(p);
            config_parser_inc(p);
        }
        else if ( p->token->kind == TokenBaseKind_LiteralInteger )
        {
            layout.type = ConfigLayoutType_Integer;
            Config_Integer value = config_parser_get_int(p);
            layout.integer = value.integer;
            config_parser_inc(p);
        }
        else
        {
            return(0);
        }
        macro_require(config_parser_eat_cpp_kind(p, TokenCppKind_Eq));
    }
    Config_RValue *rvalue = config_parser_rvalue(p);
    macro_require(rvalue != 0);
    Config_Compound_Element *element = push_array(p->arena, Config_Compound_Element, 1);
    block_zero_struct(element);
    element->l = layout;
    element->r = rvalue;
    return(element);
}

internal void
config_parser__compound__check(Config_Parser *p, Config_Compound *compound)
{
    b32 implicit_index_allowed = true;
    for (Config_Compound_Element *node = compound->first;
         node != 0;
         node = node->next)
    {
        if (node->l.type != ConfigLayoutType_Unset)
        {
            implicit_index_allowed = false;
        }
        else if (!implicit_index_allowed)
        {
            config_parser_push_error(p, node->l.pos, "encountered unlabeled member after one or more labeled members");
        }
    }
}

internal Config_Compound*
config_parser_compound(Config_Parser *p)
{
    Config_Compound_Element *first = 0;
    Config_Compound_Element *last = 0;
    i1 count = 0;
    
    Config_Compound_Element *element = config_parser_element(p);
    macro_require(element != 0);
    zdll_push_back(first, last, element);
    count += 1;
    
    while ( config_parser_eat_cpp_kind(p, TokenCppKind_Comma) )
    {
        if (config_parser_recognize_cpp_kind(p, TokenCppKind_BraceCl))
        {
            break;
        }
        element = config_parser_element(p);
        macro_require(element != 0);
        zdll_push_back(first, last, element);
        count += 1;
    }
    
    macro_require(config_parser_eat_cpp_kind(p, TokenCppKind_BraceCl));
    
    Config_Compound *compound = push_array(p->arena, Config_Compound, 1);
    block_zero_struct(compound);
    compound->first = first;
    compound->last = last;
    compound->count = count;
    config_parser__compound__check(p, compound);
    return(compound);
}

internal Config_RValue *
config_parser_rvalue(Config_Parser *p)
{
    Config_RValue *rvalue = 0;
    if ( config_parser_recognize_cpp_kind(p, TokenCppKind_Identifier) )
    {
        Config_LValue *l = config_parser_lvalue(p);
        macro_require(l != 0);
        rvalue = push_array_zero(p->arena, Config_RValue, 1);
        rvalue->type = Config_RValue_Type_LValue;
        rvalue->lvalue = l;
    }
    else if (config_parser_recognize_cpp_kind(p, TokenCppKind_BraceOp))
    {
        config_parser_inc(p);
        Config_Compound *compound = config_parser_compound(p);
        macro_require(compound != 0);
        rvalue = push_array_zero(p->arena, Config_RValue, 1);
        rvalue->type = Config_RValue_Type_Compound;
        rvalue->compound = compound;
    }
    else if (config_parser_recognize_boolean(p))
    {
        b32 b = config_parser_get_boolean(p);
        config_parser_inc(p);
        rvalue = push_array_zero(p->arena, Config_RValue, 1);
        rvalue->type = Config_RValue_Type_Boolean;
        rvalue->boolean = b;
    }
    else if ( p->token->kind == TokenBaseKind_LiteralInteger )
    {
        Config_Integer value = config_parser_get_int(p);
        config_parser_inc(p);
        rvalue = push_array_zero(p->arena, Config_RValue, 1);
        rvalue->type = Config_RValue_Type_Integer;
        if (value.is_signed)
        {
            rvalue->integer = value.integer;
        }
        else
        {
            rvalue->uinteger = value.uinteger;
        }
    }
    else if (config_parser_recognize_cpp_kind(p, TokenCppKind_LiteralString))
    {
        String8 s = config_parser_get_lexeme(p);
        config_parser_inc(p);
        s = string_chop(string_skip(s, 1), 1);
        String8 interpreted = string_interpret_escapes(p->arena, s);
        rvalue = push_array_zero(p->arena, Config_RValue, 1);
        rvalue->type = Config_RValue_Type_String;
        rvalue->string = interpreted;
    }
    return(rvalue);
}

function Config_Assignment *
config_parser_assignment(Config_Parser *p)
{
    u8 *pos = config_parser_get_pos(p);
    
    Config_LValue *l = config_parser_lvalue(p);
    if (l == 0)
    {
        config_parser_push_error_here(p, "expected an l-value; l-value formats: 'identifier', 'identifier[#]'");
        config_parser_recover(p);
        return(0);
    }
    
    if (!config_parser_eat_cpp_kind(p, TokenCppKind_Eq))
    {
        config_parser_push_error_here(p, "expected token '=' for assignment: 'l-value = r-value;'");
        config_parser_recover(p);
        return(0);
    }
    
    Config_RValue *r = config_parser_rvalue(p);
    if (r == 0){
        config_parser_push_error_here(p, "expected an r-value; r-value formats:\n"
                                          "\tconstants (true, false, integers, hexadecimal integers, strings, characters)\n"
                                          "\tany l-value that is set in the file\n"
                                          "\tcompound: '{ compound-element, compound-element, compound-element ...}'\n"
                                          "\ta compound-element is an r-value, and can have a layout specifier\n"
                                          "\tcompound-element with layout specifier: .name = r-value, .integer = r-value");
        config_parser_recover(p);
        return(0);
    }
    
    if (!config_parser_eat_cpp_kind(p, TokenCppKind_Semicolon))
    {
        config_parser_push_error_here(p, "expected token ';' for assignment: 'l-value = r-value;'");
        config_parser_recover(p);
        return(0);
    }
    
    Config_Assignment *assignment = push_array_zero(p->arena, Config_Assignment, 1);
    assignment->pos = pos;
    assignment->l = l;
    assignment->r = r;
    return(assignment);
}

function Config *
config_parser_top(Config_Parser *p)
{
    i1 *version = config_parser_version(p);
    
    Config_Assignment *first = 0;
    Config_Assignment *last = 0;
    i1 count = 0;
    for (; !config_parser_recognize_cpp_kind(p, TokenCppKind_EOF);)
    {
        Config_Assignment *assignment = config_parser_assignment(p);
        if (assignment != 0)
        {
            zdll_push_back(first, last, assignment);
            count += 1;
        }
    }
    
    Config *config = push_array(p->arena, Config, 1);
    block_zero_struct(config);
    config->version = version;
    config->first = first;
    config->last = last;
    config->count = count;
    config->errors = p->errors;
    config->filename = p->filename;
    config->data = p->data;
    return(config);
}

internal Config*
config_parse(App *app, Arena *arena, String8 filename, String8 data, Token_Array array)
{
    ProfileScope(app, "config parse");
    Temp_Memory restore_point = begin_temp(arena);
    Config_Parser p = config_parser_init(arena, filename, data, array);
    Config *config = config_parser_top(&p);
    if (config == 0)
    {
        end_temp(restore_point);
    }
    return(config);
}

internal Config*
config_from_text(App *app, Arena *arena, String8 filename, String8 data)
{
    Config *parsed = 0;
    Temp_Memory restore_point = begin_temp(arena);
    Token_Array array = token_array_from_text(app, arena, data);
    if (array.tokens != 0)
    {
        parsed = config_parse(app, arena, filename, data, array);
        if (parsed == 0)
        {
            end_temp(restore_point);
        }
    }
    return(parsed);
}

////////////////////////////////
// NOTE(allen): Dump Config to Variables

function Config_Get_Result
config_var(Config *config, String var_name, i1 subscript);

function void
def_var_dump_rvalue(App *app, Config *config, Variable_Handle dst, String_ID l_value, Config_RValue *r)
{
    Scratch_Block scratch(app);
    
    b32 *boolean = 0;
    i1 *integer = 0;
    String *string = 0;
    Config_Compound *compound = 0;
    
    Config_Get_Result get_result = {};
    
    switch (r->type)
    {
        case Config_RValue_Type_LValue:
        {
            Config_LValue *l = r->lvalue;
            if (l != 0)
            {
                get_result = config_var(config, l->identifier, l->index);
                if (get_result.success)
                {
                    switch (get_result.type)
                    {
                        case Config_RValue_Type_Boolean:
                        {
                            boolean = &get_result.boolean;
                        }break;
                        
                        case Config_RValue_Type_Integer:
                        {
                            integer = &get_result.integer;
                        }break;
                        
                        case Config_RValue_Type_String:
                        {
                            string = &get_result.string;
                        }break;
                        
                        case Config_RValue_Type_Compound:
                        {
                            compound = get_result.compound;
                        }break;
                    }
                }
            }
        }break;
        
        case Config_RValue_Type_Boolean:
        {
            boolean = &r->boolean;
        }break;
        
        case Config_RValue_Type_Integer:
        {
            integer = &r->integer;
        }break;
        
        case Config_RValue_Type_String:
        {
            string = &r->string;
        }break;
        
        case Config_RValue_Type_Compound:
        {
            compound = r->compound;
        }break;
    }
    
    if (boolean != 0)
    {
        String_ID val = 0;
        if (*boolean) val = vars_intern_lit("true");
        else          val = vars_intern_lit("false");
        vars_new_variable(dst, l_value, val);
    }
    else if (integer != 0)
    {
        // TODO(allen): signed/unsigned problem
        String_ID val = vars_intern(to_string(scratch, *integer));
        vars_new_variable(dst, l_value, val);
    }
    else if (string != 0)
    {
        String_ID val = vars_intern(*string);
        vars_new_variable(dst, l_value, val);
    }
    else if (compound != 0)
    {
        Variable_Handle sub_var = vars_new_variable(dst, l_value);
        
        i1 implicit_index   = 0;
        b32 implicit_allowed = true;
        Config_Compound_Element *node = compound->first;
        for (; 
             node != 0;
             node = node->next, implicit_index += 1)
        {
            String_ID sub_l_value = 0;
            
            switch (node->l.type)
            {
                case ConfigLayoutType_Unset:
                {
                    if (implicit_allowed)
                    {
                        sub_l_value = vars_intern(to_string(scratch, implicit_index));
                    }
                }break;
                
                case ConfigLayoutType_Identifier:
                {
                    implicit_allowed = false;
                    sub_l_value = vars_intern(node->l.identifier);
                }break;
                
                case ConfigLayoutType_Integer:
                {
                    implicit_allowed = false;
                    sub_l_value = vars_intern(to_string(scratch, node->l.integer));
                }break;
            }
            
            if ((sub_l_value != 0) &&
                (node->r != 0))
            {
                def_var_dump_rvalue(app, config, sub_var, sub_l_value, node->r);
            }
        }
    }
}

function Variable_Handle
def_fill_var_from_config(App *app, Variable_Handle parent, String_ID key, Config *config)
{
    Variable_Handle result = vars_get_nil();
    
    if (key != 0)
    {
        String_ID filename_id = vars_intern(config->filename);
        result = vars_new_variable(parent, key, filename_id);
        
        Variable_Handle var = result;
        
        Scratch_Block scratch(app);
        
        if (config->version != 0)
        {
            String_ID version_key = vars_intern_lit("version");
            String_ID version_value = vars_intern(push_stringfz(scratch, "%d", *config->version));
            vars_new_variable(parent, version_key, version_value);
        }
        
        for (Config_Assignment *node = config->first;
             node != 0;
             node = node->next){
            String_ID l_value = 0;
            Config_LValue *l = node->l;
            if (l != 0){
                String string = l->identifier;
                if (l->index != 0){
                    string = push_stringfz(scratch, "%.*s.%d", string_expand(string), l->index);
                }
                l_value = vars_intern(string);
            }
            
            if (l_value != 0){
                Config_RValue *r = node->r;
                if (r != 0){
                    def_var_dump_rvalue(app, config, var, l_value, r);
                }
            }
        }
    }
    
    return(result);
}


////////////////////////////////
// NOTE(allen): Config Variables Read

global const u64 def_config_lookup_count = 4;
global String_ID def_config_lookup_table[def_config_lookup_count] = {};

function void
_def_config_table_init(void){
    if (def_config_lookup_table[0] == 0){
        def_config_lookup_table[0] = vars_intern(str8_lit("ses_config"));
        def_config_lookup_table[1] = vars_intern(str8_lit("prj_config"));
        def_config_lookup_table[2] = vars_intern(str8_lit("usr_config"));
        def_config_lookup_table[3] = vars_intern(str8_lit("def_config"));
    }
}

function Variable_Handle
def_get_config_var(String_ID key)
{
    _def_config_table_init();
    
    Variable_Handle result = vars_get_nil();
    Variable_Handle root = vars_get_root();
    for (u64 i = 0; i < def_config_lookup_count; i += 1){
        String_ID block_key = def_config_lookup_table[i];
        Variable_Handle block_var = vars_read_key(root, block_key);
        Variable_Handle var = vars_read_key(block_var, key);
        if (!vars_is_nil(var)){
            result = var;
            break;
        }
    }
    
    return(result);
}

function void
def_set_config_var(String_ID key, String_ID val)
{
    _def_config_table_init();
    Variable_Handle root = vars_get_root();
    Variable_Handle block_var = vars_read_key(root, def_config_lookup_table[0]);
	if (vars_is_nil(block_var)){
		block_var = vars_new_variable(root, def_config_lookup_table[0]);
	}
    vars_new_variable(block_var, key, val);
}

function b32
def_get_config_b32(String_ID key)
{
    Variable_Handle var = def_get_config_var(key);
    String_ID val = vars_string_id_from_var(var);
    b32 result = (val != 0 && val != vars_intern_lit("false"));
    return(result);
}

function void
def_set_config_b32(String_ID key, b32 val){
    String_ID val_id = val?vars_intern_lit("true"):vars_intern_lit("false");
    def_set_config_var(key, val_id);
}

function String8
def_get_config_string(Arena *arena, String_ID key){
    Variable_Handle var = def_get_config_var(key);
    String result = vars_string_from_var(arena, var);
    return(result);
}

inline String8 
def_get_config_string(Arena *arena, char *key)
{
    // NOTE(kv): a bit slow but who cares
    return def_get_config_string(arena, vars_intern(SCu8(key)));
}

function void
def_set_config_string(String_ID key, String val)
{
    def_set_config_var(key, vars_intern(val));
}

function u64
def_get_config_u64(App *app, String_ID key)
{
    Scratch_Block scratch(app);
    Variable_Handle var = def_get_config_var(key);
    u64 result = vars_u64_from_var(app, var);
    return(result);
}

function void
def_set_config_u64(App *app, String_ID key, u64 val){
    Scratch_Block scratch(app);
    String val_string = push_stringfz(scratch, "%llu", val);
    def_set_config_var(key, vars_intern(val_string));
}


////////////////////////////////
// NOTE(allen): Eval

function Config_Assignment*
config_lookup_assignment(Config *config, String var_name, i1 subscript){
    Config_Assignment *assignment = 0;
    for (assignment = config->first;
         assignment != 0;
         assignment = assignment->next){
        Config_LValue *l = assignment->l;
        if (l != 0 && string_match(l->identifier, var_name) && l->index == subscript){
            break;
        }
    }
    return(assignment);
}

function Config_Get_Result
config_evaluate_rvalue(Config *config, Config_Assignment *assignment, Config_RValue *r){
    Config_Get_Result result = {};
    if (r != 0 && !assignment->visited){
        if (r->type == Config_RValue_Type_LValue){
            assignment->visited = true;
            Config_LValue *l = r->lvalue;
            result = config_var(config, l->identifier, l->index);
            assignment->visited = false;
        }
        else{
            result.success = true;
            result.pos = assignment->pos;
            result.type = r->type;
            switch (r->type){
                case Config_RValue_Type_Boolean:
                {
                    result.boolean = r->boolean;
                }break;
                
                case Config_RValue_Type_Integer:
                {
                    result.integer = r->integer;
                }break;
                
                case Config_RValue_Type_String:
                {
                    result.string = r->string;
                }break;
                
                case Config_RValue_Type_Compound:
                {
                    result.compound = r->compound;
                }break;
            }
        }
    }
    return(result);
}

function Config_Get_Result
config_var(Config *config, String8 var_name, i1 subscript)
{
    Config_Get_Result result = {};
    Config_Assignment *assignment = config_lookup_assignment(config, var_name, subscript);
    if (assignment != 0)
    {
        result = config_evaluate_rvalue(config, assignment, assignment->r);
    }
    return(result);
}


////////////////////////////////
// NOTE(allen): Nonsense from the old system

function Config_Get_Result
config_compound_member(Config *config, Config_Compound *compound, String var_name, i1 index){
    Config_Get_Result result = {};
    i1 implicit_index = 0;
    b32 implicit_index_is_valid = true;
    for (Config_Compound_Element *element = compound->first;
         element != 0;
         element = element->next, implicit_index += 1){
        b32 element_matches_query = false;
        switch (element->l.type){
            case ConfigLayoutType_Unset:
            {
                if (implicit_index_is_valid && index == implicit_index){
                    element_matches_query = true;
                }
            }break;
            
            case ConfigLayoutType_Identifier:
            {
                implicit_index_is_valid = false;
                if (string_match(element->l.identifier, var_name)){
                    element_matches_query = true;
                }
            }break;
            
            case ConfigLayoutType_Integer:
            {
                implicit_index_is_valid = false;
                if (element->l.integer == index){
                    element_matches_query = true;
                }
            }break;
        }
        if (element_matches_query){
            Config_Assignment dummy_assignment = {};
            dummy_assignment.pos = element->l.pos;
            result = config_evaluate_rvalue(config, &dummy_assignment, element->r);
            break;
        }
    }
    return(result);
}

function Config_Iteration_Step_Result
typed_array_iteration_step(Config *parsed, Config_Compound *compound, Config_RValue_Type type, i1 index);

function i1
typed_array_get_count(Config *parsed, Config_Compound *compound, Config_RValue_Type type);

function Config_Get_Result_List
typed_array_reference_list(Arena *arena, Config *parsed, Config_Compound *compound, Config_RValue_Type type);

#define config_fixed_string_var(c,v,s,o,a) config_placed_string_var((c),(v),(s),(o),(a),sizeof(a))

////////////////////////////////

function b32
config_bool_var(Config *config, String var_name, i1 subscript, b32* var_out){
    Config_Get_Result result = config_var(config, var_name, subscript);
    b32 success = (result.success && result.type == Config_RValue_Type_Boolean);
    if (success){
        *var_out = result.boolean;
    }
    return(success);
}
function b32
config_bool_var(Config *config, String var_name, i1 subscript, b8 *var_out){
    b32 temp = false;
    b32 success = config_bool_var(config, var_name, subscript, &temp);
    if (success){
        *var_out = (temp != false);
    }
    return(success);
}
function b32
config_bool_var(Config *config, char *var_name, i1 subscript, b32* var_out){
    return(config_bool_var(config, SCu8(var_name), subscript, var_out));
}
function b32
config_bool_var(Config *config, char* var_name, i1 subscript, b8 *var_out){
    b32 temp = false;
    b32 success = config_bool_var(config, SCu8(var_name), subscript, &temp);
    if (success){
        *var_out = (temp != false);
    }
    return(success);
}

function b32
config_int_var(Config *config, String var_name, i1 subscript, i1* var_out){
    Config_Get_Result result = config_var(config, var_name, subscript);
    b32 success = result.success && result.type == Config_RValue_Type_Integer;
    if (success){
        *var_out = result.integer;
    }
    return(success);
}

function b32
config_int_var(Config *config, char *var_name, i1 subscript, i1* var_out){
    return(config_int_var(config, SCu8(var_name), subscript, var_out));
}

function b32
config_uint_var(Config *config, String var_name, i1 subscript, u32* var_out){
    Config_Get_Result result = config_var(config, var_name, subscript);
    b32 success = result.success && result.type == Config_RValue_Type_Integer;
    if (success){
        *var_out = result.uinteger;
    }
    return(success);
}

function b32
config_uint_var(Config *config, char *var_name, i1 subscript, u32* var_out){
    return(config_uint_var(config, SCu8(var_name), subscript, var_out));
}

function b32
config_string_var(Config *config, String var_name, i1 subscript, String* var_out){
    Config_Get_Result result = config_var(config, var_name, subscript);
    b32 success = result.success && result.type == Config_RValue_Type_String;
    if (success){
        *var_out = result.string;
    }
    return(success);
}

function b32
config_string_var(Config *config, char *var_name, i1 subscript, String* var_out){
    return(config_string_var(config, SCu8(var_name), subscript, var_out));
}

function b32
config_placed_string_var(Config *config, String var_name, i1 subscript, String* var_out, u8 *space, u64 space_size){
    Config_Get_Result result = config_var(config, var_name, subscript);
    b32 success = (result.success && result.type == Config_RValue_Type_String);
    if (success){
        u64 size = result.string.size;
        size = clamp_max(size, space_size);
        block_copy(space, result.string.str, size);
        *var_out = SCu8(space, size);
    }
    return(success);
}

function b32
config_placed_string_var(Config *config, char *var_name, i1 subscript, String* var_out, u8 *space, u64 space_size){
    return(config_placed_string_var(config, SCu8(var_name), subscript, var_out, space, space_size));
}

function b32
config_compound_var(Config *config, String var_name, i1 subscript, Config_Compound** var_out){
    Config_Get_Result result = config_var(config, var_name, subscript);
    b32 success = (result.success && result.type == Config_RValue_Type_Compound);
    if (success){
        *var_out = result.compound;
    }
    return(success);
}

function b32
config_compound_var(Config *config, char *var_name, i1 subscript, Config_Compound** var_out){
    return(config_compound_var(config, SCu8(var_name), subscript, var_out));
}

function b32
config_compound_bool_member(Config *config, Config_Compound *compound,
                            String var_name, i1 index, b32* var_out){
    Config_Get_Result result = config_compound_member(config, compound, var_name, index);
    b32 success = result.success && result.type == Config_RValue_Type_Boolean;
    if (success){
        *var_out = result.boolean;
    }
    return(success);
}

function b32
config_compound_bool_member(Config *config, Config_Compound *compound,
                            char *var_name, i1 index, b32* var_out){
    return(config_compound_bool_member(config, compound, SCu8(var_name), index, var_out));
}

function b32
config_compound_int_member(Config *config, Config_Compound *compound,
                           String var_name, i1 index, i1* var_out){
    Config_Get_Result result = config_compound_member(config, compound, var_name, index);
    b32 success = result.success && result.type == Config_RValue_Type_Integer;
    if (success){
        *var_out = result.integer;
    }
    return(success);
}

function b32
config_compound_int_member(Config *config, Config_Compound *compound,
                           char *var_name, i1 index, i1* var_out){
    return(config_compound_int_member(config, compound, SCu8(var_name), index, var_out));
}

function b32
config_compound_uint_member(Config *config, Config_Compound *compound,
                            String var_name, i1 index, u32* var_out){
    Config_Get_Result result = config_compound_member(config, compound, var_name, index);
    b32 success = result.success && result.type == Config_RValue_Type_Integer;
    if (success){
        *var_out = result.uinteger;
    }
    return(success);
}

function b32
config_compound_uint_member(Config *config, Config_Compound *compound,
                            char *var_name, i1 index, u32* var_out){
    return(config_compound_uint_member(config, compound, SCu8(var_name), index, var_out));
}

function b32
config_compound_string_member(Config *config, Config_Compound *compound,
                              String var_name, i1 index, String* var_out){
    Config_Get_Result result = config_compound_member(config, compound, var_name, index);
    b32 success = (result.success && result.type == Config_RValue_Type_String);
    if (success){
        *var_out = result.string;
    }
    return(success);
}

function b32
config_compound_string_member(Config *config, Config_Compound *compound,
                              char *var_name, i1 index, String* var_out){
    return(config_compound_string_member(config, compound, SCu8(var_name), index, var_out));
}

function b32
config_compound_placed_string_member(Config *config, Config_Compound *compound,
                                     String var_name, i1 index, String* var_out, u8 *space, u64 space_size){
    Config_Get_Result result = config_compound_member(config, compound, var_name, index);
    b32 success = (result.success && result.type == Config_RValue_Type_String);
    if (success){
        u64 size = result.string.size;
        size = clamp_max(size, space_size);
        block_copy(space, result.string.str, size);
        *var_out = SCu8(space, size);
    }
    return(success);
}

function b32
config_compound_placed_string_member(Config *config, Config_Compound *compound,
                                     char *var_name, i1 index, String* var_out, u8 *space, u64 space_size){
    return(config_compound_placed_string_member(config, compound, SCu8(var_name), index, var_out, space, space_size));
}

function b32
config_compound_compound_member(Config *config, Config_Compound *compound,
                                String var_name, i1 index, Config_Compound** var_out){
    Config_Get_Result result = config_compound_member(config, compound, var_name, index);
    b32 success = (result.success && result.type == Config_RValue_Type_Compound);
    if (success){
        *var_out = result.compound;
    }
    return(success);
}

function b32
config_compound_compound_member(Config *config, Config_Compound *compound,
                                char *var_name, i1 index, Config_Compound** var_out){
    return(config_compound_compound_member(config, compound, SCu8(var_name), index, var_out));
}

function Iteration_Step_Result
typed_bool_array_iteration_step(Config *config, Config_Compound *compound, i1 index, b32* var_out){
    Config_Iteration_Step_Result result = typed_array_iteration_step(config, compound, Config_RValue_Type_Boolean, index);
    b32 success = (result.step == Iteration_Good);
    if (success){
        *var_out = result.get.boolean;
    }
    return(result.step);
}

function Iteration_Step_Result
typed_int_array_iteration_step(Config *config, Config_Compound *compound, i1 index, i1* var_out){
    Config_Iteration_Step_Result result = typed_array_iteration_step(config, compound, Config_RValue_Type_Integer, index);
    b32 success = (result.step == Iteration_Good);
    if (success){
        *var_out = result.get.integer;
    }
    return(result.step);
}

function Iteration_Step_Result
typed_uint_array_iteration_step(Config *config, Config_Compound *compound, i1 index, u32* var_out){
    Config_Iteration_Step_Result result = typed_array_iteration_step(config, compound, Config_RValue_Type_Integer, index);
    b32 success = (result.step == Iteration_Good);
    if (success){
        *var_out = result.get.uinteger;
    }
    return(result.step);
}

function Iteration_Step_Result
typed_string_array_iteration_step(Config *config, Config_Compound *compound, i1 index, String* var_out){
    Config_Iteration_Step_Result result = typed_array_iteration_step(config, compound, Config_RValue_Type_String, index);
    b32 success = (result.step == Iteration_Good);
    if (success){
        *var_out = result.get.string;
    }
    return(result.step);
}

function Iteration_Step_Result
typed_placed_string_array_iteration_step(Config *config, Config_Compound *compound, i1 index, String* var_out, u8 *space, u64 space_size){
    Config_Iteration_Step_Result result = typed_array_iteration_step(config, compound, Config_RValue_Type_String, index);
    b32 success = (result.step == Iteration_Good);
    if (success){
        u64 size = result.get.string.size;
        size = clamp_max(size, space_size);
        block_copy(space, result.get.string.str, size);
        *var_out = SCu8(space, size);
    }
    return(result.step);
}

function Iteration_Step_Result
typed_compound_array_iteration_step(Config *config, Config_Compound *compound, i1 index, Config_Compound** var_out){
    Config_Iteration_Step_Result result = typed_array_iteration_step(config, compound, Config_RValue_Type_Compound, index);
    b32 success = (result.step == Iteration_Good);
    if (success){
        *var_out = result.get.compound;
    }
    return(result.step);
}

function i1
typed_bool_array_get_count(Config *config, Config_Compound *compound){
    i1 count = typed_array_get_count(config, compound, Config_RValue_Type_Boolean);
    return(count);
}

function i1
typed_int_array_get_count(Config *config, Config_Compound *compound){
    i1 count = typed_array_get_count(config, compound, Config_RValue_Type_Integer);
    return(count);
}

function i1
typed_string_array_get_count(Config *config, Config_Compound *compound){
    i1 count = typed_array_get_count(config, compound, Config_RValue_Type_String);
    return(count);
}

function i1
typed_compound_array_get_count(Config *config, Config_Compound *compound){
    i1 count = typed_array_get_count(config, compound, Config_RValue_Type_Compound);
    return(count);
}

function Config_Get_Result_List
typed_bool_array_reference_list(Arena *arena, Config *config, Config_Compound *compound){
    Config_Get_Result_List list = typed_array_reference_list(arena, config, compound, Config_RValue_Type_Boolean);
    return(list);
}

function Config_Get_Result_List
typed_int_array_reference_list(Arena *arena, Config *config, Config_Compound *compound){
    Config_Get_Result_List list = typed_array_reference_list(arena, config, compound, Config_RValue_Type_Integer);
    return(list);
}

function Config_Get_Result_List
typed_string_array_reference_list(Arena *arena, Config *config, Config_Compound *compound){
    Config_Get_Result_List list = typed_array_reference_list(arena, config, compound, Config_RValue_Type_String);
    return(list);
}

function Config_Get_Result_List
typed_compound_array_reference_list(Arena *arena, Config *config, Config_Compound *compound){
    Config_Get_Result_List list = typed_array_reference_list(arena, config, compound, Config_RValue_Type_Compound);
    return(list);
}

////////////////////////////////

function Config_Iteration_Step_Result
typed_array_iteration_step(Config *parsed, Config_Compound *compound, Config_RValue_Type type, i1 index){
    Config_Iteration_Step_Result result = {};
    result.step = Iteration_Quit;
    Config_Get_Result get_result = config_compound_member(parsed, compound, string_u8_litexpr("~"), index);
    if (get_result.success){
        if (get_result.type == type){
            result.step = Iteration_Good;
            result.get = get_result;
        }
        else{
            result.step = Iteration_Skip;
        }
    }
    return(result);
}

function i1
typed_array_get_count(Config *parsed, Config_Compound *compound, Config_RValue_Type type){
    i1 count = 0;
    for (i1 i = 0;; ++i){
        Config_Iteration_Step_Result result = typed_array_iteration_step(parsed, compound, type, i);
        if (result.step == Iteration_Skip){
            continue;
        }
        else if (result.step == Iteration_Quit){
            break;
        }
        count += 1;
    }
    return(count);
}

function Config_Get_Result_List
typed_array_reference_list(Arena *arena, Config *parsed, Config_Compound *compound, Config_RValue_Type type){
    Config_Get_Result_List list = {};
    for (i1 i = 0;; ++i){
        Config_Iteration_Step_Result result = typed_array_iteration_step(parsed, compound, type, i);
        if (result.step == Iteration_Skip){
            continue;
        }
        else if (result.step == Iteration_Quit){
            break;
        }
        Config_Get_Result_Node *node = push_array(arena, Config_Get_Result_Node, 1);
        node->result = result.get;
        zdll_push_back(list.first, list.last, node);
        list.count += 1;
    }
    return(list);
}

////////////////////////////////

function void
change_mode(App *app, String mode){
    fcoder_mode = FCoderMode_Original;
    if (string_match(mode, string_u8_litexpr("4coder"))){
        fcoder_mode = FCoderMode_Original;
    }
    else if (string_match(mode, string_u8_litexpr("notepad-like"))){
        begin_notepad_mode(app);
    }
    else{
        print_message(app, string_u8_litexpr("Unknown mode.\n"));
    }
}

////////////////////////////////

// TODO(allen): cleanup this mess some more

function Config*
theme_parse__data(App *app, Arena *arena, String8 filename, String8 data, Arena *color_arena, Color_Table *color_table)
{
    Config *parsed = config_from_text(app, arena, filename, data);
    if (parsed != 0)
    {
        for (Config_Assignment *node = parsed->first;
             node != 0;
             node = node->next)
        {
            Scratch_Block scratch(app, arena);
            Config_LValue *l = node->l;
            String l_name = push_string_copyz(scratch, l->identifier);
            Managed_ID id = managed_id_get(app, str8lit("colors"), l_name);
            if (id != 0)
            {
                u32 color = 0;
                if (config_uint_var(parsed, l_name, 0, &color))
                {
                    color_table->arrays[id%color_table->count] = make_colors(color_arena, color);
                }
                else
                {
                    Config_Compound *compound = 0;
                    if (config_compound_var(parsed, l_name, 0, &compound)){
                        local_persist u32 color_array[256];
                        i1 counter = 0;
                        for (i1 i = 0;; i += 1){
                            Config_Iteration_Step_Result result = typed_array_iteration_step(parsed, compound, Config_RValue_Type_Integer, i);
                            if (result.step == Iteration_Skip){
                                continue;
                            }
                            else if (result.step == Iteration_Quit)
                            {
                                break;
                            }
                            
                            color_array[counter] = result.get.uinteger;
                            counter += 1;
                            if (counter == 256){
                                break;
                            }
                        }
                        
                        color_table->arrays[id%color_table->count] = make_colors(color_arena, color_array, counter);
                    }
                }
            }
        }
    }
    return(parsed);
}

function Config*
theme_parse__buffer(App *app, Arena *arena, Buffer_ID buffer, Arena *color_arena, Color_Table *color_table)
{
    String8 contents = push_whole_buffer(app, arena, buffer);
    Config *parsed = 0;
    if (contents.str != 0)
    {
        String8 filename = push_buffer_filename(app, arena, buffer);
        parsed = theme_parse__data(app, arena, filename, contents, color_arena, color_table);
    }
    return(parsed);
}

function Config*
theme_parse__filename(App *app, Arena *arena, char *filename, Arena *color_arena, Color_Table *color_table){
    Config *parsed = 0;
	FILE* file = fopen(filename, "rb");
    if (file == 0){
        file = def_search_normal_fopen(arena, filename, "rb");
    }
    if (file != 0){
        String data = read_entire_file_handle(arena, file);
        fclose(file);
        parsed = theme_parse__data(app, arena, SCu8(filename), data, color_arena, color_table);
    }
    if (parsed == 0){
        Scratch_Block scratch(app, arena);
        String str = push_stringfz(scratch, "Did not find %s, theme not loaded", filename);
        print_message(app, str);
    }
    return(parsed);
}

////////////////////////////////

// TODO(allen): review this function
function void
load_config_and_apply(App *app, Arena *out_arena, i1 override_font_size, b32 override_hinting){
    Scratch_Block scratch(app, out_arena);
    
    arena_clear(out_arena);
    
    Config *parsed = 0;
    FILE *file = def_search_normal_fopen(scratch, "config.4coder", "rb");
    if (file != 0){
        String data = read_entire_file_handle(scratch, file);
        fclose(file);
        if (data.str != 0){
            parsed = config_from_text(app, scratch, str8_lit("config.4coder"), data);
        }
    } 
    
    if (parsed != 0){
        // Errors
        String error_text = config_stringize_errors(app, scratch, parsed);
        if (error_text.str != 0){
            print_message(app, string_u8_litexpr("trying to load config file:\n"));
            print_message(app, error_text);
        }
        
        // NOTE(allen): Save As Variables
        if (error_text.str == 0)
        {
            // TODO(allen): this always applies to "def_config" need to get "usr_config" working too
            Variable_Handle config_var = def_fill_var_from_config(app, vars_get_root(),
                                                                  vars_intern_lit("def_config"),
                                                                  parsed);
			vars_print(app, config_var);
            print_message(app, string_u8_litexpr("\n"));
        }
    }
    else{
        print_message(app, string_u8_litexpr("Using default config:\n"));
        Face_Description description = get_face_description(app, 0);
        if (description.font.filename.str != 0){
            def_set_config_string(vars_intern_lit("default_font_name"), description.font.filename);
        }
    }
    
    String default_font_name = def_get_config_string(scratch, vars_intern_lit("default_font_name"));
    if (default_font_name.size == 0){
        default_font_name = string_u8_litexpr("liberation-mono.ttf");
    }
    
    // TODO(allen): this part seems especially weird now.
    // We want these to be effected by evals of the config system,
    // not by a state that gets evaled and saved *now*!!
    
    // Apply config
    String mode = def_get_config_string(scratch, vars_intern_lit("mode"));
    change_mode(app, mode);
    
    b32 lalt_lctrl_is_altgr = def_get_config_b32(vars_intern_lit("lalt_lctrl_is_altgr"));
    global_set_setting(app, GlobalSetting_LAltLCtrlIsAltGr, lalt_lctrl_is_altgr);
    
    String default_theme_name = def_get_config_string(scratch, vars_intern_lit("default_theme_name"));
    Color_Table *colors = get_color_table_by_name(default_theme_name);
    set_active_color(colors);
    
    Face_Description description = {};
    if (override_font_size != 0){
        description.parameters.pt_size = override_font_size;
    }
    else{
        description.parameters.pt_size = (i1)def_get_config_u64(app, vars_intern_lit("default_font_size"));
    }
    if (description.parameters.pt_size == 0){
        description.parameters.pt_size = 12;
    }
    
    b32 default_font_hinting = def_get_config_b32(vars_intern_lit("default_font_hinting"));
    description.parameters.hinting = default_font_hinting || override_hinting;
    
    Face_Antialiasing_Mode aa_mode = FaceAntialiasingMode_8BitMono;
    String8 aa_mode_string = def_get_config_string(scratch, vars_intern_lit("default_font_aa_mode"));
    if (string_match(aa_mode_string, str8_lit("8bit"))){
        aa_mode = FaceAntialiasingMode_8BitMono;
    }
    else if (string_match(aa_mode_string, str8_lit("1bit"))){
        aa_mode = FaceAntialiasingMode_1BitMono;
    }
    description.parameters.aa_mode = aa_mode;
    
    description.font.filename = default_font_name;
    if (!modify_global_face_by_description(app, description)){
        String8 name_in_fonts_folder = push_stringfz(scratch, "fonts/%.*s", string_expand(default_font_name));
        description.font.filename = def_search_normal_full_path(scratch, name_in_fonts_folder);
        modify_global_face_by_description(app, description);
    }
    
    b32 bind_by_physical_key = def_get_config_b32(vars_intern_lit("bind_by_physical_key"));
    if (bind_by_physical_key){
        system_set_key_mode(Key_Mode_Physical);
    }
    else{
        system_set_key_mode(Key_Mode_LanguageArranged);
    }
}

function void
load_theme_file_into_live_set(App *app, char *filename){
    Arena *arena = &global_theme_arena;
    Color_Table color_table = make_color_table(app, arena);
    Scratch_Block scratch(app, arena);
    Config *config = theme_parse__filename(app, scratch, filename, arena, &color_table);
    String error_text;
    if (config)
    {
      error_text = config_stringize_errors(app, scratch, config);
    }
    else 
    {
      error_text = push_stringfz(arena, "cannot find config file %s", filename);
    }
    print_message(app, error_text);
    
    String name = SCu8(filename);
    name = path_filename(name);
    if (string_match(string_postfix(name, 7), string_u8_litexpr(".4coder"))){
        name = string_chop(name, 7);
    }
    save_theme(color_table, name);
}

function void
load_folder_of_themes_into_live_set(App *app, String path){
    Scratch_Block scratch(app);
    
    File_List list = system_get_file_list(scratch, path);
    for (File_Info **ptr = list.infos, **end = list.infos + list.count;
         ptr < end;
         ptr += 1){
        File_Info *info = *ptr;
        if (!HasFlag(info->attributes.flags, FileAttribute_IsDirectory)){
            String name = info->filename;
            if (string_match(string_postfix(name, 7), str8_lit(".4coder"))){
                Temp_Memory_Block temp(scratch);
                String full_name = push_stringfz(scratch, "%.*s/%.*s",
                                                            string_expand(path),
                                                            string_expand(name));
                load_theme_file_into_live_set(app, (char*)full_name.str);
            }
        }
    }
}

////////////////////////////////
// NOTE(allen): Commands

CUSTOM_COMMAND_SIG(load_theme_current_buffer)
CUSTOM_DOC("Parse the current buffer as a theme file and add the theme to the theme list. If the buffer has a .4coder postfix in it's name, it is removed when the name is saved.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    
    Scratch_Block scratch(app);
    String filename = push_buffer_filename(app, scratch, buffer);
    if (filename.size > 0){
        Arena *arena = &global_theme_arena;
        Color_Table color_table = make_color_table(app, arena);
        Config *config = theme_parse__buffer(app, scratch, buffer, arena, &color_table);
        String error_text = config_stringize_errors(app, scratch, config);
        print_message(app, error_text);
        
        u64 problem_score = 0;
        if (color_table.count < (i1)defcolor_line_numbers_text){
            problem_score = defcolor_line_numbers_text - color_table.count;
        }
        //
        for (i1 i = 0; i < color_table.count; i += 1){
            if (color_table.arrays[i].count == 0){
                problem_score += 1;
            }
        }
        
        if (error_text.size > 0 || problem_score >= 10){
            String string = push_stringfz(scratch, "There appears to be a problem parsing %.*s; no theme change applied\n", string_expand(filename));
            print_message(app, string);
        }
        else{
            String name = path_filename(filename);
            if (string_match(string_postfix(name, 7), string_u8_litexpr(".4coder"))){
                name = string_chop(name, 7);
            }
            save_theme(color_table, name);
            
            Color_Table_Node *node = global_theme_list.last;
            if (node != 0 && string_match(node->name, name)){
                active_color_table = node->table;
            }
        }
    }
}

CUSTOM_COMMAND_SIG(go_to_user_directory)
CUSTOM_DOC("Go to the 4coder user directory")
{
    Scratch_Block scratch(app);
    String hot = push_hot_directory(app, scratch);
    String8 user_4coder_path = system_get_path(scratch, SystemPath_UserDirectory);
    String8 cmd = push_stringfz(scratch, "mkdir \"%.*s\"", string_expand(user_4coder_path));
    exec_system_command(app, 0, buffer_identifier(0), hot, cmd, 0);
    set_hot_directory(app, user_4coder_path);
}

// BOTTOM