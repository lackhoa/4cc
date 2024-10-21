//NOTE Generated at C:\Users\vodan\4ed\code/meta_entity.cpp:193:
//  C:\Users\vodan\4ed\code/meta_entity.cpp:195:
function Vertex_Index
get_p0_index_or_zero(Curve_Data &curve){switch(curve.type){case Curve_Type_v3v2: return {curve.data.v3v2.p0};
case Curve_Type_Parabola: return {curve.data.parabola.p0};
case Curve_Type_Offset: return {curve.data.offset.p0};
case Curve_Type_Unit: return {curve.data.unit.p0};
case Curve_Type_Unit2: return {curve.data.unit2.p0};
case Curve_Type_Line: return {curve.data.line.p0};
case Curve_Type_Bezd_Old: return {curve.data.bezd_old.p0};
case Curve_Type_Raw: return {curve.data.raw.p0};
case Curve_Type_C2: return {};
case Curve_Type_NegateX: return {};
case Curve_Type_Lerp: return {};
case Curve_Type_Circle: return {};
case Curve_Type_Fill3: return {};
case Curve_Type_Fill_Bez: return {};
case Curve_Type_Fill_DBez: return {};
}return {};}function Vertex_Index
get_p3_index_or_zero(Curve_Data &curve){switch(curve.type){case Curve_Type_v3v2: return {curve.data.v3v2.p3};
case Curve_Type_Parabola: return {curve.data.parabola.p3};
case Curve_Type_Offset: return {curve.data.offset.p3};
case Curve_Type_Unit: return {curve.data.unit.p3};
case Curve_Type_Unit2: return {curve.data.unit2.p3};
case Curve_Type_Line: return {curve.data.line.p3};
case Curve_Type_Bezd_Old: return {curve.data.bezd_old.p3};
case Curve_Type_Raw: return {curve.data.raw.p3};
case Curve_Type_C2: return {curve.data.c2.p3};
case Curve_Type_NegateX: return {};
case Curve_Type_Lerp: return {};
case Curve_Type_Circle: return {};
case Curve_Type_Fill3: return {};
case Curve_Type_Fill_Bez: return {};
case Curve_Type_Fill_DBez: return {};
}return {};}//  C:\Users\vodan\4ed\code/meta_entity.cpp:226:
//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_v3v2(name, p0, d0, d3, p3, lp(radii), linum);
}inline void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_v3v2(name, p0, d0, d3, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_v3v2(name, p0, d0, d3, p3, ...)\
send_bez_v3v2(strlit(#name), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bs_v3v2(p0, d0, d3, p3, ...)\
send_bez_v3v2(strlit("l"), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bb_v3v2(name, p0, d0, d3, p3, ...)\
bn_v3v2(name, p0, d0, d3, p3, __VA_ARGS__); \
Bez name = bez_v3v2(p0, d0, d3, p3);
#define ba_v3v2(name, p0, d0, d3, p3, ...)\
bn_v3v2(name, p0, d0, d3, p3, __VA_ARGS__); \
name = bez_v3v2(p0, d0, d3, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_parabola(String name, String p0, v3 d, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_parabola(String name, String p0, v3 d, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_parabola(name, p0, d, p3, lp(radii), linum);
}inline void
send_bez_parabola(String name, String p0, v3 d, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_parabola(name, p0, d, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_parabola(name, p0, d, p3, ...)\
send_bez_parabola(strlit(#name), strlit(#p0), d, strlit(#p3), __VA_ARGS__)
#define bs_parabola(p0, d, p3, ...)\
send_bez_parabola(strlit("l"), strlit(#p0), d, strlit(#p3), __VA_ARGS__)
#define bb_parabola(name, p0, d, p3, ...)\
bn_parabola(name, p0, d, p3, __VA_ARGS__); \
Bez name = bez_parabola(p0, d, p3);
#define ba_parabola(name, p0, d, p3, ...)\
bn_parabola(name, p0, d, p3, __VA_ARGS__); \
name = bez_parabola(p0, d, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_offset(name, p0, d0, d3, p3, lp(radii), linum);
}inline void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_offset(name, p0, d0, d3, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_offset(name, p0, d0, d3, p3, ...)\
send_bez_offset(strlit(#name), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bs_offset(p0, d0, d3, p3, ...)\
send_bez_offset(strlit("l"), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bb_offset(name, p0, d0, d3, p3, ...)\
bn_offset(name, p0, d0, d3, p3, __VA_ARGS__); \
Bez name = bez_offset(p0, d0, d3, p3);
#define ba_offset(name, p0, d0, d3, p3, ...)\
bn_offset(name, p0, d0, d3, p3, __VA_ARGS__); \
name = bez_offset(p0, d0, d3, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_unit(name, p0, d0, d3, unit_y, p3, lp(radii), linum);
}inline void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_unit(name, p0, d0, d3, unit_y, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_unit(name, p0, d0, d3, unit_y, p3, ...)\
send_bez_unit(strlit(#name), strlit(#p0), d0, d3, unit_y, strlit(#p3), __VA_ARGS__)
#define bs_unit(p0, d0, d3, unit_y, p3, ...)\
send_bez_unit(strlit("l"), strlit(#p0), d0, d3, unit_y, strlit(#p3), __VA_ARGS__)
#define bb_unit(name, p0, d0, d3, unit_y, p3, ...)\
bn_unit(name, p0, d0, d3, unit_y, p3, __VA_ARGS__); \
Bez name = bez_unit(p0, d0, d3, unit_y, p3);
#define ba_unit(name, p0, d0, d3, unit_y, p3, ...)\
bn_unit(name, p0, d0, d3, unit_y, p3, __VA_ARGS__); \
name = bez_unit(p0, d0, d3, unit_y, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_unit2(name, p0, d0d3, unit_y, p3, lp(radii), linum);
}inline void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_unit2(name, p0, d0d3, unit_y, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_unit2(name, p0, d0d3, unit_y, p3, ...)\
send_bez_unit2(strlit(#name), strlit(#p0), d0d3, unit_y, strlit(#p3), __VA_ARGS__)
#define bs_unit2(p0, d0d3, unit_y, p3, ...)\
send_bez_unit2(strlit("l"), strlit(#p0), d0d3, unit_y, strlit(#p3), __VA_ARGS__)
#define bb_unit2(name, p0, d0d3, unit_y, p3, ...)\
bn_unit2(name, p0, d0d3, unit_y, p3, __VA_ARGS__); \
Bez name = bez_unit2(p0, d0d3, unit_y, p3);
#define ba_unit2(name, p0, d0d3, unit_y, p3, ...)\
bn_unit2(name, p0, d0d3, unit_y, p3, __VA_ARGS__); \
name = bez_unit2(p0, d0d3, unit_y, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_line(String name, String p0, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_line(String name, String p0, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_line(name, p0, p3, lp(radii), linum);
}inline void
send_bez_line(String name, String p0, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_line(name, p0, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_line(name, p0, p3, ...)\
send_bez_line(strlit(#name), strlit(#p0), strlit(#p3), __VA_ARGS__)
#define bs_line(p0, p3, ...)\
send_bez_line(strlit("l"), strlit(#p0), strlit(#p3), __VA_ARGS__)
#define bb_line(name, p0, p3, ...)\
bn_line(name, p0, p3, __VA_ARGS__); \
Bez name = bez_line(p0, p3);
#define ba_line(name, p0, p3, ...)\
bn_line(name, p0, p3, __VA_ARGS__); \
name = bez_line(p0, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_bezd_old(name, p0, d0, d3, p3, lp(radii), linum);
}inline void
send_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_bezd_old(name, p0, d0, d3, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_bezd_old(name, p0, d0, d3, p3, ...)\
send_bez_bezd_old(strlit(#name), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bs_bezd_old(p0, d0, d3, p3, ...)\
send_bez_bezd_old(strlit("l"), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bb_bezd_old(name, p0, d0, d3, p3, ...)\
bn_bezd_old(name, p0, d0, d3, p3, __VA_ARGS__); \
Bez name = bez_bezd_old(p0, d0, d3, p3);
#define ba_bezd_old(name, p0, d0, d3, p3, ...)\
bn_bezd_old(name, p0, d0, d3, p3, __VA_ARGS__); \
name = bez_bezd_old(p0, d0, d3, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_raw(String name, String p0, v3 p1, v3 p2, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_raw(String name, String p0, v3 p1, v3 p2, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_raw(name, p0, p1, p2, p3, lp(radii), linum);
}inline void
send_bez_raw(String name, String p0, v3 p1, v3 p2, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_raw(name, p0, p1, p2, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_raw(name, p0, p1, p2, p3, ...)\
send_bez_raw(strlit(#name), strlit(#p0), p1, p2, strlit(#p3), __VA_ARGS__)
#define bs_raw(p0, p1, p2, p3, ...)\
send_bez_raw(strlit("l"), strlit(#p0), p1, p2, strlit(#p3), __VA_ARGS__)
#define bb_raw(name, p0, p1, p2, p3, ...)\
bn_raw(name, p0, p1, p2, p3, __VA_ARGS__); \
Bez name = bez_raw(p0, p1, p2, p3);
#define ba_raw(name, p0, p1, p2, p3, ...)\
bn_raw(name, p0, p1, p2, p3, __VA_ARGS__); \
name = bez_raw(p0, p1, p2, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_c2(String name, String ref, v3 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_c2(String name, String ref, v3 d3, String p3, i4 radii, i32 linum=__builtin_LINE()){
send_bez_c2(name, ref, d3, p3, lp(radii), linum);
}inline void
send_bez_c2(String name, String ref, v3 d3, String p3, v4 radii, i32 linum=__builtin_LINE()){
send_bez_c2(name, ref, d3, p3, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_c2(name, ref, d3, p3, ...)\
send_bez_c2(strlit(#name), strlit(#ref), d3, strlit(#p3), __VA_ARGS__)
#define bs_c2(ref, d3, p3, ...)\
send_bez_c2(strlit("l"), strlit(#ref), d3, strlit(#p3), __VA_ARGS__)
#define bb_c2(name, ref, d3, p3, ...)\
bn_c2(name, ref, d3, p3, __VA_ARGS__); \
Bez name = bez_c2(ref, d3, p3);
#define ba_c2(name, ref, d3, p3, ...)\
bn_c2(name, ref, d3, p3, __VA_ARGS__); \
name = bez_c2(ref, d3, p3);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_negateX(String name, String ref, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_negateX(String name, String ref, i4 radii, i32 linum=__builtin_LINE()){
send_bez_negateX(name, ref, lp(radii), linum);
}inline void
send_bez_negateX(String name, String ref, v4 radii, i32 linum=__builtin_LINE()){
send_bez_negateX(name, ref, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_negateX(name, ref, ...)\
send_bez_negateX(strlit(#name), strlit(#ref), __VA_ARGS__)
#define bs_negateX(ref, ...)\
send_bez_negateX(strlit("l"), strlit(#ref), __VA_ARGS__)
#define bb_negateX(name, ref, ...)\
bn_negateX(name, ref, __VA_ARGS__); \
Bez name = bez_negateX(ref);
#define ba_negateX(name, ref, ...)\
bn_negateX(name, ref, __VA_ARGS__); \
name = bez_negateX(ref);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_lerp(String name, String begin, String end, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_lerp(String name, String begin, String end, i4 radii, i32 linum=__builtin_LINE()){
send_bez_lerp(name, begin, end, lp(radii), linum);
}inline void
send_bez_lerp(String name, String begin, String end, v4 radii, i32 linum=__builtin_LINE()){
send_bez_lerp(name, begin, end, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_lerp(name, begin, end, ...)\
send_bez_lerp(strlit(#name), strlit(#begin), strlit(#end), __VA_ARGS__)
#define bs_lerp(begin, end, ...)\
send_bez_lerp(strlit("l"), strlit(#begin), strlit(#end), __VA_ARGS__)
#define bb_lerp(name, begin, end, ...)\
bn_lerp(name, begin, end, __VA_ARGS__); \
Bez name = bez_lerp(begin, end);
#define ba_lerp(name, begin, end, ...)\
bn_lerp(name, begin, end, __VA_ARGS__); \
name = bez_lerp(begin, end);

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_circle(String name, v3 center, v3 normal, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_circle(String name, v3 center, v3 normal, i4 radii, i32 linum=__builtin_LINE()){
send_bez_circle(name, center, normal, lp(radii), linum);
}inline void
send_bez_circle(String name, v3 center, v3 normal, v4 radii, i32 linum=__builtin_LINE()){
send_bez_circle(name, center, normal, lp(radii), linum);
}//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_circle(name, center, normal, ...)\
send_bez_circle(strlit(#name), center, normal, __VA_ARGS__)
#define bs_circle(center, normal, ...)\
send_bez_circle(strlit("l"), center, normal, __VA_ARGS__)

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_fill3(String name, String verts0, String verts1, String verts2, Line_Params params=lp(), i32 linum=__builtin_LINE());
//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_fill3(name, verts0, verts1, verts2, ...)\
send_bez_fill3(strlit(#name), strlit(#verts0), strlit(#verts1), strlit(#verts2), __VA_ARGS__)
#define bs_fill3(verts0, verts1, verts2, ...)\
send_bez_fill3(strlit("l"), strlit(#verts0), strlit(#verts1), strlit(#verts2), __VA_ARGS__)

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_fill_bez(String name, String curve, Line_Params params=lp(), i32 linum=__builtin_LINE());
//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_fill_bez(name, curve, ...)\
send_bez_fill_bez(strlit(#name), strlit(#curve), __VA_ARGS__)
#define bs_fill_bez(curve, ...)\
send_bez_fill_bez(strlit("l"), strlit(#curve), __VA_ARGS__)

//  C:\Users\vodan\4ed\code/meta_entity.cpp:239:
function void
send_bez_fill_dbez(String name, String curve1, String curve2, Line_Params params=lp(), i32 linum=__builtin_LINE());
//  C:\Users\vodan\4ed\code/meta_entity.cpp:276:
#define bn_fill_dbez(name, curve1, curve2, ...)\
send_bez_fill_dbez(strlit(#name), strlit(#curve1), strlit(#curve2), __VA_ARGS__)
#define bs_fill_dbez(curve1, curve2, ...)\
send_bez_fill_dbez(strlit("l"), strlit(#curve1), strlit(#curve2), __VA_ARGS__)

