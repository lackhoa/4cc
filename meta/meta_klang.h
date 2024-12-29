//-
global b32 DEBUG_vv_name = 0;
//-
enum Parsed_Type_Kind{
 Parsed_Type_Pointer = 0,  //NOTE(kv) Deliberately zero.
 Parsed_Type_Array,
 Parsed_Type_Reference,
};
typedef u32 Parsed_Type_Flags;
enum
{
 Parsed_Type_IsConst = 1,
};
//NOTE(kv) Cheesy type!!!
struct Parsed_Type
{
 Parsed_Type_Kind kind;
 String name;
 u32    pointer_count;
 Parsed_Type_Flags flags;
 
 //NOTE(kv) We're supposed to know the array count at compile time,
 //  only problem is we don't actually compile, so sometimes we just... don't know.
 String array_count;
 i32 array_count_int;
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

enum Expression_Kind
{
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
 Expression_Kind_Dot_Placeholder,
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
struct Expression_Unary
{
 Unary_Operator op;
 Meta_Expression *argument;
};
struct Expression_Binary
{
 Binary_Operator op;
 Meta_Expression *lhs;
 Meta_Expression *rhs;
 Meta_Expression *ternary;
};
struct Expression_Call{
 Meta_Expression *func;
 darray(Meta_Expression) args;
};
struct Expression_Array_Subscript
{
 Meta_Expression *array;
 Meta_Expression *index;
};
struct Meta_Expression
{
 Expression_Kind kind;
 Range_i32 range;
 String as_string;
 union
 {
  struct
  {
   String compound_type_name;
   darray(Compound_Item) compound_items;
  };
  Expression_Call   call;
  Expression_Unary  unary;
  Expression_Binary binary;
  Expression_Array_Subscript array_subscript;
 };
};
struct Compound_Item
{
 String key;
 Meta_Expression value;
};
Meta_Expression stub_expression = {};
//-
union Meta_Statement;
enum Statement_Kind
{
 Statement_Kind_None = 0,
 Statement_Kind_Misc,
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
struct Statement_Head
{
 Statement_Head *mom;
 Statement_Kind kind;
 i32 pos;
};

struct Statement_Block : Statement_Head
{
 sarray(Meta_Statement) block;
};
struct Statement_Declaration : Statement_Head
{
 Parsed_Type     type;
 String          name;
 Meta_Expression rhs;  // NOTE Optional assignment
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
 sarray(Meta_Statement) top_levels;
};
struct Statement_Function : Statement_Head{
 b32 has_body;
 sarray(Meta_Statement) body;
};
struct Statement_Misc : Statement_Head
{
 String as_string;
};
struct Statement_Return : Statement_Head{
 Meta_Expression return0;
};
struct Statement_Expression : Statement_Head{
 Meta_Expression expression;
};
union Meta_Statement
{
 Statement_Head head;
 //-
 Statement_Misc            misc;
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
struct Switch_Case
{
 Meta_Expression expression;
 Meta_Statement body;
 b32 break_after;
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
struct K_Struct
{
 String name;
 M_Struct_Members members;
};
#define k_struct(text)  k_struct_func(strlit(#text))
function void
k_print_struct(Printer &p, K_Struct struc);
//-
typedef Static_Array2<i32> M_Locations;

struct M_Text_Range
{
 i32 guess_begin_index;
 Range_i32 range;
};
struct Meta_Slider
{
 M_Text_Range range;
 b32 is_runtime;
 String type;
 String value;
 String options;
};
struct M_Text_Object
{
 M_Text_Range range;
 Text_Object_Kind kind;
 
 union {
  struct {
   String filename;
   String marker;
  } image;
  // or
  String preset;
 };
};
struct Meta_Vertex
{
 M_Text_Range range;
 i32 indicator_level;
 b32 overlay;
};
struct Driver_Collected
{// NOTE see @klang_main
 darray(i32)             locations;
 darray(M_Text_Range) text_ranges;
 
 darray(Meta_Slider) sliders;
 darray(M_Text_Object) objects;
 darray(Meta_Vertex) vertices;
};
struct Klang_Parser : Ed_Parser
{
 Statement_Head *current_statement;
 Driver_Collected *driver;
 Arena *arena;
 b32 do_modify;
};
//-
// NOTE(kv) Larger value = Binds weaker (unintuitive), but it's what the table says
// Table: https://en.cppreference.com/w/c/language/operator_precedence
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
  case '?':
  return Precedence_Assignment;
 }
 return {};
}
function b32
is_right_associative(Binary_Operator op)
{
 u32 op_u32 = op_to_u32_little_endian(op);
 switch(op_u32)
 {
  case '=':
  case '+=':
  case '-=':
  case '*=':
  case '/=':
  case '|=':
  case '&=':
  case '?':
  return true;
 }
 return false;
}
//-
function sarray(Meta_Statement)
parse_statement_block(Arena *arena, Klang_Parser *p);

function void
parse_statement_to_pointer(Arena *arena, Klang_Parser *p, Meta_Statement *statement);

myinline Statement_Head *
parse_statement_to_arena(Arena *arena, Klang_Parser *p)
{
 Meta_Statement *statement = push_struct(arena, Meta_Statement, push_zero());
 parse_statement_to_pointer(arena, p, statement);
 return &statement->head;
}
function M_Struct_Members
parse_struct_body(Arena *arena, String string);

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

function void
parse_expression_2(Klang_Parser *p, Precedence max_precedence,
                  Meta_Expression *result);
myinline void
parse_expression(Klang_Parser *p, Meta_Expression *result)
{
 parse_expression_2(p, Precedence_Max, result);
}
function Klang_Parser
k_parser_from_string(Arena *arena, String string)
{
 Klang_Parser parser = {};
 (Ed_Parser &)parser = ed_parser_from_string(arena, string);
 parser.arena = arena;
 return parser;
}
function void
parse_expression_from_string(Arena *arena, String string,
                             Meta_Expression *result, Range_i32 actual_range)
{// NOTE(kv) We parse the expression, without modification,
 // no collecting driver info.
 Klang_Parser parser = k_parser_from_string(arena, string);
 parse_expression(&parser, result);
 result->range = actual_range;
}
//-