#if !defined(FCODER_LEX_GEN_HAND_WRITTEN)
#define FCODER_LEX_GEN_HAND_WRITTEN

function u64
lexeme_hash(u64 seed, u8 *ptr, u64 size){
    u64 result = 0;
    for (u64 i = 0; i < size; i += 1, ptr += 1){
        result ^= ((*ptr) ^ result*59) + seed;
    }
    return(result);
}

function Lexeme_Table_Lookup
lexeme_table_lookup(u64 *hash_array, String *key_array, 
                    Lexeme_Table_Value *value_array, i1 slot_count, u64 seed,
                    u8 *ptr, u64 size){
    Lexeme_Table_Lookup result = {};
    u64 hash = lexeme_hash(seed, ptr, size);
    u64 comparison_hash = hash | 1;
    i1 first_index = (hash % slot_count);
    i1 index = first_index;
    for (;;){
        if (hash_array[index] == comparison_hash){
            if (string_match(SCu8(ptr, size), key_array[index])){
                result.found_match = true;
                result.base_kind = value_array[index].base_kind;
                result.sub_kind = value_array[index].sub_kind;
                break;
            }
        }
        else if (hash_array[index] == 0){
            break;
        }
        index += 1;
        if (index == slot_count){
            index = 0;
        }
        if (index == first_index){
            break;
        }
    }
    return(result);
}

#endif
u64 cpp_main_keys_hash_array[129] = {
0x0000000000000000,0xac4637e763d0b0af,0x71621f37c8161131,0x0000000000000000,
0x0000000000000000,0x0000000000000000,0xc2c3d67981d87907,0x0000000000000000,
0x0000000000000000,0x0000000000000000,0x0000000000000000,0x0000000000000000,
0x71621cacede20177,0x7aa8db20e9e01179,0x0000000000000000,0x551d0a641ee9ce7f,
0x0000000000000000,0x7aa8db20e904ccd9,0xac4637e752e067e9,0xea75796eecfd2787,
0x0000000000000000,0xac4637e77ac09079,0x7aa8db20e9f9e3bf,0xea7579653501746f,
0x0000000000000000,0x551d0a641ee98373,0x0000000000000000,0xc2feda0bacb92677,
0xab0d379fa0206e81,0xc2c3a0082c2119e5,0x71621ffe26def213,0x0000000000000000,
0x0000000000000000,0xc2feb1091097f34f,0xab0d379fa02063f3,0x0000000000000000,
0x7aa8db20e956d639,0xac4637e763de96e9,0xab0d379fa0206273,0x0000000000000000,
0x551d0a641ee92f5b,0xc2c3dda3ab694a97,0xdb85361e3c6535df,0xea75799e08c199e5,
0x0000000000000000,0x0000000000000000,0x0000000000000000,0x0000000000000000,
0x0000000000000000,0x0000000000000000,0x0000000000000000,0x7aa8db20e903530d,
0x7aa8db20e952f699,0x0000000000000000,0xea75796d7a720e27,0x0000000000000000,
0x0000000000000000,0x551d0a641ee9f3f3,0x0000000000000000,0xea75796689fd4c67,
0xc2fc74801efa6849,0x0000000000000000,0x0000000000000000,0xc2c3d43aee33b9d9,
0x0000000000000000,0x8e68a665a96e1c7d,0x0000000000000000,0x0000000000000000,
0x7aa8db20e900bcfd,0x0000000000000000,0xe0cfdba56dab93dd,0x0000000000000000,
0x71621fe6fef5bf2d,0xb7d90776cbff61e5,0x0000000000000000,0xea75796d059b211f,
0xc2c3b44cd8ceb5b3,0x0000000000000000,0x551d0a641eecd4bd,0xac4637e756e4eb39,
0x9618dd75c239c191,0x7aa8db20e9f9f195,0x0000000000000000,0x0000000000000000,
0x551d0a641eecda4f,0xc2fc464bc952491f,0xac4637e72dc67507,0xc2c31d0a78385ec5,
0x0000000000000000,0x0000000000000000,0xb7e38f7b75214395,0x71621f5e2ab934f7,
0x0000000000000000,0xea75796d05a6d061,0x71621cacede20d5f,0xac4637e72cdebfbf,
0x0000000000000000,0x0000000000000000,0x716211005a1bba0f,0xea75799e0613eb61,
0x0000000000000000,0xac4637e762c69d0b,0xea75799e09c8e5a3,0x0fafc20ee81b7bdd,
0x0000000000000000,0x0000000000000000,0x0000000000000000,0x0000000000000000,
0x24b690047aa23c7d,0x7aa8db20e956f4bd,0x0000000000000000,0x0000000000000000,
0xea757965973b2a59,0x0000000000000000,0xac4637e762152efd,0x0000000000000000,
0x0000000000000000,0xac4637e75642be8b,0xea75799e7f3b1cfd,0x0000000000000000,
0x0000000000000000,0x0000000000000000,0x7aa8db20e9de78b9,0x0000000000000000,
0xea75799e7a7d2cf5,0xea757963fe3b476f,0x551d0a641ee9bcc9,0xea75799e75a61f0f,
0x7aa8db20e903e6ff,
};
u8 cpp_main_keys_key_array_1[] = {0x63,0x6c,0x61,0x73,0x73,};
u8 cpp_main_keys_key_array_2[] = {0x76,0x69,0x72,0x74,0x75,0x61,0x6c,};
u8 cpp_main_keys_key_array_6[] = {0x76,0x6f,0x6c,0x61,0x74,0x69,0x6c,0x65,};
u8 cpp_main_keys_key_array_12[] = {0x61,0x6c,0x69,0x67,0x6e,0x6f,0x66,};
u8 cpp_main_keys_key_array_13[] = {0x76,0x6f,0x69,0x64,};
u8 cpp_main_keys_key_array_15[] = {0x69,0x6e,0x74,};
u8 cpp_main_keys_key_array_17[] = {0x61,0x75,0x74,0x6f,};
u8 cpp_main_keys_key_array_18[] = {0x66,0x6c,0x6f,0x61,0x74,};
u8 cpp_main_keys_key_array_19[] = {0x64,0x6f,0x75,0x62,0x6c,0x65,};
u8 cpp_main_keys_key_array_21[] = {0x66,0x61,0x6c,0x73,0x65,};
u8 cpp_main_keys_key_array_22[] = {0x74,0x68,0x69,0x73,};
u8 cpp_main_keys_key_array_23[] = {0x69,0x6e,0x6c,0x69,0x6e,0x65,};
u8 cpp_main_keys_key_array_25[] = {0x6e,0x65,0x77,};
u8 cpp_main_keys_key_array_27[] = {0x74,0x65,0x6d,0x70,0x6c,0x61,0x74,0x65,};
u8 cpp_main_keys_key_array_28[] = {0x64,0x6f,};
u8 cpp_main_keys_key_array_29[] = {0x75,0x6e,0x73,0x69,0x67,0x6e,0x65,0x64,};
u8 cpp_main_keys_key_array_30[] = {0x74,0x79,0x70,0x65,0x64,0x65,0x66,};
u8 cpp_main_keys_key_array_33[] = {0x72,0x65,0x67,0x69,0x73,0x74,0x65,0x72,};
u8 cpp_main_keys_key_array_34[] = {0x6f,0x72,};
u8 cpp_main_keys_key_array_36[] = {0x65,0x6c,0x73,0x65,};
u8 cpp_main_keys_key_array_37[] = {0x63,0x6f,0x6e,0x73,0x74,};
u8 cpp_main_keys_key_array_38[] = {0x69,0x66,};
u8 cpp_main_keys_key_array_40[] = {0x74,0x72,0x79,};
u8 cpp_main_keys_key_array_41[] = {0x6e,0x6f,0x65,0x78,0x63,0x65,0x70,0x74,};
u8 cpp_main_keys_key_array_42[] = {0x73,0x74,0x61,0x74,0x69,0x63,0x5f,0x61,0x73,0x73,0x65,0x72,0x74,};
u8 cpp_main_keys_key_array_43[] = {0x73,0x69,0x67,0x6e,0x65,0x64,};
u8 cpp_main_keys_key_array_51[] = {0x63,0x61,0x73,0x65,};
u8 cpp_main_keys_key_array_52[] = {0x67,0x6f,0x74,0x6f,};
u8 cpp_main_keys_key_array_54[] = {0x64,0x65,0x6c,0x65,0x74,0x65,};
u8 cpp_main_keys_key_array_57[] = {0x66,0x6f,0x72,};
u8 cpp_main_keys_key_array_59[] = {0x66,0x72,0x69,0x65,0x6e,0x64,};
u8 cpp_main_keys_key_array_60[] = {0x65,0x78,0x70,0x6c,0x69,0x63,0x69,0x74,};
u8 cpp_main_keys_key_array_63[] = {0x74,0x79,0x70,0x65,0x6e,0x61,0x6d,0x65,};
u8 cpp_main_keys_key_array_65[] = {0x72,0x65,0x69,0x6e,0x74,0x65,0x72,0x70,0x72,0x65,0x74,0x5f,0x63,0x61,0x73,0x74,};
u8 cpp_main_keys_key_array_68[] = {0x62,0x6f,0x6f,0x6c,};
u8 cpp_main_keys_key_array_70[] = {0x64,0x79,0x6e,0x61,0x6d,0x69,0x63,0x5f,0x63,0x61,0x73,0x74,};
u8 cpp_main_keys_key_array_72[] = {0x6e,0x75,0x6c,0x6c,0x70,0x74,0x72,};
u8 cpp_main_keys_key_array_73[] = {0x6e,0x61,0x6d,0x65,0x73,0x70,0x61,0x63,0x65,};
u8 cpp_main_keys_key_array_75[] = {0x65,0x78,0x70,0x6f,0x72,0x74,};
u8 cpp_main_keys_key_array_76[] = {0x6f,0x70,0x65,0x72,0x61,0x74,0x6f,0x72,};
u8 cpp_main_keys_key_array_78[] = {0x61,0x73,0x6d,};
u8 cpp_main_keys_key_array_79[] = {0x75,0x73,0x69,0x6e,0x67,};
u8 cpp_main_keys_key_array_80[] = {0x74,0x68,0x72,0x65,0x61,0x64,0x5f,0x6c,0x6f,0x63,0x61,0x6c,};
u8 cpp_main_keys_key_array_81[] = {0x74,0x72,0x75,0x65,};
u8 cpp_main_keys_key_array_84[] = {0x61,0x6e,0x64,};
u8 cpp_main_keys_key_array_85[] = {0x64,0x65,0x63,0x6c,0x74,0x79,0x70,0x65,};
u8 cpp_main_keys_key_array_86[] = {0x77,0x68,0x69,0x6c,0x65,};
u8 cpp_main_keys_key_array_87[] = {0x63,0x6f,0x6e,0x74,0x69,0x6e,0x75,0x65,};
u8 cpp_main_keys_key_array_90[] = {0x70,0x72,0x6f,0x74,0x65,0x63,0x74,0x65,0x64,};
u8 cpp_main_keys_key_array_91[] = {0x70,0x72,0x69,0x76,0x61,0x74,0x65,};
u8 cpp_main_keys_key_array_93[] = {0x65,0x78,0x74,0x65,0x72,0x6e,};
u8 cpp_main_keys_key_array_94[] = {0x61,0x6c,0x69,0x67,0x6e,0x61,0x73,};
u8 cpp_main_keys_key_array_95[] = {0x73,0x68,0x6f,0x72,0x74,};
u8 cpp_main_keys_key_array_98[] = {0x64,0x65,0x66,0x61,0x75,0x6c,0x74,};
u8 cpp_main_keys_key_array_99[] = {0x72,0x65,0x74,0x75,0x72,0x6e,};
u8 cpp_main_keys_key_array_101[] = {0x62,0x72,0x65,0x61,0x6b,};
u8 cpp_main_keys_key_array_102[] = {0x73,0x69,0x7a,0x65,0x6f,0x66,};
u8 cpp_main_keys_key_array_103[] = {0x73,0x74,0x61,0x74,0x69,0x63,0x5f,0x63,0x61,0x73,0x74,};
u8 cpp_main_keys_key_array_108[] = {0x63,0x6f,0x6e,0x73,0x74,0x5f,0x63,0x61,0x73,0x74,};
u8 cpp_main_keys_key_array_109[] = {0x65,0x6e,0x75,0x6d,};
u8 cpp_main_keys_key_array_112[] = {0x74,0x79,0x70,0x65,0x69,0x64,};
u8 cpp_main_keys_key_array_114[] = {0x63,0x61,0x74,0x63,0x68,};
u8 cpp_main_keys_key_array_117[] = {0x75,0x6e,0x69,0x6f,0x6e,};
u8 cpp_main_keys_key_array_118[] = {0x73,0x77,0x69,0x74,0x63,0x68,};
u8 cpp_main_keys_key_array_122[] = {0x6c,0x6f,0x6e,0x67,};
u8 cpp_main_keys_key_array_124[] = {0x73,0x74,0x72,0x75,0x63,0x74,};
u8 cpp_main_keys_key_array_125[] = {0x70,0x75,0x62,0x6c,0x69,0x63,};
u8 cpp_main_keys_key_array_126[] = {0x6e,0x6f,0x74,};
u8 cpp_main_keys_key_array_127[] = {0x73,0x74,0x61,0x74,0x69,0x63,};
u8 cpp_main_keys_key_array_128[] = {0x63,0x68,0x61,0x72,};
String cpp_main_keys_key_array[129] = {
{0, 0},
{cpp_main_keys_key_array_1, 5},
{cpp_main_keys_key_array_2, 7},
{0, 0},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_6, 8},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_12, 7},
{cpp_main_keys_key_array_13, 4},
{0, 0},
{cpp_main_keys_key_array_15, 3},
{0, 0},
{cpp_main_keys_key_array_17, 4},
{cpp_main_keys_key_array_18, 5},
{cpp_main_keys_key_array_19, 6},
{0, 0},
{cpp_main_keys_key_array_21, 5},
{cpp_main_keys_key_array_22, 4},
{cpp_main_keys_key_array_23, 6},
{0, 0},
{cpp_main_keys_key_array_25, 3},
{0, 0},
{cpp_main_keys_key_array_27, 8},
{cpp_main_keys_key_array_28, 2},
{cpp_main_keys_key_array_29, 8},
{cpp_main_keys_key_array_30, 7},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_33, 8},
{cpp_main_keys_key_array_34, 2},
{0, 0},
{cpp_main_keys_key_array_36, 4},
{cpp_main_keys_key_array_37, 5},
{cpp_main_keys_key_array_38, 2},
{0, 0},
{cpp_main_keys_key_array_40, 3},
{cpp_main_keys_key_array_41, 8},
{cpp_main_keys_key_array_42, 13},
{cpp_main_keys_key_array_43, 6},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_51, 4},
{cpp_main_keys_key_array_52, 4},
{0, 0},
{cpp_main_keys_key_array_54, 6},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_57, 3},
{0, 0},
{cpp_main_keys_key_array_59, 6},
{cpp_main_keys_key_array_60, 8},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_63, 8},
{0, 0},
{cpp_main_keys_key_array_65, 16},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_68, 4},
{0, 0},
{cpp_main_keys_key_array_70, 12},
{0, 0},
{cpp_main_keys_key_array_72, 7},
{cpp_main_keys_key_array_73, 9},
{0, 0},
{cpp_main_keys_key_array_75, 6},
{cpp_main_keys_key_array_76, 8},
{0, 0},
{cpp_main_keys_key_array_78, 3},
{cpp_main_keys_key_array_79, 5},
{cpp_main_keys_key_array_80, 12},
{cpp_main_keys_key_array_81, 4},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_84, 3},
{cpp_main_keys_key_array_85, 8},
{cpp_main_keys_key_array_86, 5},
{cpp_main_keys_key_array_87, 8},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_90, 9},
{cpp_main_keys_key_array_91, 7},
{0, 0},
{cpp_main_keys_key_array_93, 6},
{cpp_main_keys_key_array_94, 7},
{cpp_main_keys_key_array_95, 5},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_98, 7},
{cpp_main_keys_key_array_99, 6},
{0, 0},
{cpp_main_keys_key_array_101, 5},
{cpp_main_keys_key_array_102, 6},
{cpp_main_keys_key_array_103, 11},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_108, 10},
{cpp_main_keys_key_array_109, 4},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_112, 6},
{0, 0},
{cpp_main_keys_key_array_114, 5},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_117, 5},
{cpp_main_keys_key_array_118, 6},
{0, 0},
{0, 0},
{0, 0},
{cpp_main_keys_key_array_122, 4},
{0, 0},
{cpp_main_keys_key_array_124, 6},
{cpp_main_keys_key_array_125, 6},
{cpp_main_keys_key_array_126, 3},
{cpp_main_keys_key_array_127, 6},
{cpp_main_keys_key_array_128, 4},
};
Lexeme_Table_Value cpp_main_keys_value_array[129] = {
{0, 0},
{4, TokenCppKind_Class},
{4, TokenCppKind_Virtual},
{0, 0},
{0, 0},
{0, 0},
{4, TokenCppKind_Volatile},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{4, TokenCppKind_AlignOf},
{4, TokenCppKind_Void},
{0, 0},
{4, TokenCppKind_Int},
{0, 0},
{4, TokenCppKind_Auto},
{4, TokenCppKind_Float},
{4, TokenCppKind_Double},
{0, 0},
{8, TokenCppKind_LiteralFalse},
{4, TokenCppKind_This},
{4, TokenCppKind_Inline},
{0, 0},
{4, TokenCppKind_New},
{0, 0},
{4, TokenCppKind_Template},
{4, TokenCppKind_Do},
{4, TokenCppKind_Unsigned},
{4, TokenCppKind_Typedef},
{0, 0},
{0, 0},
{4, TokenCppKind_Register},
{4, TokenCppKind_OrAlt},
{0, 0},
{4, TokenCppKind_Else},
{4, TokenCppKind_Const},
{4, TokenCppKind_If},
{0, 0},
{4, TokenCppKind_Try},
{4, TokenCppKind_NoExcept},
{4, TokenCppKind_StaticAssert},
{4, TokenCppKind_Signed},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{4, TokenCppKind_Case},
{4, TokenCppKind_Goto},
{0, 0},
{4, TokenCppKind_Delete},
{0, 0},
{0, 0},
{4, TokenCppKind_For},
{0, 0},
{4, TokenCppKind_Friend},
{4, TokenCppKind_Explicit},
{0, 0},
{0, 0},
{4, TokenCppKind_Typename},
{0, 0},
{4, TokenCppKind_ReinterpretCast},
{0, 0},
{0, 0},
{4, TokenCppKind_Bool},
{0, 0},
{4, TokenCppKind_DynamicCast},
{0, 0},
{4, TokenCppKind_NullPtr},
{4, TokenCppKind_Namespace},
{0, 0},
{4, TokenCppKind_Export},
{4, TokenCppKind_Operator},
{0, 0},
{4, TokenCppKind_Asm},
{4, TokenCppKind_Using},
{4, TokenCppKind_ThreadLocal},
{8, TokenCppKind_LiteralTrue},
{0, 0},
{0, 0},
{4, TokenCppKind_AndAlt},
{4, TokenCppKind_DeclType},
{4, TokenCppKind_While},
{4, TokenCppKind_Continue},
{0, 0},
{0, 0},
{4, TokenCppKind_Protected},
{4, TokenCppKind_Private},
{0, 0},
{4, TokenCppKind_Extern},
{4, TokenCppKind_AlignAs},
{4, TokenCppKind_Short},
{0, 0},
{0, 0},
{4, TokenCppKind_Default},
{4, TokenCppKind_Return},
{0, 0},
{4, TokenCppKind_Break},
{4, TokenCppKind_SizeOf},
{4, TokenCppKind_StaticCast},
{0, 0},
{0, 0},
{0, 0},
{0, 0},
{4, TokenCppKind_ConstCast},
{4, TokenCppKind_Enum},
{0, 0},
{0, 0},
{4, TokenCppKind_TypeID},
{0, 0},
{4, TokenCppKind_Catch},
{0, 0},
{0, 0},
{4, TokenCppKind_Union},
{4, TokenCppKind_Switch},
{0, 0},
{0, 0},
{0, 0},
{4, TokenCppKind_Long},
{0, 0},
{4, TokenCppKind_Struct},
{4, TokenCppKind_Public},
{4, TokenCppKind_NotAlt},
{4, TokenCppKind_Static},
{4, TokenCppKind_Char},
};
i1 cpp_main_keys_slot_count = 129;
u64 cpp_main_keys_seed = 0x92046c31d752f99a;
u64 cpp_pp_directives_hash_array[25] = {
0x7e23397eb3143fbd,0x0000000000000000,0x7e233978bf03d9bd,0x7e23397eb3b7c3e7,
0x404ea26a9c1ac35d,0x0000000000000000,0x0000000000000000,0xbe2068a31f087a39,
0x404ea519d5c56415,0x0000000000000000,0xec08107cd126bfbd,0x404ea26a9a84dd6d,
0x7e23397e8789ae99,0x0000000000000000,0x0000000000000000,0x7e23397e801d6b85,
0x404ea2694e932333,0x0000000000000000,0x0000000000000000,0x0000000000000000,
0x4ed6c2b0fd4e46d5,0xec08107cd126bd87,0x0000000000000000,0xbe21c3e87c5f2625,
0xec08107cdbdace15,
};
u8 cpp_pp_directives_key_array_0[] = {0x75,0x6e,0x64,0x65,0x66,};
u8 cpp_pp_directives_key_array_2[] = {0x69,0x66,0x64,0x65,0x66,};
u8 cpp_pp_directives_key_array_3[] = {0x75,0x73,0x69,0x6e,0x67,};
u8 cpp_pp_directives_key_array_4[] = {0x69,0x66,0x6e,0x64,0x65,0x66,};
u8 cpp_pp_directives_key_array_7[] = {0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,};
u8 cpp_pp_directives_key_array_8[] = {0x64,0x65,0x66,0x69,0x6e,0x65,};
u8 cpp_pp_directives_key_array_10[] = {0x65,0x6c,0x69,0x66,};
u8 cpp_pp_directives_key_array_11[] = {0x69,0x6d,0x70,0x6f,0x72,0x74,};
u8 cpp_pp_directives_key_array_12[] = {0x65,0x72,0x72,0x6f,0x72,};
u8 cpp_pp_directives_key_array_15[] = {0x65,0x6e,0x64,0x69,0x66,};
u8 cpp_pp_directives_key_array_16[] = {0x70,0x72,0x61,0x67,0x6d,0x61,};
u8 cpp_pp_directives_key_array_20[] = {0x69,0x66,};
u8 cpp_pp_directives_key_array_21[] = {0x65,0x6c,0x73,0x65,};
u8 cpp_pp_directives_key_array_23[] = {0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,};
u8 cpp_pp_directives_key_array_24[] = {0x6c,0x69,0x6e,0x65,};
String cpp_pp_directives_key_array[25] = {
{cpp_pp_directives_key_array_0, 5},
{0, 0},
{cpp_pp_directives_key_array_2, 5},
{cpp_pp_directives_key_array_3, 5},
{cpp_pp_directives_key_array_4, 6},
{0, 0},
{0, 0},
{cpp_pp_directives_key_array_7, 7},
{cpp_pp_directives_key_array_8, 6},
{0, 0},
{cpp_pp_directives_key_array_10, 4},
{cpp_pp_directives_key_array_11, 6},
{cpp_pp_directives_key_array_12, 5},
{0, 0},
{0, 0},
{cpp_pp_directives_key_array_15, 5},
{cpp_pp_directives_key_array_16, 6},
{0, 0},
{0, 0},
{0, 0},
{cpp_pp_directives_key_array_20, 2},
{cpp_pp_directives_key_array_21, 4},
{0, 0},
{cpp_pp_directives_key_array_23, 7},
{cpp_pp_directives_key_array_24, 4},
};
Lexeme_Table_Value cpp_pp_directives_value_array[25] = {
{5, TokenCppKind_PPUndef},
{0, 0},
{5, TokenCppKind_PPIfDef},
{5, TokenCppKind_PPUsing},
{5, TokenCppKind_PPIfNDef},
{0, 0},
{0, 0},
{5, TokenCppKind_PPInclude},
{5, TokenCppKind_PPDefine},
{0, 0},
{5, TokenCppKind_PPElIf},
{5, TokenCppKind_PPImport},
{5, TokenCppKind_PPError},
{0, 0},
{0, 0},
{5, TokenCppKind_PPEndIf},
{5, TokenCppKind_PPPragma},
{0, 0},
{0, 0},
{0, 0},
{5, TokenCppKind_PPIf},
{5, TokenCppKind_PPElse},
{0, 0},
{5, TokenCppKind_PPVersion},
{5, TokenCppKind_PPLine},
};
i1 cpp_pp_directives_slot_count = 25;
u64 cpp_pp_directives_seed = 0x2c4f5c3e1d55ca78;
u64 cpp_pp_keys_hash_array[2] = {
0x9fc9771d807fedff,0x0000000000000000,
};
u8 cpp_pp_keys_key_array_0[] = {0x64,0x65,0x66,0x69,0x6e,0x65,0x64,};
String cpp_pp_keys_key_array[2] = {
{cpp_pp_keys_key_array_0, 7},
{0, 0},
};
Lexeme_Table_Value cpp_pp_keys_value_array[2] = {
{4, TokenCppKind_PPDefined},
{0, 0},
};
i1 cpp_pp_keys_slot_count = 2;
u64 cpp_pp_keys_seed = 0x2de435961dbf29b8;
struct Lex_State_Cpp{
u32 flags_ZF0;
u32 flags_KF0;
u16 flags_KB0;
u8 *base;
u8 *delim_first;
u8 *delim_one_past_last;
u8 *emit_ptr;
u8 *ptr;
u8 *opl_ptr;
};
function void
lex_full_input_cpp_init(Lex_State_Cpp *state_ptr, String input){
state_ptr->flags_ZF0 = 0;
state_ptr->flags_KF0 = 0;
state_ptr->flags_KB0 = 0;
state_ptr->base = input.str;
state_ptr->delim_first = input.str;
state_ptr->delim_one_past_last = input.str;
state_ptr->emit_ptr = input.str;
state_ptr->ptr = input.str;
state_ptr->opl_ptr = input.str + input.size;
}
function b32
lex_full_input_cpp_breaks(Arena *arena, Token_List *list, Lex_State_Cpp *state_ptr, u64 max){
b32 result = false;
u64 emit_counter = 0;
Lex_State_Cpp state;
block_copy_struct(&state, state_ptr);
{
state_label_1: // root
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_EOF;
token.kind = 0;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
result = true;
goto end;
}
}
switch (*state.ptr){
case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:
case 0x07:case 0x08:case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:
case 0x13:case 0x14:case 0x15:case 0x16:case 0x17:case 0x18:case 0x19:
case 0x1a:case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f:case 0x40:
case 0x60:case 0x7f:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x09:case 0x0b:case 0x0c:case 0x0d:case 0x20:
{
if ((HasFlag(state.flags_KF0, 0x2))){
state.ptr += 1;
goto state_label_4; // error_body
}
state.ptr += 1;
goto state_label_3; // whitespace
}break;
case 0x0a:
{
state.ptr += 1;
state.flags_KB0 &= ~(0x1);
state.flags_KF0 &= ~(0x1);
state.flags_KF0 &= ~(0x2);
goto state_label_3; // whitespace
}break;
case 0x21:
{
state.ptr += 1;
goto state_label_61; // op stage
}break;
case 0x22:
{
if ((HasFlag(state.flags_KF0, 0x1))){
state.ptr += 1;
goto state_label_26; // include_quotes
}
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x23:
{
if ((!HasFlag(state.flags_KB0, 0x1))){
state.ptr += 1;
goto state_label_23; // pp_directive_whitespace
}
state.ptr += 1;
goto state_label_68; // op stage
}break;
default:
{
state.ptr += 1;
goto state_label_2; // identifier
}break;
case 0x25:
{
state.ptr += 1;
goto state_label_65; // op stage
}break;
case 0x26:
{
state.ptr += 1;
goto state_label_62; // op stage
}break;
case 0x27:
{
state.ptr += 1;
state.flags_ZF0 |= 0x40;
goto state_label_32; // string
}break;
case 0x28:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_ParenOp;
token.kind = 13;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x29:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_ParenCl;
token.kind = 14;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2a:
{
state.ptr += 1;
goto state_label_64; // op stage
}break;
case 0x2b:
{
state.ptr += 1;
goto state_label_54; // op stage
}break;
case 0x2c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Comma;
token.kind = 15;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2d:
{
state.ptr += 1;
goto state_label_55; // op stage
}break;
case 0x2e:
{
state.ptr += 1;
goto state_label_6; // operator_or_fnumber_dot
}break;
case 0x2f:
{
state.ptr += 1;
goto state_label_7; // operator_or_comment_slash
}break;
case 0x30:
{
state.ptr += 1;
goto state_label_9; // znumber
}break;
case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:case 0x37:
case 0x38:case 0x39:
{
state.ptr += 1;
goto state_label_8; // number
}break;
case 0x3a:
{
state.ptr += 1;
goto state_label_53; // op stage
}break;
case 0x3b:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Semicolon;
token.kind = 15;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3c:
{
if ((!HasFlag(state.flags_KF0, 0x1))){
state.ptr += 1;
goto state_label_57; // op stage
}
state.ptr += 1;
goto state_label_25; // include_pointy
}break;
case 0x3d:
{
state.ptr += 1;
goto state_label_60; // op stage
}break;
case 0x3e:
{
state.ptr += 1;
goto state_label_58; // op stage
}break;
case 0x3f:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Ternary;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:
{
state.ptr += 1;
state.flags_ZF0 |= 0x4;
goto state_label_27; // pre_L
}break;
case 0x52:
{
state.ptr += 1;
goto state_label_31; // pre_R
}break;
case 0x55:
{
state.ptr += 1;
state.flags_ZF0 |= 0x20;
goto state_label_29; // pre_U
}break;
case 0x5b:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_BrackOp;
token.kind = 13;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x5c:
{
state.ptr += 1;
goto state_label_5; // backslash
}break;
case 0x5d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_BrackCl;
token.kind = 14;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x5e:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Xor;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x75:
{
state.ptr += 1;
state.flags_ZF0 |= 0x10;
goto state_label_28; // pre_u
}break;
case 0x7b:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_BraceOp;
token.kind = 11;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x7c:
{
state.ptr += 1;
goto state_label_63; // op stage
}break;
case 0x7d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_BraceCl;
token.kind = 12;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x7e:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Tilde;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_2: // identifier
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_KB0, 0x1)){
Lexeme_Table_Lookup lookup = lexeme_table_lookup(cpp_pp_keys_hash_array, cpp_pp_keys_key_array, cpp_pp_keys_value_array, cpp_pp_keys_slot_count, cpp_pp_keys_seed, state.emit_ptr, token.size);
if (lookup.found_match){
token.kind = lookup.base_kind;
token.sub_kind = lookup.sub_kind;
break;
}
}
Lexeme_Table_Lookup lookup = lexeme_table_lookup(cpp_main_keys_hash_array, cpp_main_keys_key_array, cpp_main_keys_value_array, cpp_main_keys_slot_count, cpp_main_keys_seed, state.emit_ptr, token.size);
if (lookup.found_match){
token.kind = lookup.base_kind;
token.sub_kind = lookup.sub_kind;
break;
}
token.sub_kind = TokenCppKind_Identifier;
token.kind = 6;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:
case 0x07:case 0x08:case 0x09:case 0x0a:case 0x0b:case 0x0c:case 0x0d:
case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:case 0x13:case 0x14:
case 0x15:case 0x16:case 0x17:case 0x18:case 0x19:case 0x1a:case 0x1b:
case 0x1c:case 0x1d:case 0x1e:case 0x1f:case 0x20:case 0x21:case 0x22:
case 0x23:case 0x25:case 0x26:case 0x27:case 0x28:case 0x29:case 0x2a:
case 0x2b:case 0x2c:case 0x2d:case 0x2e:case 0x2f:case 0x3a:case 0x3b:
case 0x3c:case 0x3d:case 0x3e:case 0x3f:case 0x40:case 0x5b:case 0x5c:
case 0x5d:case 0x5e:case 0x60:case 0x7b:case 0x7c:case 0x7d:case 0x7e:
case 0x7f:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_KB0, 0x1)){
Lexeme_Table_Lookup lookup = lexeme_table_lookup(cpp_pp_keys_hash_array, cpp_pp_keys_key_array, cpp_pp_keys_value_array, cpp_pp_keys_slot_count, cpp_pp_keys_seed, state.emit_ptr, token.size);
if (lookup.found_match){
token.kind = lookup.base_kind;
token.sub_kind = lookup.sub_kind;
break;
}
}
Lexeme_Table_Lookup lookup = lexeme_table_lookup(cpp_main_keys_hash_array, cpp_main_keys_key_array, cpp_main_keys_value_array, cpp_main_keys_slot_count, cpp_main_keys_seed, state.emit_ptr, token.size);
if (lookup.found_match){
token.kind = lookup.base_kind;
token.sub_kind = lookup.sub_kind;
break;
}
token.sub_kind = TokenCppKind_Identifier;
token.kind = 6;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
default:
{
state.ptr += 1;
goto state_label_2; // identifier
}break;
}
}
{
state_label_3: // whitespace
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Whitespace;
token.kind = 1;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Whitespace;
token.kind = 1;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x09:case 0x0b:case 0x0c:case 0x0d:case 0x20:
{
state.ptr += 1;
goto state_label_3; // whitespace
}break;
case 0x0a:
{
state.ptr += 1;
state.flags_KB0 &= ~(0x1);
state.flags_KF0 &= ~(0x1);
state.flags_KF0 &= ~(0x2);
goto state_label_3; // whitespace
}break;
}
}
{
state_label_4: // error_body
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PPErrorMessage;
token.kind = 10;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_4; // error_body
}break;
case 0x0a:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PPErrorMessage;
token.kind = 10;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_5: // backslash
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Backslash;
token.kind = 1;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Backslash;
token.kind = 1;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x0a:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Backslash;
token.kind = 1;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x0d:
{
state.ptr += 1;
goto state_label_5; // backslash
}break;
}
}
{
state_label_6: // operator_or_fnumber_dot
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Dot;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Dot;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2a:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_DotStar;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2e:
{
state.ptr += 1;
goto state_label_69; // op stage
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:
{
state.ptr += 1;
goto state_label_10; // fnumber_decimal
}break;
}
}
{
state_label_7: // operator_or_comment_slash
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Div;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Div;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2a:
{
state.ptr += 1;
goto state_label_49; // comment_block
}break;
case 0x2f:
{
state.ptr += 1;
goto state_label_51; // comment_line
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_DivEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_8: // number
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralInteger;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralInteger;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2e:
{
state.ptr += 1;
goto state_label_10; // fnumber_decimal
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:
{
state.ptr += 1;
goto state_label_8; // number
}break;
case 0x45:case 0x65:
{
state.ptr += 1;
goto state_label_11; // fnumber_exponent
}break;
case 0x4c:
{
state.ptr += 1;
goto state_label_18; // L_number
}break;
case 0x55:case 0x75:
{
state.ptr += 1;
goto state_label_17; // U_number
}break;
case 0x6c:
{
state.ptr += 1;
goto state_label_20; // l_number
}break;
}
}
{
state_label_9: // znumber
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralInteger;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralInteger;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2e:
{
state.ptr += 1;
goto state_label_10; // fnumber_decimal
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:
{
state.ptr += 1;
state.flags_ZF0 |= 0x2;
goto state_label_16; // number_oct
}break;
case 0x45:case 0x65:
{
state.ptr += 1;
goto state_label_11; // fnumber_exponent
}break;
case 0x4c:
{
state.ptr += 1;
goto state_label_18; // L_number
}break;
case 0x55:case 0x75:
{
state.ptr += 1;
goto state_label_17; // U_number
}break;
case 0x58:case 0x78:
{
state.ptr += 1;
state.flags_ZF0 |= 0x1;
goto state_label_14; // number_hex_first
}break;
case 0x6c:
{
state.ptr += 1;
goto state_label_20; // l_number
}break;
}
}
{
state_label_10: // fnumber_decimal
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:
{
state.ptr += 1;
goto state_label_10; // fnumber_decimal
}break;
case 0x45:case 0x65:
{
state.ptr += 1;
goto state_label_11; // fnumber_exponent
}break;
case 0x46:case 0x66:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat32;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:case 0x6c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_11: // fnumber_exponent
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2b:case 0x2d:
{
state.ptr += 1;
goto state_label_12; // fnumber_exponent_sign
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:
{
state.ptr += 1;
goto state_label_13; // fnumber_exponent_digits
}break;
case 0x46:case 0x66:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat32;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:case 0x6c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_12: // fnumber_exponent_sign
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:
{
state.ptr += 1;
goto state_label_13; // fnumber_exponent_digits
}break;
case 0x46:case 0x66:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat32;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:case 0x6c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_13: // fnumber_exponent_digits
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:
{
state.ptr += 1;
goto state_label_13; // fnumber_exponent_digits
}break;
case 0x46:case 0x66:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat32;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:case 0x6c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralFloat64;
token.kind = 9;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_14: // number_hex_first
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_15; // number_hex
}break;
}
}
{
state_label_15: // number_hex
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralIntegerHex;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralIntegerHex;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_15; // number_hex
}break;
case 0x4c:
{
state.ptr += 1;
goto state_label_18; // L_number
}break;
case 0x55:case 0x75:
{
state.ptr += 1;
goto state_label_17; // U_number
}break;
case 0x6c:
{
state.ptr += 1;
goto state_label_20; // l_number
}break;
}
}
{
state_label_16: // number_oct
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralIntegerOct;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LiteralIntegerOct;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:
{
state.ptr += 1;
state.flags_ZF0 |= 0x2;
goto state_label_16; // number_oct
}break;
case 0x4c:
{
state.ptr += 1;
goto state_label_18; // L_number
}break;
case 0x55:case 0x75:
{
state.ptr += 1;
goto state_label_17; // U_number
}break;
case 0x6c:
{
state.ptr += 1;
goto state_label_20; // l_number
}break;
}
}
{
state_label_17: // U_number
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexU;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctU;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerU;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexU;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctU;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerU;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:
{
state.ptr += 1;
goto state_label_19; // UL_number
}break;
case 0x6c:
{
state.ptr += 1;
goto state_label_21; // Ul_number
}break;
}
}
{
state_label_18: // L_number
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:
{
state.ptr += 1;
goto state_label_22; // LL_number
}break;
case 0x55:case 0x75:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexUL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctUL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerUL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_19: // UL_number
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexUL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctUL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerUL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexUL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctUL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerUL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x4c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexULL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctULL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerULL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_20: // l_number
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x55:case 0x75:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexUL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctUL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerUL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x6c:
{
state.ptr += 1;
goto state_label_22; // LL_number
}break;
}
}
{
state_label_21: // Ul_number
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexUL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctUL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerUL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexUL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctUL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerUL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x6c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexULL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctULL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerULL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_22: // LL_number
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexLL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctLL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerLL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexLL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctLL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerLL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x55:case 0x75:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x1)){
token.sub_kind = TokenCppKind_LiteralIntegerHexULL;
token.kind = 8;
break;
}
if (HasFlag(state.flags_ZF0, 0x2)){
token.sub_kind = TokenCppKind_LiteralIntegerOctULL;
token.kind = 8;
break;
}
token.sub_kind = TokenCppKind_LiteralIntegerULL;
token.kind = 8;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_23: // pp_directive_whitespace
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x09:case 0x0b:case 0x0c:case 0x20:
{
state.ptr += 1;
goto state_label_23; // pp_directive_whitespace
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x47:case 0x48:case 0x49:case 0x4a:case 0x4b:
case 0x4c:case 0x4d:case 0x4e:case 0x4f:case 0x50:case 0x51:case 0x52:
case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:case 0x58:case 0x59:
case 0x5a:case 0x5f:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:case 0x67:case 0x68:case 0x69:case 0x6a:case 0x6b:case 0x6c:
case 0x6d:case 0x6e:case 0x6f:case 0x70:case 0x71:case 0x72:case 0x73:
case 0x74:case 0x75:case 0x76:case 0x77:case 0x78:case 0x79:case 0x7a:
{
state.delim_first = state.ptr;
state.flags_KB0 |= 0x1;
state.ptr += 1;
goto state_label_24; // pp_directive
}break;
}
}
{
state_label_24: // pp_directive
if (state.ptr == state.opl_ptr){
if ((true)){
state.delim_one_past_last = state.ptr;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
Lexeme_Table_Lookup lookup = lexeme_table_lookup(cpp_pp_directives_hash_array, cpp_pp_directives_key_array, cpp_pp_directives_value_array, cpp_pp_directives_slot_count, cpp_pp_directives_seed, state.delim_first, (state.delim_one_past_last - state.delim_first));
if (lookup.found_match){
token.kind = lookup.base_kind;
token.sub_kind = lookup.sub_kind;
break;
}
token.sub_kind = TokenCppKind_PPUnknown;
token.kind = 2;
}while(0);
switch (token.sub_kind){
case TokenCppKind_PPInclude:
{
state.flags_KF0 |= 0x1;
}break;
case TokenCppKind_PPError:
{
state.flags_KF0 |= 0x2;
}break;
}
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
state.delim_one_past_last = state.ptr;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
Lexeme_Table_Lookup lookup = lexeme_table_lookup(cpp_pp_directives_hash_array, cpp_pp_directives_key_array, cpp_pp_directives_value_array, cpp_pp_directives_slot_count, cpp_pp_directives_seed, state.delim_first, (state.delim_one_past_last - state.delim_first));
if (lookup.found_match){
token.kind = lookup.base_kind;
token.sub_kind = lookup.sub_kind;
break;
}
token.sub_kind = TokenCppKind_PPUnknown;
token.kind = 2;
}while(0);
switch (token.sub_kind){
case TokenCppKind_PPInclude:
{
state.flags_KF0 |= 0x1;
}break;
case TokenCppKind_PPError:
{
state.flags_KF0 |= 0x2;
}break;
}
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x47:case 0x48:case 0x49:case 0x4a:case 0x4b:
case 0x4c:case 0x4d:case 0x4e:case 0x4f:case 0x50:case 0x51:case 0x52:
case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:case 0x58:case 0x59:
case 0x5a:case 0x5f:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:case 0x67:case 0x68:case 0x69:case 0x6a:case 0x6b:case 0x6c:
case 0x6d:case 0x6e:case 0x6f:case 0x70:case 0x71:case 0x72:case 0x73:
case 0x74:case 0x75:case 0x76:case 0x77:case 0x78:case 0x79:case 0x7a:
{
state.ptr += 1;
goto state_label_24; // pp_directive
}break;
}
}
{
state_label_25: // include_pointy
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x20:case 0x21:case 0x22:case 0x23:case 0x24:case 0x25:case 0x26:
case 0x27:case 0x28:case 0x29:case 0x2a:case 0x2b:case 0x2c:case 0x2d:
case 0x2e:case 0x2f:case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:
case 0x35:case 0x36:case 0x37:case 0x38:case 0x39:case 0x3a:case 0x3b:
case 0x3d:case 0x3f:case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x47:case 0x48:case 0x49:case 0x4a:case 0x4b:
case 0x4c:case 0x4d:case 0x4e:case 0x4f:case 0x50:case 0x51:case 0x52:
case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:case 0x58:case 0x59:
case 0x5a:case 0x5b:case 0x5c:case 0x5d:case 0x5e:case 0x5f:case 0x61:
case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:case 0x68:
case 0x69:case 0x6a:case 0x6b:case 0x6c:case 0x6d:case 0x6e:case 0x6f:
case 0x70:case 0x71:case 0x72:case 0x73:case 0x74:case 0x75:case 0x76:
case 0x77:case 0x78:case 0x79:case 0x7a:case 0x7b:case 0x7c:case 0x7d:
{
state.ptr += 1;
goto state_label_25; // include_pointy
}break;
case 0x3e:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PPIncludeFile;
token.kind = 10;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_26: // include_quotes
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x20:case 0x21:case 0x23:case 0x24:case 0x25:case 0x26:case 0x27:
case 0x28:case 0x29:case 0x2a:case 0x2b:case 0x2c:case 0x2d:case 0x2e:
case 0x2f:case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:
case 0x36:case 0x37:case 0x38:case 0x39:case 0x3a:case 0x3b:case 0x3c:
case 0x3d:case 0x3e:case 0x3f:case 0x40:case 0x41:case 0x42:case 0x43:
case 0x44:case 0x45:case 0x46:case 0x47:case 0x48:case 0x49:case 0x4a:
case 0x4b:case 0x4c:case 0x4d:case 0x4e:case 0x4f:case 0x50:case 0x51:
case 0x52:case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:case 0x58:
case 0x59:case 0x5a:case 0x5b:case 0x5c:case 0x5d:case 0x5e:case 0x5f:
case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:
case 0x68:case 0x69:case 0x6a:case 0x6b:case 0x6c:case 0x6d:case 0x6e:
case 0x6f:case 0x70:case 0x71:case 0x72:case 0x73:case 0x74:case 0x75:
case 0x76:case 0x77:case 0x78:case 0x79:case 0x7a:case 0x7b:case 0x7c:
case 0x7d:
{
state.ptr += 1;
goto state_label_26; // include_quotes
}break;
case 0x22:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PPIncludeFile;
token.kind = 10;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_27: // pre_L
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_2; // identifier
}
}
switch (*state.ptr){
default:
{
goto state_label_2; // identifier
}break;
case 0x22:
{
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x52:
{
state.ptr += 1;
goto state_label_31; // pre_R
}break;
}
}
{
state_label_28: // pre_u
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_2; // identifier
}
}
switch (*state.ptr){
default:
{
goto state_label_2; // identifier
}break;
case 0x22:
{
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x38:
{
state.ptr += 1;
state.flags_ZF0 |= 0x8;
goto state_label_30; // pre_u8
}break;
case 0x52:
{
state.ptr += 1;
goto state_label_31; // pre_R
}break;
}
}
{
state_label_29: // pre_U
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_2; // identifier
}
}
switch (*state.ptr){
default:
{
goto state_label_2; // identifier
}break;
case 0x22:
{
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x52:
{
state.ptr += 1;
goto state_label_31; // pre_R
}break;
}
}
{
state_label_30: // pre_u8
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_2; // identifier
}
}
switch (*state.ptr){
default:
{
goto state_label_2; // identifier
}break;
case 0x22:
{
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x52:
{
state.ptr += 1;
goto state_label_31; // pre_R
}break;
}
}
{
state_label_31: // pre_R
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_2; // identifier
}
}
switch (*state.ptr){
default:
{
goto state_label_2; // identifier
}break;
case 0x22:
{
state.ptr += 1;
state.delim_first = state.ptr;
goto state_label_45; // raw_string_get_delim
}break;
}
}
{
state_label_32: // string
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x0a:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x22:
{
if ((!HasFlag(state.flags_ZF0, 0x40))){
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x4)){
token.sub_kind = TokenCppKind_LiteralStringWide;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x8)){
token.sub_kind = TokenCppKind_LiteralStringUTF8;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x10)){
token.sub_kind = TokenCppKind_LiteralStringUTF16;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x20)){
token.sub_kind = TokenCppKind_LiteralStringUTF32;
token.kind = 10;
break;
}
token.sub_kind = TokenCppKind_LiteralString;
token.kind = 10;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x27:
{
if ((HasFlag(state.flags_ZF0, 0x40))){
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x4)){
token.sub_kind = TokenCppKind_LiteralCharacterWide;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x8)){
token.sub_kind = TokenCppKind_LiteralCharacterUTF8;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x10)){
token.sub_kind = TokenCppKind_LiteralCharacterUTF16;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x20)){
token.sub_kind = TokenCppKind_LiteralCharacterUTF32;
token.kind = 10;
break;
}
token.sub_kind = TokenCppKind_LiteralCharacter;
token.kind = 10;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x5c:
{
state.ptr += 1;
goto state_label_33; // string_esc
}break;
}
}
{
state_label_33: // string_esc
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_EOF;
token.kind = 0;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
result = true;
goto end;
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:
{
state.ptr += 1;
goto state_label_34; // string_esc_oct2
}break;
case 0x55:
{
state.ptr += 1;
goto state_label_37; // string_esc_universal_8
}break;
case 0x75:
{
state.ptr += 1;
goto state_label_41; // string_esc_universal_4
}break;
case 0x78:
{
state.ptr += 1;
goto state_label_36; // string_esc_hex
}break;
}
}
{
state_label_34: // string_esc_oct2
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:
{
state.ptr += 1;
goto state_label_35; // string_esc_oct1
}break;
}
}
{
state_label_35: // string_esc_oct1
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:
{
state.ptr += 1;
goto state_label_32; // string
}break;
}
}
{
state_label_36: // string_esc_hex
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_36; // string_esc_hex
}break;
}
}
{
state_label_37: // string_esc_universal_8
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_38; // string_esc_universal_7
}break;
}
}
{
state_label_38: // string_esc_universal_7
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_39; // string_esc_universal_6
}break;
}
}
{
state_label_39: // string_esc_universal_6
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_40; // string_esc_universal_5
}break;
}
}
{
state_label_40: // string_esc_universal_5
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_41; // string_esc_universal_4
}break;
}
}
{
state_label_41: // string_esc_universal_4
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_42; // string_esc_universal_3
}break;
}
}
{
state_label_42: // string_esc_universal_3
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_43; // string_esc_universal_2
}break;
}
}
{
state_label_43: // string_esc_universal_2
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_44; // string_esc_universal_1
}break;
}
}
{
state_label_44: // string_esc_universal_1
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_32; // string
}
}
switch (*state.ptr){
default:
{
goto state_label_32; // string
}break;
case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:case 0x36:
case 0x37:case 0x38:case 0x39:case 0x41:case 0x42:case 0x43:case 0x44:
case 0x45:case 0x46:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:
{
state.ptr += 1;
goto state_label_32; // string
}break;
}
}
{
state_label_45: // raw_string_get_delim
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_EOF;
token.kind = 0;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
result = true;
goto end;
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_45; // raw_string_get_delim
}break;
case 0x20:case 0x29:case 0x5c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x28:
{
state.delim_one_past_last = state.ptr;
state.ptr += 1;
goto state_label_46; // raw_string_find_close
}break;
}
}
{
state_label_46: // raw_string_find_close
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_EOF;
token.kind = 0;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
result = true;
goto end;
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_46; // raw_string_find_close
}break;
case 0x29:
{
state.ptr += 1;
goto state_label_47; // raw_string_try_delim
}break;
}
}
{
state_label_47: // raw_string_try_delim
u64 delim_length = state.delim_one_past_last - state.delim_first;
u64 parse_length = 0;
for (;;){
if (parse_length == delim_length){
goto state_label_48; // raw_string_try_quote
}
if (state.ptr == state.opl_ptr){
goto state_label_48; // raw_string_try_quote
}
if (*state.ptr == state.delim_first[parse_length]){
state.ptr += 1;
parse_length += 1;
}
else{
goto state_label_46; // raw_string_find_close
}
}
}
{
state_label_48: // raw_string_try_quote
if (state.ptr == state.opl_ptr){
if ((true)){
goto state_label_46; // raw_string_find_close
}
}
switch (*state.ptr){
default:
{
goto state_label_46; // raw_string_find_close
}break;
case 0x22:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
if (HasFlag(state.flags_ZF0, 0x4)){
token.sub_kind = TokenCppKind_LiteralStringWideRaw;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x8)){
token.sub_kind = TokenCppKind_LiteralStringUTF8Raw;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x10)){
token.sub_kind = TokenCppKind_LiteralStringUTF16Raw;
token.kind = 10;
break;
}
if (HasFlag(state.flags_ZF0, 0x20)){
token.sub_kind = TokenCppKind_LiteralStringUTF32Raw;
token.kind = 10;
break;
}
token.sub_kind = TokenCppKind_LiteralStringRaw;
token.kind = 10;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_49: // comment_block
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_BlockComment;
token.kind = 3;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_EOF;
token.kind = 0;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
result = true;
goto end;
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_49; // comment_block
}break;
case 0x0a:
{
state.ptr += 1;
state.flags_KB0 &= ~(0x1);
state.flags_KF0 &= ~(0x1);
goto state_label_49; // comment_block
}break;
case 0x2a:
{
state.ptr += 1;
goto state_label_50; // comment_block_try_close
}break;
}
}
{
state_label_50: // comment_block_try_close
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_BlockComment;
token.kind = 3;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_EOF;
token.kind = 0;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
result = true;
goto end;
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_49; // comment_block
}break;
case 0x2a:
{
state.ptr += 1;
goto state_label_50; // comment_block_try_close
}break;
case 0x2f:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_BlockComment;
token.kind = 3;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_51: // comment_line
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LineComment;
token.kind = 3;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_51; // comment_line
}break;
case 0x0a:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LineComment;
token.kind = 3;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x5c:
{
state.ptr += 1;
goto state_label_52; // comment_line_backslashing
}break;
}
}
{
state_label_52: // comment_line_backslashing
if (state.ptr == state.opl_ptr){
if ((true)){
result = true;
goto end;
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
goto state_label_51; // comment_line
}break;
case 0x0d:
{
state.ptr += 1;
goto state_label_52; // comment_line_backslashing
}break;
}
}
{
state_label_53: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Colon;
token.kind = 15;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Colon;
token.kind = 15;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3a:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_ColonColon;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_54: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Plus;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Plus;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2b:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PlusPlus;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PlusEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_55: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Minus;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Minus;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_MinusMinus;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_MinusEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3e:
{
state.ptr += 1;
goto state_label_56; // op stage
}break;
}
}
{
state_label_56: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Arrow;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Arrow;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2a:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_ArrowStar;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_57: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Less;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Less;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3c:
{
state.ptr += 1;
goto state_label_66; // op stage
}break;
case 0x3d:
{
state.ptr += 1;
goto state_label_59; // op stage
}break;
}
}
{
state_label_58: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Grtr;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Grtr;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_GrtrEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3e:
{
state.ptr += 1;
goto state_label_67; // op stage
}break;
}
}
{
state_label_59: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LessEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LessEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3e:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Compare;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_60: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Eq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Eq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_EqEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_61: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Not;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Not;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_NotEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_62: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_And;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_And;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x26:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_AndAnd;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_AndEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_63: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Or;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Or;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_OrEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x7c:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_OrOr;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_64: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Star;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Star;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_StarEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_65: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Mod;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_Mod;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_ModEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_66: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LeftLeft;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LeftLeft;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LeftLeftEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_67: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_RightRight;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_RightRight;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x3d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_RightRightEq;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_68: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PPStringify;
token.kind = 15;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PPStringify;
token.kind = 15;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x23:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_PPConcat;
token.kind = 15;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
{
state_label_69: // op stage
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}
}
switch (*state.ptr){
default:
{
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
case 0x2e:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
token.flags = state.flags_KB0;
do{
token.sub_kind = TokenCppKind_DotDotDot;
token.kind = 7;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
state.flags_ZF0 = 0;
goto state_label_1; // root
}break;
}
}
end:;
block_copy_struct(state_ptr, &state);
return(result);
}
function Token_List
lex_full_input_cpp(Arena *arena, String input){
Lex_State_Cpp state = {};
lex_full_input_cpp_init(&state, input);
Token_List list = {};
lex_full_input_cpp_breaks(arena, &list, &state, max_u64);
return(list);
}
