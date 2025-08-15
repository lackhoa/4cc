//
enum LLM_Op
{
 LLM_Op_Data,
 LLM_Op_Add,
 LLM_Op_Mul,
 LLM_Op_Pow,
 LLM_Op_Exp,
 LLM_Op_Tanh,
};

struct LLM_Value_2
{
 v1 value;
 LLM_Op op;
 sarray(struct LLM_Value) children;
 String label;
 v1 grad;
};

struct LLM_Value
{
 LLM_Value_2 *p;
};

function bool
operator==(LLM_Value a, LLM_Value b)
{
 return a.p == b.p;
}

function Arena *
get_llm_arena()
{
 return &notebook_frame_arena;
}

function LLM_Value_2
mk_value_2(v1 value)
{
 LLM_Value_2 out = {};
 out.value = value;
 return out;
}

function LLM_Value
mk_value(v1 value)
{
 Arena *ar = get_llm_arena();
 LLM_Value_2 *out = push_struct(ar, LLM_Value_2);
 *out = mk_value_2(value);
 return LLM_Value{out};
}

function LLM_Value
operator+(LLM_Value a, LLM_Value b)
{
 Arena *ar = get_llm_arena();
 LLM_Value_2 *out = push_struct0(ar, LLM_Value_2);
 out->op = LLM_Op_Add;
 out->value = a.p->value + b.p->value;
 
 init_static(out->children, ar, 2);
 out->children[0] = a;
 out->children[1] = b;
 
 return LLM_Value{out};
}

function LLM_Value
operator *(LLM_Value a, LLM_Value b)
{
 Arena *ar = get_llm_arena();
 LLM_Value_2 *out = push_struct0(ar, LLM_Value_2);
 out->op = LLM_Op_Mul;
 out->value = a.p->value * b.p->value;
 
 init_static(out->children, ar, 2);
 out->children[0] = a;
 out->children[1] = b;
 
 return {out};
}

// NOTE(kv) Bleh
function LLM_Value
operator*(LLM_Value a, v1 b)
{
 return a * mk_value(b);
}

function LLM_Value
operator-(LLM_Value a, LLM_Value b)
{
 LLM_Value neg_b = b * mk_value(-1.f);
 return a + neg_b;
}

function LLM_Value
mk_value_exp(LLM_Value a)
{
 Arena *ar = get_llm_arena();
 
 LLM_Value_2 *out = push_struct0(ar, LLM_Value_2);
 out->op    = LLM_Op_Exp;
 out->value = expf(a.p->value);
 
 init_static(out->children, ar, 1);
 out->children[0] = a;
 
 return {out};
}

function LLM_Value
mk_value_pow(LLM_Value a, LLM_Value b)
{
 Arena *ar = get_llm_arena();
 
 // ;assume_exponent_is_data
 kv_assert(b.p->op == LLM_Op_Data);
 
 LLM_Value_2 *out = push_struct0(ar, LLM_Value_2);
 out->op    = LLM_Op_Pow;
 out->value = powf(a.p->value, b.p->value);
 
 init_static(out->children, ar, 2);
 out->children[0] = a;
 out->children[1] = b;
 
 return {out};
}

function LLM_Value
operator/(LLM_Value a, LLM_Value b)
{
 // NOTE(kv) a/b = a * 1/b
 LLM_Value inv_b = mk_value_pow(b, mk_value(-1));
 LLM_Value result = a * inv_b;
 return result;
}

function LLM_Value
mk_value_tanh(LLM_Value a)
{
 Arena *ar = get_llm_arena();
 LLM_Value_2 *out = push_struct(ar, LLM_Value_2);
 out->op = LLM_Op_Tanh;
 v1 e_2n = expf(2.f * a.p->value);
 out->value = (e_2n - 1.f) / (e_2n + 1.f);
 
 init_static(out->children, ar, 1);
 out->children[0] = a;
 
 return {out};
}

function LLM_Value
mk_value_tanh_graph(LLM_Value a)
{// NOTE(kv) Just a test
 LLM_Value e_2x = mk_value_exp(a * mk_value(2.f));
 LLM_Value numer = e_2x - mk_value(1.f);
 LLM_Value denom = e_2x + mk_value(1.f);
 return numer / denom;
}

function void
build_topo_inner(darray(LLM_Value) *visited, LLM_Value node)
{// NOTE(kv) Very simple: visit all children, then node to list.
 for_i32(i, 0, node.p->children.count)
 {
  LLM_Value child = node.p->children[i];
  if(not darray_contains(visited, child))
  {
   build_topo_inner(visited, child);
  }
 }
 
 push(visited, node);
}

function sarray(LLM_Value)
build_topo(Arena *a, LLM_Value root)
{// NOTE(kv) We have no idea what the node list is,
 // and that's how it works.
 darray(LLM_Value) visited;
 init_dynamic(visited, a);
 
 build_topo_inner(&visited, root);
 
 return visited;
}

function void
backprop(LLM_Value root)
{
 Scratch_Block tmp;
 
 sarray(LLM_Value) sorted = build_topo(tmp, root);
 for_i32(i, 0, sorted.count)
 {// NOTE(kv) Reset gradient, dude!
  sorted[i].p->grad = 0.f;
 }
 
 root.p->grad = 1.f;  // NOTE(kv) Of course, it's the output
 kv_assert(get_last(sorted) == root);
 
 // NOTE(kv) backprop goes in the reverse topological sort order.
 for(i32 index=sorted.count-1;
     index >= 0;
     index--)
 {
  LLM_Value_2 *node = sorted[index].p;
  
  v1 g = node->grad;
  sarray(LLM_Value) children = node->children;
  LLM_Value_2 *c0 = 0;
  if(children.count > 0) { c0 = children[0].p; }
  LLM_Value_2 *c1 = 0;
  if(children.count > 1) { c1 = children[1].p; }
  
  switch(node->op)
  {// NOTE Back-propagation
   case LLM_Op_Data:
   {// NOTE(kv) Constants don't have children
   }break;
   
   case LLM_Op_Add:
   {
    c0->grad += g;
    c1->grad += g;
   }break;
   
   case LLM_Op_Mul:
   {
    c0->grad += g * c1->value;
    c1->grad += g * c0->value;
   }break;
   
   case LLM_Op_Pow:
   {
    v1 a = c0->value;
    v1 b = c1->value;
    c0->grad += g * (b * powf(a, b-1));
    
    // NOTE(kv) @assume_exponent_is_data exponent doesn't need gradient
   }break;
   
   case LLM_Op_Exp:
   {// NOTE(kv) e^x is its own derivative
    c0->grad += g * node->value;
   }break;
   
   case LLM_Op_Tanh:
   {
    v1 a = node->value;
    c0->grad += g * (1.f - a*a);
   }break;
   
   InvalidDefaultCase;
  }
 }
}

struct Neuron
{
 sarray(LLM_Value) weights;
 LLM_Value bias;
};

typedef sarray(Neuron) Layer;

// NOTE(kv) Function to generate a random number in the range [a, b]
function v1
uniform_distribution(v1 a, v1 b)
{
 double r = (double)rand();
 return v1(a + (b - a) * ((double)r / (double)RAND_MAX));
}

function Neuron
mk_neuron(i32 input_count)
{
 Arena *arena = get_llm_arena();
 Neuron result = {};
 init_static(result.weights, arena, input_count);
 for_i32(i, 0, input_count)
 {
  v1 init_weight = uniform_distribution(-1.f,1.f);
  result.weights[i] = mk_value(init_weight);
 }
 
 v1 init_bias = uniform_distribution(-1.f,1.f);
 result.bias = mk_value(init_bias);
 
 return result;
}

function LLM_Value
neuron_apply(Neuron &neuron, sarray(LLM_Value) x)
{
 kv_assert(neuron.weights.count == x.count);
 
 LLM_Value activation = mk_value(0.f);
 for_i32(i, 0, neuron.weights.count)
 {
  LLM_Value weight = neuron.weights[i];
  activation = activation + weight * x[i];
 }
 
 activation = activation + neuron.bias;
 activation = mk_value_tanh(activation);
 return activation;
}

function Layer
mk_layer(i32 input_count, i32 neuron_count)
{
 Arena *arena = get_llm_arena();
 Layer result = {};
 init_static(result, arena, neuron_count);
 for_i32(i, 0, neuron_count)
 {
  result[i] = mk_neuron(input_count);
 }
 return result;
}

function sarray(LLM_Value)
layer_apply(Arena *arena, Layer &layer, sarray(LLM_Value) input)
{
 kv_assert(layer[0].weights.count == input.count);
 
 sarray(LLM_Value) result;
 init_static(result, arena, layer.count);
 for_i32(i, 0, layer.count)
 {
  result[i] = neuron_apply(layer[i], input);
 }
 
 return result;
}

typedef sarray(Layer) MLP;

function MLP
mk_mlp(i32 first_input_count, sarray(i32) neuron_counts)
{// NOTE(kv) Multi-layer perceptron: one layer feeding into the next
 Arena *arena = get_llm_arena();
 
 MLP result = {};
 init_static(result, arena, neuron_counts.count);
 
 i32 input_count = first_input_count;
 for_i32(i, 0, neuron_counts.count)
 {
  result[i] = mk_layer(input_count, neuron_counts[i]);
  input_count = neuron_counts[i];
 }
 
 return result;
}

function sarray(LLM_Value)
mlp_apply(MLP &mlp, sarray(v1) first_input)
{
 Arena *arena = get_llm_arena();
 Scratch_Block tmp;
 
 i32 actual_first_input_count = mlp[0][0].weights.count;
 kv_assert(actual_first_input_count == first_input.count);
 
 sarray(LLM_Value) input;
 init_static(input, tmp, first_input.count);
 for_i32(i, 0, first_input.count)
 {
  input[i] = mk_value(first_input[i]);
 }
 
 for_i32(i, 0, mlp.count)
 {
  Layer &layer = mlp[i];
  input = layer_apply(tmp, layer, input);
 }
 
 sarray(LLM_Value) result;
 init_static(result, arena, input.count);
 for_i32(i, 0, input.count)
 {
  result[i] = input[i];
 }
 return result;
}

function sarray(LLM_Value)
get_mlp_parameters(Arena *arena, MLP &mlp)
{
 darray(LLM_Value) result;
 init_dynamic(result, arena);
 
 for_i32(layer_index, 0, mlp.count)
 {
  Layer layer = mlp[layer_index];
  for_i32(neuron_index, 0, layer.count)
  {
   Neuron neuron = layer[neuron_index];
   for_i32(weight_index, 0, neuron.weights.count)
   {
    LLM_Value weight = neuron.weights[weight_index];
    push(&result, weight);
   }
   push(&result, neuron.bias);
  }
 }
 
 return result;
}
//