enum Operator_Type{
 Op_Plus,
 Op_Mult,
};
struct Expression_Operator{
 Operator_Type kind;
 Expression *left;
 Expression *right;
};
enum Expression_Kind{
 Expression_Type_Operator,
 Expression_Type_Number,
};
struct Expression{
 Expression_Kind kind;
 union{
  Expression_Operator op;
  i32 number;
 };
};
int main(){
 Expression e1 = {};
 e1.kind = Expression_Type_Operator;
 Expression left = {.kind=Expression_Type_Number};
 {
  left.number = 1;
 }
 Expression right = {};;
 {
  right.kind = Expression_Type_Operator;
  right.op = Op_Mul;
  Expression right_left  = {.kind=Expression_Type_Number, .number=2,};
  Expression right_right = {.kind=Expression_Type_Number, .number=3,};
 }
 e1.op.left = left;
 e1.op.right = right;
}
//-