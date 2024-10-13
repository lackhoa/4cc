//NOTE Generated at C:\Users\vodan\4ed\code/meta_main.cpp:174:
//  C:\Users\vodan\4ed\code/meta_main.cpp:175:
//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_v3v2(name, p0, d0, d3, p3, lp(radii), linum);

}
inline void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_v3v2(name, p0, d0, d3, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_parabola(String name, String p0, v3 d, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_parabola(String name, String p0, v3 d, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_parabola(name, p0, d, p3, lp(radii), linum);

}
inline void
send_bez_parabola(String name, String p0, v3 d, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_parabola(name, p0, d, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_offset(name, p0, d0, d3, p3, lp(radii), linum);

}
inline void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_offset(name, p0, d0, d3, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_unit(name, p0, d0, d3, unit_y, p3, lp(radii), linum);

}
inline void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_unit(name, p0, d0, d3, unit_y, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_unit2(name, p0, d0d3, unit_y, p3, lp(radii), linum);

}
inline void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_unit2(name, p0, d0d3, unit_y, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_c2(String name, String ref, v3 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_c2(String name, String ref, v3 d3, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_c2(name, ref, d3, p3, lp(radii), linum);

}
inline void
send_bez_c2(String name, String ref, v3 d3, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_c2(name, ref, d3, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_line(String name, String p0, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_line(String name, String p0, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_line(name, p0, p3, lp(radii), linum);

}
inline void
send_bez_line(String name, String p0, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_line(name, p0, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_bezd_old(name, p0, d0, d3, p3, lp(radii), linum);

}
inline void
send_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_bezd_old(name, p0, d0, d3, p3, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

//  C:\Users\vodan\4ed\code/meta_main.cpp:194:
xfunction void
send_bez_negateX(String name, String ref, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_negateX(String name, String ref, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_negateX(name, ref, lp(radii), linum);

}
inline void
send_bez_negateX(String name, String ref, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_negateX(name, ref, lp(radii), linum);

}
//  C:\Users\vodan\4ed\code/meta_main.cpp:253:
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

