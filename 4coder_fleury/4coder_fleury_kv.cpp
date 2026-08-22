// NOTE(kv): This is the part the fleury layer that matters to my layer.

#pragma once

#include "4coder_fleury_ubiquitous.cpp"
#include "4coder_fleury_index.cpp"

// TODO(rjf): This is only being used to check if a font file exists because
// there's a bug in try_create_new_face that crashes the program if a font is
// not found. This function is only necessary until that is fixed.
function b32
IsFileReadable(String path)
{
    b32 result = 0;
    FILE *file = fopen((char *)path.str, "r");
    if(file)
    {
        result = 1;
        fclose(file);
    }
    return result;
}

function DELTA_RULE_SIG(F4_DeltaRule_lite)
{
    v2 *velocity = (v2*)data;
    if(velocity->x == 0.f)
    {
        velocity->x = 1.f;
        velocity->y = 1.f;
    }
    Smooth_Step step_x = smooth_camera_step(pending.x, velocity->x, 80.f, 1.f/4.f);
    Smooth_Step step_y = smooth_camera_step(pending.y, velocity->y, 80.f, 1.f/4.f);
    *velocity = V2(step_x.v, step_y.v);
    return(V2(step_x.p, step_y.p));
}

// NOTE(kv) Temp structures
struct AST_Node0
{
 AST_Node0 *parent;
 i32 children_count;
 AST_Node0 *first_child;
 AST_Node0 *last_child;
 AST_Node0 *next;
 
 union
 {
  AST_Data data;
  struct { AST_DATA_FIELDS };
 };
};
struct Buffer_AST0
{
 i32 node_count;
 AST_Node0 root;
};

function b32
is_valid(Buffer_AST *ast)
{
 return ast and ast->up_to_date;
}

function Buffer_AST0
compute_tmp_buffer_ast(Arena *arena, Token_Array *tokens)
{
 Buffer_AST0 result = {};
 
 // NOTE(kv) current_node_is_not_null
 AST_Node0 *current_node = &result.root;
 
 for(Token *token = tokens->tokens;
     token != tokens->tokens + tokens->count;
     token++)
 {
  switch(token->kind)
  {
   case TokenBaseKind_ScopeOpen:
   case TokenBaseKind_ParenOpen:
   {// NOTE(kv) New node!
    AST_Node0 *node = push_struct0(arena, AST_Node0);
    result.node_count++;
    node->token_begin = i32(token - tokens->tokens);
    
    // NOTE(kv) Add child to parent.
    AST_Node0 *parent = current_node;
    
    if(parent->last_child)
    {
     parent->last_child->next = node;
    }
    else
    {
     parent->first_child = node;
    }
    
    parent->last_child = node;
    parent->children_count++;
    node->parent = parent;
    
    current_node = node;
   } break;
   
   case TokenBaseKind_ScopeClose:
   case TokenBaseKind_ParenClose:
   {
    if(current_node->parent)
    {// NOTE(kv) Normal case
     // NOTE(kv) I guess we don't care about exact grouper character?
     // Not sure if that's good, or maybe we *should* care?
     current_node->token_end = i32(token - tokens->tokens);
     current_node = current_node->parent;
    }
    else
    {// NOTE(kv) Invalid case, but we deal with it!
    }
   } break;
  }
 }
 
 return result;
}
function Buffer_AST
persist_buffer_ast(Buffer_AST0 *ast0, AST_Node *dest_nodes)
{
 Buffer_AST ast = {};
 ast.nodes = dest_nodes;
 ast.up_to_date = 1;
 i32 dest_index = 0;
 
 AST_Node0 *source = &ast0->root;
 AST_Node *dest = &ast.root;
 
 while(1)
 {
  if(source->children_count > 0)
  {// NOTE Children
   AST_Node *dest_children = dest_nodes + dest_index;
   dest_index += source->children_count;
   dest->children = {dest_children, source->children_count};
   
   i32 child_index = 0;
   for(AST_Node0 *source_child = source->first_child;
       source_child;
       source_child = source_child->next)
   {
    AST_Node *dest_child = &dest_children[child_index++];
    dest_child->parent = dest;
    dest_child->data = source_child->data;
   }
   
   source = source->first_child;
   dest = &dest->children[0];
  }
  else
  {// NOTE We're done with "source"
   while(source != 0)
   {
    if(source->next)
    {// NOTE Go to next sibling
     source = source->next;
     dest++;  // NOTE(kv) Because siblings are in an array
     break;
    }
    else
    {// NOTE Go up
     source = source->parent;
     dest = dest->parent;
    }
   }
   
   if(source == 0)
   {
    break;
   }
  }
 }
 
 kv_assert(dest_index == ast0->node_count);
 return ast;
}
myinline Buffer_AST *
get_buffer_ast(App *app, Managed_Scope scope)
{
 Buffer_AST *ast = scope_attachment(app, scope, buffer_attachment_ast, Buffer_AST);
 return ast;
}
function Buffer_AST *
get_or_compute_buffer_ast(App *app, Buffer_ID buffer, Token_Array *tokens)
{
 Scratch_Block tmp;
 // NOTE(kv) Compute the AST
 Managed_Scope scope = buffer_get_managed_scope(app, buffer);
 kv_assert(scope != 0);
 Buffer_AST *ast = get_buffer_ast(app, scope);
 if(not ast->up_to_date)
 {
  Base_Allocator *allocator = managed_scope_allocator(app, scope);
  base_free(allocator, ast->nodes);
  zero_struct(ast);
  
  Buffer_AST0 ast0 = compute_tmp_buffer_ast(tmp, tokens);
  
  AST_Node *dest_nodes = base_array(allocator, AST_Node, ast0.node_count);
  *ast = persist_buffer_ast(&ast0, dest_nodes);
 }
 
 return ast;
}

function void
F4_DoFullLex_ASYNC_Inner(Async_Context *actx, Buffer_ID buffer)
{
 App *app = actx->app;
 ProfileBlock( "[F4] Async Lex");
 Scratch_Block tmp;
 
 Stringz contents = {};
 {
  ProfileBlock( "[F4] Async Lex Contents (before mutex)");
  acquire_global_frame_mutex(app);
  ProfileBlock( "[F4] Async Lex Contents (after mutex)");
  contents = push_whole_buffer(app, tmp, buffer);
  release_global_frame_mutex(app);
 }
 
 i1 limit_factor = 10000;
 
 Token_List list = {};
 b32 canceled = false;
 
 F4_Language *language = F4_LanguageFromBuffer(app, buffer);
 
 // NOTE(rjf): Fall back to C++ if we don't have a proper language.
 if(language == 0)
 {
  language = F4_LanguageFromExtension(S8Lit("cpp"));
 }
 
 if(language != 0)
 {
  void *lexing_state = push_array0(tmp, u8, language->lex_state_size);
  language->LexInit(lexing_state, contents);
  for(;;)
  {
   ProfileBlock( "[F4] Async Lex Block");
   
   b32 done = language->LexFullInput(tmp, &list, lexing_state, limit_factor);
   if(done)
   {
    break;
   }
   
   if(async_check_canceled(actx))
   {
    canceled = true;
    break;
   }
  }
 }
 
 if(not canceled)
 {
  ProfileBlock( "[F4] Async Lex Save Results (before mutex)");
  acquire_global_frame_mutex(app);
  ProfileBlock( "[F4] Async Lex Save Results (after mutex)");
  Managed_Scope scope = buffer_get_managed_scope(app, buffer);
  if(scope != 0)
  {
   Base_Allocator *allocator = managed_scope_allocator(app, scope);
   Token_Array *tokens = scope_attachment(app, scope, attachment_tokens, Token_Array);
   base_free(allocator, tokens->tokens);
   *tokens = {};
   tokens->tokens = base_array(allocator, Token, list.total_count);
   tokens->count = list.total_count;
   tokens->max = list.total_count;
   token_fill_memory_from_list(tokens->tokens, &list);
  }
  buffer_mark_as_modified(buffer);
  release_global_frame_mutex(app);
 }
}

function void
F4_DoFullLex_ASYNC(Async_Context *actx, String data)
{
 if(data.size == sizeof(Buffer_ID))
 {
  Buffer_ID buffer = *(Buffer_ID*)data.str;
  F4_DoFullLex_ASYNC_Inner(actx, buffer);
 }
}
function Async_Task *
submit_full_lex_work(App *app, Buffer_ID buffer)
{
 Managed_Scope scope = buffer_get_managed_scope(app, buffer);
 Async_Task *lex_task = scope_attachment(app, scope, buffer_lex_task, Async_Task);
 *lex_task = async_task_no_dep(&global_async_system, F4_DoFullLex_ASYNC, make_data_struct(&buffer));
 return lex_task;
}
function i32
F4_BufferEditRange(App *app, Buffer_ID buffer, Range_i64 new_range, Range_Cursor old_cursor_range, b32 automated)
{// NOTE(kv) see also @kv_begin_buffer
 // buffer_id, new_range, original_size
 ProfileBlock( "[F4] Buffer Edit Range");
 
 Range_i64 old_range = Ii64(old_cursor_range.min.pos, old_cursor_range.max.pos);
 
 buffer_shift_fade_ranges(buffer, old_range.max, (new_range.max - old_range.max));
 
 {
  code_index_lock();
  Code_Index_File *file = code_index_get_file(buffer);
  if(file != 0)
  {
   code_index_shift(file, old_range, range_size(new_range));
  }
  code_index_unlock();
 }
 
 i64 insert_size = range_size(new_range);
 i64 text_shift = replace_range_shift(old_range, insert_size);
 
 Scratch_Block tmp;
 
 Managed_Scope scope = buffer_get_managed_scope(app, buffer);
 
 {// NOTE(kv) Invalidate the AST
  Buffer_AST *ast = get_buffer_ast(app, scope);
  ast->up_to_date = 0;
 }
 
 Async_Task *lex_task = scope_attachment(app, scope, buffer_lex_task, Async_Task);
 
 Base_Allocator *allocator = managed_scope_allocator(app, scope);
 b32 do_full_relex = false;
 
 if(async_task_is_running_or_pending(&global_async_system, *lex_task))
 {// NOTE Invalidate running lex task.
  async_task_cancel(app, &global_async_system, *lex_task);
  buffer_unmark_as_modified(buffer);
  // NOTE(kv) Since this lex task is a "full relex",
  // we just do another full relex.
  do_full_relex = true;
  *lex_task = 0;
 }
 else
 {// NOTE(kv) Attempt relex.
  Token_Array *ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
  if (ptr != 0 and ptr->tokens != 0)
  {
   ProfileBegin( "attempt resync");
   
   i64 token_index_first = token_relex_first(ptr, old_range.first, 1);
   i64 token_index_resync_guess = token_relex_resync(ptr, old_range.opl, 16);
   
   if(token_index_resync_guess - token_index_first >= 4000)
   {
    do_full_relex = true;
   }
   else
   {
    Token *token_first = ptr->tokens + token_index_first;
    Token *token_resync = ptr->tokens + token_index_resync_guess;
    
    Range_i64 relex_range = Ii64(token_first->pos, token_resync->pos + token_resync->size + text_shift);
    Stringz partial_text = push_buffer_range(app, tmp, buffer, relex_range);
    
    //~ NOTE(rjf): Lex
    F4_Language *language = F4_LanguageFromBuffer(app, buffer);
    // NOTE(rjf): Fall back to C++ if we don't have a proper language.
    if(language == 0)
    {
     language = F4_LanguageFromExtension(S8Lit("cpp"));
    }
    Token_List relex_list = F4_Language_LexFullInput_NoBreaks(app, language, tmp, partial_text);
    //~
    
    if (relex_range.opl < buffer_get_size(app, buffer)){
     token_drop_eof(&relex_list);
    }
    
    Token_Relex relex = token_relex(relex_list, relex_range.first - text_shift, ptr->tokens, token_index_first, token_index_resync_guess);
    
    ProfileEnd();
    
    if (relex.successful_resync)
    {
     ProfileBlock( "apply resync");
     
     i64 token_index_resync = relex.first_resync_index;
     
     Range_i64 head = Ii64(0, token_index_first);
     Range_i64 replaced = Ii64(token_index_first, token_index_resync);
     Range_i64 tail = Ii64(token_index_resync, ptr->count);
     i64 resynced_count = (token_index_resync_guess + 1) - token_index_resync;
     i64 relexed_count = relex_list.total_count - resynced_count;
     i64 tail_shift = relexed_count - (token_index_resync - token_index_first);
     
     i64 new_tokens_count = ptr->count + tail_shift;
     Token *new_tokens = base_array(allocator, Token, new_tokens_count);
     
     Token *old_tokens = ptr->tokens;
     block_copy_array_dst_shift(new_tokens, old_tokens, head, 0);
     token_fill_memory_from_list(new_tokens + replaced.first, &relex_list, relexed_count);
     for (i64 i = 0, index = replaced.first; i < relexed_count; i += 1, index += 1){
      new_tokens[index].pos += relex_range.first;
     }
     for (i64 i = tail.first; i < tail.opl; i += 1){
      old_tokens[i].pos += text_shift;
     }
     block_copy_array_dst_shift(new_tokens, ptr->tokens, tail, tail_shift);
     
     base_free(allocator, ptr->tokens);
     
     ptr->tokens = new_tokens;
     ptr->count = new_tokens_count;
     ptr->max = new_tokens_count;
     
     buffer_mark_as_modified(buffer);
    }
    else { do_full_relex = true; }
   }
  }
 }
 
 if(do_full_relex)
 {
  print_message(app, SCu8("!!!do full relex!!!\n"));
  submit_full_lex_work(app, buffer);
 }
 
 // no meaning for return
 return(0);
}
//-