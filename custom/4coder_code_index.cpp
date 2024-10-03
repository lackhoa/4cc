/*
4coder_code_index.cpp - Generic code indexing system for layout, definition jumps, etc.
*/

// TOP


global Code_Index global_code_index = {};

////////////////////////////////
// NOTE(allen): Lookups

// TODO(allen): accelerator for these nest lookups?
// Looks like the only one I ever actually use is the file one, not the array one.
function Code_Index_Nest*
code_index_get_nest_(Code_Index_Nest_Ptr_Array *array, i64 pos){
  Code_Index_Nest *result = 0;
  i1 count = array->count;
  Code_Index_Nest **nest_ptrs = array->ptrs;
  for (i1 i = 0; i < count; i += 1){
    Code_Index_Nest *nest = nest_ptrs[i];
    if (nest->open.max <= pos && pos <= nest->close.min){
      Code_Index_Nest *sub_nest = code_index_get_nest_(&nest->nest_array, pos);
      if (sub_nest != 0){
        result = sub_nest;
      }
      else{
        result = nest;
      }
      break;
    }
  }
  return(result);
}

function Code_Index_Nest*
code_index_get_nest(Code_Index_File *file, i64 pos)
{
    return(code_index_get_nest_(&file->nest_array, pos));
}

function Code_Index_Note_List*
code_index__list_from_string(String string)
{
    u64 hash = table_hash_u8(string.str, string.size);
    Code_Index_Note_List *result = &global_code_index.name_hash[hash % ArrayCount(global_code_index.name_hash)];
    return(result);
}

function Code_Index_Note*
code_index_note_from_string(String string)
{
    Code_Index_Note_List *list = code_index__list_from_string(string);
    Code_Index_Note *result = 0;
    for (Code_Index_Note *node = list->first;
         node != 0;
         node = node->next_in_hash)
    {
        if (string_match(string, node->text))
        {
            result = node;
            break;
        }
    }
    return(result);
}


////////////////////////////////
// NOTE(allen): Global Code Index

function void
code_index_init(void){
  global_code_index.mutex = system_mutex_make();
  global_code_index.node_arena = make_arena_system(KB(4));
  global_code_index.buffer_to_index_file = make_table_u64_u64(global_code_index.node_arena.base_allocator, 500);
}

function Code_Index_File_Storage*
code_index__alloc_storage(void){
  Code_Index_File_Storage *result = global_code_index.free_storage;
  if (result == 0){
    result = push_array_zero(&global_code_index.node_arena, Code_Index_File_Storage, 1);
  }
  else{
    sll_stack_pop(global_code_index.free_storage);
  }
  zdll_push_back(global_code_index.storage_first, global_code_index.storage_last, result);
  global_code_index.storage_count += 1;
  return(result);
}

function void
code_index__free_storage(Code_Index_File_Storage *storage){
  zdll_remove(global_code_index.storage_first, global_code_index.storage_last, storage);
  global_code_index.storage_count -= 1;
  sll_stack_push(global_code_index.free_storage, storage);
}

function void
code_index_push_nest(Code_Index_Nest_List *list, Code_Index_Nest *nest){
  sll_queue_push(list->first, list->last, nest);
  list->count += 1;
}

function Code_Index_Nest_Ptr_Array
code_index_nest_ptr_array_from_list(Arena *arena, Code_Index_Nest_List *list){
  Code_Index_Nest_Ptr_Array array = {};
  array.ptrs = push_array_zero(arena, Code_Index_Nest*, list->count);
  array.count = list->count;
  i1 counter = 0;
  for (Code_Index_Nest *node = list->first;
       node != 0;
       node = node->next){
    array.ptrs[counter] = node;
    counter += 1;
  }
  return(array);
}

function Code_Index_Note_Ptr_Array
code_index_note_ptr_array_from_list(Arena *arena, Code_Index_Note_List *list){
  Code_Index_Note_Ptr_Array array = {};
  array.ptrs = push_array_zero(arena, Code_Index_Note*, list->count);
  array.count = list->count;
  i1 counter = 0;
  for (Code_Index_Note *node = list->first;
       node != 0;
       node = node->next){
    array.ptrs[counter] = node;
    counter += 1;
  }
  return(array);
}

function void
code_index_lock(void){
  system_mutex_acquire(global_code_index.mutex);
}

function void
code_index_unlock(void){
 system_mutex_release(global_code_index.mutex);
}

function void
code_index__hash_file(Code_Index_File *file){
 for (Code_Index_Note *node = file->note_list.first;
      node != 0;
      node = node->next){
  Code_Index_Note_List *list = code_index__list_from_string(node->text);
  zdll_push_back_NP_(list->first, list->last, node, next_in_hash, prev_in_hash);
  list->count += 1;
 }
}

function void
code_index__clear_file(Code_Index_File *file){
 for (Code_Index_Note *node = file->note_list.first;
      node != 0;
      node = node->next){
  Code_Index_Note_List *list = code_index__list_from_string(node->text);
  zdll_remove_NP_(list->first, list->last, node, next_in_hash, prev_in_hash);
  list->count -= 1;
 }
}

function void
code_index_set_file(Buffer_ID buffer, Arena arena, Code_Index_File *index){
 Code_Index_File_Storage *storage = 0;
 Table_Lookup lookup = table_lookup(&global_code_index.buffer_to_index_file, buffer);
 if (lookup.found_match){
  u64 val = 0;
  table_read(&global_code_index.buffer_to_index_file, lookup, &val);
  storage = (Code_Index_File_Storage*)IntAsPtr(val);
  code_index__clear_file(storage->file);
  arena_clear(&storage->arena);
 }
 else{
  storage = code_index__alloc_storage();
  table_insert(&global_code_index.buffer_to_index_file, buffer, (u64)PtrAsInt(storage));
 }
 storage->arena = arena;
 storage->file = index;
 
 code_index__hash_file(index);
}

function void
code_index_erase_file(Buffer_ID buffer){
  Table_Lookup lookup = table_lookup(&global_code_index.buffer_to_index_file, buffer);
  if (lookup.found_match){
    u64 val = 0;
    table_read(&global_code_index.buffer_to_index_file, lookup, &val);
    Code_Index_File_Storage *storage = (Code_Index_File_Storage*)IntAsPtr(val);
    
    code_index__clear_file(storage->file);
    
    arena_clear(&storage->arena);
    table_erase(&global_code_index.buffer_to_index_file, lookup);
    code_index__free_storage(storage);
  }
}

function Code_Index_File*
code_index_get_file(Buffer_ID buffer){
  Code_Index_File *result = 0;
  Table_Lookup lookup = table_lookup(&global_code_index.buffer_to_index_file, buffer);
  if (lookup.found_match){
    u64 val = 0;
    table_read(&global_code_index.buffer_to_index_file, lookup, &val);
    Code_Index_File_Storage *storage = (Code_Index_File_Storage*)IntAsPtr(val);
    result = storage->file;
  }
  return(result);
}

function void
index_shift(i64 *ptr, Range_i64 old_range, u64 new_size){
  i64 i = *ptr;
  if (old_range.min <= i && i < old_range.max){
    *ptr = old_range.first;
  }
  else if (old_range.max <= i){
    *ptr = i + new_size - (old_range.max - old_range.min);
  }
}

function void
code_index_shift(Code_Index_Nest_Ptr_Array *array,
                 Range_i64 old_range, u64 new_size){
  i1 count = array->count;
  Code_Index_Nest **nest_ptr = array->ptrs;
  for (i1 i = 0; i < count; i += 1, nest_ptr += 1){
    Code_Index_Nest *nest = *nest_ptr;
    index_shift(&nest->open.min, old_range, new_size);
    index_shift(&nest->open.max, old_range, new_size);
    if (nest->is_closed){
      index_shift(&nest->close.min, old_range, new_size);
      index_shift(&nest->close.max, old_range, new_size);
    }
    code_index_shift(&nest->nest_array, old_range, new_size);
  }
}

function void
code_index_shift(Code_Index_File *file, Range_i64 old_range, u64 new_size){
  code_index_shift(&file->nest_array, old_range, new_size);
}


////////////////////////////////
// NOTE(allen): Parser Helpers

function void
generic_parse_inc(Generic_Parse_State *state){
  if (!tkarr_inc_all(&state->it)){
    state->finished = true;
  }
}

function void
generic_parse_skip_soft_tokens(Code_Index_File *index, Generic_Parse_State *state){
  Token *token = tkarr_read(&state->it);
  for (;token != 0 && !state->finished;){
    if (state->in_preprocessor && !HasFlag(token->flags, TokenBaseFlag_PreprocessorBody)){
      break;
    }
    if (token->kind == TokenBaseKind_Comment){
      state->handle_comment(state->app, state->arena, index, token, state->contents);
    }
    else if (token->kind == TokenBaseKind_Whitespace){
      Range_i64 range = Ii64(token);
      u8 *ptr = state->contents.str + range.opl - 1;
      for (i64 i = range.opl - 1;
           i >= range.first;
           i -= 1, ptr -= 1){
        if (*ptr == '\n'){
          state->prev_line_start = ptr + 1;
          break;
        }
      }
    }
    else{
      break;
    }
    generic_parse_inc(state);
    token = tkarr_read(&state->it);
  }
}

function void
generic_parse_init(App *app, Arena *arena, String contents, Token_Array *tokens, Generic_Parse_Comment_Function *handle_comment, Generic_Parse_State *state){
  state->app = app;
  state->arena = arena;
  state->contents = contents;
  state->it = token_iterator(0, tokens);
  state->handle_comment = handle_comment;
  state->prev_line_start = contents.str;
}

////////////////////////////////
// NOTE(allen): Parser

/*
// NOTE(allen): grammar syntax
(X) = X
X Y = X and then Y
X? = zero or one X
$X = check for X but don't consume
[X] = zero or more Xs
X | Y = either X or Y
* = anything that does not match previous options in a X | Y | ... chain
* - X = anything that does not match X or previous options in a Y | Z | ... chain
<X> = a token of type X
"X" = literally the string "X"
X{Y} = X with flag Y

// NOTE(allen): grammar of code index parse
file: [preprocessor | scope | parens | function | type | * - <end-of-file>] <end-of-file>
preprocessor: <preprocessor> [scope | parens | stmnt]{pp-body}
scope: <scope-open> [preprocessor | scope | parens | * - <scope-close>] <scope-close>
paren: <paren-open> [preprocessor | scope | parens | * - <paren-close>] <paren-close>
stmnt-close-pattern: <scope-open> | <scope-close> | <paren-open> | <paren-close> | <stmnt-close> | <preprocessor>
stmnt: [type | * - stmnt-close-pattern] stmnt-close-pattern
type: struct | union | enum | typedef
struct: "struct" <identifier> $(";" | "{")
union: "union" <identifier> $(";" | "{")
enum: "enum" <identifier> $(";" | "{")
typedef: "typedef" [* - (<identifier> (";" | "("))] <identifier> $(";" | "(")
function: <identifier> >"(" ["(" ")" | * - ("(" | ")")] ")" ("{" | ";")
*/

function Code_Index_Note*
index_new_note(Code_Index_File *index, Generic_Parse_State *state, Range_i64 range, Code_Index_Note_Kind kind, Code_Index_Nest *parent){
  Code_Index_Note *result = push_array(state->arena, Code_Index_Note, 1);
  sll_queue_push(index->note_list.first, index->note_list.last, result);
  index->note_list.count += 1;
  result->note_kind = kind;
  result->pos = range;
  result->text = push_stringz(state->arena, string_substring(state->contents, range));
  result->file = index;
  result->parent = parent;
  return(result);
}

function void
cpp_parse_type_structure(Code_Index_File *index, Generic_Parse_State *state, Code_Index_Nest *parent)
{
    generic_parse_inc(state);
    generic_parse_skip_soft_tokens(index, state);
    if (state->finished){
        return;
    }
    Token *token = tkarr_read(&state->it);
    if (token != 0 && token->kind == TokenBaseKind_Identifier){
        generic_parse_inc(state);
        generic_parse_skip_soft_tokens(index, state);
        Token *peek = tkarr_read(&state->it);
        if (peek != 0 && 
            (peek->kind == TokenBaseKind_StatementClose || peek->kind == TokenBaseKind_ScopeOpen))
        {
            index_new_note(index, state, Ii64(token), CodeIndexNote_Type, parent);
        }
    }
}

function void
cpp_parse_type_def(Code_Index_File *index, Generic_Parse_State *state, Code_Index_Nest *parent){
  generic_parse_inc(state);
  generic_parse_skip_soft_tokens(index, state);
  for (;;){
    b32 did_advance = false;
    Token *token = tkarr_read(&state->it);
    if (token == 0 || state->finished){
      break;
    }
    if (token->kind == TokenBaseKind_Identifier){
      generic_parse_inc(state);
      generic_parse_skip_soft_tokens(index, state);
      did_advance = true;
      Token *peek = tkarr_read(&state->it);
      if (peek != 0 && (peek->kind == TokenBaseKind_StatementClose || peek->kind == TokenBaseKind_ParenOpen)){
        index_new_note(index, state, Ii64(token), CodeIndexNote_Type, parent);
        break;
      }
    }
    else if (token->kind == TokenBaseKind_StatementClose ||
             token->kind == TokenBaseKind_ScopeOpen ||
             token->kind == TokenBaseKind_ScopeClose ||
             token->kind == TokenBaseKind_ScopeOpen ||
             token->kind == TokenBaseKind_ScopeClose){
      break;
    }
    else if (token->kind == TokenBaseKind_Keyword){
      String lexeme = string_substring(state->contents, Ii64(token));
      if (string_match(lexeme, string_u8_litexpr("struct")) ||
          string_match(lexeme, string_u8_litexpr("union")) ||
          string_match(lexeme, string_u8_litexpr("enum"))){
        break;
      }
    }
    if (!did_advance){
      generic_parse_inc(state);
      generic_parse_skip_soft_tokens(index, state);
    }
  }
}

function void
cpp_parse_function(Code_Index_File *index, Generic_Parse_State *state, Code_Index_Nest *parent){
  Token *token = tkarr_read(&state->it);
  generic_parse_inc(state);
  generic_parse_skip_soft_tokens(index, state);
  if (state->finished){
    return;
  }
  Token *peek = tkarr_read(&state->it);
  Token *reset_point = peek;
  if (peek != 0 && peek->sub_kind == TokenCppKind_ParenOp){
    b32 at_paren_close = false;
    i1 paren_nest_level = 0;
    for (; peek != 0;){
      generic_parse_inc(state);
      generic_parse_skip_soft_tokens(index, state);
      peek = tkarr_read(&state->it);
      if (peek == 0 || state->finished){
        break;
      }
      
      if (peek->kind == TokenBaseKind_ParenOpen){
        paren_nest_level += 1;
      }
      else if (peek->kind == TokenBaseKind_ParenClose){
        if (paren_nest_level > 0){
          paren_nest_level -= 1;
        }
        else{
          at_paren_close = true;
          break;
        }
      }
    }
    
    if (at_paren_close){
      generic_parse_inc(state);
      generic_parse_skip_soft_tokens(index, state);
      peek = tkarr_read(&state->it);
      if (peek != 0 &&
          (peek->kind == TokenBaseKind_ScopeOpen || peek->kind == TokenBaseKind_StatementClose)){
        index_new_note(index, state, Ii64(token), CodeIndexNote_Function, parent);
      }
    }
  }
  state->it = token_iterator(state->it.user_id, state->it.tokens, state->it.count, reset_point);
}

function Code_Index_Nest*
generic_parse_statement(Code_Index_File *index, Generic_Parse_State *state);

function Code_Index_Nest*
generic_parse_preprocessor(Code_Index_File *index, Generic_Parse_State *state);

function Code_Index_Nest*
generic_parse_scope(Code_Index_File *index, Generic_Parse_State *state);

function Code_Index_Nest*
generic_parse_paren(Code_Index_File *index, Generic_Parse_State *state);

function Code_Index_Nest*
generic_parse_statement(Code_Index_File *index, Generic_Parse_State *state){
  Token *token = tkarr_read(&state->it);
  Code_Index_Nest *result = push_array_zero(state->arena, Code_Index_Nest, 1);
  result->kind = CodeIndexNest_Statement;
  result->open = Ii64(token->pos);
  result->close = Ii64(max_i64);
  result->file = index;
  
  state->in_statement = true;
  
  for (;;){
    generic_parse_skip_soft_tokens(index, state);
    token = tkarr_read(&state->it);
    if (token == 0 || state->finished){
      break;
    }
    
    if (state->in_preprocessor){
      if (!HasFlag(token->flags, TokenBaseFlag_PreprocessorBody) ||
          token->kind == TokenBaseKind_Preprocessor){
        result->is_closed = true;
        result->close = Ii64(token->pos);
        break;
      }
    }
    else{
      if (token->kind == TokenBaseKind_Preprocessor){
        result->is_closed = true;
        result->close = Ii64(token->pos);
        break;
      }
    }
    
    if (token->kind == TokenBaseKind_ScopeOpen ||
        token->kind == TokenBaseKind_ScopeClose ||
        token->kind == TokenBaseKind_ParenOpen){
      result->is_closed = true;
      result->close = Ii64(token->pos);
      break;
    }
    
    if (token->kind == TokenBaseKind_StatementClose){
      result->is_closed = true;
      result->close = Ii64(token);
      generic_parse_inc(state);
      break;
    }
    
    generic_parse_inc(state);
  }
  
  state->in_statement = false;
  
  return(result);
}

function Code_Index_Nest*
generic_parse_preprocessor(Code_Index_File *index, Generic_Parse_State *state){
  Token *token = tkarr_read(&state->it);
  Code_Index_Nest *result = push_array_zero(state->arena, Code_Index_Nest, 1);
  result->kind = CodeIndexNest_Preprocessor;
  result->open = Ii64(token->pos);
  result->close = Ii64(max_i64);
  result->file = index;
  
  state->in_preprocessor = true;
  
  b32 potential_macro  = false;
  if (state->do_cpp_parse){
    if (token->sub_kind == TokenCppKind_PPDefine){
      potential_macro = true;
    }
  }
  
  generic_parse_inc(state);
  for (;;){
    generic_parse_skip_soft_tokens(index, state);
    token = tkarr_read(&state->it);
    if (token == 0 || state->finished){
      break;
    }
    
    if (!HasFlag(token->flags, TokenBaseFlag_PreprocessorBody) ||
        token->kind == TokenBaseKind_Preprocessor){
      result->is_closed = true;
      result->close = Ii64(token->pos);
      break;
    }
    
    if (state->do_cpp_parse && potential_macro){
      if (token->sub_kind == TokenCppKind_Identifier){
        index_new_note(index, state, Ii64(token), CodeIndexNote_Macro, result);
      }
      potential_macro = false;
    }
    
    if (token->kind == TokenBaseKind_ScopeOpen){
      Code_Index_Nest *nest = generic_parse_scope(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
      continue;
    }
    
    if (token->kind == TokenBaseKind_ParenOpen){
      Code_Index_Nest *nest = generic_parse_paren(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
      continue;
    }
    
    generic_parse_inc(state);
  }
  
  result->nest_array = code_index_nest_ptr_array_from_list(state->arena, &result->nest_list);
  
  state->in_preprocessor = false;
  
  return(result);
}

function Code_Index_Nest*
generic_parse_scope(Code_Index_File *index, Generic_Parse_State *state){
  Token *token = tkarr_read(&state->it);
  Code_Index_Nest *result = push_array_zero(state->arena, Code_Index_Nest, 1);
  result->kind = CodeIndexNest_Scope;
  result->open = Ii64(token);
  result->close = Ii64(max_i64);
  result->file = index;
  
  state->scope_counter += 1;
  
  generic_parse_inc(state);
  for (;;){
    generic_parse_skip_soft_tokens(index, state);
    token = tkarr_read(&state->it);
    if (token == 0 || state->finished){
      break;
    }
    
    if (state->in_preprocessor){
      if (!HasFlag(token->flags, TokenBaseFlag_PreprocessorBody) ||
          token->kind == TokenBaseKind_Preprocessor){
        break;
      }
    }
    else{
      if (token->kind == TokenBaseKind_Preprocessor){
        Code_Index_Nest *nest = generic_parse_preprocessor(index, state);
        code_index_push_nest(&index->nest_list, nest);
        continue;
      }
    }
    
    if (token->kind == TokenBaseKind_ScopeClose){
      result->is_closed = true;
      result->close = Ii64(token);
      generic_parse_inc(state);
      break;
    }
    
    if (token->kind == TokenBaseKind_ScopeOpen){
      Code_Index_Nest *nest = generic_parse_scope(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
      continue;
    }
    
    if (token->kind == TokenBaseKind_ParenClose){
      generic_parse_inc(state);
      continue;
    }
    
    if (token->kind == TokenBaseKind_ParenOpen){
      Code_Index_Nest *nest = generic_parse_paren(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
      
      // NOTE(allen): after a parenthetical group we consider ourselves immediately
      // transitioning into a statement
      nest = generic_parse_statement(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
      
      continue;
    }
    
    {
      Code_Index_Nest *nest = generic_parse_statement(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
    }
  }
  
  result->nest_array = code_index_nest_ptr_array_from_list(state->arena, &result->nest_list);
  
  state->scope_counter -= 1;
  
  return(result);
}

function Code_Index_Nest*
generic_parse_paren(Code_Index_File *index, Generic_Parse_State *state){
  Token *token = tkarr_read(&state->it);
  Code_Index_Nest *result = push_array_zero(state->arena, Code_Index_Nest, 1);
  result->kind = CodeIndexNest_Paren;
  result->open = Ii64(token);
  result->close = Ii64(max_i64);
  result->file = index;
  
  {
    u8 *ptr = state->prev_line_start;
    u8 *end_ptr = state->contents.str + token->pos;
    // NOTE(allen): Initial whitespace
    for (;ptr < end_ptr; ptr += 1){
      if (!character_is_whitespace(*ptr)){
        break;
      }
    }
  }
  
  state->paren_counter += 1;
  
  generic_parse_inc(state);
  for (;;){
    generic_parse_skip_soft_tokens(index, state);
    token = tkarr_read(&state->it);
    if (token == 0 || state->finished){
      break;
    }
    
    if (state->in_preprocessor){
      if (!HasFlag(token->flags, TokenBaseFlag_PreprocessorBody) ||
          token->kind == TokenBaseKind_Preprocessor){
        break;
      }
    }
    else{
      if (token->kind == TokenBaseKind_Preprocessor){
        Code_Index_Nest *nest = generic_parse_preprocessor(index, state);
        code_index_push_nest(&index->nest_list, nest);
        continue;
      }
    }
    
    if (token->kind == TokenBaseKind_ParenClose){
      result->is_closed = true;
      result->close = Ii64(token);
      generic_parse_inc(state);
      break;
    }
    
    if (token->kind == TokenBaseKind_ScopeClose){
      break;
    }
    
    if (token->kind == TokenBaseKind_ScopeOpen){
      Code_Index_Nest *nest = generic_parse_scope(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
      continue;
    }
    
    if (token->kind == TokenBaseKind_ParenOpen){
      Code_Index_Nest *nest = generic_parse_paren(index, state);
      nest->parent = result;
      code_index_push_nest(&result->nest_list, nest);
      continue;
    }
    
    generic_parse_inc(state);
  }
  
  result->nest_array = code_index_nest_ptr_array_from_list(state->arena, &result->nest_list);
  
  state->paren_counter -= 1;
  
  return(result);
}

function b32
generic_parse_full_input_breaks(Code_Index_File *index, Generic_Parse_State *state, i1 limit){
  b32 result = false;
  
  i64 first_index = tkarr_index(&state->it);
  i64 one_past_last_index = first_index + limit;
  for (;;){
    generic_parse_skip_soft_tokens(index, state);
    Token *token = tkarr_read(&state->it);
    
    if (token == 0 || state->finished){
      result = true;
      break;
    }
    
    if (token->kind == TokenBaseKind_Preprocessor){
      Code_Index_Nest *nest = generic_parse_preprocessor(index, state);
      code_index_push_nest(&index->nest_list, nest);
    }
    else if (token->kind == TokenBaseKind_ScopeOpen){
      Code_Index_Nest *nest = generic_parse_scope(index, state);
      code_index_push_nest(&index->nest_list, nest);
    }
    else if (token->kind == TokenBaseKind_ParenOpen){
      Code_Index_Nest *nest = generic_parse_paren(index, state);
      code_index_push_nest(&index->nest_list, nest);
    }
    else if (state->do_cpp_parse){
      if (token->sub_kind == TokenCppKind_Struct ||
          token->sub_kind == TokenCppKind_Union ||
          token->sub_kind == TokenCppKind_Enum){
        cpp_parse_type_structure(index, state, 0);
      }
      else if (token->sub_kind == TokenCppKind_Typedef){
        cpp_parse_type_def(index, state, 0);
      }
      else if (token->sub_kind == TokenCppKind_Identifier){
        cpp_parse_function(index, state, 0);
      }
      else{
        generic_parse_inc(state);
      }
    }
    else{
      generic_parse_inc(state);
    }
    
    i64 token_index = tkarr_index(&state->it);
    if (token_index >= one_past_last_index){
      token = tkarr_read(&state->it);
      if (token == 0){
        result = true;
      }
      break;
    }
  }
  
  if (result){
    index->nest_array = code_index_nest_ptr_array_from_list(state->arena, &index->nest_list);
    index->note_array = code_index_note_ptr_array_from_list(state->arena, &index->note_list);
  }
  
  return(result);
}


////////////////////////////////
// NOTE(allen): Not sure

function void
default_comment_index(App *app, Arena *arena, Code_Index_File *index, Token *token, String contents){
}

function void
generic_parse_init(App *app, Arena *arena, String contents, Token_Array *tokens, Generic_Parse_State *state){
 generic_parse_init(app, arena, contents, tokens, default_comment_index, state);
}

// BOTTOM
