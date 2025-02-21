//-
struct Template_Node
{
 String text;
 union { i32 field_index, parameter_index; };
 b32 quoted;
};
//-