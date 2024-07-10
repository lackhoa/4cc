/*
4coder_custom.cpp
*/

// TOP

function b32
custom_get_version(i1 maj, i1 min, i1 patch){
    return(maj == MAJOR && min == MINOR && patch == PATCH);
}

#if 0
function void
custom_init_apis(API_VTable_system *system_vtable){
    (system_vtable);
}
#endif

// BOTTOM
