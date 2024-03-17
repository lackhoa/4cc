global char* keycode_lut[2*Key_Code_COUNT];

function void init_keycode_lut(){
    for(i32 i=0; i<2*Key_Code_COUNT; i++){
        keycode_lut[i] = "";
    }
    i32 shift = Key_Code_COUNT;
    char *lowercase_alpha = "a\0b\0c\0d\0e\0f\0g\0h\0i\0j\0k\0l\0m\0n\0o\0p\0q\0r\0s\0t\0u\0v\0w\0x\0y\0z";
    for(i32 i='a'; i<='z'; i++){
        i32 index = i-'a';
        keycode_lut[Key_Code_A+index+shift] = key_code_name[Key_Code_A+index];
        keycode_lut[Key_Code_A+index] = lowercase_alpha + 2*index;
    }
    char *shift_digit = ")\0!\0@\0#\0$\0%\0^\0&\0*\0(";
    for(i32 i=0; i<=9; i++){
        keycode_lut[Key_Code_0+i] = key_code_name[Key_Code_0+i];
        keycode_lut[Key_Code_0+i+shift] = shift_digit + 2*i;;
    }
    char *function_keys = "F1\0F2\0F3\0F4\0F5\0F6\0F7\0F8\0F9\0F10\0F11\0F12";
    for(i32 i=0; i<12; i++){
        keycode_lut[Key_Code_F1+i] = keycode_lut[Key_Code_F1+i+shift] = function_keys + (3*i + (i>9)*(i-9));
    }
    
    keycode_lut[Key_Code_Space]     = keycode_lut[Key_Code_Space+shift]         = "Sp";
    keycode_lut[Key_Code_Tab]       = keycode_lut[Key_Code_BackwardSlash+shift] = "Tab";
    keycode_lut[Key_Code_Escape]    = keycode_lut[Key_Code_Escape+shift]        = "ESC";
    keycode_lut[Key_Code_Pause]     = keycode_lut[Key_Code_Pause+shift]         = key_code_name[Key_Code_Pause];
    keycode_lut[Key_Code_Up]        = keycode_lut[Key_Code_Up+shift]            = key_code_name[Key_Code_Up];
    keycode_lut[Key_Code_Down]      = keycode_lut[Key_Code_Down+shift]          = key_code_name[Key_Code_Down];
    keycode_lut[Key_Code_Left]      = keycode_lut[Key_Code_Left+shift]          = key_code_name[Key_Code_Left];
    keycode_lut[Key_Code_Right]     = keycode_lut[Key_Code_Right+shift]         = key_code_name[Key_Code_Right];
    keycode_lut[Key_Code_Backspace] = keycode_lut[Key_Code_Backspace+shift]     = "BS";
    keycode_lut[Key_Code_Return]    = keycode_lut[Key_Code_Escape+shift]        = "CR";
    keycode_lut[Key_Code_Delete]    = keycode_lut[Key_Code_Delete+shift]        = "Del";
    keycode_lut[Key_Code_Insert]    = keycode_lut[Key_Code_Insert+shift]        = "Ins";
    keycode_lut[Key_Code_Home]      = keycode_lut[Key_Code_Home+shift]          = "Home";
    keycode_lut[Key_Code_End]       = keycode_lut[Key_Code_End+shift]           = "End";
    keycode_lut[Key_Code_PageUp]    = keycode_lut[Key_Code_PageUp+shift]        = "PageUp";
    keycode_lut[Key_Code_PageDown]  = keycode_lut[Key_Code_PageDown+shift]      = "PageDown";
    keycode_lut[Key_Code_CapsLock]  = keycode_lut[Key_Code_CapsLock+shift]      = "Caps";
    keycode_lut[Key_Code_NumLock]   = keycode_lut[Key_Code_NumLock+shift]       = "NumLock";
    
    keycode_lut[Key_Code_Tick]          = "`"; keycode_lut[Key_Code_Tick+shift]           = "~";
    keycode_lut[Key_Code_Minus]         = "-"; keycode_lut[Key_Code_Minus+shift]          = "_";
    keycode_lut[Key_Code_Minus]         = "-"; keycode_lut[Key_Code_Minus+shift]          = "_";
    keycode_lut[Key_Code_Equal]         = "="; keycode_lut[Key_Code_Equal+shift]          = "+";
    keycode_lut[Key_Code_LeftBracket]   = "["; keycode_lut[Key_Code_LeftBracket+shift]    = "{";
    keycode_lut[Key_Code_RightBracket]  = "]"; keycode_lut[Key_Code_RightBracket+shift]   = "}";
    keycode_lut[Key_Code_Semicolon]     = ";"; keycode_lut[Key_Code_Semicolon+shift]      = ":";
    keycode_lut[Key_Code_Quote]         = "'"; keycode_lut[Key_Code_Quote+shift]          = "\"";
    keycode_lut[Key_Code_Comma]         = ","; keycode_lut[Key_Code_Comma+shift]          = "<";
    keycode_lut[Key_Code_Period]        = "."; keycode_lut[Key_Code_Period+shift]         = ">";
    keycode_lut[Key_Code_ForwardSlash]  = "/"; keycode_lut[Key_Code_ForwardSlash+shift]   = "?";
    keycode_lut[Key_Code_BackwardSlash] = "\\"; keycode_lut[Key_Code_BackwardSlash+shift] = "|";
}
