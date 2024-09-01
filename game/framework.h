#pragma once

enum Data_Version
{
 Version_Init            = 7,
 Version_Add_Bezier_Type = 8,
};
global_const u32 data_current_version = Version_Add_Bezier_Type;

struct Data_Reader
{
 Data_Version read_version;
 STB_Parser *parser;
};

//-eof