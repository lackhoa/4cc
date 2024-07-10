/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 14.08.2019
 *
 * Logging helpers.
 *
 */

// TOP

#if !defined(FCODER_LOG_CPP)
#define FCODER_LOG_CPP

internal String
log_event(Arena *arena, String event_name, String src_name, i1 line_number, i1 buffer, i1 view, i1 thread_id){
    List_String list = {};
    string_list_pushf(arena, &list, "%.*s:%d: %.*s",
                      string_expand(src_name), line_number, string_expand(event_name));
    if (thread_id != 0){
        string_list_pushf(arena, &list, " [thread=%d]", thread_id);
    }
    if (buffer != 0){
        string_list_pushf(arena, &list, " [buffer=%d]", buffer);
    }
    if (view != 0){
        string_list_pushf(arena, &list, " [view=%d]", view);
    }
    string_list_push(arena, &list, string_u8_litexpr("\n"));
    return(string_list_flatten(arena, list));
}

#define LogEventStr(log_call, arena, B, V, T, E) \
Stmnt(Temp_Memory temp_LOG = begin_temp(arena); \
String M = log_event(arena, E, \
string_u8_litexpr(__FILE__), \
__LINE__, (B), (V), (T));    \
log_call; \
end_temp(temp_LOG); )

#define LogEventLit(log_call, arena, B, V, T, Elit) \
LogEventStr(log_call, arena, (B), (V), (T), string_u8_litexpr(Elit))

#define LogEventF(log_call, arena, B, V, T, Ef, ...) \
Stmnt(Temp_Memory temp_LOG_F = begin_temp(arena); \
String E = push_stringfz(arena, Ef, __VA_ARGS__); \
LogEventStr(log_call, arena, B, V, T, E); \
end_temp(temp_LOG_F); )

#endif

// BOTTOM

