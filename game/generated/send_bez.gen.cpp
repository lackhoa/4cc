//NOTE Generated at C:\Users\vodan\4ed\code/meta_main.cpp:374:
//  C:\Users\vodan\4ed\code/meta_main.cpp:375:
function void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Vertex_Index p0_index = get_vertex_by_name(m, p0).index;
kv_assert(p0_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.v3v2={ .p0=p0_index, .d0=d0, .d3=d3, .p3=p3_index, }};
send_bez_func(name, Curve_Type_v3v2, data, params, linum);

}
function void
send_bez_parabola(String name, String p0, v3 d, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Vertex_Index p0_index = get_vertex_by_name(m, p0).index;
kv_assert(p0_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.parabola={ .p0=p0_index, .d=d, .p3=p3_index, }};
send_bez_func(name, Curve_Type_Parabola, data, params, linum);

}
function void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Vertex_Index p0_index = get_vertex_by_name(m, p0).index;
kv_assert(p0_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.offset={ .p0=p0_index, .d0=d0, .d3=d3, .p3=p3_index, }};
send_bez_func(name, Curve_Type_Offset, data, params, linum);

}
function void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Vertex_Index p0_index = get_vertex_by_name(m, p0).index;
kv_assert(p0_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.unit={ .p0=p0_index, .d0=d0, .d3=d3, .unit_y=unit_y, .p3=p3_index, }};
send_bez_func(name, Curve_Type_Unit, data, params, linum);

}
function void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Vertex_Index p0_index = get_vertex_by_name(m, p0).index;
kv_assert(p0_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.unit2={ .p0=p0_index, .d0d3=d0d3, .unit_y=unit_y, .p3=p3_index, }};
send_bez_func(name, Curve_Type_Unit2, data, params, linum);

}
function void
send_bez_c2(String name, String ref, v3 d3, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Curve_Index ref_index = get_curve_by_name(m, ref).index;
kv_assert(ref_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.c2={ .ref=ref_index, .d3=d3, .p3=p3_index, }};
send_bez_func(name, Curve_Type_C2, data, params, linum);

}
function void
send_bez_line(String name, String p0, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Vertex_Index p0_index = get_vertex_by_name(m, p0).index;
kv_assert(p0_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.line={ .p0=p0_index, .p3=p3_index, }};
send_bez_func(name, Curve_Type_Line, data, params, linum);

}
function void
send_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Vertex_Index p0_index = get_vertex_by_name(m, p0).index;
kv_assert(p0_index.v);
Vertex_Index p3_index = get_vertex_by_name(m, p3).index;
kv_assert(p3_index.v);
Curve_Union data = {.bezd_old={ .p0=p0_index, .d0=d0, .d3=d3, .p3=p3_index, }};
send_bez_func(name, Curve_Type_Bezd_Old, data, params, linum);

}
function void
send_bez_negateX(String name, String ref, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Curve_Index ref_index = get_curve_by_name(m, ref).index;
kv_assert(ref_index.v);
Curve_Union data = {.negateX={ .ref=ref_index, }};
send_bez_func(name, Curve_Type_NegateX, data, params, linum);

}
function void
send_bez_lerp(String name, String begin, String end, Line_Params params, i32 linum)
{
Modeler &m = *painter.modeler;
Curve_Index begin_index = get_curve_by_name(m, begin).index;
kv_assert(begin_index.v);
Curve_Index end_index = get_curve_by_name(m, end).index;
kv_assert(end_index.v);
Curve_Union data = {.lerp={ .begin=begin_index, .end=end_index, }};
send_bez_func(name, Curve_Type_Lerp, data, params, linum);

}
