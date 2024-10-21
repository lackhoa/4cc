//-
#if 0
typedef u32 Entity_Type_Flags;
enum{
 Entity_Type_Is_Curve = 0x1,
};
#endif
struct M_Entity_Variant_Info{
 b32 is_curve;
};
global M_Entity_Variant_Info m_entity_variant_info_table[256];
global i32 max_entity_enum;
//-
function void
generate_entity_types(Printer &p);
//-