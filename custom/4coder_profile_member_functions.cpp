nono removeme
////////

Profile_Scope_Block::Profile_Scope_Block(Thread_Context *tctx, Profile_Global_List *list,
                                         String_Const_u8 name, String_Const_u8 location){
    profile_block__init(tctx, list, name, location, this);
}
Profile_Scope_Block::Profile_Scope_Block(Application_Links *app, String_Const_u8 name,
                                         String_Const_u8 location){
    Thread_Context *v_tctx = get_thread_context(app);
    Profile_Global_List *v_list = get_core_profile_list(app);
    profile_block__init(v_tctx, v_list, name, location, this);
}
Profile_Scope_Block::~Profile_Scope_Block(){
    this->close_now();
    profile_thread_flush(this->tctx, this->list);
}
void
Profile_Scope_Block::close_now(){
    if (!this->is_closed){
        thread_profile_record_pop(this->tctx, system_now_time(), this->id);
        this->is_closed = true;
    }
}

Profile_Block::Profile_Block(Thread_Context *tctx, Profile_Global_List *list,
                             String_Const_u8 name, String_Const_u8 location){
    profile_block__init(tctx, list, name, location, this);
}
Profile_Block::Profile_Block(Application_Links *app, String_Const_u8 name,
                             String_Const_u8 location){
    Thread_Context *v_tctx = get_thread_context(app);
    Profile_Global_List *v_list = get_core_profile_list(app);
    profile_block__init(v_tctx, v_list, name, location, this);
}
Profile_Block::~Profile_Block(){
    this->close_now();
}
void
Profile_Block::close_now(){
    if (!this->is_closed){
        thread_profile_record_pop(this->tctx, system_now_time(), this->id);
        this->is_closed = true;
    }
}

////////////////////////////////
