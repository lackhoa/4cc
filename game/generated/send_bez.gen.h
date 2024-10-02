//NOTE Generated at C:\Users\vodan\4ed\code/meta_main.cpp:164:
//  C:\Users\vodan\4ed\code/meta_main.cpp:165:
xfunction void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
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
send_bez_v2v2v3(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
#define bn_v2v2v3(name, p0, d0, d3, unit_y, p3, ...)\
send_bez_v2v2v3(strlit(#name), strlit(#p0), d0, d3, unit_y, strlit(#p3), __VA_ARGS__)
#define bs_v2v2v3(p0, d0, d3, unit_y, p3, ...)\
send_bez_v2v2v3(strlit("l"), strlit(#p0), d0, d3, unit_y, strlit(#p3), __VA_ARGS__)
#define bb_v2v2v3(name, p0, d0, d3, unit_y, p3, ...)\
bn_v2v2v3(name, p0, d0, d3, unit_y, p3, __VA_ARGS__); \
Bez name = bez_v2v2v3(p0, d0, d3, unit_y, p3);
#define ba_v2v2v3(name, p0, d0, d3, unit_y, p3, ...)\
bn_v2v2v3(name, p0, d0, d3, unit_y, p3, __VA_ARGS__); \
name = bez_v2v2v3(p0, d0, d3, unit_y, p3);

xfunction void
send_bez_c2(String name, Curve_Index ref, v3 d3, String p3, Line_Params params=lp(), i32 linum=__builtin_LINE());
#define bn_c2(name, ref, d3, p3, ...)\
send_bez_c2(strlit(#name), ref, d3, strlit(#p3), __VA_ARGS__)
#define bs_c2(ref, d3, p3, ...)\
send_bez_c2(strlit("l"), ref, d3, strlit(#p3), __VA_ARGS__)
#define bb_c2(name, ref, d3, p3, ...)\
bn_c2(name, ref, d3, p3, __VA_ARGS__); \
Bez name = bez_c2(ref, d3, p3);
#define ba_c2(name, ref, d3, p3, ...)\
bn_c2(name, ref, d3, p3, __VA_ARGS__); \
name = bez_c2(ref, d3, p3);

