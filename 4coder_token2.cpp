//-
function b32
can_relex_from_token(Token *token)
{
 // NOTE(kv) When we lex from the middle of the file,
 // and land right in the middle of a preprocessor,
 // we'll mistakenly parse that as code!
 return (not HasFlag(token->flags, TokenBaseFlag_PreprocessorBody) and
         not HasFlag(token->flags, TokenBaseFlag_SkmCode));
}
function i64
token_relex_first(Token_Array *tokens, i64 edit_range_first, i64 backup_repeats)
{
 Token_Iterator_Array it = tkarr_at_pos(0, tokens, edit_range_first);
 b32 good_status = true;
 for (i64 i = 0;
      i < backup_repeats and good_status;
      i += 1)
 {
  good_status = tkarr_dec(&it) != 0;
 }
 
 if(good_status)
 {
  for(;;)
  {
   Token *token = tkarr_read(&it);
   if(can_relex_from_token(token)) { break; }
   if(!tkarr_dec(&it)){ break; }
  }
 }
 
 return(tkarr_index(&it));
}

function i64
token_relex_resync(Token_Array *tokens, i64 edit_range_first, i64 look_ahead_repeats)
{
 Token_Iterator_Array it = tkarr_at_pos(0, tokens, edit_range_first);
 b32 good_status = true;
 for (i64 i = 0;
      (i < look_ahead_repeats) and good_status;
      i += 1)
 {
  good_status = tkarr_inc(&it) != 0;
 }
 
 if(good_status)
 {
  for(;;)
  {
   Token *token = tkarr_read(&it);
   if(can_relex_from_token(token)){ break; }
   if(!tkarr_inc(&it)) { break; }
  }
 }
 
 return(tkarr_index(&it));
}

function Token_Relex
token_relex(Token_List relex_list, i64 new_pos_to_old_pos_shift, Token *tokens, i64 relex_first, i64 relex_last)
{
 Token_Relex relex = {};
 if (relex_list.total_count > 0){
  Token_Array relexed_tokens = 
  {
   .tokens = tokens + relex_first, 
   .count = relex_last - relex_first + 1,
  };
  Token_Iterator_List it = token_iterator_index(0, &relex_list, relex_list.total_count - 1);
  for (;;){
   Token *token = token_it_read(&it);
   i64 new_pos_rebased = token->pos + new_pos_to_old_pos_shift;
   i64 old_token_index = token_index_from_pos(&relexed_tokens, new_pos_rebased);
   Token *old_token = relexed_tokens.tokens + old_token_index;
   if (new_pos_rebased == old_token->pos &&
       token->size == old_token->size &&
       token->kind == old_token->kind &&
       token->sub_kind == old_token->sub_kind &&
       token->flags == old_token->flags &&
       token->sub_flags == old_token->sub_flags){
    relex.successful_resync = true;
    relex.first_resync_index = relex_first + old_token_index;
   } else{
    break;
   }
   if (!tklist_dec_all(&it)){
    break;
   }
  }
 }
 return(relex);
}

//-