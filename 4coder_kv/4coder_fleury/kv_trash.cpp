function BUFFER_EDIT_RANGE_SIG(F4_BufferEditRange_skm)
{
    // buffer_id, new_range, original_size
    ProfileScope(app, "F4_BufferEditRange_skm");
    
    Range_i64 old_range = Ii64(old_cursor_range.min.pos,
                               old_cursor_range.max.pos);
    kv_assert(new_range.min <= new_range.max);
    kv_assert(old_range.min <= old_range.max);

    buffer_shift_fade_ranges(buffer_id, old_range.max, (new_range.max - old_range.max));
    
    {
        code_index_lock();
        Code_Index_File *file = code_index_get_file(buffer_id);
        if (file != 0){
            code_index_shift(file, old_range, range_size(new_range));
        }
        code_index_unlock();
    }
    
    Scratch_Block scratch(app);
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
    
    Base_Allocator *allocator = managed_scope_allocator(app, scope);
    b32 do_full_relex = false;
    
    // note(kv): if there's a running lex task then we gotta restart it anew, really bad
    if (async_task_is_running_or_pending(&global_async_system, *lex_task_ptr)) {
        async_task_cancel(app, &global_async_system, *lex_task_ptr);
        buffer_unmark_as_modified(buffer_id);
        do_full_relex = true;
        *lex_task_ptr = 0;
    }
    
    Token_Array *tokens = scope_attachment(app, scope, attachment_tokens, Token_Array);
    if (tokens && tokens->tokens)
    {
        ProfileBlockNamed(app, "attempt resync", profile_attempt_resync);
        
        // note(kv): back up to the first non-whitespace token
        i64 token_index_first;
        {// token_relex_first(ptr, old_range.first, 1)
          Token_Iterator_Array it = token_iterator_pos(0, tokens, old_range.first);
          token_it_dec(&it);
          token_index_first = token_it_index(&it);
        }

        i64 token_index_resync_guess;
        {// token_relex_resync(tokens, old_range.one_past_last, 16);
          Token_Iterator_Array it = token_iterator_pos(0, tokens, old_range.one_past_last);
          // note: so we look ahead by a bunch of tokens (for what purpose?)
          i64 look_ahead_repeats = 16;
          for_increment(i, 0, look_ahead_repeats) {
            if (!token_it_inc(&it)) {
              break;
            }
          }
          token_index_resync_guess = token_it_index(&it);
        }

        // Let's say each character is a token.
        // the text goes from    ac[x]bde   to    ac[yz]bde

        Token *token_first  = tokens->tokens + token_index_first;        // the "x" token
        Token *token_resync = tokens->tokens + token_index_resync_guess;
        
        // text_shift = new_range_size - old_range_size
        //            = 2 - 1 = 1
        // meaning that b would be shifted by 1
        i64 text_shift = range_size(new_range) - range_size(old_range);

        Range_i64 relex_range = Ii64(token_first->pos,
                                     token_resync->pos + token_resync->size +  text_shift);
        String_Const_u8 partial_text = push_buffer_range(app, scratch, buffer_id, relex_range);

        // note: So now the text buffer has been updated.
            
        //~ NOTE(rjf): Lex
        F4_Language *language = F4_LanguageFromBuffer(app, buffer_id);
        if (!language) language = F4_LanguageFromString(S8Lit("cpp"));
        //
        Token_List relex_list =
          F4_Language_LexFullInput_NoBreaks(app, language, scratch, partial_text);
        //
        if (relex_range.one_past_last < buffer_get_size(app, buffer_id))
        {
          token_drop_eof(&relex_list);
        }
        b32 successful_resync  = false;
        i64 first_resync_index = 0;
        if (!do_full_relex)
        {
          // token_relex(relex_list, relex_range.first - right_delta,
          //             tokens->tokens, token_index_first, token_index_resync_guess)
          // example: new_pos_to_old_pos_shift = 2 - 1 = 1
          Token_Array relexed_tokens = {token_first, token_index_resync_guess - token_index_first + 1};
          // relexed_tokens = y,z,b,d,e
          if (relex_list.total_count > 0)
          {
            i64 relex_list_last_index = relex_list.total_count - 1;
            Token_Iterator_List it = token_iterator_index(0, &relex_list, relex_list_last_index);
            do {
              Token *token = token_it_read(&it);  // token = b
              // note(kv): "rebased" because remember "token" is from the relexed_token array
              i64 new_pos_rebased = token_first->pos + token->pos;  // 2+2 = 4
              i64 old_token_index = token_index_from_pos(&relexed_tokens, new_pos_rebased - text_shift);  // token_index_from_pos(relexed_tokens, 3) -> it is indeed the old "b"
              Token *old_token = relexed_tokens.tokens + old_token_index;
              if (new_pos_rebased  == old_token->pos+text_shift &&
                  token->size      == old_token->size     &&
                  token->kind      == old_token->kind     &&
                  token->sub_kind  == old_token->sub_kind &&
                  token->flags     == old_token->flags    &&
                  token->sub_flags == old_token->sub_flags)
              {
                successful_resync = true;
                first_resync_index = token_index_first + old_token_index;
              }
              else {
                break;
              }
            } while (token_it_dec_all(&it));
          }
            
          if (successful_resync == true) {
            // nochicken: can't believe this works...
            print_message(app, SCu8("successful_resync\n"));
          }

          do_full_relex = !successful_resync; 
        }

        ProfileCloseNow(profile_attempt_resync);

        if (successful_resync)
        {
          ProfileBlock(app, "apply resync");
                
          i64 token_index_resync = first_resync_index;
                
          Range_i64 head = Ii64(0, token_index_first);
          Range_i64 replaced = Ii64(token_index_first, token_index_resync);
          Range_i64 tail = Ii64(token_index_resync, tokens->count);
          i64 resynced_count = (token_index_resync_guess + 1) - token_index_resync;
          i64 relexed_count = relex_list.total_count - resynced_count;
          i64 tail_shift = relexed_count - (token_index_resync - token_index_first);
                
          i64 new_tokens_count = tokens->count + tail_shift;
          Token *new_tokens = base_array(allocator, Token, new_tokens_count);
                
          Token *old_tokens = tokens->tokens;
          block_copy_array_shift(new_tokens, old_tokens, head, 0);
          token_fill_memory_from_list(new_tokens + replaced.first, &relex_list, relexed_count);
          for (i64 i = 0, index = replaced.first; i < relexed_count; i += 1, index += 1){
            new_tokens[index].pos += relex_range.first;
          }
          for (i64 i = tail.first; i < tail.one_past_last; i += 1){
            old_tokens[i].pos += text_shift;
          }
          block_copy_array_shift(new_tokens, tokens->tokens, tail, tail_shift);
                
          base_free(allocator, tokens->tokens);
                
          tokens->tokens = new_tokens;
          tokens->count = new_tokens_count;
          tokens->max = new_tokens_count;
                
          buffer_mark_as_modified(buffer_id);
        }
    }
    
    if (do_full_relex){
      print_message(app, SCu8("!!!do full relex!!!\n"));
      *lex_task_ptr = async_task_no_dep(&global_async_system, F4_DoFullLex_ASYNC,
                                        make_data_struct(&buffer_id));
    }
    
    // no meaning for return
    return(0);
}