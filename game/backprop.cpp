//
enum LLM_Op
{
 LLM_Op_Data,
 LLM_Op_Add,
 LLM_Op_Mul,
 LLM_Op_Tanh,
};

struct LLM_Value
{
 v1 value;
 LLM_Op op;
 sarray(LLM_Value *) children;
 String label;
 v1 grad;
};

function LLM_Value
mk_value(v1 value, String label)
{
 LLM_Value out = {};
 out.value = value;
 out.label = label;
 return out;
}

function LLM_Value
mk_value_add(LLM_Value *a, LLM_Value *b, String label)
{
 Arena *tmp = &notebook_frame_arena;
 LLM_Value out = {};
 out.label = label;
 out.op = LLM_Op_Add;
 out.value = a->value + b->value;
 
 init_static(out.children, tmp, 2);
 out.children[0] = a;
 out.children[1] = b;
 
 return out;
}

function LLM_Value
mk_value_mul(LLM_Value *a, LLM_Value *b, String label)
{
 Arena *tmp = &notebook_frame_arena;
 LLM_Value out = {};
 out.label = label;
 out.op = LLM_Op_Mul;
 out.value = a->value * b->value;
 
 init_static(out.children, tmp, 2);
 out.children[0] = a;
 out.children[1] = b;
 
 return out;
}

function LLM_Value
mk_value_tanh(LLM_Value *a, String label)
{
 Arena *tmp = &notebook_frame_arena;
 LLM_Value out = {};
 out.label = label;
 out.op = LLM_Op_Tanh;
 v1 e_2n = expf(2.f * a->value);
 out.value = (e_2n - 1.f) / (e_2n + 1.f);
 
 init_static(out.children, tmp, 1);
 out.children[0] = a;
 
 return out;
}

function sarray(LLM_Value *)
backprop_sort(Arena *tmp, LLM_Value *root)
{
 darray(LLM_Value *) result;
 init_dynamic(result, tmp);
 
 push(&result, root);
 
 for_i32(current_index, 0, result.count)
 {
  LLM_Value *node = result[current_index];
  for_i32(index, 0, node->children.count)
  {
   LLM_Value *child = node->children[index];
   push(&result, child);
  }
 }
 
 return result;
}

function void
backprop(LLM_Value *root)
{
 Scratch_Block tmp;
 
 sarray(LLM_Value *) sorted = backprop_sort(tmp, root);
 
 root->grad = 1.f;
 
 for_i32(index, 0, sorted.count)
 {
  LLM_Value *node = sorted[index];
  /*// NOTE Pre-fill with the parent's gradient
  for_i32(child_index, 0, node->children.count)
  {
  LLM_Value *child = node->children[child_index];
  child->grad += node->grad;
  }*/
  
  v1 g = node->grad;
  switch(node->op)
  {// NOTE Back-propagation
   case LLM_Op_Add:
   {
    node->children[0]->grad += g;
    node->children[1]->grad += g;
   }break;
   
   case LLM_Op_Mul:
   {
    node->children[0]->grad += g * node->children[1]->value;
    node->children[1]->grad += g * node->children[0]->value;
   }break;
   
   case LLM_Op_Tanh:
   {
    v1 v = node->value;
    node->children[0]->grad += g * (1.f - v*v);
   }break;
  }
 }
}
function v1
get_gradient(LLM_Value *root, LLM_Value *the_node)
{
 
}
//