//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:272:
// NOTE: source: C:\Users\vodan\4ed\code\game\test.kc
#pragma once
// Generates: test.gen.cpp
function Arm
render_arm(){

v3 function_condyle, external_condyle;
{
va(function_condyle, V3(-0.1053f, -1.0756f, -0.1775f));
va(external_condyle, function_condyle+(V3(0.4031f, 0.1064f, 0.0402f)));
{
hl_block;
vv(sock, (V3(-0.0675f, 0.733f, -0.2688f)));
{
vv(v89, function_condyle+(V3(0.127f, 0.1897f, -0.f)));
bs_parabola(sock, V3(0.137f, 0.5108f, 0.0235f), v89, lp(V4(0.5f)));
bs_unit2(v89, V4(0.f, 0.4207f, 0.f, 0.f), V3(0.5647f, -0.8253f, 0.f), function_condyle, lp(0.5f*big_to_small()));
}
{
vv(a, external_condyle+V3(-0.0777f, 0.3589f, -0.0402f));
vv(b, sock+V3(0.2662f, -0.0344f, 0.0375f));
bs_unit2(a, V4(0.f, 1.0357f, 0.f, 0.f), V3(-0.1623f, -0.9867f, 0.f), external_condyle);
bs_parabola(b, V3(-0.0431f, 0.296f, 0.f), a, lp(.5f));
vv(c, b+V3(-0.152f, 0.2585f, 0.f));
bs_v3v2(b, V3(0.1369f, 0.0569f, 0.f), V2(0.0914f, 0.1488f), c, lp(.5f));
}
vv(v98, sock+(V3(0.0947f, 0.1683f, 0.2105f)));
vv(v99, v98+(V3(-0.f, -0.6114f, -0.0063f)));
bs_parabola(v98, V3(-0.f, -0.1087f, 0.0612f), v99, lp(.9f));
vv(v01, v99+(V3(0.f, -1.2272f, 0.03f)));
bs_v3v2(v99, (V3(-0.f, 0.f, -0.0834f)), V2(0.2691f, 0.0874f), v01, lp(.9f));
vv(v20, v01+fvert(V3(-0.0184f, -0.1643f, -0.0453f)));
bs_parabola(v01, fval(V3(0.f, 0.0368f, 0.1927f)), v20, lp(.9f));
}
}}
