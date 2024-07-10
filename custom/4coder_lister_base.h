/*
 * Lister base
 */

// TOP

#if !defined(FCODER_LISTER_BASE_H)
#define FCODER_LISTER_BASE_H

typedef i1 Lister_Activation_Code;
enum{
    ListerActivation_Finished = 0,
    ListerActivation_Continue = 1,
    ListerActivation_ContinueAndRefresh = 2,
};

typedef void Lister_Regenerate_List_Function_Type(App *app, struct Lister *lister);

struct Lister_Node{
    Lister_Node *next;
    Lister_Node *prev;
    String string;
    union{
        String status;
        i1 index;
    };
    void *user_data;
    i1 raw_index;
};

struct Lister_Node_List{
    Lister_Node *first;
    Lister_Node *last;
    i1 count;
};

struct Lister_Node_Ptr_Array{
    Lister_Node **node_ptrs;
    i1 count;
};

typedef Lister_Activation_Code Lister_Write_Character_Function(App *app);
typedef Lister_Activation_Code Lister_Key_Stroke_Function(App *app);
typedef void Lister_Navigate_Function(App *app,
                                      View_ID view, struct Lister *lister,
                                      i1 index_delta);

struct Lister_Handlers{
    Lister_Regenerate_List_Function_Type *refresh;
    Lister_Write_Character_Function *write_character;
    Custom_Command_Function *backspace;
    Lister_Navigate_Function *navigate;
    Lister_Key_Stroke_Function *key_stroke;
};

struct Lister_Result{
    b32 canceled;
    b32 activated_by_click;
    String text_field;
    void *user_data;
};

struct Lister{
    Arena *arena;
    Temp_Memory restore_all_point;
    
    Lister_Handlers handlers;
    
    Mapping *mapping;
    Command_Map *map;
    
    u8 query_space[256];
    u8 text_field_space[256];
    u8 key_string_space[256];
    String_u8 query;
    String_u8 text_field;
    String_u8 key_string;
    
    Lister_Node_List options;
    Temp_Memory filter_restore_point;
    Lister_Node_Ptr_Array filtered;
    
    b32 set_vertical_focus_to_item;
    Lister_Node *highlighted_node;
    void *hot_user_data;
    i1 item_index;
    i1 raw_item_index;
    
    Basic_Scroll scroll;
    i1 visible_count;
    
    Lister_Result out;
};

struct Lister_Prev_Current{
    Lister *prev;
    Lister *current;
};

struct Lister_Block{
    App *app;
    Lister_Prev_Current lister;
    Lister_Block(App *app, Arena *arena);
    ~Lister_Block();
    operator Lister *();
};

struct Lister_Prealloced_String{
    String string;
};

struct Lister_Filtered{
    Lister_Node_Ptr_Array exact_matches;
    Lister_Node_Ptr_Array before_extension_matches;
    Lister_Node_Ptr_Array substring_matches;
};

////////////////////////////////

struct Lister_Choice{
    Lister_Choice *next;
    String string;
    String status;
    Key_Code key_code;
    union{
        u64 user_data;
        void *user_data_ptr;
    };
};

struct Lister_Choice_List{
    Lister_Choice *first;
    Lister_Choice *last;
};

#endif

// BOTTOM

