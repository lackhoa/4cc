#pragma once

/*
* Hooks:
*  BeginBuffer     - [fold_begin_buffer_hook], [fold_begin_buffer_inner, fold_set_layouts]
*  BufferEditRange - [fold_buffer_edit_range_inner]
*  Tick            - [fold_tick], (only required if animating the folds)
*  RenderCaller    - [fold_draw] before draw_text_layout
*
* NOTE:
* Text_Layout_ID dependent rendering is potentially affected
*   and might not necessarily be correct (e.g. line numbers)
 *   To skip folded lines, check text_layout_line_on_screen for min != max
*/


#define FOLD_ANIMATE 1

#ifndef FOLD_ANIMATE
#define FOLD_ANIMATE 0
#endif

CUSTOM_ID(attachment, buffer_folds);

struct Fold_Params{
	Range_i64 range;
	f32 cur_active;
	b32 active;
};

// NOTE(BYP): Dynamic array with the lifetime of the buffer (reallocates using the buffer's scope's allocator)
// Nested folds are contiguous in memory where inner nested elements appear later in the array
// Partial overlaps are stretched to be full overlaps
struct Fold_List{
	Fold_Params *folds;
	i32 fold_count, fold_cap;
};


// NOTE(BYP): Recomputes the layout without modifying Dirty_State or History_Record_Index
function void fold_invalidate_buffer(App *app, Buffer_ID buffer){
	Dirty_State prev_state = buffer_get_dirty_state(app, buffer);
	buffer_replace_range(app, buffer, Ii64(0,0), empty_string);
	undo(app);
	buffer_set_dirty_state(app, buffer, prev_state);
}

function void fold_toggle(App *app, View_ID view, Buffer_ID buffer, i64 pos){
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list == 0){ return; }
	//Range_i64 line_range = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, pos));
	Range_i64 line_range = Ii64(view_get_cursor_pos(app, view));
	const i32 N = fold_list->fold_count;
	for(i32 i=0; i<N; i++){
		Fold_Params *fold = fold_list->folds + (N-1-i);
		if(range_overlap(fold->range, line_range)){
			fold->active ^= 1;
			if(!FOLD_ANIMATE){
				fold->cur_active = f32(fold->active);
				fold_invalidate_buffer(app, buffer);
			}
			view_set_cursor(app, view, seek_pos(fold->range.min));
			break;
		}
	}
}

function void fold_push(App *app, Buffer_ID buffer, Range_i64 range){
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list == 0){ return; }
	if(fold_list->fold_count+1 == fold_list->fold_cap){
		Base_Allocator *allocator = managed_scope_allocator(app, scope);
		fold_list->fold_cap = i32(1.5*fold_list->fold_cap);
		String new_data = base_allocate2(allocator, fold_list->fold_cap);
		if(new_data.size == 0){ return; }
		block_copy(new_data.str, fold_list->folds, fold_list->fold_count*sizeof(Fold_Params));
		base_free(allocator, fold_list->folds);
		fold_list->folds = (Fold_Params *)new_data.str;
	}

	range.min = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, range.min)).min;
	range.max = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, range.max)).max-1;

	Fold_Params *fold = fold_list->folds;
	i32 i=0;
	for(; i < fold_list->fold_count; i++){
		if(range_contains(range, fold->range.min) && range_contains(range, fold->range.max)){
			break;
		}else if(range_overlap(fold->range, range)){
			fold->range = range_union(fold->range, range);
		}else{
			break;
		}

		fold++;
	}

	block_copy(fold+1, fold, (fold_list->fold_count - i)*sizeof(Fold_Params));
	*fold = Fold_Params{ range, 0.f, 1 };
	fold_list->fold_count++;
	if(!FOLD_ANIMATE){
		fold->cur_active = f32(fold->active);
		fold_invalidate_buffer(app, buffer);
	}
}

function void fold_pop_inner(App *app, Buffer_ID buffer, Fold_List *fold_list, i32 index){
	Fold_Params *fold = fold_list->folds + index;
	u64 size = (fold_list->fold_count - index - 1)*sizeof(Fold_Params);
	block_copy(fold, fold+1, size);
	fold_list->fold_count--;
	fold_invalidate_buffer(app, buffer);
}

function void fold_pop(App *app, Buffer_ID buffer, i64 pos){
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list == 0){ return; }
	const i32 N = fold_list->fold_count;
	for(i32 i=0; i<N; i++){
		i32 index = N-1-i;
		Fold_Params *fold = fold_list->folds + index;
		if(range_contains(fold->range, pos)){
			fold_pop_inner(app, buffer, fold_list, index);
			break;
		}
	}
}

CUSTOM_COMMAND_SIG(fold_clear)
CUSTOM_DOC("Clears all folds in buffer")
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list){
		fold_list->fold_count = 0;
		fold_invalidate_buffer(app, buffer);
	}
}

CUSTOM_COMMAND_SIG(fold_pop_cursor)
CUSTOM_DOC("Pops fold at cursor")
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	fold_pop(app, buffer, view_get_cursor_pos(app, view));
}

CUSTOM_COMMAND_SIG(fold_toggle_cursor)
CUSTOM_DOC("Toggles fold at cursor")
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	fold_toggle(app, view, buffer, view_get_cursor_pos(app, view));
}

CUSTOM_COMMAND_SIG(fold_range)
CUSTOM_DOC("Folds cursor mark range")
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	fold_push(app, buffer, get_view_range(app, view));
}

// NOTE(BYP): Call after buffer text has been colored
function void fold_draw(App *app, Buffer_ID buffer, Text_Layout_ID text_layout_id){
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list == 0){ return; }
	Rect_f32 region = text_layout_region(app, text_layout_id);

	for(i32 i=0; i<fold_list->fold_count; i++){
		Fold_Params *fold = fold_list->folds + i;

		Range_i64 range = fold->range;
		i64 line_num = get_line_number_from_pos(app, buffer, range.min);

		if(fold->active){
			ARGB_Color folding_color = fcolor_resolve(fcolor_id(defcolor_text_default));
			ARGB_Color fold_color = argb_color_blend(folding_color, fold->cur_active, folding_color & 0x00FFFFFF);

			Range_i64 line_range = get_line_pos_range(app, buffer, line_num);
			Range_f32 line_range_pixels = text_layout_line_on_screen(app, text_layout_id, line_num);
			if(line_range_pixels.min == line_range_pixels.max){
				line_range = {};
			}else{
				range.min = line_range.max;
			}

			paint_text_color(app, text_layout_id, range, fold_color);
			paint_text_color(app, text_layout_id, line_range, fcolor_resolve(fcolor_id(defcolor_ghost_character)));

			//for(i32 j=i+1; j<fold_list->fold_count; j++){
			//Fold_Params *next_fold = fold_list->folds + j;
			//i = j;
			//if(!(fold->range.min <= next_fold->range.min && next_fold->range.max <= fold->range.max)){
			//break;
			//}
			//}
		}else{
			i32 nest_level = 1;
			for(i32 j=i-1; j>=0; j--){
				Fold_Params *prev_fold = fold_list->folds + j;
				if(prev_fold->range.min <= fold->range.min && fold->range.max <= prev_fold->range.max){
					nest_level++;
				}
			}
			Rect_f32 rect = rect_split_left_right(region, (f32)nest_level*3.f).a;
			Rect_f32 prev_clip = draw_set_clip(app, rect);
			Range_i64 line_nums = Ii64(get_line_number_from_pos(app, buffer, fold->range.min),
									   get_line_number_from_pos(app, buffer, fold->range.max));
			draw_line_highlight(app, text_layout_id, line_nums, fcolor_id(defcolor_highlight));
			draw_set_clip(app, prev_clip);
		}

		Range_f32 y = text_layout_line_on_screen(app, text_layout_id, line_num);
		if(range_size(y) > 0.f){
			Range_f32 x = rect_range_x(region);
			x.max *= fold->cur_active;
			draw_rect_fcolor(app, Rf32(x, y), 0.f, fcolor_id(defcolor_highlight));
		}
	}
}

function Layout_Item_List fold_items(App *app, Buffer_ID buffer, Layout_Item_List list){
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Fold_List *fold_list_ = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list_ == 0){ return list; }
	Fold_List fold_list = *fold_list_;

	//Range_i64 range = list.manifested_index_range;
	Range_i64 range = list.input_index_range;
	f32 line_height = get_face_metrics(app, get_face_id(app, buffer)).line_height;
	i64 base_line_num = get_line_number_from_pos(app, buffer, range.min);

	for(i32 i=0; i < fold_list.fold_count; i++){
		Fold_Params *fold = fold_list.folds + i;
		if(fold->cur_active > 0.1f && range_overlap(fold->range, range)){
			i64 fold_line_num = get_line_number_from_pos(app, buffer, fold->range.min);

			if(base_line_num == fold_line_num){ break; }
			list.height *= (1.f - fold->cur_active);

			Layout_Item_Block *block = list.first;
			for(;;){
				for(i32 j=0; j < block->item_count; j++){
					block->items[j].rect.x0 -= fold->cur_active*line_height;
					block->items[j].rect.x1 -= fold->cur_active*line_height;
					//block->items[j].rect.y0 -= fold->cur_active*line_height;
					//block->items[j].rect.y1 -= fold->cur_active*line_height;
				}

				if(block == list.last || block->next == 0){ break; }
				block = block->next;
			}
			break;
		}
	}

	return list;
}

function void fold_tick(App *app, Frame_Info frame_info){
#if FOLD_ANIMATE
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list == 0){ return; }
	for(i32 i=0; i < fold_list->fold_count; i++){
		Fold_Params *fold = fold_list->folds + i;
		f32 nxt_active = f32(fold->active);
		f32 diff = (nxt_active - fold->cur_active);
		if(fabs(diff) > 0.01f){
			fold->cur_active += diff*frame_info.animation_dt*30.f;
			fold_invalidate_buffer(app, buffer);
			animate_in_n_milliseconds(app, 0);
		}else{
			fold->cur_active = nxt_active;
		}
	}
#endif
}

function void fold_begin_buffer_inner(App *app, Buffer_ID buffer_id)
{
	Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
	Base_Allocator *allocator = managed_scope_allocator(app, scope);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list == 0){ return; }
	String fold_data = base_allocate2(allocator, 10*sizeof(Fold_Params));
	fold_list->folds = (Fold_Params *)fold_data.str;
	fold_list->fold_cap = 10;
	fold_list->fold_count = 0;
}

function void fold_buffer_edit_range_inner(App *app, Buffer_ID buffer_id, Range_i64 new_range, Range_Cursor old_cursor_range){
	Range_i64 old_range = Ii64(old_cursor_range.min.pos, old_cursor_range.max.pos);
	i64 text_shift = replace_range_shift(old_range, range_size(new_range));

	i64 edit_begin = old_cursor_range.min.pos;
	i64 edit_end   = old_cursor_range.max.pos;

	Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
	Fold_List *fold_list = scope_attachment(app, scope, buffer_folds, Fold_List);
	if(fold_list == 0){ return; }

	for(i32 i=0; i<fold_list->fold_count; i++){
		Fold_Params *fold = fold_list->folds + i;
		Range_i64 fold_range = fold->range;
  
		if(fold_range.max <= edit_begin && fold_range.max <= edit_end){
			// Edit after the fold
			continue;
		}else if(edit_begin <= fold_range.min && edit_end <= fold_range.min){
			// Edit before the fold
			fold_range.min += text_shift;
			fold_range.max += text_shift;
		}else if(range_contains(fold_range, edit_begin) && range_contains(fold_range, edit_end)){
			// Edit within fold
			fold_range.max += text_shift;
		}else{
			// Edit past the fold
			fold_range = {};
		}

		if(fold_range.max <= fold_range.min){
			fold_pop_inner(app, buffer_id, fold_list, i);
			i--;
			continue;
		}
  
		fold->range.min = get_line_pos_range(app, buffer_id, get_line_number_from_pos(app, buffer_id, fold_range.min)).min;
		fold->range.max = get_line_pos_range(app, buffer_id, get_line_number_from_pos(app, buffer_id, fold_range.max)).max;
	}
}

//-