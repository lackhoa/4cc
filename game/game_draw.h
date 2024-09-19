#pragma once

typedef u32 Line_Flags;
enum{
 Line_Overlay  = 0x1,
 Line_Straight = 0x2,
 Line_No_SymX  = 0x4,
};

typedef u32 Poly_Flags;
enum{
 Poly_Shaded  = 0x1,  //NOTE: Has to be 1, for compatibility with old code.
 Poly_Line    = 0x2,
 Poly_Overlay = 0x4,
};

struct Line_Params{
 v4 radii;
 v1 nslice_per_meter;
 v1 visibility;
 v1 alignment_threshold;
 argb color;
 Line_Flags flags;
};

struct Fill_Params{
 //NOTE(kv) Might need to make a different set of flags for fill versus polygons.
 // because the polygon routine is shared by both fills and lines.
 b32 non_default;
 Poly_Flags flags;
};

//~EOF