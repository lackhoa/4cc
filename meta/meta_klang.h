//-
// TODO(kv) Why did I name this language "klang?" Totally not gonna backfire later!

enum Parsed_Type_Kind
{
 Parsed_Type_Pointer = 0,  //NOTE(kv) Deliberately zero.
 Parsed_Type_Array,
 Parsed_Type_Reference,
 Parsed_Type_Function,
};
typedef u32 Parsed_Type_Flags;
enum
{
 Parsed_Type_IsConst = 1,
};
struct Type_And_Name;
//NOTE(kv) Cheesy type!!!
struct Parsed_Type
{
 Parsed_Type_Kind kind;
 String name;
 u32 pointer_count;
 Parsed_Type_Flags flags;
 
 // NOTE(kv) Function type (for lambdas, which we don't care much about right now)
 Parsed_Type *return_type;
 String parameters;
 
 // NOTE(kv) We're supposed to know the array count at compile time,
 // only problem is we don't actually compile, so sometimes we just... don't know.
 String array_count;
 i32 array_count_int;
};
struct Type_And_Name
{
 Parsed_Type type;
 String name;
};
//-
struct M_Struct_Member
{
 union
 {
  Type_And_Name type_and_name;
  struct
  {
   Parsed_Type type;
   String name;
  };
 };
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
struct Meta_Expression;
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
struct Meta_Statement;

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

typedef sarray(Meta_Statement) Statement_Block;

struct Statement_Declaration 
{
 union
 {
  Type_And_Name type_and_name;
  struct
  {
   Parsed_Type type;
   String      name;
  };
 };
 Meta_Expression rhs;  // NOTE Optional assignment
};
struct Statement_Header_And_Body 
{
 String header;  // NOTE(kv): optional
 Meta_Statement *body;
};
struct Statement_If 
{
 Meta_Expression condition;
 Meta_Statement *body;
 Meta_Statement *else0;
};
struct Switch_Case;
struct Statement_Switch 
{
 Meta_Expression expression;
 darray(Switch_Case) cases;
};

typedef Statement_Declaration Cache_Item;

struct Statement_Root 
{
 sarray(Meta_Statement) top_levels;
};
struct Statement_Function 
{
 union
 {
  Type_And_Name type_and_name;
  struct
  {
   Parsed_Type type;
   String name;
  };
 };
 b32 has_body;
 sarray(Meta_Statement) body;
};
struct Statement_Misc 
{
 String as_string;
};
struct Statement_Head
{
 Statement_Head *mom;
 Statement_Kind kind;
 i32 pos;
};
// NOTE(kv) The statement union
struct Meta_Statement : Statement_Head
{
 union
 {
  Statement_Declaration dummy;
  
  Statement_Declaration     declaration;
  Statement_Switch          switch0;
  Meta_Expression           expression;
  Statement_Function        function0;
  Statement_Misc            misc;
  Statement_Block           block;
  Statement_Header_And_Body header_and_body;
  Meta_Expression           return0;
  Statement_If              if0;
  //-Top-level things
  Statement_Root     root;
 };
};
static_assert(sizeof(((Meta_Statement*)0)->dummy) ==
              (sizeof(Meta_Statement) - sizeof(Statement_Head)));

struct Switch_Case
{
 Meta_Expression expression;
 Meta_Statement body;
 b32 break_after;
};
//~
struct Union_Variant
{
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
//#define k_struct(text)  k_struct_func(strlit(#text))
function void
k_print_struct(Printer &p, K_Struct struc);
//-
typedef sarray(i32) M_Marked_Positions;

struct M_Text_Range
{
 i32 guess_begin_index;
 Range_i32 range;
};
struct Meta_Slider
{
 i32 file;
 M_Text_Range range;
 String type;
 String id;  // NOTE(kv) `<type>_<n>`, stamped in the source; keys the values file

 String options;
};
struct M_Text_Object
{
 i32 file;
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
 i32 file;
 M_Text_Range range;
 i32 indicator_level;
 b32 overlay;
};
struct FUI_Collector_File
{// NOTE @finish_fui_file
 String name;
 String path;  // NOTE(kv) For error messages (@kv_jump_syntax)
 b32 is_driver;
 
 darray(i32) positions;
 
 Range_i32 text_objects_slice;
 Range_i32 sliders_slice;
 Range_i32 vertices_slice;
};
struct FUI_Collector
{// NOTE see @klang_main
 b32 is_driver;
 Arena *arena;
 Arena *file_tmp;
 FUI_Collector_File file;
 darray(FUI_Collector_File) files;
 
 // NOTE(kv) Positions isn't stored in a central array because it'd be too confusing.
 // We don't want positions from different files to mix up.
 //darray(i32)           positions;
 
 darray(M_Text_Object) text_objects;
 
 // NOTE(kv) Sliders and vertices need to be arranged in source-code order.
 // TODO(kv) What a silly requirement! We only need to iterate on them,
 // (Well but maybe we need slider values to be in kinda source order).
 // Relaxing that requirement makes an argument for creating the file map
 // in the metaprogram itself, so we won't need to arrange stuff in order anymore.
 // Well, but we can do that whenever, just do a search and then fix-up all the values.
 // But I feel like that won't be pretty...
 darray(Meta_Slider) sliders;
 darray(Meta_Vertex) vertices;
};
myinline i32
get_file_index(FUI_Collector *driver)
{
 return driver->files.count;
}

struct Klang_Parser : Ed_Parser
{
 Statement_Head *current_statement;
 FUI_Collector *driver;
 Arena *arena;  // NOTE(kv) Arena to house statements and expressions.
 b32 do_generate_cpp;
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
parse_statement_block(Klang_Parser *p);

function void
parse_statement_to_pointer(Klang_Parser *p, Meta_Statement *statement);

myinline Meta_Statement *
parse_statement_to_arena(Klang_Parser *p)
{
 Meta_Statement *statement = push_struct(p->arena, Meta_Statement, push_zero());
 parse_statement_to_pointer(p, statement);
 return statement;
}
function M_Struct_Members
parse_struct_body(Arena *arena, Stringz string);

myinline String
get_function_name(Expression_Call *call)
{
 String result = call->func->as_string;
 return result;
}
struct String_Mapping
{
 String key;
 String val;
};

function void
parse_expression(Klang_Parser *p, Precedence max_precedence,
                 Meta_Expression *result);
function b32
modify_expression(Arena *arena, Meta_Expression *result);

function b32
to_cpp_expression(Arena *arena, FUI_Collector *driver,
                  Meta_Expression *result);
function void
parse_expression_full(Klang_Parser *p, Precedence max_precedence,
                      Meta_Expression *result)
{
 parse_expression(p, max_precedence, result);
 b32 ok = modify_expression(p->arena, result);
 if(ok and p->do_generate_cpp)
 {
  ok = to_cpp_expression(p->arena, p->driver, result);
 }
 if(not ok){ p->fail(); }
}
myinline void
parse_expression_full(Klang_Parser *p, Meta_Expression *result)
{
 parse_expression_full(p, Precedence_Max, result);
}
function Klang_Parser
k_parser_from_string(Arena *arena, Stringz string)
{
 Klang_Parser parser = {};
 (Ed_Parser &)parser = ed_parser_from_string(arena, string);
 parser.arena = arena;
 return parser;
}
function b32
is_function_keyword(String string)
{
 return (string == strlit("xfunction")  or
         string == strlit("function")   or
         string == strlit("inline")     or
         string == strlit("myinline")   or
         string == strlit("dll_export") or
         false);
}
//-