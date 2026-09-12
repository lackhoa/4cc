/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 26.09.2017
 *
 * Mac error box implementation.
 *
 */

// TOP

function void
system_error_box(char *msg){
    //LOGF("error box: %s\n", msg);
    osx_error_dialogue(msg);
    exit(1);
}

// TODO(kv) No native dialog on mac yet: always confirm (matches the win32 default button).
function i32
system_confirm_box(char *title, char *message, b32 offer_save){
    return(1);
}

// BOTTOM

