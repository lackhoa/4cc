// C:\Users\vodan\4ed\code/meta_introspect.cpp:410:
#define Game_Input_Public_Embedding \
 union\
{\
struct\
{\
Key_Mods active_mods;\
b8 *key_states;\
u8 *key_state_changes;\
Frame_Info frame;\
b32 debug_camera_on;\
Mouse_State mouse;\
\
};\
Game_Input_Public Game_Input_Public;\
};\
