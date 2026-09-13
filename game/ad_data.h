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
 Version_RecordedVertices     = 27,  // Recording.vertices table + Recorded_Primitive.corner_vertex
 Version_TvertPoly3Disk       = 28,  // Recorded_Poly3 / Disk.center become tvert (per-point bone)
 Version_NamedPresets         = 29,  // Preset_Settings grows name + scene; table = count + rows; Saved_Viewport.reference_preset removed
 Version_PresetsInStateFile   = 30,  // preset table + settings_size leave recording.ad (presets live in data/state.txt)
 Version_GroupTagsByName      = 31,  // Recorded_Group.vis_tag also written as its name after the raw block (enum order no longer matters)
 Version_RecordingOnly        = 32,  // vis-tag names gone again: driver.document.ad is self-describing now (ad_serialize_schema.cpp), recording.ad is the only raw-block file
 Version_CurveMidline         = 33,  // Recorded_Curve grows `midline` (plan-focus-radii-midline Q8)
 //-
 Version_OPL,
 Version_Inf                 = 0xFFFF,
};
// NOTE(kv) Only data/recording.ad (debug state) checks this; a bump just drops the old
// file. driver.document.ad carries its own schema instead of a version.
global Data_Version Version_Current = (Data_Version)(Version_OPL-1);

global Arena global_meta_arena_value;
global Arena *global_meta_arena = &global_meta_arena_value;

//~