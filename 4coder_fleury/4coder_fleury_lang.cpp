
#pragma once

#include "4coder_fleury_lang.h"
#include "4coder_fleury_index.h"

global F4_Language_State f4_langs = {};

function F4_Language *
F4_LanguageFromExtension(String extension)
{
 F4_Language *result = 0;
 if(f4_langs.initialized)
 {
  u64 hash = table_hash_u8(extension.str, extension.size);
  u64 slot = hash % ArrayCount(f4_langs.language_table);
  for(F4_Language *l = f4_langs.language_table[slot];
      l;
      l = l->next)
  {
   if(l->hash == hash && string_match(l->extension, extension))
   {
    result = l;
    break;
   }
  }
 }
 return result;
}

#define F4_RegisterLanguage(name, extension, IndexFile, LexInit, LexFullInput, PosContext, Highlight, lex_state_type) \
_F4_RegisterLanguage(name, extension, IndexFile, LexInit, LexFullInput, \
(F4_Language_PosContext *)PosContext, (F4_Language_Highlight *)Highlight, sizeof(lex_state_type))

function void
_F4_RegisterLanguage(String name,
                     String extension,
                     F4_Language_IndexFile          *IndexFile,
                     F4_Language_LexInit            *LexInit,
                     F4_Language_LexFullInput       *LexFullInput,
                     F4_Language_PosContext         *PosContext,
                     F4_Language_Highlight          *Highlight,
                     u64 lex_state_size)
{
 if(f4_langs.initialized == 0)
 {
  f4_langs.initialized = 1;
  f4_langs.arena = make_arena(KB(16));
 }
 
 F4_Language *language = F4_LanguageFromExtension(extension);
 
 if(language == 0)
 {
  u64 hash = table_hash_u8(extension.str, extension.size);
  u64 slot = hash % ArrayCount(f4_langs.language_table);
  language = push_array(&f4_langs.arena, F4_Language, 1);
  language->name = name;
  language->next = f4_langs.language_table[slot];
  f4_langs.language_table[slot] = language;
  language->hash = hash;
  language->extension = push_stringz(&f4_langs.arena, extension);
  language->lex_state_size     = lex_state_size;
  language->IndexFile          = IndexFile;
  language->LexInit            = LexInit;
  language->LexFullInput       = LexFullInput;
		language->PosContext         = PosContext;
  language->Highlight          = Highlight;
 }
}

function F4_Language *
F4_LanguageFromBuffer(App *app, Buffer_ID buffer)
{
 F4_Language *language = 0;
 Scratch_Block scratch(app);
 String8 filename = push_buffer_filepath(app, scratch, buffer);
 String8 extension = path_extension(filename);
 language = F4_LanguageFromExtension(extension);
 return language;
}

function void
F4_Language_PosContext_PushData(Arena *arena,
                                F4_Language_PosContextData **first_ptr,
                                F4_Language_PosContextData **last_ptr,
                                F4_Index_Note *note,
                                Token *query,
                                int arg_index)
{
    F4_Language_PosContextData *first = *first_ptr;
    F4_Language_PosContextData *last = *last_ptr;
    F4_Language_PosContextData *func = push_array0(arena, F4_Language_PosContextData, 1);
    func->relevant_note = note;
    func->query_token = query;
    func->argument_index = arg_index;
    if(last == 0)
    {
        first = last = func;
    }
    else
    {
        last->next = func;
        last = last->next;
    }
    *first_ptr = first;
    *last_ptr = last;
}

function void
F4_Language_PosContext_PushData_Call(Arena *arena,
                                     F4_Language_PosContextData **first_ptr,
                                     F4_Language_PosContextData **last_ptr,
                                     String string, int param_idx)
{
    F4_Language_PosContext_PushData(arena, first_ptr, last_ptr, F4_Index_LookupNote(string, 0), 0, param_idx);
}

function void
F4_Language_PosContext_PushData_Dot(Arena *arena,
                                    F4_Language_PosContextData **first_ptr,
                                    F4_Language_PosContextData **last_ptr,
                                    String string, Token *query)
{
 F4_Language_PosContext_PushData(arena, first_ptr, last_ptr, F4_Index_LookupNote(string, 0), query, 0);
}

function Token_List
F4_Language_LexFullInput_NoBreaks(App *app, F4_Language *language, Arena *arena, Stringz text)
{
 Token_List list = {};
 if(language != 0)
 {
  Scratch_Block scratch(app, arena);
  void *state = push_array0(scratch, u8, language->lex_state_size);
		language->LexInit(state, text);
  language->LexFullInput(arena, &list, state, max_u64);
 }
 return list;
}

// #include "4coder_fleury_lang_list.h"
