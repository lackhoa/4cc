//@generates driver.gen.cpp
#define AD_HAS_OS_CODE 0
#define AD_HAS_INTRINSIC 1
#define AD_IS_DRIVER 1
#define KV_H_NO_GLOBAL_ARENA_CHUNK_STORE
#include "kv.h"

#include "kv_math.h"

//NOTE(kv) We need to know about some ed api defines, so here you go!
#include "ed_api.gen.h"

#include "framework_driver_shared.h"
#include "4coder_debug_value.h"
#include "basic_types.gen.h"
#include "game_fui.h"
#include "send_bez.gen.h"

#include "game_body.cpp"
#include "game_anime.cpp"
#include "driver_utils.cpp"

myinline Slider
mk_slider_(Basic_Type type, i32 file, i16 begin, i16 end, i32 index, Fui_Options options)
{
 return {.type=type, .location={file,{begin,end}}, .index=index, .options=options};
}
//