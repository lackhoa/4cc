#if !defined(FCODER_LEX_GEN_HAND_WRITTEN_TYPES)
#define FCODER_LEX_GEN_HAND_WRITTEN_TYPES

struct Lexeme_Table_Value{
    Token_Base_Kind base_kind;
    u16 sub_kind;
};

struct Lexeme_Table_Lookup{
    b32 found_match;
    Token_Base_Kind base_kind;
    u16 sub_kind;
};

#endif
typedef u16 Token_Skm_Kind;
enum{
TokenSkmKind_EOF = 0,
TokenSkmKind_Whitespace = 1,
TokenSkmKind_LexError = 2,
TokenSkmKind_Text = 3,
TokenSkmKind_ParenOp = 4,
TokenSkmKind_BrackOp = 5,
TokenSkmKind_BraceOp = 6,
TokenSkmKind_ParenCl = 7,
TokenSkmKind_BrackCl = 8,
TokenSkmKind_BraceCl = 9,
TokenSkmKind_COUNT = 10,
};
char *token_skm_kind_names[] = {
"EOF",
"Whitespace",
"LexError",
"Text",
"ParenOp",
"BrackOp",
"BraceOp",
"ParenCl",
"BrackCl",
"BraceCl",
};
