#pragma once
// NOTE(kv): this file used to be generated

#define graphics_get_texture_return  Texture_ID
#define graphics_get_texture_params  Vec3_i32 dim, Texture_Kind texture_kind
//
#define graphics_fill_texture_return  b32
#define graphics_fill_texture_params  Texture_Kind texture_kind, Texture_ID texture, Vec3_i32 p, Vec3_i32 dim, void* data
//
#define graphics_set_game_texture_return  void
#define graphics_set_game_texture_params  Texture_ID texture

#define XListHelper(X, N)  X(graphics_##N, graphics_##N##_return, (graphics_##N##_params))

#define XList(X)  \
    XListHelper(X, get_texture)  \
    XListHelper(X, fill_texture)  \
    XListHelper(X, set_game_texture)

// typedef
XList(XTypedef);

struct API_VTable_graphics
{
    XList(XPointer)
};

#if defined(STATIC_LINK_API)

XList(XInternalFunction)

internal void
graphics_api_fill_vtable(API_VTable_graphics *vtable)
{
    vtable->graphics_get_texture      = graphics_get_texture;
    vtable->graphics_fill_texture     = graphics_fill_texture;
    vtable->graphics_set_game_texture = graphics_set_game_texture;
}


#undef STATIC_LINK_API

#elif defined(DYNAMIC_LINK_API)
//
XList(XGlobalPointer);

internal void
graphics_api_read_vtable(API_VTable_graphics *vtable)
{
    graphics_get_texture      = vtable->graphics_get_texture;
    graphics_fill_texture     = vtable->graphics_fill_texture;
    graphics_set_game_texture = vtable->graphics_set_game_texture;
}

#undef DYNAMIC_LINK_API
#endif

#undef XList
#undef XListHelper
