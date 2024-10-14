#pragma once

struct Printer_Pair{
 Printer h;
 Printer c;
};
inline b32 okp(Printer &p){ return p.FILE != 0; }
inline b32 okp(Printer_Pair &ps){ return ps.h.FILE && ps.c.FILE; }
inline void close_file(Printer &p){ close_file(p.FILE); }
inline void close_pair(Printer_Pair &ps){ close_file(ps.h.FILE); close_file(ps.c.FILE); }

struct Meta_Type_Names{
 String type_name;
 String info_function_name;
 String global_info_name;
 String read_function_name;
};
global arrayof<Meta_Type_Names> meta_type_name_store;

struct Enclosed_in_strlit{ String string; };
function void
print(Printer &p, Enclosed_in_strlit item){
 printn3(p, "strlit(\"", item.string, "\")");
}
inline Enclosed_in_strlit
enclosed_in_strlit(String string){ return Enclosed_in_strlit{ string, }; }
//-
struct Repeated_Printee{
 String string;
 i32 count;
};
inline Repeated_Printee
repeated(String string, i32 count){ return {string, count}; }
inline Repeated_Printee
repeated(char *string, i32 count){ return {SCu8(string), count}; }
function void
print(Printer &p, Repeated_Printee item){
 for_repeat(item.count){ p < item.string; };
}
//-
function void
print_comma_separated(Printer &p, arrayof<String> list){
 for_i32(i,0,list.count){
  if(i){ p<", "; }
  p<list.items[i];
 }
}

#define m_parens       defer_block((p << "("), (p << ")"))
#define m_comma_list   defer_block(((p << "("), begin_separator(p, ", ")),\
((p << ")"), end_separator(p)))
#define m_braces       defer_block((p << "\n{\n"), (p << "\n}\n"))
#define m_braces_sm    defer_block((p << "\n{\n"), (p << "\n};\n"))
#define m_macro_braces defer_block((p << "\\\n{\\\n"), (p << "\\\n}\\\n"))
#define m_macro_braces_sm  defer_block((p << "\\\n{\\\n"), (p << "\\\n};\\\n"))
#define m_locationp(p) p<<"// "<<      " "<<filename_linum<<"\n"
#define m_location     m_locationp(p)
#define m_note(note)   p<<"// "<<note<<" "<<filename_linum<<"\n"

//-