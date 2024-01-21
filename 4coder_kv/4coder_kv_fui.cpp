internal b32
fui_handle_slider(Application_Links *app, View_ID view, Buffer_ID buffer)
{
  DEBUG_clear;
  Token_Iterator_Array it = kv_token_it_at_cursor(app);
  if ( !it.tokens ) return false;

  Token *token = token_it_read( &it );
  
  Scratch_Block scratch(app);
  String_Const_u8 string = push_token_lexeme(app, scratch, buffer, token);
  
  bool is_slider = string_compare(string, SCu8("fslider")) == 0;
  if ( is_slider )
  {
    breakable_block
    {
      token_it_inc(&it);
      token = token_it_read( &it );
      if (!token) break;
      if (token->kind != TokenBaseKind_ParentheticalOpen) break;
     
      token_it_inc(&it);
      token = token_it_read( &it );
      if (!token) break;
      if (token->kind != TokenBaseKind_LiteralFloat) break;
    
      string = push_token_lexeme(app, scratch, buffer, token);
      {
        printf_message(app, scratch, "value: %.*s\n", string_expand(string));
      }
      
      for (;;)
      { // note: ui loop
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort) break; 
        
        b32 inc = match_key_code(&in.event, KeyCode_L);
        b32 dec = match_key_code(&in.event, KeyCode_H);
        if (inc || dec)
        {
          const char *message = inc ? "inc" : "dec";
          printf_message(app, scratch, "%s\n", message);
        }
        else
        {
          leave_current_input_unhandled(app);
        }
      }
    }
  }
  return is_slider;
}