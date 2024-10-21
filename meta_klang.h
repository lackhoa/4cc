#pragma once
const b32 DEBUG_parse_test_file = 0;
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
 result.count = count;
 return result;
}
//-
struct M_Struct_Member{
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
 Expression_Kind_Function_Call,
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
struct Meta_Statement;
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
};
struct Statement_Declaration{
 Parsed_Type     type;
 String          name;
 Meta_Expression rhs;  //NOTE Without "rhs", it means "declaration-only"
};
typedef arrayof<Meta_Statement> Meta_Statements;
typedef arrayof<Meta_Statement> Statement_Block;
struct Statement_Header_And_Body{
 String header;  //NOTE(kv): optional
 Meta_Statement *body;
};
struct Statement_If{
 Meta_Expression condition;
 Meta_Statement  *body;
 Meta_Statement  *else0;
};
struct Switch_Case;
struct Statement_Switch{
 Meta_Expression expression;
 arrayof<Switch_Case> cases;
};
typedef Statement_Declaration Cache_Item;
struct Statement_Cache{
 i32 id;
 arrayof<Cache_Item> cache_items;
 Meta_Statement *body;
};
struct Meta_Statement{
 Statement_Kind kind;
 union{
  String                    unknown;
  Meta_Expression           expression;
  Statement_Block           block;
  Statement_Header_And_Body header_and_body;
  Statement_Declaration     declaration;
  Meta_Expression           return0;
  Statement_If              if0;
  Statement_Switch          switch0;
  Statement_Cache           cache0;
 };
};
struct Switch_Case{
 Meta_Expression expression;
 Meta_Statement  body;
 b32             break_after;
};
//-
struct Union_Variant{
 //-NOTE Input data
 i32    enum_value;
 String name;
 String name_lower;
 M_Struct_Members struct_members;
 //-NOTE Auto-generated fields
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
struct Klang_Parser : Ed_Parser{
 arrayof<Statement_Cache*> cache_list;
};
//-
function arrayof<Meta_Statement>
k_parse_statement_block(Arena *arena, Klang_Parser *p);
function void
k_parse_statement_to_pointer(Arena *arena, Klang_Parser *p, Meta_Statement *statement);
function Meta_Statement *
k_parse_statement_to_arena(Arena *arena, Klang_Parser *p);
//-