#pragma once

global u32 game_save_magic_number;
global u32 game_save_version;

enum Data_Version{
 Version_Init                 = 7,
 Version_Add_Curve_Type      = 8,
 Version_Rename_Object_Index  = 9,
 Version_Rename_Object_Index2 = 10,
 Version_Bezier_Revamp        = 11,
 Version_Add_Cursor           = 12,
 Version_Remove_Bone_Index    = 13,
 //-
 Version_OPL,
 Version_Inf                 = 0xFFFF,
};
global Data_Version Version_Current = Data_Version(Version_OPL-1);

struct Data_Reader{
 Data_Version read_version;
 STB_Parser *parser;
};

// NOTE: ;read_basic_types
#define X(T) \
force_inline void \
read_##T(Data_Reader &r, T &DST)  \
{ DST = eat_##T(r.parser) ; }
//
X_Basic_Types(X)
//
#undef X


//~