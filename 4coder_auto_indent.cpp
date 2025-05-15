/*#processed
Commands for automatic indentation.
*/

// TOP

function Token *
find_first_indented_token(App *app, Buffer_ID buffer, Token_Array *tokens, i64 first_indented_line)
{
 ProfileBlock( "find anchor token");
 Token *result = 0;
 
 if (tokens != 0 && tokens->tokens != 0)
 {
  result = tokens->tokens;
  i64 indent_start_pos = get_line_start_pos(app, buffer, first_indented_line);
  i32 scope_counter = 0;
  i32 paren_counter = 0;
  Token *token = tokens->tokens;
  for (;;token += 1)
  {
   if (token->pos > indent_start_pos)
   {
    break;
   }
   
   if(not HasFlag(token->flags, TokenBaseFlag_PreprocessorBody))
   {
    if(scope_counter == 0 and
       paren_counter == 0)
    {// NOTE
     result = token;
    }
    
    switch (token->kind)
    {
     case TokenBaseKind_ScopeOpen: { scope_counter += 1; }break;
     case TokenBaseKind_ScopeClose:
     {
      if (scope_counter > 0) { scope_counter -= 1; }
     }break;
     
     case TokenBaseKind_ParenOpen: { paren_counter += 1; }break;
     case TokenBaseKind_ParenClose:
     {
      if (paren_counter > 0) { paren_counter -= 1; }
     }break;
    }
   }
  }
 }
 
 return(result);
}
function i64
find_first_non_whitespace(App *app, Buffer_ID buffer, Range_i64 range)
{
 Scratch_Block tmp;
 i64 result = -1;
 Stringz string = push_buffer_range(app, tmp, buffer, range);
 for(u8 *c = string.str;
     c != 0;
     c++)
 {
  if(not char_is_whitespace(*c))
  {
   result = range.min + (c - string.str);
   break;
  }
 }
 
 kv_assert(range_contains(range, result));
 return result;
}
function i64 *
get_indentation_array(App *app, Arena *arena, Buffer_ID buffer,
                      Range_i64 *in_out_indented_lines,
                      Indent_Flag flags, i32 tab_width, i32 indent_width)
{
 i64 total_line_count = buffer_get_line_count(app, buffer);
 i64 *indentations = 0;
 ProfileBlock( "get indentation array");
 Scratch_Block tmp;
 Range_i64 indented_lines = *in_out_indented_lines;
 ClampBot(indented_lines.min, 1);
 
 Token_Array token_array = get_token_array_from_buffer(app, buffer);
 Token_Array *tokens = &token_array;
 
 Token *start_token = find_first_indented_token(app, buffer, tokens, indented_lines.first);
 if (start_token != 0 &&
     start_token >= tokens->tokens &&
     start_token < tokens->tokens + tokens->count)
 {
  // TODO(kv) #Bug We can't indent the line of the starting token.
  indented_lines.min = get_line_number_from_pos(app, buffer, start_token->pos);
  ClampBot(indented_lines.max, indented_lines.min);
  i64 indented_lines_count = indented_lines.max - indented_lines.min + 1;
  indentations = push_array_zero(arena, i64, indented_lines_count);
  auto get_indentation = [&](i64 line) -> i64 &
  {
   i64 index = line - indented_lines.min;
   kv_assert(u64(index) < u64(indented_lines_count));
   return indentations[index];
  };
  
  Token_Iterator token_it = make_token_iterator(token_iterator(0, tokens, start_token));
  Ed_Parser parser_value = ed_parser_from_buffer(app, buffer, token_it, tmp);
  Ed_Parser *parser = &parser_value;
  
  Nest *nest = 0;
  Nest *free_nest = 0;
  
  auto push_nest = [&](Token_Base_Kind kind, i64 indentation)
  {
   Nest *new_nest = free_nest;
   if (new_nest) { sll_stack_pop(free_nest); }
   else { new_nest = push_struct(tmp, Nest); }
   
   *new_nest = {};
   new_nest->kind = kind;
   new_nest->indentation = indentation;
   
   sll_stack_push(nest, new_nest);
  };
  
  auto pop_nest = [&]()
  {
   Nest *freed = nest;
   sll_stack_pop(nest);
   sll_stack_push(free_nest, freed);
  };
  
  auto current_indentation = [&]() -> i64
  {
   return nest ? nest->indentation : 0;
  };
  
  Indent_Line_Cache token_line = {};
  auto update_token_line = [&](i64 line_number)
  {
   token_line = {};
   token_line.line_number = line_number;
   token_line.range = kv_get_line_pos_range(app, buffer, line_number);
  };
  
  update_token_line(indented_lines.min);
  
  b32 parsing = 1;
  while(parsing)
  {
   Token *token = ep_get_token(parser);
   
   // ;indent_skip_whitespace_token
   while(parser->ok_ and token->kind == TokenBaseKind_Whitespace)
   {
    ep_eat_inc_all(parser);
    token = ep_get_token(parser);
   }
   
   if(parsing)
   {// NOTE Figure out line info
    i64 updated_line_number = 0;
    b32 line_is_wrong = token->pos >= token_line.range.max;
    if(line_is_wrong)
    {// NOTE Advance line
     if(token->kind != TokenBaseKind_EOF)
     {
      i64 opl_line = total_line_count+1;
      for_i64(line_number, token_line.line_number+1, opl_line)
      {
       Range_i64 line_range = kv_get_line_pos_range(app, buffer, line_number);
       if(range_contains(line_range, token->pos))
       {// NOTE
        updated_line_number = line_number;
        break;
       }
      }
      
      if(updated_line_number == 0)
      {// NOTE(kv) In case we somehow can't find the line for this token...
       updated_line_number = indented_lines.max;
      }
     }
     else { updated_line_number = indented_lines.max; }
    }
    
    if(updated_line_number > 0)
    {// NOTE Update line
     i64 old_line = token_line.line_number;
     update_token_line(updated_line_number);
     {//NOTE Write out indentations up to the current token.
      i64 min_line_to_write = clamp_min(old_line+1, indented_lines.min);
      i64 max_line_to_write = clamp_max(token_line.line_number, indented_lines.max);
      i64 indentation = current_indentation();
      for_i64(line_number, min_line_to_write, max_line_to_write+1)
      {
       get_indentation(line_number) = indentation;
      }
     }
    }
    
    if(token->kind == TokenBaseKind_EOF or
       token_line.line_number > indented_lines.max)
    {// NOTE(kv) Even if the token was on the last line,
     // you'd still have to parse it, since it could be a closing brace,
     // which changes the indentation. I know the logic is compilcated.
     parsing = 0;
    }
   }
   
   if(parsing)
   {
    if(is_preprocessor_body(token))
    {// NOTE(kv) Here's the messiness: preprocessor doesn't respect scope,
     get_indentation(token_line.line_number) = 0;
    }
    else
    {
     switch(token->kind)
     {
      case TokenBaseKind_ScopeOpen:
      {
       // TODO(kv) Dummy logic, idk what to do here
       push_nest(TokenBaseKind_ScopeOpen, current_indentation() + indent_width);
      }break;
      
      case TokenBaseKind_ScopeClose:
      {
       while(nest and nest->kind != TokenBaseKind_ScopeOpen)
       {// NOTE(kv) In case of malformed input
        pop_nest();
       }
       
       if(nest and nest->kind == TokenBaseKind_ScopeOpen)
       {
        pop_nest();
       }
       
       i64 first_non_whitespace = find_first_non_whitespace(app, buffer, token_line.range);
       if(token->pos == first_non_whitespace)
       {// NOTE Yeah, closing braces are indented differently...
        get_indentation(token_line.line_number) = current_indentation();
       }
      }break;
      
      case TokenBaseKind_ParenOpen:
      {
       i64 line_indentation = get_indentation(token_line.line_number);
       i64 first_non_whitespace = find_first_non_whitespace(app, buffer, token_line.range);
       i64 token_pos_after_indented = token->pos - first_non_whitespace + line_indentation;
       push_nest(TokenBaseKind_ParenOpen, token_pos_after_indented+1);
      }break;
      
      case TokenBaseKind_ParenClose:
      {
       if(nest and nest->kind == TokenBaseKind_ParenOpen)
       {
        pop_nest();
       }
      }break;
     }
    }
    
    ep_eat_inc_all(parser);
    parsing = parsing and parser->ok_;
   }
  }
 }
 else
 {
  indentations = 0;
  indented_lines = {};
 }
 
 *in_out_indented_lines = indented_lines;
 return(indentations);
}

function Batch_Edit*
make_batch_from_indentations(App *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, i64 *indentations, Indent_Flag flags, i32 tab_width)
{
 i64 *shifted_indentations = indentations - lines.first;
 
 Batch_Edit *batch_first = 0;
 Batch_Edit *batch_last = 0;
 
 for (i64 line_number = lines.first;
      line_number <= lines.max;
      ++line_number)
 {
  i64 line_start_pos = get_line_start_pos(app, buffer, line_number);
  Indent_Info indent_info = get_indent_info_line_number_and_start(app, buffer, line_number, line_start_pos, tab_width);
  
  i64 correct_indentation = shifted_indentations[line_number];
  if (indent_info.is_blank && HasFlag(flags, Indent_ClearLine)){
   correct_indentation = 0;
  }
  if (correct_indentation <= -1){
   correct_indentation = indent_info.indent_pos;
  }
  
  if (correct_indentation != indent_info.indent_pos){
   u64 str_size = 0;
   u8 *str = 0;
   if (HasFlag(flags, Indent_UseTab)){
    i64 tab_count = correct_indentation/tab_width;
    i64 indent = tab_count*tab_width;
    i64 space_count = correct_indentation - indent;
    str_size = tab_count + space_count;
    str = push_array(arena, u8, str_size);
    block_fill_u8(str, tab_count, '\t');
    block_fill_u8(str + tab_count, space_count, ' ');
   }
   else{
    str_size = correct_indentation;
    str = push_array(arena, u8, str_size);
    block_fill_u8(str, str_size, ' ');
   }
   
   Batch_Edit *batch = push_array(arena, Batch_Edit, 1);
   sll_queue_push(batch_first, batch_last, batch);
   batch->edit.text = SCu8(str, str_size);
   batch->edit.range = Ii64(line_start_pos, indent_info.first_char_pos);
  }
 }
 
 return(batch_first);
}

function b32
set_line_indents(App_Cmd *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, i64 *indentations, Indent_Flag flags, i32 tab_width)
{
 b32 result = false;
 Batch_Edit *batch = make_batch_from_indentations(app, arena, buffer, lines, indentations, flags, tab_width);
 if (batch != 0) {
  result = true;
  buffer_batch_edit(app, buffer, batch);
 }
 return result;
}

function b32
auto_indent_buffer(App_Cmd *app, Buffer_ID buffer, Range_i64 pos, 
                   Indent_Flag flags, i32 tab_width, i32 indent_width)
{
 ProfileBlock( "auto indent buffer");
 Token_Array token_array = get_token_array_from_buffer(app, buffer);
 Token_Array *tokens = &token_array;
 
 b32 result = false;
 if (tokens->tokens != 0)
 {
  Scratch_Block scratch(app);
  Range_i64 line_numbers = {};
  if (HasFlag(flags, Indent_FullTokens))
  {
   i32 safety_counter = 0;
   for (;;)
   {
    Range_i64 expanded = enclose_tokens(app, buffer, pos);
    expanded = enclose_whole_lines(app, buffer, expanded);
    if (expanded == pos) { break; }
    else
    {
     pos = expanded;
     safety_counter += 1;
     if (safety_counter >= 20)
     {
      pos = buffer_range(app, buffer);
      break;
     }
    }
   }
  }
  line_numbers = get_line_range_from_pos_range(app, buffer, pos);
  
  i64 *indentations = get_indentation_array(app, scratch, buffer, &line_numbers, flags, tab_width, indent_width);
  result = set_line_indents(app, scratch, buffer, line_numbers, indentations, flags, tab_width);
 }
 
 return(result);
}

function b32
auto_indent_buffer(App_Cmd *app, Buffer_ID buffer, Range_i64 pos, Indent_Flag flags)
{
 i32 indent_width = (i32)def_get_config_u64(app, vars_intern_lit("indent_width"));
 i32 tab_width = (i32)def_get_config_u64(app, vars_intern_lit("default_tab_width"));
 tab_width = clamp_min(1, tab_width);
 AddFlag(flags, Indent_FullTokens);
 b32 indent_with_tabs = def_get_config_b32(vars_intern_lit("indent_with_tabs"));
 if (indent_with_tabs){
  AddFlag(flags, Indent_UseTab);
 }
 return auto_indent_buffer(app, buffer, pos, flags, indent_width, tab_width);
}

function b32
auto_indent_buffer(App_Cmd *app, Buffer_ID buffer, Range_i64 pos)
{
 return auto_indent_buffer(app, buffer, pos, 0);
}

////////////////////////////////

// CUSTOM_DOC("Auto-indents the line on which the cursor sits.")
function b32
auto_indent_line_at_cursor(App_Cmd *app)
{
 View_ID view = get_active_view(app, Access_ReadWriteVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 i64 pos = view_get_cursor_pos(app, view);
 b32 result = auto_indent_buffer(app, buffer, Ii64(pos));
 move_past_lead_whitespace(app, view, buffer);
 return result;
}

function void vim_normal_mode(App *app);

function void
auto_indent_range(App_Cmd *app){
 View_ID view = get_active_view(app, Access_ReadWriteVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 Range_i64 range = get_view_range(app, view);
 auto_indent_buffer(app, buffer, range);
 move_past_lead_whitespace(app, view, buffer);
 vim_normal_mode(app);
}

function void
write_text_and_auto_indent(App_Cmd *app)

{
 ProfileBlock( "write and auto indent");
 User_Input in = get_current_input(app);
 String insert = to_writable(&in);
 if (insert.str != 0 && insert.size > 0){
  b32 do_auto_indent = false;
  for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1){
   switch (insert.str[i]){
    case ';': case ':':
    case '{': case '}':
    case '(': case ')':
    case '[': case ']':
    case '#':
    case '\n': case '\t':
    {
     do_auto_indent = true;
    }break;
   }
  }
  if (do_auto_indent){
   View_ID view = get_active_view(app, Access_ReadWriteVisible);
   Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
   
   Range_i64 pos = {};
   if (view_has_highlighted_range(app, view)){
    pos = get_view_range(app, view);
   }
   else{
    pos.min = pos.max = view_get_cursor_pos(app, view);
   }
   
   write_text_input(app);
   
   i64 end_pos = view_get_cursor_pos(app, view);
   pos.min = Min(pos.min, end_pos);
   pos.max = Max(pos.max, end_pos);
   
   auto_indent_buffer(app, buffer, pos, 0);
   move_past_lead_whitespace(app, view, buffer);
  }
  else{
   write_text_input(app);
  }
 }
}

// BOTTOM

