/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 31.03.2019
 *
 * Text layout representation
 *
 */

// TOP

internal void
text_layout_init(Thread_Context *tctx, Text_Layout_Container *container){
 block_zero_struct(container);
 container->node_arena = make_arena_system();
 container->table = make_table_u64_u64(tctx->allocator, 20);
}

internal Text_Layout*
text_layout_new__alloc_layout(Text_Layout_Container *container)
{
 Text_Layout *node = container->free_nodes;
 if (node == 0) {
  node = push_struct(&container->node_arena, Text_Layout);
 } else {
  sll_stack_pop(container->free_nodes);
 }
 return(node);
}

internal void
text_layout_release(Thread_Context *tctx, Models *models, Text_Layout_Container *container, Text_Layout *layout)
{
 Arena arena = *layout->arena;
 arena_clear(&arena);
 sll_stack_push(container->free_nodes, layout);
}


function Text_Layout_ID
text_layout_new(Text_Layout_Container *container, Arena *arena,
                Buffer_ID buffer_id, Buffer_Point point,
                Range_i64 visible_range, Range_i64 visible_line_number_range,
                Rect_f32 rect, ARGB_Color *item_colors, Layout_Function *layout_func)
{
 Text_Layout *new_layout_data = text_layout_new__alloc_layout(container);
 *new_layout_data = {
  .arena                     = arena,
  .buffer_id                 = buffer_id,
  .point                     = point,
  .visible_range             = visible_range,
  .visible_line_number_range = visible_line_number_range,
  .rect                      = rect,
  .item_colors               = item_colors,
  .layout_func               = layout_func,
 };
 Text_Layout_ID new_id = ++container->id_counter;
 table_insert(&container->table, new_id, (u64)PtrAsInt(new_layout_data));
 return(new_id);
}

function Text_Layout*
text_layout_get(Text_Layout_Container *container, Text_Layout_ID id){
    Text_Layout *result = 0;
    Table_Lookup lookup = table_lookup(&container->table, id);
    if (lookup.found_match){
        u64 ptr_val = 0;
        table_read(&container->table, lookup, &ptr_val);
        result = (Text_Layout*)IntAsPtr(ptr_val);
    }
    return(result);
}

internal b32
text_layout_erase(Thread_Context *tctx, Models *models, Text_Layout_Container *container, Text_Layout_ID id){
 b32 result = false;
 Table_Lookup lookup = table_lookup(&container->table, id);
 if (lookup.found_match){
  u64 ptr_val = 0;
  table_read(&container->table, lookup, &ptr_val);
  Text_Layout *ptr = (Text_Layout*)IntAsPtr(ptr_val);
  text_layout_release(tctx, models, container, ptr);
  table_erase(&container->table, lookup);
  result = true;
 }
 return(result);
}

////////////////////////////////

function void
text_layout_render(Thread_Context *tctx, Models *models,
                   Text_Layout *layout,
                   ARGB_Color special_color, ARGB_Color ghost_color)
{
 Editing_File *file = imp_get_file(models, layout->buffer_id);
 if (file != 0)
 {
  Face *face = file_get_face(models, file);
  f32 width = rect_width(layout->rect);
  
  v2 delta = V2(1.f, 0.f);
  
  v2 shift_p = layout->rect.p0 - layout->point.pixel_shift;
  i64 linum = layout->visible_line_number_range.min;
  for (;
       linum <= layout->visible_line_number_range.max;
       linum += 1)
  {
   Layout_Item_List line_layout = file_get_line_layout(tctx, models, file,
                                                       layout->layout_func, width, face,
                                                       linum);
   for (Layout_Item_Block *block = line_layout.first;
        block != 0;
        block = block->next)
   {
    Layout_Item *item = block->items;
    for (i1 counter = 0;
         counter < block->item_count;
         counter++, item++)
    {
     if (item->codepoint != 0)
     {
      ARGB_Color color = 0;
      if        (HasFlag(item->flags, LayoutItemFlag_Special_Character)) {
       color = special_color;
      } else if (HasFlag(item->flags, LayoutItemFlag_Ghost_Character))   {
       color = ghost_color;
      } else {
       i64 index = item->index - layout->visible_range.first;
       kv_assert(index < range_size_inclusive(layout->visible_range));
       color = layout->item_colors[index];
      }
      v2 p = item->rect.p0 + shift_p;
      draw_font_glyph(get_render_target(0), face, item->codepoint, p, color, GlyphFlag_None, delta);
     }
    }
   }
   shift_p.y += line_layout.height;
  }
 }
}

// BOTTOM
