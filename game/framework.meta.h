// C:\Users\vodan\4ed\code/meta_introspect.cpp:165:
function Type_Info
get_type_info_Keyboard_Cursor()
{
Type_Info result = {};
result.name = strlit("Keyboard_Cursor");
result.size = sizeof(Keyboard_Cursor);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Keyboard_Cursor, pos)};
result.members[1] = {.type=&Type_Info_v1, .name=strlit("vel"), .offset=offsetof(Keyboard_Cursor, vel)};
return result;
}
// C:\Users\vodan\4ed\code/meta_introspect.cpp:140:
global Type_Info Type_Info_Keyboard_Cursor = get_type_info_Keyboard_Cursor();

function Type_Info &type_info_from_pointer(Keyboard_Cursor*pointer)
{
return Type_Info_Keyboard_Cursor;
}
function void
read_Keyboard_Cursor(Data_Reader &r, Keyboard_Cursor &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
v3 m_pos = {};

{
eat_id(p, strlit("pos"));
read_v3(r, m_pos);
}
pointer.pos = m_pos;

v1 m_vel = {};

{
eat_id(p, strlit("vel"));
read_v1(r, m_vel);
}
pointer.vel = m_vel;

eat_char(p, '}');
}
// C:\Users\vodan\4ed\code/meta_introspect.cpp:165:
function Type_Info
get_type_info_Serialized_State()
{
Type_Info result = {};
result.name = strlit("Serialized_State");
result.size = sizeof(Serialized_State);
result.kind = Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_Keyboard_Cursor, .name=strlit("kb_cursor"), .offset=offsetof(Serialized_State, kb_cursor)};
return result;
}
// C:\Users\vodan\4ed\code/meta_introspect.cpp:140:
global Type_Info Type_Info_Serialized_State = get_type_info_Serialized_State();

function Type_Info &type_info_from_pointer(Serialized_State*pointer)
{
return Type_Info_Serialized_State;
}
function void
read_Serialized_State(Data_Reader &r, Serialized_State &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Keyboard_Cursor m_kb_cursor = {};

{
eat_id(p, strlit("kb_cursor"));
read_Keyboard_Cursor(r, m_kb_cursor);
}
pointer.kb_cursor = m_kb_cursor;

eat_char(p, '}');
}
// C:\Users\vodan\4ed\code/meta_introspect.cpp:410:
#define Serialized_State_Embed \
 union\
{\
struct\
{\
Keyboard_Cursor kb_cursor;\
\
};\
Serialized_State Serialized_State;\
};\
