#pragma once

enum Data_Version
{
 Version_Init                 = 7,
 Version_Add_Bezier_Type      = 8,
 Version_Rename_Object_Index  = 9,
 Version_Rename_Object_Index2 = 10,
 Version_Bezier_Revamp        = 11,
 //-
 Version_OPL,
 Version_Inf                 = 0xFFFF,
};
global Data_Version Version_Current = Data_Version(Version_OPL-1);

struct Data_Reader
{
 Data_Version read_version;
 STB_Parser *parser;
};

//-eof