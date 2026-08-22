#pragma once

api(custom)
typedef u32 Text_Layout_ID;

struct App
{
 struct Thread_Context *tctx;
 void *cmd_context;
};
// NOTE(kv) C++ needs App to be defined,
//  otw it won't allow App_Cmd to be a subclass of App.
//  VERY GOOD OOP LANGUAGE!!!
struct App_Cmd : App
{
 b32 automated;
};
struct Models;
myinline Models *
app_get_models(App *app)
{
 return (Models *)app->cmd_context;
}
myinline App_Cmd
app_cmd(App *app, b32 automated)
{
 App_Cmd result = {};
 (App &)result = *app;
 result.automated = automated;
 return result;
}
myinline App_Cmd
app_cmd_automated(App *app)
{
 App_Cmd result = {};
 (App &)result = *app;
 result.automated = true;
 return result;
}

typedef void Custom_Command_Function(App_Cmd *app);

#define BODY  return(clamp_min(0, a.max - a.min));
function i32 range_size(Range_i32 a){ BODY; }
function i64 range_size(Range_i64 a){ BODY; }
function u64 range_size(Range_u64 a){ BODY; }
function f32 range_size(Range_f32 a){ BODY; }
#undef BODY

api(custom)
typedef i32 View_ID;

api(custom)
typedef i32 Buffer_ID;

api(custom)
typedef u32 Face_ID;

struct Texture_Handle{ u32 v; };
// NOTE Reserve zero, checked true on opengl
// https://stackoverflow.com/questions/18193327/when-are-opengl-object-names-ever-zero-or-does-zero-ever-have-semantics
myinline b32
is_valid(Texture_Handle texture)
{
 return texture.v != 0;
}

api(custom)
struct Mouse_State
{
 b8 left;
 b8 right;
 b8 press_left;
 b8 press_right;
 b8 release_left;
 b8 release_right;
 b8 out_of_window;
 i32 wheel;
 union {
  i2 p;
  struct{ i32 x; i32 y; };
 };
};

api(custom)
struct Frame_Info{
 i32 index;
 v1 literal_dt;
 v1 animation_dt;
 u32 work_cycles;
 v1  work_useconds;
 u32 hot_prim_id;
};

enum{
 Access_Always = 0,
 Access_ReadWrite = Access_Write|Access_Read,
 Access_ReadVisible = Access_Read|Access_Visible,
 Access_ReadWriteVisible = Access_Write|Access_Read|Access_Visible,
};

api(custom)
typedef i32 Buffer_Seek_Type;
enum
{
 buffer_seek_pos,
 buffer_seek_line_col,
};

api(custom)
struct Buffer_Seek
{
 Buffer_Seek_Type type;
 union
 {
  struct { i64 pos; };
  struct { i64 line; i64 col; };
 };
};

api(custom)
struct Buffer_Cursor
{
 i64 pos;
 i64 line;
 i64 col;
};

api(custom)
union Range_Cursor{
 struct{
  Buffer_Cursor min;
  Buffer_Cursor max;
 };
 struct{
  Buffer_Cursor start;
  Buffer_Cursor end;
 };
 struct{
  Buffer_Cursor first;
  Buffer_Cursor one_past_last;
 };
};

api(custom)
typedef u32 Set_Buffer_Flag;
enum{
 SetBuffer_KeepOriginalGUI = 0x1
};

#if !AD_IS_GAME
api(custom)
struct Face_Metrics{
    f32 text_height;
    f32 line_height;
    f32 ascent;
    f32 descent;
    f32 line_skip;
    
    f32 underline_yoff1;
    f32 underline_yoff2;
    
    f32 max_advance;
    f32 space_advance;
    f32 decimal_digit_advance;
    f32 hex_digit_advance;
    f32 byte_advance;
    f32 byte_sub_advances[3];
    f32 normal_lowercase_advance;
    f32 normal_uppercase_advance;
    f32 normal_advance;
};

api(custom)
typedef u16 ID_Color;

api(custom)
union FColor{
    struct{
        u8 padding__[3];
        u8 a_byte;
    };
    ARGB_Color argb;
    struct{
  ID_Color id;
  u8 sub_index;
  u8 padding_;
 };
};

api(custom)
typedef u64 Managed_ID;

#include "4coder_table.h"

api(custom)
struct Codepoint_Index_Map{
 b32 has_zero_index;
 u16 zero_index;
 u16 max_index;
 Table_u32_u16 table;
};

struct Coroutine;

api(custom)
struct Thread_Context_Extra_Info{
 Coroutine *coroutine;
 void *async_thread;
};

////////////////////////////////

api(custom)
struct Theme_Color{
    ID_Color tag;
    ARGB_Color color;
};

api(custom)
struct Color_Picker{
    String title;
    ARGB_Color *dest;
    b32 *finished;
};

////////////////////////////////

api(custom)
typedef i32 Panel_ID;

api(custom)
typedef u32 Child_Process_ID;

api(custom)
typedef i32 UI_Highlight_Level;
enum{
    UIHighlight_None,
    UIHighlight_Hover,
    UIHighlight_Active,
};

api(custom)
struct Buffer_Point{
    i64 line_number;
    v2 pixel_shift;
};

api(custom)
struct Line_Shift_Vertical{
    i64 line;
    f32 y_delta;
};

api(custom)
struct Line_Shift_Character{
    i64 line;
    i64 character_delta;
};

api(custom)
typedef u32 Child_Process_Set_Target_Flags;
enum{
    ChildProcessSet_FailIfBufferAlreadyAttachedToAProcess = 1,
    ChildProcessSet_FailIfProcessAlreadyAttachedToABuffer = 2,
    ChildProcessSet_NeverOverrideExistingAttachment = 3,
    ChildProcessSet_CursorAtEnd = 4,
};

api(custom)
typedef u32 Memory_Protect_Flags;
enum{
    MemProtect_Read    = 0x1,
    MemProtect_Write   = 0x2,
    MemProtect_Execute = 0x4,
};

api(custom)
typedef i32 Wrap_Indicator_Mode;
enum{
    WrapIndicator_Hide,
    WrapIndicator_Show_After_Line,
    WrapIndicator_Show_At_Wrap_Edge,
};

api(custom)
typedef i32 Global_Setting_ID;
enum{
 GlobalSetting_Null,
 GlobalSetting_LAltLCtrlIsAltGr,
};

api(custom)
typedef u32 Buffer_Setting_ID;
enum {
 BufferSetting_Unimportant    = 0x1,
 BufferSetting_ReadOnly       = 0x2,
 BufferSetting_RecordsHistory = 0x4,
 BufferSetting_Unkillable     = 0x8,
 BufferSetting_IsGame         = 0x10,
};

api(custom)
struct Character_Predicate{
 u8 b[32];
};

api(custom)
typedef i32 View_Setting_ID;
enum{
 ViewSetting_Null,
 ViewSetting_ShowWhitespace,
 ViewSetting_ShowScrollbar,
 ViewSetting_ShowFileBar,
};

api(custom)
typedef u32 Buffer_Create_Flag;
enum{
 BufferCreate_Background          = 0x1,
 BufferCreate_AlwaysNew           = 0x2,
 BufferCreate_NeverNew            = 0x4,
 BufferCreate_JustChangedFile     = 0x8,
 BufferCreate_MustAttachToFile    = 0x10,
 BufferCreate_NeverAttachToFile   = 0x20,
 BufferCreate_SuppressNewFileHook = 0x40,
 BufferCreate_LimitedEdit         = 0x80,
};

api(custom)
typedef u32 Buffer_Save_Flag;
enum{
    BufferSave_IgnoreDirtyFlag = 0x1,
};

api(custom)
typedef u32 Buffer_Kill_Flag;
enum{
    BufferKill_AlwaysKill  = 0x2,
};

api(custom)
typedef u32 Buffer_Reopen_Flag;
enum{};

api(custom)
typedef u32 Buffer_Kill_Result;
enum{
    BufferKillResult_Killed = 0,
    BufferKillResult_Dirty = 1,
    BufferKillResult_Unkillable = 2,
    BufferKillResult_DoesNotExist = 3,
};

api(custom)
typedef u32 Buffer_Reopen_Result;
enum{
    BufferReopenResult_Reopened = 0,
    BufferReopenResult_Failed = 1,
};

api(custom)
typedef i32 Dirty_State;
enum{
    DirtyState_UpToDate = 0,
    DirtyState_UnsavedChanges = 1,
    DirtyState_UnloadedChanges = 2,
    DirtyState_UnsavedChangesAndUnloadedChanges = 3,
};

api(custom)
typedef u32 Command_Line_Interface_Flag;
enum{
    CLI_OverlapWithConflict = 0x1,
    CLI_AlwaysBindToView    = 0x2,
    CLI_CursorAtEnd         = 0x4,
    CLI_SendEndSignal       = 0x8,
};


api(custom)
typedef i32 Mouse_Cursor_Show_Type;
enum{
    MouseCursorShow_Never,
    MouseCursorShow_Always,
};

api(custom)
typedef i32 View_Split_Position;
enum{
    ViewSplit_Top,
    ViewSplit_Bottom,
    ViewSplit_Left,
    ViewSplit_Right,
};

api(custom)
typedef i32 Panel_Split_Kind;
enum{
    PanelSplitKind_Ratio_Min = 0,
    PanelSplitKind_Ratio_Max = 1,
    PanelSplitKind_FixedPixels_Min = 2,
    PanelSplitKind_FixedPixels_Max = 3,
};

api(custom)
typedef u8 Key_Modifier;

api(custom)
struct Parser_String_And_Type{
    char *str;
    u32 length;
    u32 type;
};

api(custom)
struct Buffer_Identifier{
    char *name;
    i32 name_len;
    Buffer_ID id;
};

api(custom)
typedef i32 Set_Buffer_Scroll_Rule;
enum{
 SetBufferScroll_NoCursorChange,
 SetBufferScroll_SnapCursorIntoView,
};

api(custom)
struct Buffer_Scroll{
 Buffer_Point position;
 Buffer_Point target;
};

api(custom)
struct Basic_Scroll
{
 v2 position;
 v2 target;
};

api(custom)
struct Marked_Position
{
 i64 pos;
 b32 lean_right;
};

api(custom)
typedef i32 Managed_Prim_Type;
enum{
    ManagedObjectType_Error = 0,
    ManagedObjectType_Memory = 1,
    ManagedObjectType_Markers = 2,
    
    ManagedObjectType_COUNT = 4,
};

api(custom)
typedef u64 Managed_Scope;
api(custom)
typedef u64 Managed_Object;

api(custom)
struct Marker_Visual{
    Managed_Scope scope;
    u32 slot_id;
    u32 gen_id;
};

api(custom)
typedef u32 Glyph_Flag;
enum{
    GlyphFlag_None = 0x0,
};

api(custom)
struct Query_Bar{
    String prompt;
    String string;
    u64 string_capacity;
};

api(custom)
struct Query_Bar_Ptr_Array{
    Query_Bar **ptrs;
    i32 count;
};

api(custom)
struct Query_Bar_Group{
    App *app;
    View_ID view;
    
    Query_Bar_Group(App *app);
    Query_Bar_Group(App *app, View_ID view);
    ~Query_Bar_Group();
};

api(custom)
struct Font_Load_Location{
    String filename;
};

api(custom)
typedef u32 Face_Antialiasing_Mode;
enum{
    FaceAntialiasingMode_8BitMono,
    FaceAntialiasingMode_1BitMono,
};

api(custom)
struct Face_Load_Parameters{
    u32 pt_size;
    Face_Antialiasing_Mode aa_mode;
    b8 bold;
    b8 italic;
    b8 underline;
    b8 hinting;
};

api(custom)
struct Face_Description{
    Font_Load_Location font;
    Face_Load_Parameters parameters;
};

api(custom)
struct Face_Advance_Map{
    Codepoint_Index_Map codepoint_to_index;
    f32 *advance;
    u16 index_count;
};

api(custom)
struct Edit{
    String text;
    Range_i64 range;
};

api(custom)
struct Batch_Edit{
    Batch_Edit *next;
    Edit edit;
};

api(custom)
typedef i32 Record_Kind;
enum{
 RecordKind_Single,
 RecordKind_Group,
};

api(custom)
typedef i32 Record_Error;
enum{
 RecordError_NoError,
 RecordError_InvalidBuffer,
 RecordError_NoHistoryAttached,
 RecordError_IndexOutOfBounds,
 RecordError_SubIndexOutOfBounds,
 RecordError_InitialStateDummyRecord,
 RecordError_WrongRecordTypeAtIndex,
};

api(custom)
typedef u32 Record_Merge_Flag;
enum{
 RecordMergeFlag_StateInRange_MoveStateForward = 0x0,
 RecordMergeFlag_StateInRange_MoveStateBackward = 0x1,
 RecordMergeFlag_StateInRange_ErrorOut = 0x2,
};

api(custom)
struct Record_Info
{
 Record_Error error;
 Record_Kind kind;
 i64 pos_before_edit;
 i32 edit_number;
 union
 {
  struct
  {
   String single_string_forward;
   String single_string_backward;
   i64 single_first;
  };
  struct
  {
   i32 group_count;
  };
 };
};

api(custom)
typedef i32 Hook_ID;
enum{
    HookID_Tick,
    HookID_RenderCaller,
    HookID_WholeScreenRenderCaller,
    HookID_DeltaRule,
    HookID_BufferViewerUpdate,
    HookID_ViewEventHandler,
    HookID_BufferNameResolver,
    HookID_BeginBuffer,
    HookID_EndBuffer,
    HookID_NewFile,
    HookID_SaveFile,
    HookID_BufferEditRange,
    HookID_BufferRegion,
    HookID_Layout,
    HookID_ViewChangeBuffer,
};

api(custom)
typedef i32 Hook_Function(App *app);
#define HOOK_SIG(name) i32 name(App *app)

api(custom)
struct Buffer_Name_Conflict_Entry{
    Buffer_ID buffer_id;
    String filename;
    String base_name;
    u8 *unique_name_in_out;
    u64 unique_name_len_in_out;
    u64 unique_name_capacity;
};

api(custom)
typedef void Buffer_Name_Resolver_Function(App *app, Buffer_Name_Conflict_Entry *conflicts, i32 conflict_count);
#define BUFFER_NAME_RESOLVER_SIG(n) void n(App *app, Buffer_Name_Conflict_Entry *conflicts, i32 conflict_count)

api(custom)
typedef i32 Buffer_Hook_Function(App *app, Buffer_ID buffer_id);
#define BUFFER_HOOK_SIG(name) i32 name(App *app, Buffer_ID buffer_id)

#define BUFFER_EDIT_RANGE_SIG(name) \
i32 name(App *app, Buffer_ID buffer_id, Range_i64 new_range, Range_Cursor old_cursor_range, b32 automated)

api(custom)
typedef BUFFER_EDIT_RANGE_SIG(Buffer_Edit_Range_Function);

api(custom)
typedef v2 Delta_Rule_Function(v2 pending, b32 is_new_target, f32 dt, void *data);
#define DELTA_RULE_SIG(name) v2 name(v2 pending, b32 is_new_target, f32 dt, void *data)

api(custom)
typedef Rect_f32 Buffer_Region_Function(App *app, View_ID view_id, Rect_f32 region);

api(custom)
typedef void New_Clipboard_Contents_Function(App *app, String contents);
#define NEW_CLIPBOARD_CONTENTS_SIG(name) void name(App *app, String contents)

api(custom)
typedef void Tick_Function(App *app, Frame_Info frame_info);

api(custom)
typedef void Render_Caller_Function(App *app, Frame_Info frame_info, View_ID view);

api(custom)
typedef void Whole_Screen_Render_Caller_Function(App *app, Frame_Info frame_info);

api(custom)
typedef void View_Change_Buffer_Function(App *app, View_ID view_id,
                                         Buffer_ID old_buffer_id, Buffer_ID new_buffer_id);

api(custom)
typedef u32 Layout_Item_Flag;
enum{
 LayoutItemFlag_Special_Character = (1 << 0),
 LayoutItemFlag_Ghost_Character = (1 << 1)
};

api(custom)
struct Layout_Item{
 i64 index;  //todo(kv): what is this index?
 u32 codepoint;
 Layout_Item_Flag flags;
 Rect_f32 rect;
 f32 padded_y1;
};

api(custom)
struct Layout_Item_Block{
 Layout_Item_Block *next;
 Layout_Item *items;
 i64 item_count;
 i64 character_count;
 Face_ID face;
};

api(custom)
struct Layout_Item_List {
 Layout_Item_Block *first;
 Layout_Item_Block *last;
 i64 item_count;
 i64 character_count;
 i32 node_count;
 f32 height;
 f32 bottom_padding;
 Range_i64 input_index_range;
 Range_i64 manifested_index_range;
};

api(custom)
typedef Layout_Item_List Layout_Function(App *app, Arena *arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width);

api(custom)
struct View_Context{
    Render_Caller_Function *render_caller;
    Delta_Rule_Function *delta_rule;
    u64 delta_rule_memory_size;
    b32 hides_buffer;
    struct Mapping *mapping;
    i64 map_id;
};

api(custom)
typedef u32 String_Match_Flag;
enum{
    StringMatch_CaseSensitive = 1,
    StringMatch_LeftSideSloppy = 2,
    StringMatch_RightSideSloppy = 4,
    StringMatch_Straddled = 8,
};

api(custom)
struct String_Match
{
 String_Match *next;
 Buffer_ID buffer;
 i32 string_id;
 String_Match_Flag flags;
 Range_i64 range;
};

api(custom)
struct String_Match_List{
    String_Match *first;
    String_Match *last;
    i32 count;
};

api(custom)
struct Process_State{
    b32 valid;
    b32 updating;
    i64 return_code;
};

////////////////////////////////

// NOTE(allen): buffers are allocate with:
// array_count = channel_count*sample_count
// channel_count = 2
typedef void Audio_Mix_Sources_Function(void *ctx, f32 *buffer, u32 sample_count);
typedef void Audio_Mix_Destination_Function(i16 *dst, f32 *src, u32 sample_count);
#endif

#include "4coder_kv_debug.h"
//~
