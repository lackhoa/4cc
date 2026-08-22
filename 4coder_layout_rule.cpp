/*
4coder_layout_rule.cpp - Built in layout rules and layout rule helpers.
*/

// TOP

function i64
layout_index_from_ptr(u8 *ptr, u8 *string_base, i64 index_base)
{
 i64 result = (i64)(ptr - string_base) + index_base;
 return(result);
}

function Layout_Item_List
get_empty_item_list(Range_i64 input_range)
{
 Layout_Item_List list = {};
 list.input_index_range = input_range;
 list.manifested_index_range = Ii64_neg_inf;
 return(list);
}

function void
layout_item_list_finish(Layout_Item_List *list, f32 bottom_padding)
{
 list->bottom_padding = bottom_padding;
 list->height += bottom_padding;
}

function void
layout_write(Layout_State *layout, i64 pos, u32 codepoint, Layout_Item_Flag flags, Rect_f32 rect, f32 padded_y1)
{
 Arena *arena = layout->arena;
 Layout_Item_List *list = &layout->list;
 Temp_Memory restore_point = begin_temp_memory(arena);
 Layout_Item *item;
 Layout_Item_Block *block;
 {//NOTE(kv) Trying to extend the existing block.
  block = list->last;
  item = push_struct0(arena, Layout_Item);
  if (block != 0) {
   if (block->face != layout->face) {
    block = 0;
   } else if (block->items + block->item_count == item) {
    block->item_count += 1;
   } else {
    block = 0;
   }
  }
 }
 
 if (block == 0)
 { // NOTE(kv) New block
  end_temp_memory(restore_point);
  block = push_array(arena, Layout_Item_Block, 1, push_zero());
  item  = push_array(arena, Layout_Item, 1, push_zero());
  sll_queue_push(list->first, list->last, block);
  list->node_count += 1;
  block->items = item;
  block->item_count = 1;
  block->face = layout->face;
 }
 
 list->item_count += 1;
 list->manifested_index_range.min = Min(list->manifested_index_range.min, pos);
 list->manifested_index_range.max = Max(list->manifested_index_range.max, pos);
 
 if (!HasFlag(flags, LayoutItemFlag_Ghost_Character))
 {
  block->character_count += 1;
  list->character_count += 1;
 }
 
 item->index = pos;
 item->codepoint = codepoint;
 item->flags = flags;
 item->rect = rect;
 item->padded_y1 = padded_y1;
 list->height = Max(list->height, rect.y1);
}

////

function Newline_Layout_Vars
get_newline_layout_vars(void){
 Newline_Layout_Vars result = {};
 result.newline_character_index = -1;
 return(result);
}

function void
newline_layout_consume_CR(Newline_Layout_Vars &vars, i64 index){
 if (!vars.consuming_newline_characters){
  vars.consuming_newline_characters = true;
  vars.newline_character_index = index;
 }
 vars.prev_did_emit_newline = false;
}

function i64
newline_layout_consume_LF(Newline_Layout_Vars &vars, i64 index) {
 if ( !vars.consuming_newline_characters ) {
  vars.newline_character_index = index;
 }
 vars.prev_did_emit_newline = true;
 vars.consuming_newline_characters = false;
 return(vars.newline_character_index);
}

function void
newline_layout_consume_default(Newline_Layout_Vars &vars) {
 vars.consuming_newline_characters = false;
 vars.prev_did_emit_newline        = false;
}

function b32
newline_layout_consume_finish(Newline_Layout_Vars &vars){
 return(!vars.prev_did_emit_newline);
}

////

function b32
lr_tb_crosses_width(Layout_State *layout, f32 advance, f32 width){
 return(layout->p.x + advance > width);
}

function b32
lr_tb_crosses_width(Layout_State *layout, f32 advance){
 return(layout->p.x + advance > layout->width);
}

function f32
lr_tb_advance(Layout_State *layout, u32 codepoint)
{
 return(font_get_glyph_advance(layout->advance_map, layout->metrics, codepoint, layout->tab_width));
}

function void
lr_tb_write_with_advance(Layout_State *layout, f32 advance, i64 pos, u32 codepoint)
{
 if(codepoint == '\t'){ codepoint = ' '; }
 
 layout->p.x = ceilv1(layout->p.x);
 f32 next_x = layout->p.x + advance;
 rect2 rect =  Rf32(layout->p, V2(next_x, layout->text_y));
 
 if(layout->is_skm)
 {
  b32 do_raise = 0;
  
  Buffer_AST *ast = layout->ast;
  Token_Array tokens = layout->tokens;
  AST_Node *root = &ast->root;
  i32 begin = 0;
  i32 end   = root->children.count;
  // TODO(kv) @Slow Don't do this per-position?
  while(begin < end)
  {
   i32 current = begin + (end-begin) / 2;
   AST_Node *node = &root->children[current];
   Token *token_begin = &tokens.tokens[node->token_begin];
   Token *token_end   = &tokens.tokens[node->token_end];
   if(token_begin->pos == pos or
      token_end->pos == pos)
   {// NOTE Match
    do_raise = 1;
    break;
   }
   else if(token_end->pos < pos)
   {
    begin = current + 1;
   }
   else if(token_begin->pos > pos)
   {
    end = current;
   }
   else
   {// NOTE This pos somewhere in between the current node.
    break;
   }
  }
  
  if(do_raise and KV_INTERNAL)
  {// NOTE Test code
   v1 raise = 8.f;
   rect.min.y -= raise;
   rect.max.y -= raise;
  }
 }
 
 layout_write(layout, pos, codepoint, /*flags*/0, rect, layout->line_y);
 layout->p.x = next_x;
}

function f32
lr_tb_advance_byte(Layout_State *layout)
{
 return(layout->metrics->byte_advance);
}

function void
lr_tb_write_byte_with_advance(Layout_State *layout, f32 advance, i64 index, u8 byte)
{
 Face_Metrics *metrics = layout->metrics;
 
 f32 final_next_x = layout->p.x + advance;
 u32 lo = ((u32)byte     )&0xF;
 u32 hi = ((u32)byte >> 4)&0xF;
 
 v2 p = layout->p;
 p.x = ceilv1(p.x);
 f32 next_x = p.x + metrics->byte_sub_advances[0];
 f32 text_y = layout->text_y;
 
 Layout_Item_Flag flags = LayoutItemFlag_Special_Character;
 layout_write(layout, index, '\\', flags, Rf32(p, V2(next_x, text_y)), layout->line_y);
 p.x = next_x;
 
 flags = LayoutItemFlag_Ghost_Character;
 next_x += metrics->byte_sub_advances[1];
 layout_write(layout, index, integer_symbols[hi], flags, Rf32(p, V2(next_x, text_y)), layout->line_y);
 p.x = next_x;
 next_x += metrics->byte_sub_advances[2];
 layout_write(layout, index, integer_symbols[lo], flags, Rf32(p, V2(next_x, text_y)), layout->line_y);
 
 layout->p.x = final_next_x;
}

function void
lr_tb_write_byte(Layout_State *layout, i64 index, u8 byte)
{
 lr_tb_write_byte_with_advance(layout, layout->metrics->byte_advance, index, byte);
}

function void
lr_tb_write_blank_dim(Layout_State *layout, v2 dim, i64 index)
{
 layout_write(layout, index, ' ', 0, Rf32_xy_wh(layout->p, dim), layout->line_y);
 layout->p.x += dim.x;
}

function void
lr_tb_write_blank(Layout_State *layout, i64 index)
{
 lr_tb_write_blank_dim(layout, layout->blank_dim, index);
}

function void
lr_tb_next_line(Layout_State *layout)
{
 layout->p.x = 0.f;
 layout->p.y = layout->line_y;
 layout->line_y += layout->metrics->line_height;
 layout->text_y = layout->line_y + layout->line_to_text_shift;
}

function void
lr_tb_next_line_padded(Layout_State *layout, f32 top, f32 bot)
{
 layout->p.x = 0.f;
 layout->p.y = layout->line_y + top;
 layout->line_y += top + layout->metrics->line_height;
 layout->text_y = layout->line_y + layout->line_to_text_shift;
 layout->line_y += bot;
}

function void
lr_tb_advance_x_without_item(Layout_State *layout, f32 advance){
 layout->p.x += advance;
}

function void
lr_tb_align_rightward(Layout_State *layout, f32 align_x){
 layout->p.x = clamp_min(align_x, layout->p.x);
}

////////////////////////////////

function Buffer_AST *
get_or_compute_buffer_ast(App *app, Buffer_ID buffer, Token_Array *tokens);

function Layout_Item_List
layout_unwrapped(App *app, Arena *arena, Buffer_ID buffer,
                 Range_i64 range, Face_ID face, f32 width)
{
 Scratch_Block scratch;
 String text = push_buffer_range(app, scratch, buffer, range);
 
 Face_Advance_Map advance_map = get_face_advance_map(app, face);
 Face_Metrics metrics = get_face_metrics(app, face);
 u64 tab_width = def_get_config_u64(app, vars_intern_lit("default_tab_width"));
 tab_width = clamp_min(1, tab_width);
 Layout_State layout = {};
 
 {
  F4_Language *language = F4_LanguageFromBuffer(app, buffer);
  if(language)
  {
   layout.is_skm = language->name == strlit("skm");
  }
  
  if(layout.is_skm)
  {
   Managed_Scope scope = buffer_get_managed_scope(app, buffer);
   Token_Array tokens = get_token_array_from_buffer(app, buffer);
   layout.tokens = tokens;
   layout.ast = get_or_compute_buffer_ast(app, buffer, &tokens);
  }
  
  f32 text_height = metrics.text_height;
  f32 line_height = metrics.line_height;
  
  layout.list = get_empty_item_list(range);
  layout.arena = arena;
  layout.advance_map = &advance_map;
  layout.face = face;
  layout.metrics = &metrics;
  layout.tab_width = (f32)tab_width;
  layout.line_to_text_shift = text_height - line_height;
  
  layout.blank_dim = V2(metrics.space_advance, text_height);
  
  layout.line_y = line_height;
  layout.text_y = text_height;
  layout.width = width;
 }
 
 if(text.size == 0)
 {
  lr_tb_write_blank(&layout, range.first);
 }
 else
 {
  Newline_Layout_Vars newline_vars = get_newline_layout_vars();
  u8 *ptr = text.str;
  u8 *end_ptr = ptr + text.size;
  while(ptr < end_ptr)
  {
   Character_Consume_Result consume = utf8_consume(ptr, (u64)(end_ptr - ptr));
   i64 index = layout_index_from_ptr(ptr, text.str, range.first);
   kv_assert(index < range.max);
   
   switch(consume.codepoint)
   {
    case '\r':
    {
     newline_layout_consume_CR(newline_vars, index);
    }break;
    
    case '\n':
    {
     i64 newline_index = newline_layout_consume_LF(newline_vars, index);
     lr_tb_write_blank(&layout, newline_index);
     lr_tb_next_line(&layout);
    }break;
    
    case max_u32:
    {
     newline_layout_consume_default(newline_vars);
     lr_tb_write_byte(&layout, index, *ptr);
    }break;
    
    default:
    {
     newline_layout_consume_default(newline_vars);
     f32 advance = lr_tb_advance(&layout, consume.codepoint);
     lr_tb_write_with_advance(&layout, advance, index, consume.codepoint);
    }break;
   }
   
   ptr += consume.inc;
  }
  
  kv_assert(ptr-text.str == range_size(range));
  
  if(newline_layout_consume_finish(newline_vars))
  {
   i64 index = layout_index_from_ptr(ptr, text.str, range.first);
   if (index < range.max)
   {// NOTE(kv): original code got out of range here
    lr_tb_write_blank(&layout, index);
   }
  }
 }
 
 layout_item_list_finish(&layout.list, -layout.line_to_text_shift);
 
 return(layout.list);
}

function Layout_Item_List
layout_basic(App *app, Arena *arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width)
{
 Layout_Item_List result = layout_unwrapped(app, arena, buffer, range, face, width);
 return(result);
}

// BOTTOM

