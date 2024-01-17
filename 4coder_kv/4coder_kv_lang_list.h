/* date = January 29th 2021 9:37 pm */

#ifndef FCODER_FLEURY_LANG_LIST_H
#define FCODER_FLEURY_LANG_LIST_H

// NOTE(rjf): Include language files here.
#include "4coder_fleury/4coder_fleury_lang_cpp.cpp"
#include "4coder_kv_lang_skm.cpp"

// NOTE(rjf): @f4_register_languages Register languages.
function void
F4_RegisterLanguages(void)
{
    // NOTE(rjf): C/C++
    {
        String_Const_u8 extensions[] =
        {
            S8Lit("cpp"), S8Lit("cc"), S8Lit("c"), S8Lit("cxx"),
            S8Lit("C"), S8Lit("h"), S8Lit("hpp"),
        };
        for(int i = 0; i < ArrayCount(extensions); i += 1)
        {
            F4_RegisterLanguage(extensions[i],
                                F4_CPP_IndexFile,
                                lex_full_input_cpp_init,
                                lex_full_input_cpp_breaks,
                                F4_CPP_PosContext,
                                F4_CPP_Highlight,
                                Lex_State_Cpp);
        }
    }
    
    {
        F4_RegisterLanguage(S8Lit("skm"),
                            F4_Skm_IndexFile,
                            lex_full_input_skm_init,
                            lex_full_input_skm_breaks,
                            F4_Skm_PosContext,
                            F4_Skm_Highlight,
                            Lex_State_Skm);
    }
}

#endif //FCODER_FLEURY_LANG_LIST_H
