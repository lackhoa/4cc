//-
myinline void
meta_parse_key(Ed_Parser *p, String key)
{
 ep_id(p, key);
 ep_char(p, '=');
}
function b32
meta_maybe_key(Ed_Parser *p, String key)
{
 b32 result = false;
 if(ep_maybe_id(p, key)){
  ep_char(p, '=');
  result = p->ok_;
 }
 return result;
}
function b32
k_test_char(Ed_Parser *p, String terminators)
{
 String chr = ep_print_token(p);
 if(chr.count == 1){
  for_i32(i,0,i32(terminators.count)){
   if(terminators[i] == chr[0]){
    return true;
   }
  }
 }
 return false;
}
function String
k_string_from_token_to_current(Ed_Parser *p, Token *token_start)
{
 String start = ep_print_token(p, token_start);
 String end   = ep_print_token(p);
 String result = {start.str, u64(end.str - start.str)};
 return result;
}
function char
k_eat_until_char(Ed_Parser *p, String terminators)
{// TODO(kv) what's the difference between this and @ep_eat_until_char?
 char result = 0;
 {
  while(!result && p->ok_){
   Scratch_Block scratch;
   String token = ep_print_token(scratch, p);//TODO(kv) OMG this is bad!
   b32 should_eat = true;
   if(token.len == 1){
    char char0 = token.str[0];
    if(ep__char_in_string_new(terminators, char0)){
     result = char0;
     should_eat = false;
    }else{
     u8 closer = get_matching_group_closer(char0);
     if(closer){
      ep_eat(p);
      k_eat_until_char(p, String{&closer, 1});
     }else{
      u8 opener = get_matching_group_opener(char0);
      if(opener){//NOTE(kv) Encountered unbalanced group closer, we should stop!
       p->fail();
      }
     }
    }
   }
   if(should_eat){
    ep_eat(p);
   }
  }
 }
 return result;
}
//-