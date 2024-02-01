#define graphics_get_texture_sig() Texture_ID graphics_get_texture(Vec3_i32 dim, Texture_Kind texture_kind)
#define graphics_fill_texture_sig() b32 graphics_fill_texture(Texture_Kind texture_kind, Texture_ID texture, Vec3_i32 p, Vec3_i32 dim, void* data)
typedef Texture_ID graphics_get_texture_type(Vec3_i32 dim, Texture_Kind texture_kind);
typedef b32 graphics_fill_texture_type(Texture_Kind texture_kind, Texture_ID texture, Vec3_i32 p, Vec3_i32 dim, void* data);
typedef void graphics_set_game_texture_type(Texture_ID);
struct API_VTable_graphics{
graphics_get_texture_type *get_texture;
graphics_fill_texture_type *fill_texture;
graphics_set_game_texture_type *set_game_texture;
};
#if defined(STATIC_LINK_API)
internal Texture_ID graphics_get_texture(Vec3_i32 dim, Texture_Kind texture_kind);
internal b32 graphics_fill_texture(Texture_Kind texture_kind, Texture_ID texture, Vec3_i32 p, Vec3_i32 dim, void* data);
internal void graphics_set_game_texture(Texture_ID texture);
#undef STATIC_LINK_API
#elif defined(DYNAMIC_LINK_API)
global graphics_get_texture_type *graphics_get_texture = 0;
global graphics_fill_texture_type *graphics_fill_texture = 0;
global graphics_set_game_texture_type *graphics_set_game_texture = 0;
#undef DYNAMIC_LINK_API
#endif
