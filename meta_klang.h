//-
global b32 DEBUG_vv_name = 0;
global b32 DEBUG_parse_test_file = 0;  // test.kc
//-
enum Parsed_Type_Kind{
 Parsed_Type_Pointer = 0,  //NOTE(kv) Deliberately zero.
 Parsed_Type_Array,
};
//NOTE(kv) Cheesy type!!!
struct Parsed_Type{
 Parsed_Type_Kind kind;
 String name;
 u32    pointer_count;
 
 //NOTE(kv) We're supposed to know the array count at compile time,
 //  only problem is we don't actually compile, so sometimes we just... don't know.
 u32    array_count;
 String array_count_str;
};
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
typedef darray(M_Struct_Member) M_Struct_Members;
myinline b32
member_was_removed(M_Struct_Member &member){
 return member.version_removed.len != 0;
}
//-
struct Meta_Expression;
struct Compound_Item;

enum Expression_Kind{
 Expression_Kind_None = 0,
 Expression_Kind_Unknown,
 Expression_Kind_String,
 Expression_Kind_Unary,
 Expression_Kind_Binary,
 Expression_Kind_Call,
 Expression_Kind_Array_Subscript,
 Expression_Kind_Identifier,
 Expression_Kind_Float,  //NOTE(kv) Support double too, why not?
 Expression_Kind_Int,
 Expression_Kind_Compound,
};
//-
enum Unary_Operator{
 Unary_Char_End_ = 128,
 
 Postfix_Increment, // a++
 Postfix_Decrement, // a--
 
 Unary_Named_End_ = 256,
};
function b32
is_prefix(Unary_Operator op)
{
 switch(op)
 {
  case Postfix_Increment: 
  case Postfix_Decrement:
  return false;
 }
 return true;
}
myinline char *
op_to_cstring(Unary_Operator *op)
{
 return (char *)op;
}
function String
to_string(Unary_Operator *op)
{
 u32 u32_op = u32(*op);
 if(u32_op < Unary_Char_End_ or
    u32_op >= Unary_Named_End_)
 {
  char *cstring = op_to_cstring(op);
  return SCu8(cstring);
 }else{
  switch(*op) {
   case Postfix_Increment: return SCu8("++");
   case Postfix_Decrement: return SCu8("--");
  }
 }
 
 invalid_code_path;
 return {};
}
//-
typedef u32 Binary_Operator;

#define PP_SingleQuote(x) ((#x)[0])
#define binary_op_xlist(X) \
X(not) X(and) X(or) \
X(->) X(+=)

myinline char *
op_to_cstring(Binary_Operator *op)
{
 return (char *)op;
}
myinline String
to_string(Binary_Operator *op)
{
 char *op2 = op_to_cstring(op);
 return SCu8(op2);
}
//-
struct Expression_Unary{
 Unary_Operator op;
 Meta_Expression *argument;
};
struct Expression_Binary{
 Binary_Operator op;
 Meta_Expression *lhs;
 Meta_Expression *rhs;
};
struct Expression_Call{
 Meta_Expression *func;
 darray(Meta_Expression) arguments;
};
struct Expression_Array_Subscript{
 Meta_Expression *array;
 Meta_Expression *index;
};
struct Meta_Expression{
 Expression_Kind kind;
 String as_string;
 union{
  darray(Compound_Item) compound_items;
  Expression_Call   call;
  Expression_Unary  unary;
  Expression_Binary binary;
  Expression_Array_Subscript array_subscript;
 };
};
struct Compound_Item{
 String key;
 Meta_Expression value;
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

struct Statement_Block:Statement_Head{
 darray(Statement_Union) block;
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
 darray(Switch_Case) cases;
};

typedef Statement_Declaration Cache_Item;

struct Statement_Cache : Statement_Head{
 i32 id;
 darray(Cache_Item) cache_items;
 Statement_Head *body;
};
struct Statement_Root : Statement_Head{
 sarray(Statement_Union) top_levels;
};
struct Statement_Function : Statement_Head{
 b32 has_body;
 darray(Statement_Union) body;
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
 String options;
 
 u32 pos;
 u32 size;
};
struct Klang_Parser : Ed_Parser
{
 Statement_Head *current_statement;
 darray(K_Slider) *sliders;
 Arena *arena;
};
//-
//NOTE(kv) Larger value = Binds weaker (unintuitive), but it's what the table says
//  Table: https://en.cppreference.com/w/c/language/operator_precedence
enum Precedence
{
 Precedence_None = 0,
 
 Precedence_Dot_Arrow,
 Precedence_Unary,
 
 Precedence_Mul_Div,
 Precedence_Add_Sub,
 
 Precedence_Less_Than,
 Precedence_LogicalEqual,
 
 Precedence_BitwiseAnd,
 Precedence_BitwiseOr,
 
 Precedence_LogicalAnd,
 Precedence_LogicalOr,
 
 Precedence_Assignment,
 
 Precedence_Max,
};

function u32
op_to_u32_little_endian_inner(u32 op)
{
 u8 *op_array = (u8 *)&op;
 u32 last_index = 0;
 for_u32(i, 1, 4){
  if(op_array[i] != 0){
   last_index = i;
  }
 }
 
 u32 result = 0;
 u8 *result_array = (u8 *)&result;
 for_u32(i, 0, last_index+1){
  result_array[i] = op_array[last_index-i];
 }
 return result;
}
myinline u32
op_to_u32_little_endian(Unary_Operator op)
{
 return op_to_u32_little_endian_inner(op);
}
myinline u32
op_to_u32_little_endian(Binary_Operator op)
{
 return op_to_u32_little_endian_inner(op);
}

function Precedence
precedence_of(Unary_Operator op)
{
 u32 op_u32 = op_to_u32_little_endian(op);
 switch(op_u32)
 {
  case Postfix_Increment:
  case Postfix_Decrement:
  return Precedence_Dot_Arrow;
  
  case '++':
  case '--':
  case '+':
  case '-':
  case '*':
  case '!':
  case 'not':
  case '~':
  case '&':
  return Precedence_Unary;
 }
 return {};
}
function Precedence
precedence_of(Binary_Operator op)
{
 u32 op_u32 = op_to_u32_little_endian(op);
 switch(op_u32)
 {
  case '.':
  case '->':
  return Precedence_Dot_Arrow;
  
  case '*':
  case '/':
  case '%':
  return Precedence_Mul_Div;
  
  case '+':
  case '-':
  return Precedence_Add_Sub;
  
  case '<':
  case '<=':
  case '>':
  case '>=':
  return Precedence_Less_Than;
  
  case '==':
  case '!=':
  return Precedence_LogicalEqual;
  
  case '&':
  return Precedence_BitwiseAnd;
  
  case '|':
  return Precedence_BitwiseOr;
  
  case '&&':
  case 'and':
  return Precedence_LogicalAnd;
  
  case '||':
  case 'or':
  return Precedence_LogicalOr;
  
  case '=':
  case '+=':
  case '-=':
  case '*=':
  case '/=':
  case '|=':
  case '&=':
  return Precedence_Assignment;
 }
 return {};
}
//-
function darray(Statement_Union)
k_parse_statement_block(Arena *arena, Klang_Parser *p);

function void
k_parse_statement_to_pointer(Arena *arena, Klang_Parser *p, Statement_Union *statement);

myinline Statement_Head *
k_parse_statement_to_arena(Arena *arena, Klang_Parser *p)
{
 Statement_Union *statement = push_struct(arena, Statement_Union, push_zero());
 k_parse_statement_to_pointer(arena, p, statement);
 return &statement->head;
}
function M_Struct_Members
parse_struct_body(Arena *arena, char *string);

myinline String
get_function_name(Expression_Call *call)
{
 String result = call->func->as_string;
 return result;
}
struct String_Mapping{
 String key;
 String val;
};
//-