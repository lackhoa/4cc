#pragma once
const b32 DEBUG_vv_name = 0;
const b32 DEBUG_parse_test_file = 0;
//-
enum Parsed_Type_Kind{
 Parsed_Type_None,
 Parsed_Type_Named,
 Parsed_Type_Pointer,
 Parsed_Type_Array,
};
//NOTE(kv) Cheesy type!!!
struct Parsed_Type{
 Parsed_Type_Kind kind;
 String name;
 i32    count; // NOTE either pointer count, or array count
};
inline Parsed_Type
make_type_named(String name){
 Parsed_Type result = {};
 result.kind = Parsed_Type_Named;
 result.name = name;
 return result;
}
inline Parsed_Type
make_type_pointer(String name, i32 count){
 Parsed_Type result = {};
 result.kind  = Parsed_Type_Pointer;
 result.name  = name;
 result.count = count;
 return result;
}
inline Parsed_Type
make_type_array(String name, i32 count){
 Parsed_Type result = {};
 result.kind  = Parsed_Type_Array;
 result.name  = name;
 result.count = count;
 return result;
}
//-
struct M_Struct_Member
{
 String name;
 Parsed_Type type;
 String version_added;
 String version_removed;
 String default_value;
 String discriminator;  //NOTE(kv) for union type only
 b32    unserialized;
};
typedef arrayof<M_Struct_Member> M_Struct_Members;
inline b32
member_was_removed(M_Struct_Member &member){
 return member.version_removed.len != 0;
}
//-
struct Meta_Expression;
enum Expression_Kind{
 Expression_Kind_None = 0,
 Expression_Kind_Unknown,
 Expression_Kind_Assignment,
 Expression_Kind_Call,
 Expression_Kind_Identifier,
 Expression_Kind_If,
 Expression_Kind_Loop,
 Expression_Kind_Float,  //NOTE(kv) Support double too, why not?
 Expression_Kind_Int,
};
struct Expression_Assignment{
 String          lhs;
 Meta_Expression *rhs;
};
struct Expression_Function_Call{
 String function_name;
 arrayof<Meta_Expression> arguments;
};
struct Meta_Expression{
 Expression_Kind kind;
 union{
  Expression_Assignment assignment;
  Expression_Function_Call function_call;
  String int_value;
  String float_value;
  String identifier;
  String unknown;
 };
};
Meta_Expression stub_expression = {};
//-
union Statement_Union;
enum Statement_Kind{
 Statement_Kind_None = 0,
 Statement_Kind_Unknown,
 Statement_Kind_Empty,
 Statement_Kind_Expression,
 Statement_Kind_Block,
 Statement_Kind_Header_And_Body,
 Statement_Kind_Return,
 Statement_Kind_Declaration,
 Statement_Kind_If,
 Statement_Kind_Switch,
 Statement_Kind_Cache,
 //-Top level
 Statement_Kind_Root,
 Statement_Kind_Function,
};
struct Statement_Head{
 Statement_Head *mom;
 Statement_Kind kind;
 i32 pos;
};
typedef arrayof<Statement_Union> Meta_Statements;

struct Statement_Block:Statement_Head{
 Meta_Statements block;
};
struct Statement_Declaration : Statement_Head{
 Parsed_Type     type;
 String          name;
 Meta_Expression rhs;  //NOTE Without "rhs", it means "declaration-only"
};
struct Statement_Header_And_Body : Statement_Head{
 String header;  //NOTE(kv): optional
 Statement_Head *body;
};
struct Statement_If : Statement_Head{
 Meta_Expression condition;
 Statement_Head  *body;
 Statement_Head  *else0;
};
struct Switch_Case;
struct Statement_Switch : Statement_Head{
 Meta_Expression expression;
 arrayof<Switch_Case> cases;
};

typedef Statement_Declaration Cache_Item;

struct Statement_Cache : Statement_Head{
 i32 id;
 arrayof<Cache_Item> cache_items;
 Statement_Head *body;
};
struct Statement_Root : Statement_Head{
 String source_path;
 Meta_Statements top_levels;
};
struct Statement_Function : Statement_Head{
 Meta_Statements body;
};
struct Statement_Unknown : Statement_Head{
 String unknown;
};
struct Statement_Return : Statement_Head{
 Meta_Expression return0;
};
struct Statement_Expression : Statement_Head{
 Meta_Expression expression;
};
union Statement_Union{
 Statement_Head head;
 //-
 Statement_Unknown         unknown;
 Statement_Expression      expression;
 Statement_Block           block;
 Statement_Header_And_Body header_and_body;
 Statement_Declaration     declaration;
 Statement_Return          return0;
 Statement_If              if0;
 Statement_Switch          switch0;
 Statement_Cache           cache0;
 //-Top-level things
 Statement_Root     root;
 Statement_Function function0;
};
struct Switch_Case{
 Meta_Expression expression;
 Statement_Union body;
 b32             break_after;
};
//~
struct Union_Variant{
 //-Input data
 i32    enum_value;
 String name;
 String name_lower;
 M_Struct_Members struct_members;
 //-Auto-generated fields
 String enum_name;
 String struct_name;
};
struct K_Struct{
 String name;
 M_Struct_Members members;
};
#define k_struct(text)  k_struct_func(strlit(#text))
function void
k_print_struct(Printer &p, K_Struct struc);
//-
struct K_Slider
{
 String type;
 String value;
};
struct Klang_Parser : Ed_Parser
{
 arrayof<Statement_Cache*> function_cache_list;
 Statement_Head *current_statement;
 arrayof<K_Slider> *sliders;
};
//-
function Meta_Statements
k_parse_statement_block(Arena *arena, Klang_Parser *p);

function void
k_parse_statement_to_pointer(Arena *arena, Klang_Parser *p, Statement_Union *statement);

inline Statement_Head *
k_parse_statement_to_arena(Arena *arena, Klang_Parser *p)
{
 Statement_Union *statement = push_struct(arena, Statement_Union, push_zero());
 k_parse_statement_to_pointer(arena, p, statement);
 return &statement->head;
}
//-