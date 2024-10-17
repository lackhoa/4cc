/*
 * Printf style operations for strings and what-not
 */

// TOP

#pragma once

#include <stdio.h>
#include "4ed_base.h"

////////////////////////////////

// yyyy
function void
push_year_full(Arena *arena, List_String *list, u32 year){
    string_list_pushf(arena, list, "%u", year);
}
// yy
function void
push_year_abrev(Arena *arena, List_String *list, u32 year){
    string_list_pushf(arena, list, "%u", year % 100);
}

// m
function void
push_month_num(Arena *arena, List_String *list, u8 mon){
    string_list_pushf(arena, list, "%u", mon + 1);
}
// mm
function void
push_month_num_zeros(Arena *arena, List_String *list, u8 mon){
    string_list_pushf(arena, list, "%02u", mon + 1);
}
// month
function void
push_month_name(Arena *arena, List_String *list, u8 mon){
    string_list_push(arena, list, month_full_name[mon%12]);
}
// mon
function void
push_month_abrev(Arena *arena, List_String *list, u8 mon){
    string_list_push(arena, list, String{month_full_name[mon%12].str,3});
}

// d
function void
push_day_num(Arena *arena, List_String *list, u8 day){
    string_list_pushf(arena, list, "%u", day + 1);
}
// dd
function void
push_day_num_zeroes(Arena *arena, List_String *list, u8 day){
    string_list_pushf(arena, list, "%02u", day + 1);
}
// day
function void
push_day_ord(Arena *arena, List_String *list, u8 day){
    string_list_pushf(arena, list, "%d", day);
}

// h24
function void
push_hour_24(Arena *arena, List_String *list, u8 hour){
    string_list_pushf(arena, list, "%u", hour);
}
// hh24
function void
push_hour_24_zeroes(Arena *arena, List_String *list, u8 hour){
    string_list_pushf(arena, list, "%02u", hour);
}
// h
function void
push_hour_12(Arena *arena, List_String *list, u8 hour){
    string_list_pushf(arena, list, "%u", hour%12);
}
// hh
function void
push_hour_12_zeroes(Arena *arena, List_String *list, u8 hour){
    string_list_pushf(arena, list, "%02u", hour%12);
}
// ampm
function void
push_hour_am_pm(Arena *arena, List_String *list, u8 hour){
    if (hour >= 12){
        string_list_push(arena, list, strlit("pm"));
    }
    else{
        string_list_push(arena, list, strlit("am"));
    }
}

// mi
function void
push_minute(Arena *arena, List_String *list, u8 min){
    string_list_pushf(arena, list, "%u", min);
}
// mimi
function void
push_minute_zeroes(Arena *arena, List_String *list, u8 min){
    string_list_pushf(arena, list, "%02u", min);
}

// s
function void
push_second(Arena *arena, List_String *list, u8 sec){
    string_list_pushf(arena, list, "%u", sec);
}
// ss
function void
push_second_zeroes(Arena *arena, List_String *list, u8 sec){
    string_list_pushf(arena, list, "%02u", sec);
}

// ms
function void
push_millisecond_zeroes(Arena *arena, List_String *list, u16 msec){
 string_list_pushf(arena, list, "%03u", msec);
}

function void
date_time_format8(Arena *arena, List_String *list, String format, Date_Time *date_time){
 u8 *ptr = format.str;
 u8 *end = format.str + format.size;
 for (;ptr < end;){
  if (character_is_alnum(*ptr)){
   u8 *start = ptr;
   for (;ptr < end; ptr += 1){
    if (!character_is_alnum(*ptr)){
     break;
    }
   }
   
   String field = SCu8(start, ptr);
   for (; field.size > 0;){
    if (string_match(string_prefix(field, 5), strlit("month"))){
     field = string_skip(field, 5);
     push_month_name(arena, list, date_time->mon);
    }
    
    else if (string_match(string_prefix(field, 4), strlit("yyyy"))){
     field = string_skip(field, 4);
     push_year_full(arena, list, date_time->year);
    }
    else if (string_match(string_prefix(field, 4), strlit("hh24"))){
     field = string_skip(field, 4);
     push_hour_24_zeroes(arena, list, date_time->hour);
    }
    else if (string_match(string_prefix(field, 4), strlit("ampm"))){
     field = string_skip(field, 4);
     push_hour_am_pm(arena, list, date_time->hour);
    }
    else if (string_match(string_prefix(field, 4), strlit("mimi"))){
     field = string_skip(field, 4);
     push_minute_zeroes(arena, list, date_time->min);
    }
    
    else if (string_match(string_prefix(field, 3), strlit("mon"))){
     field = string_skip(field, 3);
     push_month_abrev(arena, list, date_time->mon);
    }
    else if (string_match(string_prefix(field, 3), strlit("day"))){
     field = string_skip(field, 3);
     push_day_ord(arena, list, date_time->day);
    }
    else if (string_match(string_prefix(field, 3), strlit("h24"))){
     field = string_skip(field, 3);
     push_hour_24(arena, list, date_time->hour);
    }
    
    else if (string_match(string_prefix(field, 2), strlit("yy"))){
     field = string_skip(field, 2);
     push_year_abrev(arena, list, date_time->year);
    }
    else if (string_match(string_prefix(field, 2), strlit("mm"))){
     field = string_skip(field, 2);
     push_month_num_zeros(arena, list, date_time->mon);
    }
    else if (string_match(string_prefix(field, 2), strlit("dd"))){
     field = string_skip(field, 2);
     push_day_num_zeroes(arena, list, date_time->day);
    }
    else if (string_match(string_prefix(field, 2), strlit("hh"))){
     field = string_skip(field, 2);
     push_hour_12_zeroes(arena, list, date_time->hour);
    }
    else if (string_match(string_prefix(field, 2), strlit("mi"))){
     field = string_skip(field, 2);
     push_minute(arena, list, date_time->min);
    }
    else if (string_match(string_prefix(field, 2), strlit("ss"))){
     field = string_skip(field, 2);
     push_second_zeroes(arena, list, date_time->sec);
    }
    else if (string_match(string_prefix(field, 2), strlit("ms"))){
     field = string_skip(field, 2);
     push_millisecond_zeroes(arena, list, date_time->msec);
    }
    
    else if (string_match(string_prefix(field, 1), strlit("m"))){
     field = string_skip(field, 1);
     push_month_num(arena, list, date_time->mon);
    }
    else if (string_match(string_prefix(field, 1), strlit("d"))){
     field = string_skip(field, 1);
     push_day_num(arena, list, date_time->day);
    }
    else if (string_match(string_prefix(field, 1), strlit("h"))){
     field = string_skip(field, 1);
     push_hour_12(arena, list, date_time->hour);
    }
    else if (string_match(string_prefix(field, 1), strlit("s"))){
     field = string_skip(field, 1);
     push_second(arena, list, date_time->sec);
    }
    
    else{
     string_list_push(arena, list, SCu8(start, ptr));
     break;
    }
   }
  }
  else{
   u8 *start = ptr;
   for (;ptr < end; ptr += 1){
    if (character_is_alnum(*ptr)){
     break;
    }
   }
   string_list_push(arena, list, SCu8(start, ptr));
  }
 }
}
function void
date_time_format(Arena *arena, List_String *list, String format, Date_Time *date_time){
 date_time_format8(arena, list, format, date_time);
}
function void
date_time_format(Arena *arena, List_String *list, char *format, Date_Time *date_time){
 date_time_format8(arena, list, SCu8(format), date_time);
}

function String
date_time_format(Arena *arena, String format, Date_Time *date_time){
 List_String list = {};
 date_time_format(arena, &list, format, date_time);
 return(string_list_flatten(arena, list));
}
function String
date_time_format(Arena *arena, char *cformat, Date_Time *date_time){
 String format = SCu8(cformat);
 return(date_time_format(arena, format, date_time));
}

// BOTTOM
