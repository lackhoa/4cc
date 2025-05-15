//-
struct Lex_State_Skm
{
 u8 *base;
 u8 *emit_ptr;
 u8 *ptr;
 
 i32 brace_nesting;
};

function void
lex_full_input_skm_init(void *state_ptr0, Stringz input)
{
 Lex_State_Skm *state_ptr = (Lex_State_Skm *)state_ptr0;
 state_ptr->base = input.str;
 state_ptr->emit_ptr = input.str;
 state_ptr->ptr = input.str;
}

#define WhiteSpaceCases   case '\t':case '\n':case '\v':case '\f':case '\r':case ' ':

myinline b32
skm_character_is_whitespace(char c)
{
 switch(c)
 {
  WhiteSpaceCases return 1;
  default:        return 0;
 }
}
myinline b32
is_a_to_z(u8 c)
{
 return (('a' <= c) and (c <= 'z') or
         ('A' <= c) and (c <= 'Z'));
}
function b32
skm_is_identifier_start(u8 c0, u8 c1)
{// NOTE Double underscore == Subscript operator
 return (is_a_to_z(c0) or
         (c0 == '_') and (c1 != '_'));
}
function b32
skm_is_identifier(u8 c0, u8 c1)
{// NOTE Double underscore == Subscript operator
 return ('0' <= c0 and c0 <= '9') or skm_is_identifier_start(c0, c1);
}

function b32
lex_full_input_skm_breaks(Arena *arena, Token_List *list, void *state_ptr0, u64 max)
{
 Lex_State_Skm *state_ptr = (Lex_State_Skm *)state_ptr0;
 b32 done = 0;
 u64 emit_counter = 0;
 Lex_State_Skm state = *state_ptr;
 
 auto not_eof = [&]() -> b32 { return *state.ptr != 0; };
 
 auto is_in_code = [&]() -> b32 { return state.brace_nesting > 0; };
 
 auto get_this_char = [&]() -> u8 { return *state.ptr; };
 auto get_next_char = [&]() -> u8 { return *state.ptr ? *(state.ptr+1) : 0; };
 
 auto emit_token = [&](Token_Base_Kind kind) -> Token *
 {
  Token *result = &stub_token;
  i64 size = (i64)(state.ptr - state.emit_ptr);
  if(size == 0){ kv_assert(kind  == TokenBaseKind_EOF); }
  
  {// NOTE(kv) There is no "empty token", beside the dummy EOF
   Token token = {};
   token.pos      = (i64)(state.emit_ptr - state.base);
   token.size     = size;
   token.kind     = kind;
   if(is_in_code()){ token.flags |= TokenBaseFlag_SkmCode; }
   
   result = token_list_push(arena, list, &token);
   emit_counter++;
   state.emit_ptr = state.ptr;
  }
  return result;
 };
 
 while(emit_counter < max)
 {//-Root
  if(*state.ptr == 0)
  {
   emit_token(TokenBaseKind_EOF);
   done = true;
   break;
  }
  else
  {
   switch(*state.ptr)
   {
    case '{':
    {
     state.ptr += 1;
     state.brace_nesting++;
     emit_token(TokenBaseKind_ScopeOpen);
    }break;
    
    case '}':
    {
     state.ptr += 1;
     emit_token(TokenBaseKind_ScopeClose);
     if(state.brace_nesting > 0){ state.brace_nesting--; }
    }break;
    
    case '(':
    {
     state.ptr += 1;
     emit_token(TokenBaseKind_ParenOpen);
    }break;
    
    case ')':
    {
     state.ptr += 1;
     emit_token(TokenBaseKind_ParenClose);
    }break;
    
    case '[':
    {
     state.ptr += 1;
     emit_token(TokenBaseKind_ParenOpen);
    }break;
    
    case ']':
    {
     state.ptr += 1;
     emit_token(TokenBaseKind_ParenClose);
    }break;
    
    default:
    {
     if(is_in_code())
     {//-Code mode
      u8 c0 = *state.ptr;
      u8 c1 = get_next_char();
      if(skm_is_identifier_start(c0,c1))
      {
       state.ptr++;
       while(skm_is_identifier(get_this_char(), get_next_char()))
       {
        state.ptr++;
       }
       emit_token(TokenBaseKind_Identifier);
      }
      else if(c0 == '_')
      {// NOTE Since we're not an identifier, we must be subscript.
       kv_assert(*(state.ptr+1) == '_');
       state.ptr += 2;
       emit_token(TokenBaseKind_Operator);
      }
      else if(skm_character_is_whitespace(c0))
      {
       state.ptr += 1;
       while(not_eof() and skm_character_is_whitespace(*state.ptr))
       {
        state.ptr += 1;
       }
       emit_token(TokenBaseKind_Whitespace);
      }
      else
      {
       char op_char = *state.ptr;
       Token_Base_Kind token_kind = TokenBaseKind_LexError;
       state.ptr++;
       char next_char = *state.ptr;
       switch(op_char)
       {//-Operators
        case '/':
        {
         switch(next_char)
         {
          case '/':
          {
           token_kind = TokenBaseKind_Comment;
           while(not_eof() and *state.ptr != '\n')
           {
            state.ptr++;
           }
          }break;
          
          case '*':
          {
           token_kind = TokenBaseKind_Comment;
           while(not_eof())
           {
            if(*state.ptr == '*' and
               *(state.ptr+1) == '/')
            {
             state.ptr += 2;
             break;
            }
            else { state.ptr++; }
           } 
          }break;
          
          case '=': { token_kind = TokenBaseKind_Operator; state.ptr++; }break;
          
          default: { token_kind = TokenBaseKind_Operator; }break;
         }
        }break;
        
        case ';':
        case ',':
        {
         token_kind = TokenBaseKind_StatementClose;
        }break;
        
        case '+':
        case '-':
        case '*':
        case '=':
        case '^':
        {
         token_kind = TokenBaseKind_Operator;
         
         switch(op_char)
         {
          case '+':
          {
           switch(*state.ptr){ case '+': case '=': state.ptr++; }
          }break;
          
          case '-':
          {
           switch(*state.ptr){ case '-': case '=': state.ptr++; }
          }break;
          
          case '*':
          case '=':
          {// Things that go with '='
           switch(*state.ptr){ case '=': state.ptr++; }
          }break;
         }
        }break;
       }
       
       emit_token(token_kind);
      }
     }
     else
     {//-Text state
      while(not_eof())
      {
       switch(*state.ptr)
       {
        case '\0':
        case '(': case ')': case '[': case ']': case '{': case '}':
        {
         goto end_of_text_state;
        }break;
        
        default: { state.ptr += 1; }break;
       }
      }
      
      end_of_text_state:
      emit_token(TokenBaseKind_Comment);
     }
    }break;
   }//switch on character
  }
 }//while
 
 *state_ptr = state;
 return(done);
}

#undef WhiteSpaceCases

function Token_List
lex_full_input_skm(Arena *arena, Stringz input)
{
 Lex_State_Skm state = {};
 lex_full_input_skm_init(&state, input);
 Token_List list = {};
 lex_full_input_skm_breaks(arena, &list, &state, max_u64);
 return(list);
}
//-