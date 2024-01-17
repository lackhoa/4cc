
global Color_Table target_color_table = {};
global Color_Table cached_color_table = {};

// TODO(BYP): Check for way to get highest color sub_id instead of hard-coding 14
function Color_Table
byp_init_color_table(Application_Links *app){
	Color_Table result = make_color_table(app, &global_permanent_arena);
	foreach(i, result.count){
		result.arrays[i].vals = push_array(&global_permanent_arena, u32, 14);
		result.arrays[i].count = 14;
	}
	return result;
}

function void
byp_copy_color_table(Color_Table *dst, Color_Table table){
	foreach(i, table.count){
		Color_Array *nxt_array = &dst->arrays[i];
		Color_Array *array = &table.arrays[i];
		foreach(j, array->count){
			nxt_array->vals[j] = array->vals[j];
		}
	}
}
