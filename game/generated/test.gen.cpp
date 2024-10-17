//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:259:
// NOTE: source: C:\Users\vodan\4ed\code\game\test.kc
#pragma once
// Generates: test.gen.cpp
struct Cache_Storage_91{
v1 tblink;
};
global Cache_Storage_91 cache_storage_91;
function void
render_character(Pose *pose, v1 animation_time){
v1 tblink = pose->tblink;
if(not(tblink == cache_storage_91.tblink &&
true)){
{
cache_storage_91.tblink = tblink;
}
{
b32 show_eye_guideline = painter.show_grid;
}
}
}
