//-
global u32 autodraw_data_magic = 'adda';
typedef u32 Data_Version;
enum
{
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
 Version_Remove_Binary_Again  = 17,
 Version_Remove_Camera_Pan    = 18,
 Version_AddCursorOn          = 19,
 Version_AddViewport          = 20,
 Version_AddReferencePreset   = 21,
 // NOTE(kv) recording.ad (ad_serialize_recording.cpp) requires version == current:
 // bump on ANY change to the recorded structs (Recorded_Primitive/Recorded_Group
 // and everything embedded), or the raw-block load misreads old files.
 Version_AddRecordingFile     = 22,
 Version_PresetSettingsTable  = 23,
 Version_PresetFieldXMacro    = 24,  // bool fields reordered after the i32s
 Version_MultiBonePoints      = 25,  // tvert grows bone_id (per-control-point bone refs)
 Version_ShapeKeys            = 26,  // Recorded_Curve / Dual_Bezier grow {key, deltas}
 //-
 Version_OPL,
 Version_Inf                 = 0xFFFF,
};
global Data_Version Version_Current = (Data_Version)(Version_OPL-1);

global Arena global_meta_arena_value;
global Arena *global_meta_arena = &global_meta_arena_value;

//~