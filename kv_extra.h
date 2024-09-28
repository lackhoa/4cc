function void
thread_ctx_init(Thread_Context *tctx, Thread_Kind kind, Base_Allocator *allocator,
                Base_Allocator *prof_allocator){
 block_zero_struct(tctx);
 tctx->kind = kind;
 tctx->allocator = allocator;
 tctx->node_arena = make_arena(allocator, KB(4));
 
 tctx->prof_allocator = prof_allocator;
 tctx->prof_id_counter = 1;
 tctx->prof_arena = make_arena(prof_allocator, KB(16));
}

function void
thread_ctx_release(Thread_Context *tctx){
 for (Arena_Node *node = tctx->free_arenas;
      node != 0;
      node = node->next){
  arena_clear(&node->arena);
 }
 for (Arena_Node *node = tctx->used_first;
      node != 0;
      node = node->next){
  arena_clear(&node->arena);
 }
 arena_clear(&tctx->node_arena);
 block_zero_struct(tctx);
}

function Arena_Node*
tctx__alloc_arena_node(Thread_Context *tctx){
 Arena_Node *result = tctx->free_arenas;
 if (result != 0){
  sll_stack_pop(tctx->free_arenas);
 }
 else{
  result = push_array(&tctx->node_arena, Arena_Node, 1, true);
  result->arena = make_arena(tctx->allocator, KB(16));
 }
 return(result);
}

function void
tctx__free_arena_node(Thread_Context *tctx, Arena_Node *node){
 sll_stack_push(tctx->free_arenas, node);
}

function Arena*
tctx_reserve(Thread_Context *tctx){
 Arena_Node *node = tctx->used_first;
 if (node == 0){
  node = tctx__alloc_arena_node(tctx);
  zdll_push_back(tctx->used_first, tctx->used_last, node);
 }
 node->ref_counter += 1;
 return(&node->arena);
}

function Arena*
tctx_reserve(Thread_Context *tctx, Arena *a1){
 Arena_Node *node = tctx->used_first;
 for (; node != 0; node = node->next){
  Arena *na = &node->arena;
  if (na != a1){
   break;
  }
 }
 if (node == 0){
  node = tctx__alloc_arena_node(tctx);
  zdll_push_back(tctx->used_first, tctx->used_last, node);
 }
 node->ref_counter += 1;
 return(&node->arena);
}

function Arena*
tctx_reserve(Thread_Context *tctx, Arena *a1, Arena *a2){
 Arena_Node *node = tctx->used_first;
 for (; node != 0; node = node->next){
  Arena *na = &node->arena;
  if (na != a1 && na != a2){
   break;
  }
 }
 if (node == 0){
  node = tctx__alloc_arena_node(tctx);
  zdll_push_back(tctx->used_first, tctx->used_last, node);
 }
 node->ref_counter += 1;
 return(&node->arena);
}

function Arena*
tctx_reserve(Thread_Context *tctx, Arena *a1, Arena *a2, Arena *a3){
 Arena_Node *node = tctx->used_first;
 for (; node != 0; node = node->next){
  Arena *na = &node->arena;
  if (na != a1 && na != a2 && na != a3){
   break;
  }
 }
 if (node == 0){
  node = tctx__alloc_arena_node(tctx);
  zdll_push_back(tctx->used_first, tctx->used_last, node);
 }
 node->ref_counter += 1;
 return(&node->arena);
}

function void
tctx_release(Thread_Context *tctx, Arena *arena){
 Arena_Node *node = CastFromMember(Arena_Node, arena, arena);
#if 0
 CastFromMember(S=Arena_Node, m=arena, ptr=arena)
 ( (u8*)(arena) - OffsetOfMember(Arena_Node,arena) )
 ( (u8*)(arena) - PtrAsInt(&Member(Arena_Node,arena)) )
#endif
 node->ref_counter -= 1;
 if (node->ref_counter == 0){
  // TODO(allen): make a version of clear that keeps memory allocated from the sytem level
  // but still resets to zero.
  arena_clear(&node->arena);
  zdll_remove(tctx->used_first, tctx->used_last, node);
  sll_stack_push(tctx->free_arenas, node);
 }
}

#define heap__sent_init(s) (s)->next=(s)->prev=(s)
#define heap__insert_next(p,n) ((n)->next=(p)->next,(n)->prev=(p),(n)->next->prev=(n),(p)->next=(n))
#define heap__insert_prev(p,n) ((n)->prev=(p)->prev,(n)->next=(p),(n)->prev->next=(n),(p)->prev=(n))
#define heap__remove(n) ((n)->next->prev=(n)->prev,(n)->prev->next=(n)->next)

#if defined(DO_HEAP_CHECKS)
function void
heap_assert_good(Heap *heap){
 if (heap->in_order.next != 0){
  Assert(heap->in_order.prev != 0);
  Assert(heap->free_nodes.next != 0);
  Assert(heap->free_nodes.prev != 0);
  for (Heap_Basic_Node *node = &heap->in_order;;){
   Assert(node->next->prev == node);
   Assert(node->prev->next == node);
   node = node->next;
   if (node == &heap->in_order){
    break;
   }
  }
  for (Heap_Basic_Node *node = &heap->free_nodes;;){
   Assert(node->next->prev == node);
   Assert(node->prev->next == node);
   node = node->next;
   if (node == &heap->free_nodes){
    break;
   }
  }
 }
}
#else
#define heap_assert_good(heap) ((void)(heap))
#endif

function void
heap_init(Heap *heap, Base_Allocator *allocator){
 heap->arena_ = make_arena(allocator);
 heap->arena = &heap->arena_;
 heap__sent_init(&heap->in_order);
 heap__sent_init(&heap->free_nodes);
 heap->used_space = 0;
 heap->total_space = 0;
}

function void
heap_init(Heap *heap, Arena *arena){
 heap->arena = arena;
 heap__sent_init(&heap->in_order);
 heap__sent_init(&heap->free_nodes);
 heap->used_space = 0;
 heap->total_space = 0;
}

function Base_Allocator*
heap_get_base_allocator(Heap *heap){
 return(heap->arena->base_allocator);
}

function void
heap_free_all(Heap *heap){
 if (heap->arena == &heap->arena_){
  arena_clear(heap->arena);
 }
 block_zero_struct(heap);
}

function void
heap__extend(Heap *heap, void *memory, u64 size){
 heap_assert_good(heap);
 if (size >= sizeof(Heap_Node)){
  Heap_Node *new_node = (Heap_Node*)memory;
  heap__insert_prev(&heap->in_order, &new_node->order);
  heap__insert_next(&heap->free_nodes, &new_node->alloc);
  new_node->size = size - sizeof(*new_node);
  heap->total_space += size;
 }
 heap_assert_good(heap);
}

function void
heap__extend_automatic(Heap *heap, u64 size){
 void *memory = push_array(heap->arena, u8, size);
 heap__extend(heap, memory, size);
}

function void*
heap__reserve_chunk(Heap *heap, Heap_Node *node, u64 size){
 u8 *ptr = (u8*)(node + 1);
 Assert(node->size >= size);
 u64 left_over_size = node->size - size;
 if (left_over_size > sizeof(*node)){
  u64 new_node_size = left_over_size - sizeof(*node);
  Heap_Node *new_node = (Heap_Node*)(ptr + size);
  heap__insert_next(&node->order, &new_node->order);
  heap__insert_next(&node->alloc, &new_node->alloc);
  new_node->size = new_node_size;
 }
 heap__remove(&node->alloc);
 node->alloc.next = 0;
 node->alloc.prev = 0;
 node->size = size;
 heap->used_space += sizeof(*node) + size;
 return(ptr);
}

function void*
heap_allocate(Heap *heap, u64 size)
{
 b32 first_try = true;
 for (;;)
 {
  if (heap->in_order.next != 0)
  {
   heap_assert_good(heap);
   u64 aligned_size = (size + sizeof(Heap_Node) - 1);
   aligned_size = aligned_size - (aligned_size%sizeof(Heap_Node));
   for (Heap_Basic_Node *n = heap->free_nodes.next;
        n != &heap->free_nodes;
        n = n->next){
    Heap_Node *node = CastFromMember(Heap_Node, alloc, n);
    if (node->size >= aligned_size){
     void *ptr = heap__reserve_chunk(heap, node, aligned_size);
     heap_assert_good(heap);
     return(ptr);
    }
   }
   heap_assert_good(heap);
  }
  
  if (first_try){
   u64 extension_size = clamp_min(KB(64), size*2);
   heap__extend_automatic(heap, extension_size);
   first_try = false;
  }
  else{
   break;
  }
 }
 return(0);
}

function void
heap__merge(Heap *heap, Heap_Node *l, Heap_Node *r){
 if (&l->order != &heap->in_order && &r->order != &heap->in_order &&
     l->alloc.next != 0 && l->alloc.prev != 0 &&
     r->alloc.next != 0 && r->alloc.prev != 0){
  u8 *ptr = (u8*)(l + 1) + l->size;
  if (PtrDif(ptr, r) == 0){
   heap__remove(&r->order);
   heap__remove(&r->alloc);
   heap__remove(&l->alloc);
   l->size += r->size + sizeof(*r);
   heap__insert_next(&heap->free_nodes, &l->alloc);
  }
 }
}

function void
heap_free(Heap *heap, void *memory){
 if (heap->in_order.next != 0 && memory != 0){
  Heap_Node *node = ((Heap_Node*)memory) - 1;
  Assert(node->alloc.next == 0);
  Assert(node->alloc.prev == 0);
  heap->used_space -= sizeof(*node) + node->size;
  heap_assert_good(heap);
  heap__insert_next(&heap->free_nodes, &node->alloc);
  heap_assert_good(heap);
  heap__merge(heap, node, CastFromMember(Heap_Node, order, node->order.next));
  heap_assert_good(heap);
  heap__merge(heap, CastFromMember(Heap_Node, order, node->order.prev), node);
  heap_assert_good(heap);
 }
}

#define heap_array(heap, T, c) (T*)(heap_allocate((heap), sizeof(T)*(c)))

////////////////////////////////

function void*
base_reserve__heap(void *user_data, u64 size, u64 *size_out, String location){
 Heap *heap = (Heap*)user_data;
 void *memory = heap_allocate(heap, size);
 *size_out = size;
 return(memory);
}

function void
base_free__heap(void *user_data, void *ptr){
 Heap *heap = (Heap*)user_data;
 heap_free(heap, ptr);
}

function Base_Allocator
base_allocator_on_heap(Heap *heap){
 return(make_base_allocator_generic(base_reserve__heap, 0, 0, base_free__heap, 0, heap));
}

////////////////////////////////

function String8
push_data(Arena *arena, u64 size){
 String result = {};
 result.str = push_array(arena, u8, size);
 result.size = size;
 return(result);
}

function String
push_string8(Arena *arena, String data){
 String8 result = {
  .str = push_array_copy(arena, u8, data.size, data.str),
  .len = data.len,
 };
 return(result);
}
function String
push_string(Arena *arena, String data){
 return push_string8(arena, data);
}
//
force_inline String8
push_string(Arena *arena, const char *data){
 return(push_string8(arena, SCu8(data)));
}

inline b32
string_match(String a, String b)
{
 return(a.size == b.size &&
        block_match(a.str, b.str, a.size));
}
//
force_inline bool
operator==(String a, String b)
{
 return string_match(a,b);
}
//
force_inline bool
operator==(String a, const char *b)
{
 return a == strlit(b);
}

force_inline b32
string_match(char *a, char *b)
{
 return gb_strcmp(a,b) == 0;
}

function b32
character_is_whitespace(char c){
 return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}
force_inline b32
character_is_whitespace(u8 c){
 return(character_is_whitespace(char(c)));
}

function u64
string_find_first_whitespace(String str){
 u64 i = 0;
 for (;i < str.size && !character_is_whitespace(str.str[i]); i += 1);
 return(i);
}
function i64
string_find_last_whitespace(String str){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && !character_is_whitespace(str.str[i]); i -= 1);
 return(i);
}

function i64
string_find_last_non_whitespace(String str){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && character_is_whitespace(str.str[i]); i -= 1);
 return(i);
}

function b32
character_is_slash(char c){
 return((c == '/') || (c == '\\'));
}
force_inline b32 character_is_slash(u8 c){ return(character_is_slash(char(c))); }

function u64
string_find_first_slash(String str){
 u64 i = 0;
 for (;i < str.size && !character_is_slash(str.str[i]); i += 1);
 return(i);
}
function i64
string_find_last_slash(String str){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && !character_is_slash(str.str[i]); i -= 1);
 return(i);
}

function String
path_dirname(String str)
{
 if (str.size > 0)
 {// NOTE: Remove the last slash
  str.size -= 1;
 }
 i64 slash_pos = string_find_last_slash(str);
 
 str.size = 0;
 if (slash_pos >= 0) str.size = slash_pos + 1;
 
 return(str);
}
function String
string_skip(String str, u64 n){
 n = clamp_max(n, str.size);
 str.str += n;;
 str.size -= n;
 return(str);
}

function String
path_filename(String str)
{
 i64 slash_pos = string_find_last_slash(str);
 if (slash_pos >= 0)
 {
  str = string_skip(str, slash_pos + 1);
 }
 return(str);
}

function i64
string_find_last(String str, u8 c){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && c != str.str[i]; i -= 1);
 return(i);
}

function String
path_extension(String string){
 return(string_skip(string, string_find_last(string, '.') + 1));
}

function String
string_prefix(String str, u64 size){
 size = clamp_max(size, str.size);
 str.size = size;
 return(str);
}

function String
path_no_extension(String string){
 i64 pos = string_find_last(string, '.');
 if(pos > 0){
  string = string_prefix(string, pos);
 }
 return(string);
}

inline String
SCu8(u8 *str, u8 *one_past_last){
 return(SCu8(str, (u64)(one_past_last - str)));
}
//
force_inline String
SCu8(char *str, u64 length){
 return(SCu8((u8*)str, length));
}
//
force_inline String
SCu8(char *first, char *one_past_last){
 return(SCu8((u8*)first, (u8*)one_past_last));
}

function String
string_substring(String str, Range_i64 range)
{
 return SCu8(str.str+range.min,
             str.str+range.max);
}

function b32
string_contains(String big, String small, i1 *first_match=0)
{
 b32 result = false;
 if (small.len == 0)
 {
  result = big.len == 0;
 }
 else
 {
  i1 lendiff = i1(big.len - small.len);
  for_i1(index,0,lendiff+1)
  {
   if (big.str[index] == small.str[0])
   {
    String substring = string_substring(big, Ii64_size(index, small.len));
    if (substring == small)
    {
     result = true;
     if (first_match) { *first_match = index; }
     break;
    }
   }
  }
 }
 return result;
}

internal b32
starts_with(String str, String prefix)
{
 return string_match(string_prefix(str, prefix.size), prefix);
}
#define starts_with_lit(string,prefix) starts_with(string, strlit(prefix))

#define string_match_lit(a,b)  string_match(a,strlit(b))

//-

function String
push_stringfv(Arena *arena, char *format, va_list args, b32 zero_terminated)
{
 va_list args2;
 va_copy(args2, args);
 
 i32 pushed_size = vsnprintf(0, 0, format, args);
 // NOTE(kv): vsnprintf is always terminated, and it won't print unless you reserve the buffer for nil-termination
 pushed_size++;
 
 String result = push_data(arena, pushed_size);
 vsnprintf((char*)result.str, (size_t)result.size, format, args2);
 
 result.size -= 1;
 if (zero_terminated)
 {
  result.str[result.size] = 0;
 }
 else
 {// NOTE: Enable string concatenation
  // NOTE(kv): This is legal since the string has gotta be contiguous
  arena_pop(arena, 1);
 }
 
 return(result);
}

inline u32
cast_u64_to_u32(u64 u)
{
 kv_assert(u < (1ULL << 32));
 return (u32)u;
}

//~eof