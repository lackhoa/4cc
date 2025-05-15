////////

function void
profile_begin_cmd(App_Cmd *app)
{
 ProfileBegin("begin marker");
 ProfileEnd();
}

function void
profile_end_cmd(App_Cmd *app)
{
 ProfileBlock("end marker");
}

function void
profile_clear(App_Cmd *app)
{
 Profile_Global_List *list = get_core_profile_list(app);
 profile_clear(list);
}
//
