//-
global u32 autodraw_data_magic = 'adda';
typedef u32 Data_Version;
enum{
 Version_Init                 = 7,
 Version_Add_Curve_Type       = 8,
 Version_Rename_Object_Index  = 9,
 Version_Rename_Object_Index2 = 10,
 Version_Bezier_Revamp        = 11,
 Version_Add_Cursor           = 12,
 Version_Remove_Bone_Index    = 13,
 Version_We_So_Back           = 14,
 Version_Binary               = 15,
 Version_Binary_Vertex        = 16,
 //-
 Version_OPL,
 Version_Inf                 = 0xFFFF,
};
global Data_Version Version_Current = (Data_Version)(Version_OPL-1);

global Arena global_meta_arena_value;
global Arena *global_meta_arena = &global_meta_arena_value;

//~