//NOTE Generated at C:\Users\vodan\4ed\code/meta_main.cpp:163:
//  C:\Users\vodan\4ed\code/meta_main.cpp:164:
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

xfunction void
send_bez_offsets(String name, String p0, v3 d0, v3 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
inline void
send_bez_offsets(String name, String p0, v3 d0, v3 d3, String p3, i4 radii, i32 linum=__builtin_LINE())
{
send_bez_offsets(name, p0, d0, d3, p3, lp(radii), linum);

}
inline void
send_bez_offsets(String name, String p0, v3 d0, v3 d3, String p3, v4 radii, i32 linum=__builtin_LINE())
{
send_bez_offsets(name, p0, d0, d3, p3, lp(radii), linum);

}
#define bn_offsets(name, p0, d0, d3, p3, ...)\
send_bez_offsets(strlit(#name), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bs_offsets(p0, d0, d3, p3, ...)\
send_bez_offsets(strlit("l"), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)
#define bb_offsets(name, p0, d0, d3, p3, ...)\
bn_offsets(name, p0, d0, d3, p3, __VA_ARGS__); \
Bez name = bez_offsets(p0, d0, d3, p3);
#define ba_offsets(name, p0, d0, d3, p3, ...)\
bn_offsets(name, p0, d0, d3, p3, __VA_ARGS__); \
name = bez_offsets(p0, d0, d3, p3);

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

