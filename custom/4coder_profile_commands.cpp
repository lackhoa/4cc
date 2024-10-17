////////

CUSTOM_COMMAND_SIG(profile_enable)
CUSTOM_DOC("Allow 4coder's self profiler to gather new profiling information.")
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

CUSTOM_COMMAND_SIG(profile_clear)
CUSTOM_DOC("Clear all profiling information from 4coder's self profiler.")
{
 Profile_Global_List *list = get_core_profile_list(app);
 profile_clear(list);
}

CUSTOM_COMMAND_SIG(kv_profile_disable_and_inspect)
CUSTOM_DOC("disable and inspect profile")
{
 profile_disable(app);
 profile_inspect(app);
}
