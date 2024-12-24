////////

function void
profile_enable(App_Cmd *app)
{
 Profile_Global_List *list = get_core_profile_list(app);
 profile_set_enabled(list, true, ProfileEnable_UserBit);
}

//CUSTOM_DOC("Prevent 4coder's self profiler from gathering new profiling information.")
function void profile_disable(App *app)
{
 Profile_Global_List *list = get_core_profile_list(app);
 profile_set_enabled(list, false, ProfileEnable_UserBit);
}

function void
profile_clear(App_Cmd *app)
{
 Profile_Global_List *list = get_core_profile_list(app);
 profile_clear(list);
}

function void
kv_profile_disable_and_inspect(App_Cmd *app)
{
 profile_disable(app);
 profile_inspect(app);
}
