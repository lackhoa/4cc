#pragma once

//-NOTE(kv) Annoying parsing job
inline void m_brace_open(Ed_Parser *p)  { ep_char(p, '{'); }
inline void m_bracket_open(Ed_Parser *p){ ep_char(p, '['); }
inline void m_paren_open(Ed_Parser *p)  { ep_char(p, '('); }
inline void m_brace_close(Ed_Parser *p){ ep_char(p, '}'); }
inline void m_paren_close(Ed_Parser *p){ ep_char(p, ')'); }
inline b32 m_maybe_bracket_open(Ed_Parser *p){ return ep_maybe_char(p, '['); }
inline b32 m_maybe_paren_close  (Ed_Parser *p){ return ep_maybe_char(p, ')'); }
inline b32 m_maybe_bracket_close(Ed_Parser *p){ return ep_maybe_char(p, ']'); }
inline b32 m_maybe_brace_close  (Ed_Parser *p){ return ep_maybe_char(p, '}'); }
#define mpa_parens     defer_block(m_paren_open(p), m_paren_close(p))

//-