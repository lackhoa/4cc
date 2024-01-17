#if !defined(FCODER_LEX_GEN_HAND_WRITTEN)
#define FCODER_LEX_GEN_HAND_WRITTEN

internal u64
lexeme_hash(u64 seed, u8 *ptr, u64 size){
    u64 result = 0;
    for (u64 i = 0; i < size; i += 1, ptr += 1){
        result ^= ((*ptr) ^ result*59) + seed;
    }
    return(result);
}

internal Lexeme_Table_Lookup
lexeme_table_lookup(u64 *hash_array, String_Const_u8 *key_array, 
                    Lexeme_Table_Value *value_array, i32 slot_count, u64 seed,
                    u8 *ptr, u64 size){
    Lexeme_Table_Lookup result = {};
    u64 hash = lexeme_hash(seed, ptr, size);
    u64 comparison_hash = hash | 1;
    i32 first_index = (hash % slot_count);
    i32 index = first_index;
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
struct Lex_State_Skm{
u8 *base;
u8 *delim_first;
u8 *delim_one_past_last;
u8 *emit_ptr;
u8 *ptr;
u8 *opl_ptr;
};
internal void
lex_full_input_skm_init(Lex_State_Skm *state_ptr, String_Const_u8 input){
state_ptr->base = input.str;
state_ptr->delim_first = input.str;
state_ptr->delim_one_past_last = input.str;
state_ptr->emit_ptr = input.str;
state_ptr->ptr = input.str;
state_ptr->opl_ptr = input.str + input.size;
}
internal b32
lex_full_input_skm_breaks(Arena *arena, Token_List *list, Lex_State_Skm *state_ptr, u64 max){
b32 result = false;
u64 emit_counter = 0;
Lex_State_Skm state;
block_copy_struct(&state, state_ptr);
{
state_label_1: // root
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_EOF;
token.kind = 0;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
result = true;
goto end;
}
}
switch (*state.ptr){
default:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_LexError;
token.kind = 2;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07:
case 0x08:case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:case 0x13:
case 0x14:case 0x15:case 0x16:case 0x17:case 0x18:case 0x19:case 0x1a:
case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f:case 0x21:case 0x22:
case 0x23:case 0x24:case 0x25:case 0x26:case 0x27:case 0x2a:case 0x2b:
case 0x2c:case 0x2d:case 0x2e:case 0x2f:case 0x30:case 0x31:case 0x32:
case 0x33:case 0x34:case 0x35:case 0x36:case 0x37:case 0x38:case 0x39:
case 0x3a:case 0x3b:case 0x3c:case 0x3d:case 0x3e:case 0x3f:case 0x40:
case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x47:
case 0x48:case 0x49:case 0x4a:case 0x4b:case 0x4c:case 0x4d:case 0x4e:
case 0x4f:case 0x50:case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:
case 0x56:case 0x57:case 0x58:case 0x59:case 0x5a:case 0x5c:case 0x5e:
case 0x5f:case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:case 0x67:case 0x68:case 0x69:case 0x6a:case 0x6b:case 0x6c:
case 0x6d:case 0x6e:case 0x6f:case 0x70:case 0x71:case 0x72:case 0x73:
case 0x74:case 0x75:case 0x76:case 0x77:case 0x78:case 0x79:case 0x7a:
case 0x7c:case 0x7e:case 0x7f:
{
state.ptr += 1;
goto state_label_2; // text
}break;
case 0x09:case 0x0a:case 0x0b:case 0x0c:case 0x0d:case 0x20:
{
state.ptr += 1;
goto state_label_3; // whitespace
}break;
case 0x28:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_ParenOp;
token.kind = 13;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x29:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_ParenCl;
token.kind = 14;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x5b:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_BrackOp;
token.kind = 13;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x5d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_BrackCl;
token.kind = 14;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x7b:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_BraceOp;
token.kind = 13;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x7d:
{
state.ptr += 1;
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_BraceCl;
token.kind = 14;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
}
}
{
state_label_2: // text
if (state.ptr == state.opl_ptr){
if ((true)){
{
Token token = {};
token.pos = (i64)(state.emit_ptr - state.base);
token.size = (i64)(state.ptr - state.emit_ptr);
do{
token.sub_kind = TokenSkmKind_Text;
token.kind = 3;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
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
do{
token.sub_kind = TokenSkmKind_Text;
token.kind = 3;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07:
case 0x08:case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:case 0x13:
case 0x14:case 0x15:case 0x16:case 0x17:case 0x18:case 0x19:case 0x1a:
case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f:case 0x21:case 0x22:
case 0x23:case 0x24:case 0x25:case 0x26:case 0x27:case 0x2a:case 0x2b:
case 0x2c:case 0x2d:case 0x2e:case 0x2f:case 0x30:case 0x31:case 0x32:
case 0x33:case 0x34:case 0x35:case 0x36:case 0x37:case 0x38:case 0x39:
case 0x3a:case 0x3b:case 0x3c:case 0x3d:case 0x3e:case 0x3f:case 0x40:
case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x47:
case 0x48:case 0x49:case 0x4a:case 0x4b:case 0x4c:case 0x4d:case 0x4e:
case 0x4f:case 0x50:case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:
case 0x56:case 0x57:case 0x58:case 0x59:case 0x5a:case 0x5c:case 0x5e:
case 0x5f:case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:
case 0x66:case 0x67:case 0x68:case 0x69:case 0x6a:case 0x6b:case 0x6c:
case 0x6d:case 0x6e:case 0x6f:case 0x70:case 0x71:case 0x72:case 0x73:
case 0x74:case 0x75:case 0x76:case 0x77:case 0x78:case 0x79:case 0x7a:
case 0x7c:case 0x7e:case 0x7f:
{
state.ptr += 1;
goto state_label_2; // text
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
do{
token.sub_kind = TokenSkmKind_Whitespace;
token.kind = 1;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
}
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
do{
token.sub_kind = TokenSkmKind_Whitespace;
token.kind = 1;
}while(0);
token_list_push(arena, list, &token);
emit_counter += 1;
state.emit_ptr = state.ptr;
if (emit_counter == max){
goto end;
}
}
goto state_label_1; // root
}break;
case 0x09:case 0x0a:case 0x0b:case 0x0c:case 0x0d:case 0x20:
{
state.ptr += 1;
goto state_label_3; // whitespace
}break;
}
}
end:;
block_copy_struct(state_ptr, &state);
return(result);
}
internal Token_List
lex_full_input_skm(Arena *arena, String_Const_u8 input){
Lex_State_Skm state = {};
lex_full_input_skm_init(&state, input);
Token_List list = {};
lex_full_input_skm_breaks(arena, &list, &state, max_u64);
return(list);
}
