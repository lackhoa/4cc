//-
#include "4coder_custom_types.h"

function Range_i32
Ii32(i32 a, i32 b){
 Range_i32 interval = {a, b};
 if (b < a){
  interval.min = b;
  interval.max = a;
 }
 return(interval);
}
function Range_i64
Ii64(i64 a, i64 b){
 Range_i64 interval = {a, b};
 if (b < a){
  interval.min = b;
  interval.max = a;
 }
 return(interval);
}
function Range_u64
Iu64(u64 a, u64 b){
 Range_u64 interval = {a, b};
 if (b < a){
  interval.min = b;
  interval.max = a;
 }
 return(interval);
}

function Range_f32
If32(f32 a, f32 b){
 Range_f32 interval = {a, b};
 if(b < a){
  interval.min = b;
  interval.max = a;
 }
 return(interval);
}

function Range_i32
Ii32_size(i32 pos, i32 size){
 return(Ii32(pos, pos + size));
}
function Range_i64
Ii64_size(i64 pos, i64 size){
 return(Ii64(pos, pos + size));
}
function Range_u64
Iu64_size(u64 pos, u64 size){
 return(Iu64(pos, pos + size));
}
function Range_f32
If32_size(f32 pos, f32 size){
 return(If32(pos, pos + size));
}

function Range_i32
Ii32(i32 a){
 Range_i32 interval = {a, a};
 return(interval);
}
function Range_i64
Ii64(i64 a){
 Range_i64 interval = {a, a};
 return(interval);
}
function Range_u64
Iu64(u64 a){
 Range_u64 interval = {a, a};
 return(interval);
}
function Range_f32
If32(f32 a){
 Range_f32 interval = {a, a};
 return(interval);
}

function Range_i32
Ii32(){
 Range_i32 interval = {};
 return(interval);
}
function Range_i64
Ii64(){
 Range_i64 interval = {};
 return(interval);
}
function Range_u64
Iu64(){
 Range_u64 interval = {};
 return(interval);
}
function Range_f32
If32(){
 Range_f32 interval = {};
 return(interval);
}

function Range_u64
Iu64(Range_i32 r){
 return(Iu64(r.min, r.max));
}


function i32
replace_range_shift(i32 replace_length, i32 insert_length){
 return(insert_length - replace_length);
}
function i32
replace_range_shift(i32 start, i32 end, i32 insert_length){
 return(insert_length - (end - start));
}
function i32
replace_range_shift(Range_i32 range, i32 insert_length){
 return(insert_length - (range.end - range.start));
}
function i64
replace_range_shift(i64 replace_length, i64 insert_length){
 return(insert_length - replace_length);
}
function i64
replace_range_shift(i64 start, i64 end, i64 insert_length){
 return(insert_length - (end - start));
}
function i64
replace_range_shift(Range_i64 range, i64 insert_length){
 return(insert_length - (range.end - range.start));
}
function i64
replace_range_shift(u64 replace_length, u64 insert_length){
 return((i64)insert_length - replace_length);
}
function i64
replace_range_shift(i64 start, i64 end, u64 insert_length){
 return((i64)insert_length - (end - start));
}
function i64
replace_range_shift(Range_i64 range, u64 insert_length){
 return((i64)insert_length - (range.end - range.start));
}

//~

struct i8_Array{
 i8 *vals;
 i32 count;
};
struct i16_Array{
 i16 *vals;
 i32 count;
};
struct i32_Array{
 i32 *vals;
 i32 count;
};
struct i64_Array{
 i64 *vals;
 i32 count;
};

struct u8_Array{
 u8 *vals;
 i32 count;
};
struct u16_Array{
 u16 *vals;
 i32 count;
};
struct u32_Array{
 u32 *vals;
 i32 count;
};
struct u64_Array{
 u64 *vals;
 i32 count;
};

////////////////////////////////

typedef u32 String_Separator_Flag;
enum{
 StringSeparator_NoFlags = 0,
};
enum{
 StringSeparator_BeforeFirst = 1,
 StringSeparator_AfterLast = 2,
};
typedef i32 String_Match_Rule;
enum{
 StringMatch_Exact = 0,
 StringMatch_CaseInsensitive = 1,
};

struct String_Const_char{
 char *str;
 u64 size;
};

struct String_Const_u32{
 u32 *str;
 u64 size;
};

struct String_Const_char_Array{
 union{
  String_Const_char *strings;
  String_Const_char *vals;
 };
 i32 count;
};
typedef String_Array String8_Array;
struct String_Const_u16_Array{
 union{
  String_Const_u16 *strings;
  String_Const_u16 *vals;
 };
 i32 count;
};
struct String_Const_u32_Array{
 union{
  String_Const_u32 *strings;
  String_Const_u32 *vals;
 };
 i32 count;
};

typedef String_Array String8Array;

typedef i32 String_Encoding;
enum{
 StringEncoding_ASCII = 0,
 StringEncoding_UTF8  = 1,
 StringEncoding_UTF16 = 2,
 StringEncoding_UTF32 = 3,
};

struct String_Const_Any{
 String_Encoding encoding;
 union{
  struct{
   void *str;
   u64 size;
  };
  String_Const_char s_char;
  String s_u8;
  String_Const_u16 s_u16;
  String_Const_u32 s_u32;
 };
};

#define str8_lit(s) {(u8*)(s), sizeof(s) - 1}
#define string_litinit(s) {(u8*)(s), sizeof(s) - 1}
#define string_u8_litinit(s) {(u8*)(s), sizeof(s) - 1}

struct Node_String_Const_char{
 Node_String_Const_char *next;
 String_Const_char string;
};
struct Node_String_Const_u16{
 Node_String_Const_u16 *next;
 String_Const_u16 string;
};
struct Node_String_Const_u32{
 Node_String_Const_u32 *next;
 String_Const_u32 string;
};
struct List_String_Const_char{
 Node_String_Const_char *first;
 Node_String_Const_char *last;
 u64 total_size;
 i32 node_count;
};
struct List_String_Const_u16{
 Node_String_Const_u16 *first;
 Node_String_Const_u16 *last;
 u64 total_size;
 i32 node_count;
};
struct List_String_Const_u32{
 Node_String_Const_u32 *first;
 Node_String_Const_u32 *last;
 u64 total_size;
 i32 node_count;
};

typedef Node_String String8Node;
typedef List_String String8List;

struct Node_String_Const_Any{
 Node_String_Const_Any *next;
 String_Const_Any string;
};
struct List_String_Const_Any{
 Node_String_Const_Any *first;
 Node_String_Const_Any *last;
 u64 total_size;
 i32 node_count;
};

struct String_char{
 union{
  String_Const_char string;
  struct{
   char *str;
   u64 size;
  };
 };
 u64 cap;
};
struct String_u32{
 union{
  String_Const_u32 string;
  struct{
   u32 *str;
   u64 size;
  };
 };
 u64 cap;
};

struct String_Any{
 String_Encoding encoding;
 union{
  struct{
   void *str;
   u64 size;
   u64 cap;
  };
  String_char s_char;
  String_u8 s_u8;
  String_u16 s_u16;
  String_u32 s_u32;
 };
};

typedef i32 Line_Ending_Kind;
enum{
 LineEndingKind_Binary,
 LineEndingKind_LF,
 LineEndingKind_CRLF,
};


global u32 surrogate_min = 0xD800;
global u32 surrogate_max = 0xDFFF;

global u32 nonchar_min = 0xFDD0;
global u32 nonchar_max = 0xFDEF;

////////////////////////////////

typedef u32 Access_Flag;
enum{
 AccessFlag_Read  = 1,
 AccessFlag_Write = 2,
 AccessFlag_Exec  = 4,
};

typedef i32 Dimension;
enum{
 Dimension_X = 0,
 Dimension_Y = 1,
 Dimension_Z = 2,
 Dimension_W = 3,
};

typedef i32 Coordinate;
enum{
 Coordinate_X = 0,
 Coordinate_Y = 1,
 Coordinate_Z = 2,
 Coordinate_W = 3,
};

typedef i32 Side;
enum{
 Side_Min = 0,
 Side_Max = 1,
};


//~///////////////////////////////

struct Date_Time{
 u32 year; // Real year, no adjustment
 u8 mon;   // [0,11]
 u8 day;   // [0,30]
 u8 hour;  // [0,23]
 u8 min;   // [0,59]
 u8 sec;   // [0,60]
 u16 msec; // [0,999]
};

global String month_full_name[] = {
 str8_lit("January"),
 str8_lit("February"),
 str8_lit("March"),
 str8_lit("April"),
 str8_lit("May"),
 str8_lit("June"),
 str8_lit("July"),
 str8_lit("August"),
 str8_lit("September"),
 str8_lit("October"),
 str8_lit("November"),
 str8_lit("December"),
};

//~

////////////////////////////////


function b32
rect_equals(Rect_i32 a, Rect_i32 b){
 return(a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1);
}
function b32
rect_equals(Rect_f32 a, Rect_f32 b){
 return(a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1);
}

function b32
rect_contains_point(Rect_i32 a, Vec2_i32 b){
 return(a.x0 <= b.x && b.x < a.x1 && a.y0 <= b.y && b.y < a.y1);
}
function b32
rect_contains_point(Rect_f32 a, v2 b){
 return(a.x0 <= b.x && b.x < a.x1 && a.y0 <= b.y && b.y < a.y1);
}

function Rect_i32
rect_inner(Rect_i32 r, i32 m){
 r.x0 += m;
 r.y0 += m;
 r.x1 -= m;
 r.y1 -= m;
 return(r);
}
function Rect_f32
rect_inner(Rect_f32 r, f32 m){
 r.x0 += m;
 r.y0 += m;
 r.x1 -= m;
 r.y1 -= m;
 return(r);
}

function Range_i32
rect_x(Rect_i32 r){
 return(Ii32(r.x0, r.x1));
}
function Range_i32
rect_y(Rect_i32 r){
 return(Ii32(r.y0, r.y1));
}
function i32
rect_width(Rect_i32 r){
 return(r.x1 - r.x0);
}
function i32
rect_height(Rect_i32 r){
 return(r.y1 - r.y0);
}
function Range_f32
rect_x(Rect_f32 r){
 return(If32(r.x0, r.x1));
}
function Range_f32
rect_y(Rect_f32 r){
 return(If32(r.y0, r.y1));
}
function f32
rect_width(Rect_f32 r){
 return(r.x1 - r.x0);
}
function f32
rect_height(Rect_f32 r){
 return(r.y1 - r.y0);
}


function Range_i32
rect_range_x(Rect_i32 r){
 return(Ii32(r.x0, r.x1));
}
function Range_i32
rect_range_y(Rect_i32 r){
 return(Ii32(r.y0, r.y1));
}
function Range_f32
rect_range_x(Rect_f32 r){
 return(If32(r.x0, r.x1));
}
function Range_f32
rect_range_y(Rect_f32 r){
 return(If32(r.y0, r.y1));
}

function i32
rect_area(Rect_i32 r){
 return((r.x1 - r.x0)*(r.y1 - r.y0));
}
function f32
rect_area(Rect_f32 r){
 return((r.x1 - r.x0)*(r.y1 - r.y0));
}


function b32
rect_overlap(Rect_i32 a, Rect_i32 b){
 return(range_overlap(rect_range_x(a), rect_range_x(b)) &&
        range_overlap(rect_range_y(a), rect_range_y(b)));
}
function b32
rect_overlap(Rect_f32 a, Rect_f32 b){
 return(range_overlap(rect_range_x(a), rect_range_x(b)) &&
        range_overlap(rect_range_y(a), rect_range_y(b)));
}

function Rect_i32
rect_intersect(Rect_i32 a, Rect_i32 b){
 a.x0 = Max(a.x0, b.x0);
 a.y0 = Max(a.y0, b.y0);
 a.x1 = Min(a.x1, b.x1);
 a.y1 = Min(a.y1, b.y1);
 a.x0 = Min(a.x0, a.x1);
 a.y0 = Min(a.y0, a.y1);
 return(a);
}
function Rect_i32
rect_union(Rect_i32 a, Rect_i32 b){
 a.x0 = Min(a.x0, b.x0);
 a.y0 = Min(a.y0, b.y0);
 a.x1 = Max(a.x1, b.x1);
 a.y1 = Max(a.y1, b.y1);
 return(a);
}
function Rect_f32
rect_intersect(Rect_f32 a, Rect_f32 b){
 a.x0 = Max(a.x0, b.x0);
 a.y0 = Max(a.y0, b.y0);
 a.x1 = Min(a.x1, b.x1);
 a.y1 = Min(a.y1, b.y1);
 a.x0 = Min(a.x0, a.x1);
 a.y0 = Min(a.y0, a.y1);
 return(a);
}

////////////////////////////////

union rect2_Pair{
 struct{
  Rect_f32 a;
  Rect_f32 b;
 };
 struct{
  Rect_f32 min;
  Rect_f32 max;
 };
 struct{
  Rect_f32 e[2];
 };
};
function rect2_Pair
rect_split_top_bottom__inner(Rect_f32 rect, f32 y){
 y = clamp_between(rect.y0, y, rect.y1);
 rect2_Pair pair = {};
 pair.a = Rf32(rect.x0, rect.y0, rect.x1, y      );
 pair.b = Rf32(rect.x0, y      , rect.x1, rect.y1);
 return(pair);
}

function rect2_Pair
rect_split_left_right__inner(Rect_f32 rect, f32 x){
 x = clamp_between(rect.x0, x, rect.x1);
 rect2_Pair pair = {};
 pair.a = Rf32(rect.x0, rect.y0, x      , rect.y1);
 pair.b = Rf32(x      , rect.y0, rect.x1, rect.y1);
 return(pair);
}

function rect2_Pair
rect_split_top_bottom(Rect_f32 rect, f32 y){
 return(rect_split_top_bottom__inner(rect, rect.y0 + y));
}

function rect2_Pair
rect_split_left_right(Rect_f32 rect, f32 x){
 return(rect_split_left_right__inner(rect, rect.x0 + x));
}

function rect2_Pair
rect_split_top_bottom_neg(Rect_f32 rect, f32 y){
 return(rect_split_top_bottom__inner(rect, rect.y1 - y));
}

function rect2_Pair
rect_split_left_right_neg(Rect_f32 rect, f32 x){
 return(rect_split_left_right__inner(rect, rect.x1 - x));
}

function rect2_Pair
rect_split_top_bottom_lerp(Rect_f32 rect, f32 t){
 return(rect_split_top_bottom__inner(rect, lerp(rect.y0, t, rect.y1)));
}

function rect2_Pair
rect_split_left_right_lerp(Rect_f32 rect, f32 t){
 return(rect_split_left_right__inner(rect, lerp(rect.x0, t, rect.x1)));
}

////////////////////////////////

function Scan_Direction
flip_direction(Scan_Direction direction){
 switch (direction){
  case Scan_Forward:
  {
   direction = Scan_Backward;
  }break;
  case Scan_Backward:
  {
   direction = Scan_Forward;
  }break;
 }
 return(direction);
}

function Side
flip_side(Side side){
 switch (side){
  case Side_Min:
  {
   side = Side_Max;
  }break;
  case Side_Max:
  {
   side = Side_Min;
  }break;
 }
 return(side);
}

//~

function String_char
Schar(char *str, u64 size, u64 cap){
 String_char string = {str, size, cap};
 return(string);
}
function String_u8
Su8(u8 *str, u64 size, u64 cap){
 String_u8 string = {str, size, cap};
 return(string);
}
function String_u16
Su16(u16 *str, u64 size, u64 cap){
 String_u16 string = {str, size, cap};
 return(string);
}
function String_u32
Su32(u32 *str, u64 size, u64 cap){
 String_u32 string = {str, size, cap};
 return(string);
}

function String_Any
Sany(void *str, u64 size, u64 cap, String_Encoding encoding){
 String_Any string = {.encoding=encoding};
 switch (encoding){
  case StringEncoding_ASCII: string.s_char = Schar((char*)str, size, cap); break;
  case StringEncoding_UTF8:  string.s_u8 = Su8((u8*)str, size, cap); break;
  case StringEncoding_UTF16: string.s_u16 = Su16((u16*)str, size, cap); break;
  case StringEncoding_UTF32: string.s_u32 = Su32((u32*)str, size, cap); break;
 }
 return(string);
}

function String_char
Schar(char *str, u64 size){
 String_char string = {str, size, size + 1};
 return(string);
}
function String_u8
Su8(u8 *str, u64 size){
 String_u8 string = {str, size, size + 1};
 return(string);
}
function String_u16
Su16(u16 *str, u64 size){
 String_u16 string = {str, size, size + 1};
 return(string);
}
function String_char
Schar(char *str, char *one_past_last){
 return(Schar(str, (u64)(one_past_last - str)));
}
function String_u8
Su8(u8 *str, u8 *one_past_last){
 return(Su8(str, (u64)(one_past_last - str)));
}
function String_u16
Su16(u16 *str, u16 *one_past_last){
 return(Su16(str, (u64)(one_past_last - str)));
}

function String_u8
Su8(u8 *str){
 u64 size = cstring_length(str);
 String_u8 string = {str, size, size + 1};
 return(string);
}
function u64
cstring_length(u32 *str){
 u64 length = 0;
 for (;str[length] != 0; length += 1);
 return(length);
}
function String_u16
Su16(u16 *str){
 u64 size = cstring_length(str);
 String_u16 string = {str, size, size + 1};
 return(string);
}
function String_u32
Su32(u32 *str){
 u64 size = cstring_length(str);
 String_u32 string = {str, size, size + 1};
 return(string);
}


function String_char
Schar(String_Const_char str, u64 cap){
 String_char string = {str.str, str.size, cap};
 return(string);
}
function String_u8
Su8(String str, u64 cap){
 String_u8 string = {str.str, str.size, cap};
 return(string);
}
function String_u16
Su16(String_Const_u16 str, u64 cap){
 String_u16 string = {str.str, str.size, cap};
 return(string);
}
function String_u32
Su32(String_Const_u32 str, u64 cap){
 String_u32 string = {str.str, str.size, cap};
 return(string);
}

function String_Any
SCany(String_char str){
 String_Any string = {.encoding=StringEncoding_ASCII};
 string.s_char = str;
 return(string);
}
function String_Any
SCany(String_u8 str){
 String_Any string = {.encoding=StringEncoding_UTF8};
 string.s_u8 = str;
 return(string);
}
function String_Any
SCany(String_u16 str){
 String_Any string = {.encoding=StringEncoding_UTF16};
 string.s_u16 = str;
 return(string);
}
function String_Any
SCany(String_u32 str){
 String_Any string = {.encoding=StringEncoding_UTF32};
 string.s_u32 = str;
 return(string);
}

function String_Const_char
SCchar(char *str, u64 size){
 String_Const_char string = {str, size};
 return(string);
}
function String_Const_u32
SCu32(u32 *str, u64 size){
 String_Const_u32 string = {str, size};
 return(string);
}

function String_Const_Any
SCany(void *str, u64 size, String_Encoding encoding){
 String_Const_Any string = {.encoding=encoding};
 switch (encoding){
  case StringEncoding_ASCII: string.s_char = SCchar((char*)str, size); break;
  case StringEncoding_UTF8:  string.s_u8 = SCu8((u8*)str, size); break;
  case StringEncoding_UTF16: string.s_u16 = SCu16((u16*)str, size); break;
  case StringEncoding_UTF32: string.s_u32 = SCu32((u32*)str, size); break;
 }
 return(string);
}

function String_Const_char
SCchar(void){
 String_Const_char string = {};
 return(string);
}
function String_Const_u16
SCu16(void){
 String_Const_u16 string = {};
 return(string);
}
function String_Const_u32
SCu32(void){
 String_Const_u32 string = {};
 return(string);
}

function String_Const_char
SCchar(char *str, char *one_past_last){
 return(SCchar(str, (u64)(one_past_last - str)));
}
function String_Const_u16
SCu16(u16 *str, u16 *one_past_last){
 return(SCu16(str, (u64)(one_past_last - str)));
}

function String_Const_char
SCchar(char *str){
 u64 size = cstring_length(str);
 String_Const_char string = {str, size};
 return(string);
}
function String_Const_u32
SCu32(u32 *str){
 u64 size = cstring_length(str);
 String_Const_u32 string = {str, size};
 return(string);
}

function String_Const_char
SCchar(String_char string){
 return(string.string);
}
function String
SCu8(String_u8 string){
 return(string.string);
}
function String_Const_u16
SCu16(String_u16 string){
 return(string.string);
}
function String_Const_u32
SCu32(String_u32 string){
 return(string.string);
}

function String_Const_char
SCchar(String str){
 return(SCchar((char*)str.str, str.size));
}
myinline String
SCu8(String_Const_char str){
 return(SCu8((u8*)str.str, str.size));
}



function String_Const_Any
SCany(void *str, String_Encoding encoding){
 String_Const_Any string = {.encoding=encoding};
 switch (encoding){
  case StringEncoding_ASCII: string.s_char = SCchar((char*)str); break;
  case StringEncoding_UTF8:  string.s_u8   = SCu8  ((char*)str); break;
  case StringEncoding_UTF16: string.s_u16  = SCu16 ((u16*)str); break;
  case StringEncoding_UTF32: string.s_u32  = SCu32 ((u32*)str); break;
 }
 return(string);
}

function String_Const_Any
SCany(String_Const_char str){
 String_Const_Any string = {.encoding=StringEncoding_ASCII};
 string.s_char = str;
 return(string);
}
function String_Const_Any
SCany(String str){
 String_Const_Any string = {.encoding=StringEncoding_UTF8};
 string.s_u8 = str;
 return(string);
}
function String_Const_Any
SCany(String_Const_u16 str){
 String_Const_Any string = {.encoding=StringEncoding_UTF16};
 string.s_u16 = str;
 return(string);
}
function String_Const_Any
SCany(String_Const_u32 str){
 String_Const_Any string = {.encoding=StringEncoding_UTF32};
 string.s_u32 = str;
 return(string);
}
//~
function b32
character_is_basic_ascii(char c){
 return(' ' <= c && c <= '~');
}
function b32
character_is_basic_ascii(u8 c){
 return(' ' <= c && c <= '~');
}
function b32
character_is_basic_ascii(u16 c){
 return(' ' <= c && c <= '~');
}
function b32
character_is_basic_ascii(u32 c){
 return(' ' <= c && c <= '~');
}
function b32
character_is_upper(char c)
{
 return(('A' <= c) && (c <= 'Z'));
}
inline b32 is_uppercase(u8 c)
{
 return (('A' <= c) && (c <= 'Z'));
}
function b32
character_is_upper(u8 c){
 return(('A' <= c) && (c <= 'Z'));
}
function b32
character_is_upper(u16 c){
 return(('A' <= c) && (c <= 'Z'));
}
function b32
character_is_upper(u32 c){
 return(('A' <= c) && (c <= 'Z'));
}

function b32
character_is_lower(char c)
{
 return(('a' <= c) && (c <= 'z'));
}
function b32
character_is_lower(u8 c){
 return(('a' <= c) && (c <= 'z'));
}
inline b32 is_lowercase(u8 c) { return(('a' <= c) && (c <= 'z'));
}

function b32
character_is_lower(u16 c){
 return(('a' <= c) && (c <= 'z'));
}
function b32
character_is_lower(u32 c){
 return(('a' <= c) && (c <= 'z'));
}

function b32
character_is_lower_unicode(u8 c){
 return((('a' <= c) && (c <= 'z')) || c >= 128);
}
function b32
character_is_lower_unicode(u16 c){
 return((('a' <= c) && (c <= 'z')) || c >= 128);
}
function b32
character_is_lower_unicode(u32 c){
 return((('a' <= c) && (c <= 'z')) || c >= 128);
}

function char
character_to_upper(char c){
 if (('a' <= c) && (c <= 'z')){
  c -= 'a' - 'A';
 }
 return(c);
}
function u8
character_to_upper(u8 c){
 if (('a' <= c) && (c <= 'z')){
  c -= 'a' - 'A';
 }
 return(c);
}
function u16
character_to_upper(u16 c){
 if (('a' <= c) && (c <= 'z')){
  c -= 'a' - 'A';
 }
 return(c);
}
function u32
character_to_upper(u32 c){
 if (('a' <= c) && (c <= 'z')){
  c -= 'a' - 'A';
 }
 return(c);
}
function char
character_to_lower(char c){
 if (('A' <= c) && (c <= 'Z')){
  c += 'a' - 'A';
 }
 return(c);
}
function u8
character_to_lower(u8 c){
 if (('A' <= c) && (c <= 'Z')){
  c += 'a' - 'A';
 }
 return(c);
}
function u16
character_to_lower(u16 c){
 if (('A' <= c) && (c <= 'Z')){
  c += 'a' - 'A';
 }
 return(c);
}
function u32
character_to_lower(u32 c){
 if (('A' <= c) && (c <= 'Z')){
  c += 'a' - 'A';
 }
 return(c);
}

function b32
char_is_whitespace(u16 c){
 return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}
function b32
char_is_whitespace(u32 c){
 return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}

function b32
character_is_base10(char c){
 return('0' <= c && c <= '9');
}
function b32
character_is_base10(u8 c){
 return('0' <= c && c <= '9');
}
function b32
character_is_base10(u16 c){
 return('0' <= c && c <= '9');
}
function b32
character_is_base10(u32 c){
 return('0' <= c && c <= '9');
}

function b32
character_is_base16(char c){
 return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}
function b32
character_is_base16(u8 c){
 return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}
function b32
character_is_base16(u16 c){
 return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}
function b32
character_is_base16(u32 c){
 return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}

function b32
character_is_base64(char c){
 return(('0' <= c && c <= '9') ||
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        c == '_' || c == '$' || c == '?');
}
function b32
character_is_base64(u8 c){
 return(('0' <= c && c <= '9') ||
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        c == '_' || c == '$' || c == '?');
}
function b32
character_is_base64(u16 c){
 return(('0' <= c && c <= '9') ||
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        c == '_' || c == '$' || c == '?');
}
function b32
character_is_base64(u32 c){
 return(('0' <= c && c <= '9') ||
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        c == '_' || c == '$' || c == '?');
}

function b32
character_is_alpha(char c){
 return (('a' <= c) && (c <= 'z')) or (('A' <= c) && (c <= 'Z')) or c == '_';
}
function b32
character_is_alpha(u8 c){
 return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_');
}
function b32
character_is_alpha(u16 c){
 return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_');
}
function b32
character_is_alpha(u32 c){
 return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_');
}

function b32
character_is_alnum(char c){
 return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}
function b32
character_is_alnum(u8 c){
 return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}
function b32
character_is_alnum(u16 c){
 return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}
function b32
character_is_alnum(u32 c){
 return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}


function b32
character_is_alpha_unicode(u8 c){
 return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_' || c >= 128);
}
function b32
character_is_alpha_unicode(u16 c){
 return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_' || c >= 128);
}
function b32
character_is_alpha_unicode(u32 c){
 return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_' || c >= 128);
}

function b32
character_is_alnum_unicode(u8 c){
 return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_' || c >= 128);
}
function b32
character_is_alnum_unicode(u16 c){
 return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_' || c >= 128);
}
function b32
character_is_alnum_unicode(u32 c){
 return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_' || c >= 128);
}

function char
string_get_character(String_Const_char str, u64 i){
 char r = 0;
 if (i < str.size){
  r = str.str[i];
 }
 return(r);
}
function u16
string_get_character(String_Const_u16 str, u64 i){
 u16 r = 0;
 if (i < str.size){
  r = str.str[i];
 }
 return(r);
}

function String_Const_char
string_prefix(String_Const_char str, u64 size){
 size = clamp_max(size, str.size);
 str.size = size;
 return(str);
}


function String_Const_u16
string_prefix(String_Const_u16 str, u64 size){
 size = clamp_max(size, str.size);
 str.size = size;
 return(str);
}

function String
string_postfix(String str, u64 size){
 size = clamp_max(size, str.size);
 str.str += (str.size - size);
 str.size = size;
 return(str);
}

function String_Const_char
string_skip(String_Const_char str, u64 n){
 n = clamp_max(n, str.size);
 str.str += n;;
 str.size -= n;
 return(str);
}

function String
string_chop(String str, u64 n){
 n = clamp_max(n, str.size);
 str.size -= n;
 return(str);
}


function u64
string_find_first(String_Const_char str, u64 start_pos, char c)
{
 u64 i = start_pos;
 for (;i < str.size && c != str.str[i]; i += 1);
 return(i);
}

function u64
string_find_first(String_Const_char str, char c){
 return(string_find_first(str, 0, c));
}

function b32
string_looks_like_drive_letter(String string)
{
 b32 result = false;
 if (string.size == 3 &&
     character_is_alpha(string.str[0]) &&
     string.str[1] == ':' &&
     is_file_slash(string.str[2])){
  result = true;
 }
 return(result);
}

function String
string_remove_front_of_path(String str)
{
 i64 slash_pos = string_find_last_slash(str);
 if (slash_pos < 0) { str.size = 0; }
 else               { str.size = slash_pos + 1; }
 return(str);
}


function String
string_remove_front_folder_of_path(String str){
 i64 slash_pos = string_find_last_slash(string_chop(str, 1));
 if (slash_pos < 0){
  str.size = 0;
 }
 else{
  str.size = slash_pos + 1;
 }
 return(str);
}
function String
string_front_folder_of_path(String str){
 i64 slash_pos = string_find_last_slash(string_chop(str, 1));
 if (slash_pos >= 0){
  str = string_skip(str, slash_pos + 1);
 }
 return(str);
}

function String
string_chop_whitespace(String str){
 i64 e = string_find_last_non_whitespace(str);
 str = string_prefix(str, (u64)(e + 1));
 return(str);
}

function String
string_skip_chop_whitespace(String str){
 u64 f = string_find_first_non_whitespace(str);
 str = string_skip(str, f);
 i64 e = string_find_last_non_whitespace(str);
 str = string_prefix(str, (u64)(e + 1));
 return(str);
}

function b32
string_match(String_Const_char a, String_Const_char b)
{
 b32 result = false;
 if (a.size == b.size)
 {
  result = true;
  for (u64 i = 0; i < a.size; i += 1)
  {
   if (a.str[i] != b.str[i]){
    result = false;
    break;
   }
  }
 }
 return(result);
}
function b32
string_match(String_Const_u32 a, String_Const_u32 b){
 b32 result = false;
 if (a.size == b.size){
  result = true;
  for (u64 i = 0; i < a.size; i += 1){
   if (a.str[i] != b.str[i]){
    result = false;
    break;
   }
  }
 }
 return(result);
}


function b32
string_ends_with(String string, String test)
{
 b32 result = string_match(string_postfix(string, test.size), test);
 return result;
}

function b32
string_match_insensitive(String a, String b){
 b32 result = false;
 if (a.size == b.size){
  result = true;
  for (u64 i = 0; i < a.size; i += 1){
   if (character_to_upper(a.str[i]) != character_to_upper(b.str[i])){
    result = false;
    break;
   }
  }
 }
 return(result);
}

function b32
string_match(String8 a, String8 b, String_Match_Rule rule)
{
 b32 result = false;
 switch (rule)
 {
  case StringMatch_Exact:
  {
   result = string_match(a, b);
  }break;
  
  case StringMatch_CaseInsensitive:
  {
   result = string_match_insensitive(a, b);
  }break;
 }
 return(result);
}

function u64
string_find_first(String str, String needle, String_Match_Rule rule)
{
 u64 i = 0;
 if(needle.size > 0)
 {
  i = str.size;
  if (str.size >= needle.size){
   i = 0;
   u8 c = character_to_upper(needle.str[0]);
   u64 one_past_last = str.size - needle.size + 1;
   for (;i < one_past_last; i += 1){
    if (character_to_upper(str.str[i]) == c){
     String source_part = string_prefix(string_skip(str, i), needle.size);
     if (string_match(source_part, needle, rule)){
      break;
     }
    }
   }
   if (i == one_past_last){
    i = str.size;
   }
  }
 }
 return(i);
}

function u64
string_find_first(String str, String needle){
 return(string_find_first(str, needle, StringMatch_Exact));
}
function u64
string_find_first_insensitive(String str, String needle){
 return(string_find_first(str, needle, StringMatch_CaseInsensitive));
}

function b32
string_has_substr(String str, String needle, String_Match_Rule rule){
 return(string_find_first(str, needle, rule) < str.size);
}

function b32
string_has_substr(String str, String needle){
 return(string_find_first(str, needle, StringMatch_Exact) < str.size);
}

function i32
string_compare(String8 a, String8 b)
{
 i32 result = 0;
 for (u64 i = 0; i < a.size || i < b.size; i += 1){
  u8 ca = (i < a.size)?a.str[i]:0;
  u8 cb = (i < b.size)?b.str[i]:0;
  i32 dif = ((ca) - (cb));
  if (dif != 0){
   result = (dif > 0)?1:-1;
   break;
  }
 }
 return(result);
}

inline b32 string_equal(String8 a, String8 b) { return string_match(a,b); }

inline b32
string_equal(String8 a, char *b)
{
 return string_match(a, SCu8(b));
}
inline b32
string_equal(String8 a, char c)
{
 return (a.size == 1) && (a.str[0] == c);
}
function i32
string_compare_insensitive(String a, String b){
 i32 result = 0;
 for (u64 i = 0; i < a.size || i < b.size; i += 1){
  u8 ca = (i <= a.size)?0:a.str[i];
  u8 cb = (i <= b.size)?0:b.str[i];
  i32 dif = character_to_upper(ca) - character_to_upper(cb);
  if (dif != 0){
   result = (dif > 0)?1:-1;
   break;
  }
 }
 return(result);
}

function String
string_mod_upper(String str){
 for (u64 i = 0; i < str.size; i += 1){
  str.str[i] = character_to_upper(str.str[i]);
 }
 return(str);
}
function String
string_mod_lower(String str){
 for (u64 i = 0; i < str.size; i += 1){
  str.str[i] = character_to_lower(str.str[i]);
 }
 return(str);
}

function void
string_mod_replace_character(String str, u8 o, u8 n){
 for (u64 i = 0; i < str.size; i += 1){
  u8 c = str.str[i];
  str.str[i] = (c == o)?(n):(c);
 }
}

function b32
string_concat(String_char *dst, String_Const_char src){
 b32 result = false;
 u64 available = dst->cap - dst->size;
 if (src.size <= available){
  result = true;
 }
 u64 copy_size = clamp_max(src.size, available);
 block_copy(dst->str + dst->size, src.str, copy_size);
 dst->size += copy_size;
 return(result);
}
function b32
string_concat(String_u16 *dst, String_Const_u16 src){
 b32 result = false;
 u64 available = dst->cap - dst->size;
 if (src.size <= available){
  result = true;
 }
 u64 copy_size = clamp_max(src.size, available);
 block_copy(dst->str + dst->size, src.str, copy_size);
 dst->size += copy_size;
 return(result);
}
function b32
string_concat(String_u32 *dst, String_Const_u32 src){
 b32 result = false;
 u64 available = dst->cap - dst->size;
 if (src.size <= available){
  result = true;
 }
 u64 copy_size = clamp_max(src.size, available);
 block_copy(dst->str + dst->size, src.str, copy_size);
 dst->size += copy_size;
 return(result);
}

function b32
string_concat_character(String_char *dst, char c){
 return(string_concat(dst, SCchar(&c, 1)));
}
function b32
string_concat_character(String_u8 *dst, u8 c){
 return(string_concat(dst, SCu8(&c, 1)));
}
function b32
string_concat_character(String_u16 *dst, u16 c){
 return(string_concat(dst, SCu16(&c, 1)));
}
function b32
string_concat_character(String_u32 *dst, u32 c){
 return(string_concat(dst, SCu32(&c, 1)));
}

function b32
string_null_terminate(String_char *str){
 b32 result = false;
 if (str->size < str->cap){
  str->str[str->size] = 0;
 }
 return(result);
}
function b32
string_null_terminate(String_u32 *str){
 b32 result = false;
 if (str->size < str->cap){
  str->str[str->size] = 0;
 }
 return(result);
}

function String_char
string_char_push(Arena *arena, u64 size){
 String_char string = {};
 string.str = push_array(arena, char, size);
 string.cap = size;
 return(string);
}
function String_u8
string_u8_push(Arena *arena, u64 size){
 String_u8 string = {};
 string.str = push_array(arena, u8, size);
 string.cap = size;
 return(string);
}
function String_u16
string_u16_push(Arena *arena, u64 size){
 String_u16 string = {};
 string.str = push_array(arena, u16, size);
 string.cap = size;
 return(string);
}
function String_u32
string_u32_push(Arena *arena, u64 size){
 String_u32 string = {};
 string.str = push_array(arena, u32, size);
 string.cap = size;
 return(string);
}

function String_Any
string_any_push(Arena *arena, u64 size, String_Encoding encoding){
 String_Any string = {};
 switch (encoding){
  case StringEncoding_ASCII: string.s_char = string_char_push(arena, size); break;
  case StringEncoding_UTF8:  string.s_u8   = string_u8_push  (arena, size); break;
  case StringEncoding_UTF16: string.s_u16  = string_u16_push (arena, size); break;
  case StringEncoding_UTF32: string.s_u32  = string_u32_push (arena, size); break;
 }
 return(string);
}

#define push_string_u8 string_u8_push
#define push_string_u16 string_u16_push
#define push_string_u32 string_u32_push
#define push_string_u64 string_u64_push

function String_Const_char
string_const_char_push(Arena *arena, u64 size){
 String_Const_char string = {};
 string.str = push_array(arena, char, size);
 string.size = size;
 return(string);
}
function String_Const_u16
string_const_u16_push(Arena *arena, u64 size){
 String_Const_u16 string = {};
 string.str = push_array(arena, u16, size);
 string.size = size;
 return(string);
}
function String_Const_u32
string_const_u32_push(Arena *arena, u64 size){
 String_Const_u32 string = {};
 string.str = push_array(arena, u32, size);
 string.size = size;
 return(string);
}

function String_Const_Any
string_const_any_push(Arena *arena, u64 size, String_Encoding encoding){
 String_Const_Any string = {};
 switch (encoding){
  case StringEncoding_ASCII: string.s_char = string_const_char_push(arena, size); break;
  case StringEncoding_UTF8:  string.s_u8   = push_data  (arena, size); break;
  case StringEncoding_UTF16: string.s_u16  = string_const_u16_push (arena, size); break;
  case StringEncoding_UTF32: string.s_u32  = string_const_u32_push (arena, size); break;
 }
 return(string);
}

#define push_string_const_u8 push_data
#define push_string_const_u16 string_const_u16_push
#define push_string_const_u32 string_const_u32_push
#define push_string_const_u64 string_const_u64_push

function String_Const_char
push_stringz(Arena *arena, String_Const_char src)
{
 String_Const_char string = {};
 string.str  = push_array(arena, char, src.size + 1);
 string.size = src.size;
 block_copy_count(string.str, src.str, src.size);
 string.str[string.size] = 0;
 return(string);
}

function String_Const_u16
push_stringz(Arena *arena, String_Const_u16 src){
 String_Const_u16 string = {};
 string.str = push_array(arena, u16, src.size + 1);
 string.size = src.size;
 block_copy_count(string.str, src.str, src.size);
 string.str[string.size] = 0;
 return(string);
}
function String_Const_u32
push_stringz(Arena *arena, String_Const_u32 src){
 String_Const_u32 string = {};
 string.str = push_array(arena, u32, src.size + 1);
 string.size = src.size;
 block_copy_count(string.str, src.str, src.size);
 string.str[string.size] = 0;
 return(string);
}

function String_Array
push_string_array_copy(Arena *arena, String_Array src){
 String_Array result = {};
 result.vals = push_array(arena, String, src.count);
 result.count = src.count;
 for (i32 i = 0; i < src.count; i += 1){
  result.vals[i] = push_stringz(arena, src.vals[i]);
 }
 return(result);
}

function String_Const_Any
push_stringz(Arena *arena, u64 size, String_Const_Any src){
 String_Const_Any string = {};
 switch (src.encoding){
  case StringEncoding_ASCII: string.s_char = push_stringz(arena, src.s_char); break;
  case StringEncoding_UTF8:  string.s_u8   = push_stringz(arena, src.s_u8  ); break;
  case StringEncoding_UTF16: string.s_u16  = push_stringz(arena, src.s_u16 ); break;
  case StringEncoding_UTF32: string.s_u32  = push_stringz(arena, src.s_u32 ); break;
 }
 return(string);
}


function void
string_list_push_overlap(Arena *arena, List_String *list, u8 overlap, String string)
{
 b32 tail_has_overlap = false;
 b32 string_has_overlap = false;
 if (list->last != 0)
 {
  String tail = list->last->string;
  if (string_get_character(tail, tail.size - 1) == overlap) {
   tail_has_overlap = true;
  }
 }
 if (string_get_character(string, 0) == overlap) {
  string_has_overlap = true;
 }
 if (tail_has_overlap == string_has_overlap)
 {
  if (!tail_has_overlap) {
   string_list_push(arena, list, push_stringz(arena, SCu8(&overlap, 1)));
  } else {
   string = string_skip(string, 1);
  }
 }
 if (string.size > 0) {
  string_list_push(arena, list, string);
 }
}

#define push_string_list string_list_push
#define push_string_list_lit(a,l,s) string_list_push_lit(a,l,s)
#define push_string_list_u8_lit(a,l,s) string_list_u8_push_lit(a,l,s)
#define push_string_list_overlap(a,l,o,s) string_list_push_overlap(a,l,o,s)

typedef String_Const_char String_char_Mod_Function_Type(String_Const_char string);
typedef String String_u8_Mod_Function_Type(String string);
typedef String_Const_u16 String_u16_Mod_Function_Type(String_Const_u16 string);
typedef String_Const_u32 String_u32_Mod_Function_Type(String_Const_u32 string);

function String
string_list_flatten(Arena *arena, List_String list,
                    String_u8_Mod_Function_Type *mod, String separator,
                    String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule)
{
 u64 term_padding = (rule == StringFill_NullTerminate)?(1):(0);
 b32 before_first = HasFlag(separator_flags, StringSeparator_BeforeFirst);
 b32 after_last = HasFlag(separator_flags, StringSeparator_AfterLast);
 u64 separator_size = separator.size*(list.node_count + before_first + after_last - 1);
 String_u8 string = string_u8_push(arena, list.total_size + separator_size + term_padding);
 if(before_first){
  string_concat(&string, separator);
 }
 for (Node_String *node = list.first;
      node != 0;
      node = node->next)
 {
  block_copy_count(string.str + string.size, node->string.str, node->string.size);
  if(mod != 0){
   mod(SCu8(string.str + string.size, node->string.size));
  }
  string.size += node->string.size;
  string_concat(&string, separator);
 }
 if (after_last){
  string_concat(&string, separator);
 }
 if (term_padding == 1){
  string_null_terminate(&string);
 }
 return(string.string);
}
function String
string_list_flatten(Arena *arena, List_String list, String separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
 return(string_list_flatten(arena, list, 0, separator, separator_flags, rule));
}
function String
string_list_flatten(Arena *arena, List_String list, String_u8_Mod_Function_Type *mod, String_Fill_Terminate_Rule rule){
 return(string_list_flatten(arena, list, mod, SCu8(), 0, rule));
}
function String
string_list_flatten(Arena *arena, List_String string, String_Fill_Terminate_Rule rule){
 return(string_list_flatten(arena, string, 0, SCu8(), 0, rule));
}
function String
string_list_flatten(Arena *arena, List_String string)
{
 return(string_list_flatten(arena, string, 0, SCu8(), 0, StringFill_NoTerminate));
}

function void
string_list_pushfv(Arena *arena, List_String *list, char *format, va_list args)
{
 String string = push_stringfv(arena, format, args);
 string_list_push(arena, list, string);
}
function void
string_list_pushf(Arena *arena, List_String *list, char *format, ...){
 va_list args;
 va_start(args, format);
 string_list_pushfv(arena, list, format, args);
 va_end(args);
}


function List_String
string_split_needle(Arena *arena, String string, String needle){
 List_String list = {};
 for (;string.size > 0;){
  u64 pos = string_find_first(string, needle);
  String prefix = string_prefix(string, pos);
  if (pos < string.size){
   string_list_push(arena, &list, needle);
  }
  if (prefix.size > 0){
   string_list_push(arena, &list, prefix);
  }
  string = string_skip(string, prefix.size + needle.size);
 }
 return(list);
}

function void
string_list_insert_separators(Arena *arena, List_String *list, String separator, String_Separator_Flag flags){
 Node_String *last = list->last;
 for (Node_String *node = list->first, *next = 0;
      node != last;
      node = next){
  next = node->next;
  Node_String *new_node = push_array(arena, Node_String, 1);
  node->next = new_node;
  new_node->next = next;
  new_node->string = separator;
  list->node_count += 1;
  list->total_size += separator.size;
 }
 if (HasFlag(flags, StringSeparator_BeforeFirst)){
  Node_String *new_node = push_array(arena, Node_String, 1);
  new_node->next = list->first;
  list->first = new_node;
  new_node->string = separator;
  list->node_count += 1;
  list->total_size += separator.size;
 }
 if (HasFlag(flags, StringSeparator_AfterLast)){
  Node_String *new_node = push_array(arena, Node_String, 1);
  list->last->next = new_node;
  list->last = new_node;
  new_node->next = 0;
  new_node->string = separator;
  list->node_count += 1;
  list->total_size += separator.size;
 }
}

function void
string_list_rewrite_nodes(Arena *arena, List_String *list, String needle, String new_value){
 for (Node_String *node = list->first;
      node != 0;
      node = node->next){
  if (string_match(node->string, needle)){
   node->string = new_value;
   list->total_size += new_value.size;
   list->total_size -= needle.size;
  }
 }
}

function String
string_condense_whitespace(Arena *arena, String string){
 u8 split_characters[] = { ' ', '\t', '\n', '\r', '\f', '\v', };
 List_String list = string_split(arena, string, split_characters, ArrayCount(split_characters));
 string_list_insert_separators(arena, &list, SCu8(split_characters, 1), StringSeparator_NoFlags);
 return(string_list_flatten(arena, list, StringFill_NullTerminate));
}

function List_String
string_split_wildcards(Arena *arena, String string){
 List_String list = {};
 if (string_get_character(string, 0) == '*'){
  string_list_push(arena, &list, SCu8());
 }
 {
  List_String splits = string_split(arena, string, (u8*)"*", 1);
  string_list_push(&list, &splits);
 }
 if (string.size > 1 && string_get_character(string, string.size - 1) == '*'){
  string_list_push(arena, &list, SCu8());
 }
 return(list);
}

function b32
string_wildcard_match(List_String list, String string, String_Match_Rule rule)
{
 b32 success = true;
 if (list.node_count > 0){
  String head = list.first->string;
  if (!string_match(head, string_prefix(string, head.size), rule)){
   success = false;
  }
  else if (list.node_count > 1){
   string = string_skip(string, head.size);
   String tail = list.last->string;
   if (!string_match(tail, string_postfix(string, tail.size), rule)){
    success = false;
   }
   else if (list.node_count > 2){
    string = string_chop(string, tail.size);
    Node_String *one_past_last = list.last;
    for (Node_String *node = list.first->next;
         node != one_past_last;
         node = node->next){
     u64 position = string_find_first(string, node->string, rule);
     if (position < string.size){
      string = string_skip(string, position + node->string.size);
     }
     else{
      success = false;
      break;
     }
    }
   }
  }
 }
 return(success);
}

function b32
string_wildcard_match(List_String list, String string){
 return(string_wildcard_match(list, string, StringMatch_Exact));
}
function b32
string_wildcard_match_insensitive(List_String list, String string){
 return(string_wildcard_match(list, string, StringMatch_CaseInsensitive));
}

function void
string_list_reverse(List_String *list){
 Node_String *first = 0;
 Node_String *last = list->first;
 for (Node_String *node = list->first, *next = 0;
      node != 0;
      node = next){
  next = node->next;
  sll_stack_push(first, node);
 }
 list->first = first;
 list->last = last;
}

function b32
string_list_match(List_String a, List_String b){
 b32 result = true;
 for (Node_String *a_node = a.first, *b_node = b.first;
      a_node != 0 && b_node != 0;
      a_node = a_node->next, b_node = b_node->next){
  if (!string_match(a_node->string, b_node->string)){
   result = false;
   break;
  }
 }
 return(result);
}

function String_u8
string_u8_from_string_char(Arena *arena, String_Const_char string, String_Fill_Terminate_Rule rule)
{
 String_u8 out = {};
 out.cap = string.size;
 if (rule == StringFill_NullTerminate){
  out.cap += 1;
 }
 out.str = push_array(arena, u8, out.cap);
 for (u64 i = 0; i < string.size; i += 1){
  out.str[i] = ((u8)string.str[i])&bitmask_7;
 }
 out.size = string.size;
 if (rule == StringFill_NullTerminate){
  string_null_terminate(&out);
 }
 return(out);
}

////////////////////////////////

function String_u8
string_u8_from_string_u32(Arena *arena, String_Const_u32 string, String_Fill_Terminate_Rule rule){
 String_u8 out = {};
 out.cap = string.size*4;
 if (rule == StringFill_NullTerminate){
  out.cap += 1;
 }
 out.str = push_array(arena, u8, out.cap);
 u32 *ptr = string.str;
 u32 *one_past_last = ptr + string.size;
 for (;ptr < one_past_last; ptr += 1){
  out.size += utf8_write(out.str + out.size, *ptr);
 }
 if (rule == StringFill_NullTerminate){
  string_null_terminate(&out);
 }
 return(out);
}

function String
string_u8_from_any(Arena *arena, String_Const_Any string){
 String result = {};
 switch (string.encoding){
  case StringEncoding_ASCII: result = string_u8_from_string_char(arena, string.s_char, StringFill_NullTerminate).string; break;
  case StringEncoding_UTF8:  result = string.s_u8; break;
  case StringEncoding_UTF16: result = string_u8_from_string_u16(arena, string.s_u16, StringFill_NullTerminate).string; break;
  case StringEncoding_UTF32: result = string_u8_from_string_u32(arena, string.s_u32, StringFill_NullTerminate).string; break;
 }
 return(result);
}


////////////////////////////////

function Line_Ending_Kind
string_guess_line_ending_kind(String string){
 b32 looks_like_binary = false;
 u64 crlf_count = 0;
 u64 lf_count = 0;
 for (u64 i = 0; i < string.size; i += 1){
  if (string.str[i] == 0){
   looks_like_binary = true;
   break;
  }
  if (string.str[i] == '\r' &&
      (i + 1 == string.size || string.str[i + 1] != '\n')){
   looks_like_binary = true;
   break;
  }
  if (string.str[i] == '\n'){
   if (i > 0 && string.str[i - 1] == '\r'){
    crlf_count += 1;
   }
   else{
    lf_count += 1;
   }
  }
 }
 Line_Ending_Kind kind = LineEndingKind_Binary;
 if (!looks_like_binary){
  if (crlf_count > lf_count){
   kind = LineEndingKind_CRLF;
  }
  else{
   kind = LineEndingKind_LF;
  }
 }
 return(kind);
}

////////////////////////////////

function List_String
string_replace_list(Arena *arena, String source, String needle, String replacement){
 List_String list = {};
 for (;;){
  u64 i = string_find_first(source, needle);
  string_list_push(arena, &list, string_prefix(source, i));
  if (i < source.size){
   string_list_push(arena, &list, replacement);
   source = string_skip(source, i + needle.size);
  }
  else{
   break;
  }
 }
 return(list);
}

function String
string_replace(Arena *arena, String source, String needle, String replacement, String_Fill_Terminate_Rule rule){
 List_String list = string_replace_list(arena, source, needle, replacement);
 return(string_list_flatten(arena, list, rule));
}

function String
string_replace(Arena *arena, String source, String needle, String replacement){
 return(string_replace(arena, source, needle, replacement, StringFill_NullTerminate));
}

////////////////////////////////

function b32
byte_is_ascii(u8 byte){
 return(byte == '\r' || byte == '\n' || byte == '\t' || (' ' <= byte && byte <= '~'));
}

function b32
data_is_ascii(String data){
 u8 *ptr = data.str;
 u8 *one_past_last = ptr + data.size;
 b32 result = true;
 for (;ptr < one_past_last; ptr += 1){
  if (!byte_is_ascii(*ptr) && !(*ptr == 0 && ptr + 1 == one_past_last)){
   result = false;
   break;
  }
 }
 return(result);
}

////////////////////////////////

function String
string_escape(Arena *arena, String string){
 List_String list = string_replace_list(arena, string, strlit("\\"),
                                                 strlit("\\\\"));
 Node_String **fixup_ptr = &list.first;
 for (Node_String *node = list.first, *next = 0;
      node != 0;
      node = next){
  next = node->next;
  List_String relist = string_replace_list(arena, node->string, strlit("\""),
                                                    strlit("\\\""));
  if (relist.first != 0){
   *fixup_ptr = relist.first;
   relist.last->next = next;
   fixup_ptr = &relist.last->next;
   list.last = relist.last;
  }
  else{
   *fixup_ptr = next;
  }
 }
 return(string_list_flatten(arena, list, StringFill_NullTerminate));
}

function String_Const_char
string_interpret_escapes(Arena *arena, String_Const_char string)
{
 char *space = push_array(arena, char, string.size + 1);
 String_char result = Schar(space, 0, string.size);
 for (;;)
 {
  u64 back_slash_pos = string_find_first(string, '\\');
  string_concat(&result, string_prefix(string, back_slash_pos));
  string = string_skip(string, back_slash_pos + 1);
  if (string.size == 0){
   break;
  }
  switch (string.str[0]){
   case '\\':
   {
    string_concat_character(&result, '\\');
   }break;
   
   case 'n':
   {
    string_concat_character(&result, '\n');
   }break;
   
   case 't':
   {
    string_concat_character(&result, '\t');
   }break;
   
   case '"':
   {
    string_concat_character(&result, '\"');
   }break;
   
   case '0':
   {
    string_concat_character(&result, '\0');
   }break;
   
   default:
   {
    char c[2] = {'\\'};
    c[1] = string.str[0];
    string_concat(&result, SCchar(c, 2));
   }break;
  }
  string = string_skip(string, 1);
 }
 result.str[result.size] = 0;
 pop_array(arena, char, result.cap - result.size);
 return(result.string);
}

function String
string_interpret_escapes(Arena *arena, String string){
 return(SCu8(string_interpret_escapes(arena, SCchar(string))));
}

global_const u8 integer_symbols[] = {
 '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
};

global_const u8 integer_symbol_reverse[128] = {
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

global_const u8 base64[64] = {
 '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
 '_', '$',
};

global_const u8 base64_reverse[128] = {
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0xFF,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
 0xFF,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,
 0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0xFF,0xFF,0xFF,0xFF,0x3E,
 0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
 0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0xFF,0xFF,0xFF,0xFF,0xFF,
};

function u64
digit_count_from_integer(u64 x, u32 radix){
 u64 result = 0;
 if (radix >= 2 && radix <= 16){
  if (x == 0){
   result = 1;
  }
  else{
   do{
    x /= radix;
    result += 1;
   } while(x > 0);
  }
 }
 return(result);
}

function String
string_from_integer(Arena *arena, u64 x, u32 radix){
 String result = {};
 if (radix >= 2 && radix <= 16){
  if (x == 0){
   result = push_stringz(arena, strlit("0"));
  }
  else{
   u8 string_space[64];
   u64 length = 0;
   for (u64 X = x;
        X > 0;
        X /= radix, length += 1){
    string_space[length] = integer_symbols[X%radix];
   }
   for (u64 j = 0, i = length - 1;
        j < i;
        j += 1, i -= 1){
    macro_swap(string_space[i], string_space[j]);
   }
   result = push_stringz(arena, SCu8(string_space, length));
  }
 }
 return(result);
}

function b32
string_is_integer(String string, u32 radix){
 b32 is_integer = false;
 if (string.size > 0 && radix <= 16){
  is_integer = true;
  for (u64 i = 0; i < string.size; i += 1){
   if (string.str[i] < 128){
    u8 x = integer_symbol_reverse[character_to_upper(string.str[i])];
    if (x >= radix){
     is_integer = false;
     break;
    }
   }
   else{
    is_integer = false;
    break;
   }
  }
 }
 return(is_integer);
}

function u64
string_to_u64(String8 string, u32 radix=10)
{
 u64 x = 0;
 if(radix <= 16)
 {
  for (u64 i = 0; i < string.size; i += 1)
  {
   x *= radix;
   if (string.str[i] < 128)
   {
    x += integer_symbol_reverse[character_to_upper(string.str[i])];
   }
   else
   {
    x += 0xFF;
   }
  }
 }
 return(x);
}

//~

function u64
time_stamp_from_date_time(Date_Time *date_time){
 u64 result = 0;
 result += date_time->year;
 result *= 12;
 result += date_time->mon;
 result *= 30;
 result += date_time->day;
 result *= 24;
 result += date_time->hour;
 result *= 60;
 result += date_time->min;
 result *= 61;
 result += date_time->sec;
 result *= 1000;
 result += date_time->msec;
 return(result);
}

function Date_Time
date_time_from_time_stamp(u64 time_stamp){
 Date_Time result = {};
 result.msec = time_stamp%1000;
 time_stamp /= 1000;
 result.sec = time_stamp%61;
 time_stamp /= 61;
 result.min = time_stamp%60;
 time_stamp /= 60;
 result.hour = time_stamp%24;
 time_stamp /= 24;
 result.day = time_stamp%30;
 time_stamp /= 30;
 result.mon = time_stamp%12;
 time_stamp /= 12;
 result.year = (u32)time_stamp;
 return(result);
}


//~

function i32
range_side(Range_i32 a, Side side){
 return(side == Side_Min?a.min:a.max);
}
function i64
range_side(Range_i64 a, Side side){
 return(side == Side_Min?a.min:a.max);
}
function u64
range_side(Range_u64 a, Side side){
 return(side == Side_Min?a.min:a.max);
}
function f32
range_side(Range_f32 a, Side side){
 return(side == Side_Min?a.min:a.max);
}
//~
inline Base_Allocator *
get_default_allocator(){
 return &malloc_base_allocator;
}
////////////////////////////////
typedef u64 Profile_ID;
struct Profile_Record{
 Profile_Record *next;
 Profile_ID id;
 u64 time;
 String location;
 String name;
};

struct Profile_Thread{
 Profile_Thread *next;
 Profile_Record *first_record;
 Profile_Record *last_record;
 i32 record_count;
 i32 thread_id;
 String name;
};

typedef u32 Profile_Enable_Flag;
enum{
 ProfileEnable_UserBit    = 0x1,
 ProfileEnable_InspectBit = 0x2,
};

// NOTE(allen): full definition in 4coder_profile.h, due to dependency on System_Mutex.
struct Profile_Global_List;

typedef i32 Thread_Kind;
enum{
 ThreadKind_None = 0,
 ThreadKind_Main,
 ThreadKind_MainCoroutine,
 ThreadKind_AsyncTasks,
};

//-
struct Thread_Context{
 Thread_Kind kind;
 
 Base_Allocator *prof_allocator;
 Profile_ID prof_id_counter;
 Arena prof_arena;
 Profile_Record *prof_first;
 Profile_Record *prof_last;
 i32 prof_record_count;
 
 void *user_data;
};
function void
thread_context_init(Thread_Context *tctx, Thread_Kind kind,
                    Base_Allocator *allocator,
                    Base_Allocator *prof_allocator){
 if(kind == ThreadKind_Main){
  //-
 }
 block_zero_struct(tctx);
 tctx->kind       = kind;
 
 tctx->prof_allocator  = prof_allocator;
 tctx->prof_id_counter = 1;
 tctx->prof_arena      = make_arena(KB(16));
}
thread_local Thread_Context global_thread_context;

function Thread_Context *
get_thread_context(){
 return &global_thread_context;
}

typedef App Application_Links;  // NOTE: has to be here for the 4coder meta-generator.

api(custom) function Thread_Context*
get_thread_context(App *app)
{
 return(app->tctx);
}
//-
struct Heap_Basic_Node{
 Heap_Basic_Node *next;
 Heap_Basic_Node *prev;
};
struct Heap_Node{
 union{
  struct{
   Heap_Basic_Node order;
   Heap_Basic_Node alloc;
   u64 size;
  };
  u8 force_size__[64];
 };
};
struct Heap{
 Arena arena_;
 Arena *arena;
 Heap_Basic_Node in_order;
 Heap_Basic_Node free_nodes;
 u64 used_space;
 u64 total_space;
};
//~
#define heap__sent_init(s) (s)->next=(s)->prev=(s)
#define heap__insert_next(p,n) ((n)->next=(p)->next,(n)->prev=(p),(n)->next->prev=(n),(p)->next=(n))
#define heap__insert_prev(p,n) ((n)->prev=(p)->prev,(n)->next=(p),(n)->prev->next=(n),(p)->prev=(n))
#define heap__remove(n) ((n)->next->prev=(n)->prev,(n)->prev->next=(n)->next)

#if defined(DO_HEAP_CHECKS)
function void
heap_assert_good(Heap *heap){
 if (heap->in_order.next != 0){
  Assert(heap->in_order.prev != 0);
  Assert(heap->free_nodes.next != 0);
  Assert(heap->free_nodes.prev != 0);
  for (Heap_Basic_Node *node = &heap->in_order;;){
   Assert(node->next->prev == node);
   Assert(node->prev->next == node);
   node = node->next;
   if (node == &heap->in_order){
    break;
   }
  }
  for (Heap_Basic_Node *node = &heap->free_nodes;;){
   Assert(node->next->prev == node);
   Assert(node->prev->next == node);
   node = node->next;
   if (node == &heap->free_nodes){
    break;
   }
  }
 }
}
#else
#define heap_assert_good(heap) ((void)(heap))
#endif

function void
heap_init(Heap *heap){
 heap->arena_ = make_arena();
 heap->arena = &heap->arena_;
 heap__sent_init(&heap->in_order);
 heap__sent_init(&heap->free_nodes);
 heap->used_space = 0;
 heap->total_space = 0;
}

function void
heap_init(Heap *heap, Arena *arena){
 heap->arena = arena;
 heap__sent_init(&heap->in_order);
 heap__sent_init(&heap->free_nodes);
 heap->used_space = 0;
 heap->total_space = 0;
}

function void
heap_free_all(Heap *heap){
 if (heap->arena == &heap->arena_){
  arena_free(heap->arena);
 }
 block_zero_struct(heap);
}

function void
heap__extend(Heap *heap, void *memory, u64 size){
 heap_assert_good(heap);
 if (size >= sizeof(Heap_Node)){
  Heap_Node *new_node = (Heap_Node*)memory;
  heap__insert_prev(&heap->in_order, &new_node->order);
  heap__insert_next(&heap->free_nodes, &new_node->alloc);
  new_node->size = size - sizeof(*new_node);
  heap->total_space += size;
 }
 heap_assert_good(heap);
}

function void
heap__extend_automatic(Heap *heap, u64 size, DEBUG_File_Line file_line){
 void *memory = push_array(heap->arena, u8, size, push_default(), file_line);
 heap__extend(heap, memory, size);
}

function void*
heap__reserve_chunk(Heap *heap, Heap_Node *node, u64 size){
 u8 *ptr = (u8*)(node + 1);
 Assert(node->size >= size);
 u64 left_over_size = node->size - size;
 if (left_over_size > sizeof(*node)){
  u64 new_node_size = left_over_size - sizeof(*node);
  Heap_Node *new_node = (Heap_Node*)(ptr + size);
  heap__insert_next(&node->order, &new_node->order);
  heap__insert_next(&node->alloc, &new_node->alloc);
  new_node->size = new_node_size;
 }
 heap__remove(&node->alloc);
 node->alloc.next = 0;
 node->alloc.prev = 0;
 node->size = size;
 heap->used_space += sizeof(*node) + size;
 return(ptr);
}

function void*
heap_allocate(Heap *heap, u64 size, DEBUG_File_Line file_line=DEBUG_file_line())
{
 b32 first_try = true;
 for (;;)
 {
  if (heap->in_order.next != 0)
  {
   heap_assert_good(heap);
   u64 aligned_size = (size + sizeof(Heap_Node) - 1);
   aligned_size = aligned_size - (aligned_size%sizeof(Heap_Node));
   for (Heap_Basic_Node *n = heap->free_nodes.next;
        n != &heap->free_nodes;
        n = n->next){
    Heap_Node *node = CastFromMember(Heap_Node, alloc, n);
    if (node->size >= aligned_size){
     void *ptr = heap__reserve_chunk(heap, node, aligned_size);
     heap_assert_good(heap);
     return(ptr);
    }
   }
   heap_assert_good(heap);
  }
  
  if (first_try){
   u64 extension_size = clamp_min(KB(64), size*2);
   heap__extend_automatic(heap, extension_size, file_line);
   first_try = false;
  }
  else{
   break;
  }
 }
 return(0);
}

function void
heap__merge(Heap *heap, Heap_Node *l, Heap_Node *r){
 if (&l->order != &heap->in_order && &r->order != &heap->in_order &&
     l->alloc.next != 0 && l->alloc.prev != 0 &&
     r->alloc.next != 0 && r->alloc.prev != 0){
  u8 *ptr = (u8*)(l + 1) + l->size;
  if (PtrDif(ptr, r) == 0){
   heap__remove(&r->order);
   heap__remove(&r->alloc);
   heap__remove(&l->alloc);
   l->size += r->size + sizeof(*r);
   heap__insert_next(&heap->free_nodes, &l->alloc);
  }
 }
}

function void
heap_free(Heap *heap, void *memory){
 if (heap->in_order.next != 0 && memory != 0){
  Heap_Node *node = ((Heap_Node*)memory) - 1;
  Assert(node->alloc.next == 0);
  Assert(node->alloc.prev == 0);
  heap->used_space -= sizeof(*node) + node->size;
  heap_assert_good(heap);
  heap__insert_next(&heap->free_nodes, &node->alloc);
  heap_assert_good(heap);
  heap__merge(heap, node, CastFromMember(Heap_Node, order, node->order.next));
  heap_assert_good(heap);
  heap__merge(heap, CastFromMember(Heap_Node, order, node->order.prev), node);
  heap_assert_good(heap);
 }
}

#define heap_array(heap, T, c) (T*)(heap_allocate((heap), sizeof(T)*(c)))

////////////////////////////////

function void*
base_allocate__heap(void *user_data, u64 size, u64 *size_out,
                    DEBUG_File_Line file_line=DEBUG_file_line())
{
 Heap *heap = (Heap*)user_data;
 void *memory = heap_allocate(heap, size, file_line);
 *size_out = size;
 return(memory);
}

function void
base_free__heap(void *user_data, void *ptr){
 Heap *heap = (Heap*)user_data;
 heap_free(heap, ptr);
}

function Base_Allocator
base_allocator_on_heap(Heap *heap){
 Base_Allocator result = make_base_allocator_generic(base_allocate__heap, base_free__heap, heap);
 return(result);
}

struct Range_i32_Array{
 Range_i32 *ranges;
 i32 count;
};
struct Range_i64_Array{
 Range_i64 *ranges;
 i32 count;
};
struct Range_u64_Array{
 Range_u64 *ranges;
 i32 count;
};
struct Range_f32_Array{
 Range_f32 *ranges;
 i32 count;
};

function void
block_range_copy__inner(void *dst, void *src, Range_u64 range, i64 shift){
 block_copy((u8*)dst + range.first + shift, (u8*)src + range.first, range.max - range.min);
}

function void
block_range_copy__inner(void *dst, void *src, Range_u64 range, i64 shift, u64 item_size){
 range.first *= item_size;
 range.opl *= item_size;
 shift *= item_size;
 block_range_copy__inner(dst, src, range, shift);
}

#define block_range_copy(d,s,r,h) block_range_copy__inner((d),(s),Iu64(r),(i64)(h))
#define block_range_copy_sized(d,s,r,h,i) block_range_copy__inner((d),(s),Iu64(r),(i64)(h),(i))
#define block_range_copy_typed(d,s,r,h) block_range_copy_sized((d),(s),(r),(h),sizeof(*(d)))

function void
block_copy_array_dst_shift__inner(void *dst, void *src, u64 it_size, Range_i64 range, i64 shift){
 u8 *dptr = (u8*)dst;
 u8 *sptr = (u8*)src;
 dptr += it_size*(range.first + shift);
 sptr += it_size*range.first;
 block_copy(dptr, sptr, (u64)(it_size*(range.opl - range.first)));
}
function void
block_copy_array_dst_shift__inner(void *dst, void *src, u64 it_size, Range_i32 range, i64 shift){
 u8 *dptr = (u8*)dst;
 u8 *sptr = (u8*)src;
 dptr += it_size*(range.first + shift);
 sptr += it_size*range.first;
 block_copy(dptr, sptr, (u64)(it_size*(range.opl - range.first)));
}

#define block_copy_array_dst_shift(d,s,r,h) block_copy_array_dst_shift__inner((d),(s),sizeof(*(d)),(r),(h))

global Range_i32 Ii32_neg_inf = {max_i32, min_i32};
global Range_i64 Ii64_neg_inf = {max_i64, min_i64};
global Range_u64 Iu64_neg_inf = {max_u64, 0};
global Range_f32 If32_neg_inf = {max_f32, -max_f32};

function bool
operator==(Range_i32 a, Range_i32 b){
 return(a.min == b.min && a.max == b.max);
}
function bool
operator==(Range_i64 a, Range_i64 b){
 return(a.min == b.min && a.max == b.max);
}
function bool
operator==(Range_u64 a, Range_u64 b){
 return(a.min == b.min && a.max == b.max);
}
function bool
operator==(Range_f32 a, Range_f32 b){
 return(a.min == b.min && a.max == b.max);
}

function Range_i32
operator+(Range_i32 r, i32 s){
 return(Ii32(r.min + s, r.max + s));
}
function Range_i64
operator+(Range_i64 r, i64 s){
 return(Ii64(r.min + s, r.max + s));
}
function Range_u64
operator+(Range_u64 r, u64 s){
 return(Iu64(r.min + s, r.max + s));
}
function Range_f32
operator+(Range_f32 r, f32 s){
 return(If32(r.min + s, r.max + s));
}

function Range_i32
operator-(Range_i32 r, i32 s){
 return(Ii32(r.min - s, r.max - s));
}
function Range_i64
operator-(Range_i64 r, i64 s){
 return(Ii64(r.min - s, r.max - s));
}
function Range_u64
operator-(Range_u64 r, u64 s){
 return(Iu64(r.min - s, r.max - s));
}
function Range_f32
operator-(Range_f32 r, f32 s){
 return(If32(r.min - s, r.max - s));
}

function Range_i32&
operator+=(Range_i32 &r, i32 s){
 r = r + s;
 return(r);
}
function Range_i64&
operator+=(Range_i64 &r, i64 s){
 r = r + s;
 return(r);
}
function Range_u64&
operator+=(Range_u64 &r, u64 s){
 r = r + s;
 return(r);
}
function Range_f32&
operator+=(Range_f32 &r, f32 s){
 r = r + s;
 return(r);
}

function Range_i32&
operator-=(Range_i32 &r, i32 s){
 r = r - s;
 return(r);
}
function Range_i64&
operator-=(Range_i64 &r, i64 s){
 r = r - s;
 return(r);
}
function Range_u64&
operator-=(Range_u64 &r, u64 s){
 r = r - s;
 return(r);
}
function Range_f32&
operator-=(Range_f32 &r, f32 s){
 r = r - s;
 return(r);
}

function Range_i32
range_margin(Range_i32 range, i32 margin){
 range.min += margin;
 range.max += margin;
 return(range);
}
function Range_i64
range_margin(Range_i64 range, i64 margin){
 range.min += margin;
 range.max += margin;
 return(range);
}
function Range_u64
range_margin(Range_u64 range, u64 margin){
 range.min += margin;
 range.max += margin;
 return(range);
}
function Range_f32
range_margin(Range_f32 range, f32 margin){
 range.min += margin;
 range.max += margin;
 return(range);
}

function Range_i32
range_intersect(Range_i32 a, Range_i32 b){
 Range_i32 result = {};
 if (range_overlap(a, b)){
  result = Ii32(Max(a.min, b.min), Min(a.max, b.max));
 }
 return(result);
}
function Range_i64
range_intersect(Range_i64 a, Range_i64 b){
 Range_i64 result = {};
 if (range_overlap(a, b)){
  result = Ii64(Max(a.min, b.min), Min(a.max, b.max));
 }
 return(result);
}
function Range_u64
range_intersect(Range_u64 a, Range_u64 b){
 Range_u64 result = {};
 if (range_overlap(a, b)){
  result = Iu64(Max(a.min, b.min), Min(a.max, b.max));
 }
 return(result);
}
function Range_f32
range_intersect(Range_f32 a, Range_f32 b){
 Range_f32 result = {};
 if (range_overlap(a, b)){
  result = If32(Max(a.min, b.min), Min(a.max, b.max));
 }
 return(result);
}

function Range_i32
range_union(Range_i32 a, Range_i32 b){
 return(Ii32(Min(a.min, b.min), Max(a.max, b.max)));
}
function Range_i64
range_union(Range_i64 a, Range_i64 b){
 return(Ii64(Min(a.min, b.min), Max(a.max, b.max)));
}
function Range_u64
range_union(Range_u64 a, Range_u64 b){
 return(Iu64(Min(a.min, b.min), Max(a.max, b.max)));
}
function Range_f32
range_union(Range_f32 a, Range_f32 b){
 return(If32(Min(a.min, b.min), Max(a.max, b.max)));
}

#define BODY  return(a.min <= p && p <= a.max);
function b32 range_contains_inclusive(Range_i32 a, i32 p){ BODY }
function b32 range_contains_inclusive(Range_i64 a, i64 p){ BODY }
function b32 range_contains_inclusive(Range_u64 a, u64 p){ BODY }
function b32 range_inclusive_contains(Range_f32 a, f32 p){ BODY }
#undef BODY

#define BODY return(a.min <= p && p < a.max);
function b32 range_contains(Range_i32 a, i32 p){ BODY }
function b32 range_contains(Range_i64 a, i64 p){ BODY }
function b32 range_contains(Range_u64 a, u64 p){ BODY }
function b32 range_contains(Range_f32 a, f32 p){ BODY }
#undef BODY

#define BODY  return(clamp_min(0, a.max - a.min + 1));
function i32 range_size_inclusive(Range_i32 a) { BODY }
function i64 range_size_inclusive(Range_i64 a) { BODY }
function u64 range_size_inclusive(Range_u64 a) { BODY }
function f32 range_size_inclusive(Range_f32 a) { BODY }
#undef BODY

function Range_i32 rectify(Range_i32 a){ return(Ii32(a.min, a.max)); }
function Range_i64 rectify(Range_i64 a){ return(Ii64(a.min, a.max)); }
function Range_u64 rectify(Range_u64 a){ return(Iu64(a.min, a.max)); }
function Range_f32 rectify(Range_f32 a){ return(If32(a.min, a.max)); }

function Range_i32
range_clamp_size(Range_i32 a, i32 max_size){
 i32 max = a.min + max_size;
 a.max = clamp_max(a.max, max);
 return(a);
}
function Range_i64
range_clamp_size(Range_i64 a, i64 max_size){
 i64 max = a.min + max_size;
 a.max = clamp_max(a.max, max);
 return(a);
}
function Range_u64
range_clamp_size(Range_u64 a, u64 max_size){
 u64 max = a.min + max_size;
 a.max = clamp_max(a.max, max);
 return(a);
}
function Range_f32
range_clamp_size(Range_f32 a, f32 max_size){
 f32 max = a.min + max_size;
 a.max = clamp_max(a.max, max);
 return(a);
}

function b32
range_is_valid(Range_i32 a){
 return(a.min <= a.max);
}
function b32
range_is_valid(Range_i64 a){
 return(a.min <= a.max);
}
function b32
range_is_valid(Range_u64 a){
 return(a.min <= a.max);
}
function b32
range_is_valid(Range_f32 a){
 return(a.min <= a.max);
}

function i32
range_distance(Range_i32 a, Range_i32 b){
 i32 result = 0;
 if (!range_overlap(a, b)){
  if (a.max < b.min){
   result = b.min - a.max;
  }
  else{
   result = a.min - b.max;
  }
 }
 return(result);
}
function i64
range_distance(Range_i64 a, Range_i64 b){
 i64 result = 0;
 if (!range_overlap(a, b)){
  if (a.max < b.min){
   result = b.min - a.max;
  }
  else{
   result = a.min - b.max;
  }
 }
 return(result);
}
function u64
range_distance(Range_u64 a, Range_u64 b){
 u64 result = 0;
 if (!range_overlap(a, b)){
  if (a.max < b.min){
   result = b.min - a.max;
  }
  else{
   result = a.min - b.max;
  }
 }
 return(result);
}
function f32
range_distance(Range_f32 a, Range_f32 b){
 f32 result = 0;
 if (!range_overlap(a, b)){
  if (a.max < b.min){
   result = b.min - a.max;
  }
  else{
   result = a.min - b.max;
  }
 }
 return(result);
}

function f32
lerp(f32 t, Range_f32 x){
 return(x.min + (x.max - x.min)*t);
}


function Range_f32
lerp(f32 a, Range_f32 x, f32 b){
 x.min = lerp(a, x.min, b);
 x.max = lerp(a, x.max, b);
 return(x);
}

function f32
lerp(Range_f32 range, f32 t){
 return(lerp(range.min, t, range.max));
}

function f32
clamp_range(Range_f32 range, f32 x)
{
 return(clamp_between(range.min, x, range.max));
}

function String
string_substring(String str, i64 min, i64 max)
{
 String result = {};
 if(min <= max)
 {
  ClampBot(min, 0);
  ClampTop(max, (i64)str.count);
  result.str   = str.str + min;
  result.count = max - min;
 }
 return result;
}
myinline String
string_substring(String str, Range_i64 range)
{
 return string_substring(str, RangeExpand(range));
}
function b32
is_generated_file_name(String name)
{
 b32 is_generated = false;
 {//NOTE Is the file generated?
  i64 last_dot = string_find_last(name, '.');
  if(last_dot != i64(name.count)){
   String dot_gen = string_substring(name, last_dot-4, last_dot);
   is_generated = (dot_gen == strlit(".gen"));
  }
 }
 return is_generated;
}
//~EOF
