/*
4coder_custom.cpp
*/

// TOP

extern "C" b32
custom_get_version(i32 maj, i32 min, i32 patch){
    return(maj == MAJOR && min == MINOR && patch == PATCH);
}

extern "C" void
custom_init_apis(API_VTable_system *system_vtable){
    system_api_read_vtable(system_vtable);
}

// BOTTOM
