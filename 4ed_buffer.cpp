/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 06.01.2017
 *
 * The 4coder base buffer data structure.
 *
 */

// TOP

//
// Buffer low level operations
//

internal void
write_cursor_with_index(Cursor_With_Index *positions, i1 *count, i64 pos){
    positions[*count].index = *count;
    positions[*count].pos = pos;
    ++(*count);
}

internal void
buffer_quick_sort_cursors(Cursor_With_Index *positions, i1 first, i1 one_past_last){
    if (first + 1 < one_past_last){
        i1 pivot = one_past_last - 1;
        i64 pivot_pos = positions[pivot].pos;
        i1 j = first;
        for (i1 i = first; i < pivot; i += 1){
            i64 pos = positions[i].pos;
            if (pos < pivot_pos){
                macro_swap(positions[j], positions[i]);
                j += 1;
            }
        }
        macro_swap(positions[j], positions[pivot]);
        buffer_quick_sort_cursors(positions, first, j);
        buffer_quick_sort_cursors(positions, j + 1, one_past_last);
    }
}

internal void
buffer_sort_cursors(Cursor_With_Index *positions, i1 count){
    if (count > 0){
        buffer_quick_sort_cursors(positions, 0, count);
    }
}

internal void
buffer_unsort_cursors(Cursor_With_Index *positions, i1 count){
    if (count > 0){
        i1 i = 0;
        for (;;){
            if (positions[i].index == i){
                i += 1;
                if (i >= count){
                    break;
                }
            }
            else{
                i1 j = positions[i].index;
                macro_swap(positions[i], positions[j]);
            }
        }
    }
}

#if 0
function void
buffer_sort_batch(Edit *batch, i1 first, i1 one_past_last){
    if (first + 1 < one_past_last){
        i1 pivot = one_past_last - 1;
        i64 pivot_pos = batch[pivot].range.first;
        i1 j = first;
        for (i1 i = first; i < pivot; i += 1){
            i64 pos = batch[i].range.first;
            if (pos < pivot_pos){
                Swap(Edit, batch[j], batch[i]);
                j += 1;
            }
        }
        Swap(Edit, batch[j], batch[pivot]);
        buffer_sort_batch(batch, first, j);
        buffer_sort_batch(batch, j + 1, one_past_last);
    }
}

function Edit_Array
buffer_batch_array_from_linked_list(Arena *arena, Batch_Edit *batch, i1 count){
    Edit_Array result = {};
    result.count = count;
    result.vals = push_array(arena, Edit, count);
    i1 counter = 0;
    for (Batch_Edit *node = batch;
         counter < count && node != 0;
         node = node->next){
        result.vals[counter] = node->edit;
        counter += 1;
    }
    return(result);
}

function Edit_Array
buffer_sort_batch(Arena *arena, Batch_Edit *batch, i1 count){
    Edit_Array result = buffer_batch_array_from_linked_list(arena, batch, count);
    buffer_sort_batch(result.vals, 0, result.count);
    return(result);
}
#endif

internal void
buffer_update_cursors_lean_l(Cursor_With_Index *sorted_positions, i1 count,
                             Batch_Edit *batch){
    Cursor_With_Index *pos = sorted_positions;
    Cursor_With_Index *end_pos = sorted_positions + count;
    i64 shift_amount = 0;
    for (; batch != 0 && pos < end_pos;
         batch = batch->next){
        Range_i64 range = batch->edit.range;
        i64 len = batch->edit.text.size;
        if (shift_amount != 0){
            for (;pos < end_pos && pos->pos < range.first; pos += 1){
                pos->pos += shift_amount;
            }
        }
        else{
            for (;pos < end_pos && pos->pos < range.first; pos += 1);
        }
        i64 new_pos = range.first + shift_amount;
        for (;pos < end_pos && pos->pos <= range.one_past_last; pos += 1){
            pos->pos = new_pos;
        }
        shift_amount += len - (range.one_past_last - range.first);
    }
    if (shift_amount != 0){
        for (;pos < end_pos; pos += 1){
            pos->pos += shift_amount;
        }
    }
}

internal void
buffer_update_cursors_lean_r(Cursor_With_Index *sorted_positions, i1 count,
                             Batch_Edit *batch){
    Cursor_With_Index *pos = sorted_positions;
    Cursor_With_Index *end_pos = sorted_positions + count;
    i64 shift_amount = 0;
    for (; batch != 0 && pos < end_pos;
         batch = batch->next){
        Range_i64 range = batch->edit.range;
        i64 len = batch->edit.text.size;
        if (shift_amount != 0){
            for (;pos < end_pos && pos->pos < range.first; pos += 1){
                pos->pos += shift_amount;
            }
        }
        else{
            for (;pos < end_pos && pos->pos < range.first; pos += 1);
        }
        i64 new_pos = range.first + len + shift_amount;
        for (;pos < end_pos && pos->pos < range.one_past_last; pos += 1){
            pos->pos = new_pos;
        }
        shift_amount += len - (range.one_past_last - range.first);
    }
    if (shift_amount != 0){
        for (;pos < end_pos; pos += 1){
            pos->pos += shift_amount;
        }
    }
}

//////////////////////////////////////

inline b32
buffer_good(Gap_Buffer *buffer) {
    return(buffer->data != 0);
}

inline i64
buffer_size(Gap_Buffer *buffer){
 return(buffer->size1 + buffer->size2);
}

inline i64
buffer_line_count(Gap_Buffer *buffer){
 return(buffer->line_start_count - 1);
}

inline u8 *
gap_buffer_data2(Gap_Buffer *buffer)
{
 return (buffer->data + (buffer->size1 + buffer->gap_size));
}

inline u8 *
gap_buffer_gap(Gap_Buffer *buffer)
{
 return (buffer->data + buffer->size1);
}


function void
buffer_init(Gap_Buffer *buffer, u8 *data, u64 size0, Base_Allocator *allocator) {
 i64 size = (i64)size0;
 i64 capacity = round_up_i64(size*2, KB(4));
 u8 *memory = base_allocate(allocator, capacity);
 i64 size1 = size/2;
 *buffer = {
  .allocator = allocator,
  .data      = memory,
  .size1     = size1,
  .gap_size  = capacity - size,
  .size2     = size - size1,
  .cap       = capacity,
 };
 
 block_copy(buffer->data, data, buffer->size1);
 block_copy(gap_buffer_data2(buffer), data + buffer->size1, buffer->size2);
}

function void
gap_buffer_replace_range(Gap_Buffer *buffer, Range_i64 range, String text)
{
 i64 delta_size = (i64)text.size-(range.end-range.start);
 i64 size = buffer_size(buffer);
 Assert(0 <= range.start);
 Assert(range.start <= range.end);
 Assert(range.end <= size);
 
 i64 new_size = size+delta_size;
 if (new_size > buffer->cap) {
  // NOTE(kv): Expand buffer
  i64 new_cap = round_up_i64(2*(new_size), KB(4));
  i64 new_gap_size = new_cap - size;
  u8 *new_memory = base_allocate(buffer->allocator, new_cap);
  block_copy(new_memory, buffer->data, buffer->size1);
  block_copy(new_memory + (buffer->size1 + new_gap_size),
             gap_buffer_data2(buffer),
             buffer->size2);
  base_free(buffer->allocator, buffer->data);
  buffer->data     = new_memory;
  buffer->gap_size = new_gap_size;
  buffer->cap      = new_cap;
 }
 Assert(new_size <= buffer->cap);
 
 if (range.end < buffer->size1) {
  // NOTE(kv): Change entirely on the left
  i64 move_size = buffer->size1 - range.end;
  block_copy(gap_buffer_data2(buffer) - move_size,
             buffer->data + range.end,
             move_size);
  buffer->size1 -= move_size;
  buffer->size2 += move_size;
 }
 
 if (range.start > buffer->size1) {
  // NOTE(kv): Change entirely on the right
  i64 move_size = range.start - buffer->size1;
  block_copy(gap_buffer_gap(buffer),
             gap_buffer_data2(buffer),
             move_size);
  buffer->size1 += move_size;
  buffer->size2 -= move_size;
 }
 
 block_copy(buffer->data + range.start, text.str, text.size);
 buffer->size2     = size - range.end;
 buffer->size1     = range.start + text.size;
 buffer->gap_size -= delta_size;
 kv_assert(buffer->gap_size >= 0);
 
 Assert(buffer->size1 + buffer->size2 == new_size);
 Assert(buffer->size1 + buffer->gap_size + buffer->size2 == buffer->cap);
}

////////////////////////////////

function List_String
buffer_get_chunks(Arena *arena, Gap_Buffer *buffer) {
    List_String list = {};
    if (buffer->size1 > 0){
        string_list_push(arena, &list, SCu8(buffer->data, buffer->size1));
    }
    if (buffer->size2 > 0){
        u64 gap_2_pos = buffer->size1 + buffer->gap_size;
        string_list_push(arena, &list, SCu8(buffer->data + gap_2_pos, buffer->size2));
    }
    return(list);
}

function void
buffer_chunks_clamp(List_String *chunks, Range_i64 range){
    i64 p = 0;
    List_String list = {};
    for (Node_String *node = chunks->first, *next = 0;
         node != 0;
         node = next){
        next = node->next;
        Range_i64 node_range = Ii64(p, p + node->string.size);
        if (range_overlap(range, node_range)){
            i64 first = Max(node_range.first, range.first) - node_range.first;
            i64 one_past_last = Min(node_range.one_past_last, range.one_past_last) - node_range.first;
            String s = string_prefix(node->string, one_past_last);
            node->string = string_skip(s, first);
            sll_queue_push(list.first, list.last, node);
            list.total_size += node->string.size;
            list.node_count += 1;
        }
        p = node_range.one_past_last;
    }
    *chunks = list;
}

internal String
buffer_stringify(Arena *arena, Gap_Buffer *buffer, Range_i64 range){
 List_String list = buffer_get_chunks(arena, buffer);
 buffer_chunks_clamp(&list, range);
 return(string_list_flatten(arena, list, StringFill_NullTerminate));
}

function String
buffer_eol_convert_out(Arena *arena, Gap_Buffer *buffer, Range_i64 range)
{
 List_String list = buffer_get_chunks(arena, buffer);
 buffer_chunks_clamp(&list, range);
 u64 cap = list.total_size*2;
 u8 *memory = push_array(arena, u8, cap);
 u8 *memory_opl = memory + cap;
 u8 *ptr = memory;
 for (Node_String *node = list.first;
      node != 0;
      node = node->next)
 {
  u8 *byte = node->string.str;
  u8 *byte_opl = byte + node->string.size;
  for (;byte < byte_opl; byte += 1)
  {
   if (*byte == '\n')
   {
    *ptr = '\r';
    ptr += 1;
    *ptr = '\n';
    ptr += 1;
   }
   else
   {
    *ptr = *byte;
    ptr += 1;
   }
  }
 }
 arena_pop(arena, memory_opl - ptr);
 return(SCu8(memory, ptr));
}

#if 0
internal i64
buffer_count_newlines(Arena *scratch, Gap_Buffer *buffer, i64 start, i64 end){
    Temp_Memory temp = begin_temp(scratch);
    List_String list = buffer_get_chunks(scratch, buffer);
    buffer_chunks_clamp(&list, Ii64(start, end));
    
    i64 count = 0;
    for (Node_String *node = list.first;
         node != 0;
         node = node->next){
        u8 *byte = node->string.str;
        u8 *byte_opl = byte + node->string.size;
        for (;byte < byte_opl; byte += 1){
            if (*byte == '\n'){
                count += 1;
            }
        }
    }
    
    end_temp(temp);
    
    return(count);
}
#endif

internal void
buffer_starts__ensure_max_size(Gap_Buffer *buffer, i64 max_size){
    if (max_size > buffer->line_start_max){
        i64 new_max = round_up_i64(max_size*2, KB(1));
        String memory = base_allocate2(buffer->allocator, sizeof(*buffer->line_starts)*new_max);
        i64 *new_line_starts = (i64*)memory.str;
        block_copy_count(new_line_starts, buffer->line_starts, buffer->line_start_count);
        buffer->line_start_max = new_max;
        base_free(buffer->allocator, buffer->line_starts);
        buffer->line_starts = new_line_starts;
    }
}

internal void
buffer_measure_starts__write(Gap_Buffer *buffer, i64 pos){
    buffer_starts__ensure_max_size(buffer, buffer->line_start_count + 1);
    buffer->line_starts[buffer->line_start_count] = pos;
    buffer->line_start_count += 1;
}

internal void
buffer_measure_starts(Arena *scratch, Gap_Buffer *buffer){
    Temp_Memory temp = begin_temp(scratch);
    List_String list = buffer_get_chunks(scratch, buffer);
    buffer->line_start_count = 0;
    buffer_measure_starts__write(buffer, 0);
    i64 index = 0;
    for (Node_String *node = list.first;
         node != 0;
         node = node->next){
        u8 *byte = node->string.str;
        u8 *byte_opl = byte + node->string.size;
        for (;byte < byte_opl; byte += 1){
            index += 1;
            if (*byte == '\n'){
                buffer_measure_starts__write(buffer, index);
            }
        }
    }
    buffer_measure_starts__write(buffer, buffer_size(buffer));
    end_temp(temp);
}

internal i64
buffer_get_line_index(Gap_Buffer *buffer, i64 pos){
    i64 i = 0;
    if (buffer->line_start_count > 2){
        i64 start = 0;
        i64 one_past_last = buffer->line_start_count - 1;
        i64 *array = buffer->line_starts;
        pos = clamp_min(0, pos);
        for (;;){
            i = (start + one_past_last) >> 1;
            if (array[i] < pos){
                start = i;
            }
            else if (array[i] > pos){
                one_past_last = i;
            }
            else{
                break;
            }
            if (start + 1 >= one_past_last){
                i = start;
                break;
            }
        }
    }
    return(i);
}

Line_Move*
push_line_move(Arena *arena, Line_Move *moves, i64 new_line_first,
               i64 old_line_first, i64 old_line_opl, i64 text_shift){
    Line_Move *move = push_array(arena, Line_Move, 1);
    move->next = moves;
    move->kind = LineMove_ShiftOldValues;
    move->new_line_first = new_line_first;
    move->old_line_first = old_line_first;
    move->old_line_opl = old_line_opl;
    move->text_shift = text_shift;
    return(move);
}

Line_Move*
push_line_move(Arena *arena, Line_Move *moves, i64 new_line_first,
               String string, i64 text_base){
    Line_Move *move = push_array(arena, Line_Move, 1);
    move->next = moves;
    move->kind = LineMove_MeasureString;
    move->new_line_first = new_line_first;
    move->string = string;
    move->text_base = text_base;
    return(move);
}

function i64
count_lines(String string){
    i64 result = 0;
    for (u64 i = 0; i < string.size; i += 1){
        if (string.str[i] == '\n'){
            result += 1;
        }
    }
    return(result);
}

function void
fill_line_starts(i64 *lines_starts, String string, i64 text_base){
    i64 *ptr = lines_starts;
    for (u64 i = 0; i < string.size; i += 1){
        if (string.str[i] == '\n'){
            *ptr = text_base + i + 1;
            ptr += 1;
        }
    }
}

function void
buffer_remeasure_starts(Thread_Context *tctx, Gap_Buffer *buffer, Batch_Edit *batch){
    Scratch_Block scratch(tctx);
    
    i64 line_start_count = buffer_line_count(buffer) + 1;
    
    Line_Move *moves = 0;
    i64 current_line = 0;
    i64 text_shift = 0;
    i64 line_shift = 0;
    for (Batch_Edit *node = batch;
         node != 0;
         node = node->next){
        i64 first_line = buffer_get_line_index(buffer, node->edit.range.first);
        i64 opl_line = buffer_get_line_index(buffer, node->edit.range.one_past_last);
        i64 new_line_count = count_lines(node->edit.text);
        i64 deleted_line_count = opl_line - first_line;
        
        Assert(first_line <= opl_line);
        Assert(opl_line <= line_start_count);
        
        if (current_line <= first_line &&
            (text_shift != 0 || line_shift != 0)){
            moves = push_line_move(scratch, moves, current_line + line_shift,
                                   current_line, first_line + 1, text_shift);
        }
        
        if (new_line_count != 0){
            moves = push_line_move(scratch, moves, first_line + 1 + line_shift,
                                   node->edit.text, node->edit.range.first + text_shift);
        }
        
        text_shift += node->edit.text.size - range_size(node->edit.range);
        line_shift += new_line_count - deleted_line_count;
        current_line = opl_line + 1;
    }
    
    moves = push_line_move(scratch, moves, current_line + line_shift,
                           current_line, line_start_count, text_shift);
    line_start_count = line_start_count + line_shift;
    
    buffer_starts__ensure_max_size(buffer, line_start_count + 1);
    buffer->line_start_count = line_start_count;
    
    i64 *array = buffer->line_starts;
    
    for (Line_Move *node = moves;
         node != 0;
         node = node->next){
        if (node->kind == LineMove_ShiftOldValues){
            i64 line_index_shift = node->new_line_first - node->old_line_first;
            i64 move_text_shift = node->text_shift;
            if (line_index_shift > 0){
                for (i64 i = node->old_line_opl - 1;
                     i >= node->old_line_first;
                     i -= 1){
                    array[i + line_index_shift] = array[i] + move_text_shift;
                }
            }
            else{
                for (i64 i = node->old_line_first;
                     i < node->old_line_opl;
                     i += 1){
                    array[i + line_index_shift] = array[i] + move_text_shift;
                }
            }
        }
    }
    
    for (Line_Move *node = moves;
         node != 0;
         node = node->next){
  if (node->kind == LineMove_MeasureString){
   fill_line_starts(array + node->new_line_first, node->string, node->text_base);
  }
 }
}

internal Range_i64
buffer_get_pos_range_from_line_number(Gap_Buffer *buffer, i64 line_number)
{
 Range_i64 result = {};
 if (1 <= line_number && line_number < buffer->line_start_count)
 {
  result.first         = buffer->line_starts[line_number-1];
  result.one_past_last = buffer->line_starts[line_number];
 }
 return(result);
}

internal i64
buffer_get_first_pos_from_line_number(Gap_Buffer *buffer, i64 line_number){
 i64 result = 0;
 if (line_number < 1) {
  result = 0;
 }
 else if (line_number >= buffer->line_start_count) {
  result = buffer_size(buffer);
 }
 else {
  result = buffer->line_starts[line_number - 1];
 }
 return(result);
}

internal i64
buffer_get_last_pos_from_line_number(Gap_Buffer *buffer, i64 line_number) {
 i64 result = 0;
 if (line_number < 1) {
  result = 0;
 }
 else if (line_number >= buffer->line_start_count - 1) {
  result = buffer_size(buffer);
 }
 else {
  result = buffer->line_starts[line_number] - 1;
 }
 return(result);
}

internal Buffer_Cursor
buffer_cursor_from_pos(Gap_Buffer *buffer, i64 pos){
    i64 size = buffer_size(buffer);
    pos = clamp_between(0, pos, size);
    i64 line_index = buffer_get_line_index(buffer, pos);
    
    Buffer_Cursor result = {};
    result.pos = pos;
    result.line = line_index + 1;
    result.col = pos - buffer->line_starts[line_index] + 1;
    return(result);
}

internal Buffer_Cursor
buffer_cursor_from_line_col(Gap_Buffer *buffer, i64 line, i64 col){
    i64 line_index = line - 1;
    i64 line_count = buffer_line_count(buffer);
    line_index = clamp_between(0, line_index, line_count - 1);
    
    i64 this_start = buffer->line_starts[line_index];
    i64 max_col = (buffer->line_starts[line_index + 1] - this_start);
    if (line_index + 1 == line_count){
        max_col += 1;
    }
    max_col = clamp_min(1, max_col);
    
    if (col < 0){
        if (-col > max_col){
            col = 1;
        }
        else{
            col = max_col + col + 1;
        }
    }
    else if (col == 0){
        col = 1;
    }
    else{
        col = clamp_max(col, max_col);
    }
    Assert(col > 0);
    i64 adjusted_pos = col - 1;
    
    i64 pos = this_start + adjusted_pos;
    
    Buffer_Cursor result = {};
    result.pos = pos;
    result.line = line_index + 1;
    result.col = col;
    return(result);
}

internal String
buffer_invert_edit_shift(Arena *arena, Gap_Buffer *buffer, Edit edit, Edit *inv, i64 shift_amount){
    String string = buffer_stringify(arena, buffer, edit.range);
    inv->text = string;
    inv->range = Ii64(edit.range.start + shift_amount, edit.range.start + edit.text.size + shift_amount);
    return(string);
}

internal Buffer_Chunk_Position
buffer_get_chunk_position(String_Array chunks, i64 buffer_size, i64 real_pos){
    Buffer_Chunk_Position pos = {};
    pos.real_pos = real_pos;
    pos.chunk_pos = real_pos;
    if (pos.real_pos != buffer_size){
        for (;(i64)(chunks.vals[pos.chunk_index].size) <= pos.chunk_pos;){
            Assert(pos.chunk_index < chunks.count);
            pos.chunk_pos -= (i1)chunks.vals[pos.chunk_index].size;
            pos.chunk_index += 1;
        }
    }
    else{
        pos.chunk_index = chunks.count - 1;
        pos.chunk_pos = (i1)chunks.vals[pos.chunk_index].size;
    }
    return(pos);
}

internal i1
buffer_chunk_position_iterate(String_Array chunks, Buffer_Chunk_Position *pos, Scan_Direction direction){
    i1 past_end = 0;
    pos->real_pos += direction;
    pos->chunk_pos += direction;
    if (pos->chunk_pos < 0){
        if (pos->chunk_index == 0){
            past_end = -1;
        }
        else{
            pos->chunk_index -= 1;
            pos->chunk_pos = (i1)chunks.vals[pos->chunk_index].size - 1;
        }
    }
    else if (pos->chunk_pos >= (i64)(chunks.vals[pos->chunk_index].size)){
        pos->chunk_index += 1;
        if (pos->chunk_index == chunks.count){
            past_end = 1;
        }
        else{
            pos->chunk_pos = 0;
        }
    }
    return(past_end);
}

// BOTTOM

