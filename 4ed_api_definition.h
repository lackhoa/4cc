/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 03.10.2019
 *
 * System API definition types.
 *
 */

// TOP

#if !defined(FRED_API_DEFINITION_H)
#define FRED_API_DEFINITION_H

struct API_Param{
    API_Param *next;
    String type_name;
    String name;
};

struct API_Param_List{
    API_Param *first;
    API_Param *last;
    i1 count;
};

struct API_Call{
    API_Call *next;
    String name;
    String return_type;
    String location_string;
    API_Param_List params;
};

typedef i1 API_Type_Structure_Kind;
enum{
    APITypeStructureKind_Struct,
    APITypeStructureKind_Union,
};
struct API_Type_Structure{
    API_Type_Structure_Kind kind;
    List_String member_names;
    String definition_string;
};

struct API_Enum_Value{
    API_Enum_Value *next;
    String name;
    String val;
};
struct API_Type_Enum{
    String type_name;
    API_Enum_Value *first_val;
    API_Enum_Value *last_val;
    i1 val_count;
};

struct API_Type_Typedef{
    String name;
    String definition_text;
};

typedef i1 API_Type_Kind;
enum{
    APITypeKind_Structure,
    APITypeKind_Enum,
    APITypeKind_Typedef,
};
struct API_Type{
    API_Type *next;
    API_Type_Kind kind;
    String name;
    String location_string;
    union{
        API_Type_Structure struct_type;
        API_Type_Enum enum_type;
        API_Type_Typedef typedef_type;
    };
};

struct API_Definition{
    API_Definition *next;
    
    API_Call *first_call;
    API_Call *last_call;
    i1 call_count;
    
    API_Type *first_type;
    API_Type *last_type;
    i1 type_count;
    
    String name;
};

struct API_Definition_List{
    API_Definition *first;
    API_Definition *last;
    i1 count;
};

typedef u32 API_Generation_Flag;
enum{
    APIGeneration_NoAPINameOnCallables = 1,
};

typedef u32 API_Check_Flag;
enum{
    APICheck_ReportMissingAPI = 1,
    APICheck_ReportExtraAPI = 2,
    APICheck_ReportMismatchAPI = 4,
};
enum{
    APICheck_ReportAll = APICheck_ReportMissingAPI|APICheck_ReportExtraAPI|APICheck_ReportMismatchAPI,
};

#endif

// BOTTOM

