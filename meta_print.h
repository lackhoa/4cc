#pragma once

struct Printer_Pair{
 Printer h;
 Printer c;
};
struct Meta_Type_Names {
 String type_name;
 String info_function_name;
 String global_info_name;
 String read_function_name;
};
global arrayof<Meta_Type_Names> meta_type_name_store;

struct Enclosed_in_strlit { String string; };
//
function void
print(Printer &p, Enclosed_in_strlit item){
 printn3(p, "strlit(\"", item.string, "\")");
}
force_inline Enclosed_in_strlit enclosed_in_strlit(String string){
 return Enclosed_in_strlit{ string, };
}

#define m_braces       defer_block((p << "\n{\n"), (p << "\n}\n"))
#define m_braces_sm    defer_block((p << "\n{\n"), (p << "\n};\n"))
#define m_macro_braces defer_block((p << "\\\n{\\\n"), (p << "\\\n}\\\n"))
#define m_macro_braces_sm  defer_block((p << "\\\n{\\\n"), (p << "\\\n};\\\n"))
#define m_location     p<<"// "<<      " "<<filename_linum<<"\n"
#define m_note(note)   p<<"// "<<note<<" "<<filename_linum<<"\n"

//-