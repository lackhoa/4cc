/*
  NOTE(kv): OLD Rules for the renderer:
  - When you push render entries, dimensions are specified in "meter"
  - Textures and bitmaps are srgb premultiplied-alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
  - Packed colors are rgba in memory order (i.e abgr in u32 register order)
  pma = pre-multiplied alpha

  NOTE(kv): NEW rules for 3D
  - Matrices are arrays of column vectors
 */

#include "tr_model.cpp"

global u32 yellow_argb = 0xFF777700;
global u32 gray_argb   = pack_argb({.5,.5,.5,1});
global u32 red_argb    = pack_argb({1,0,0,1});
global u32 green_argb  = pack_argb({0,1,0,1});
global u32 blue_argb   = pack_argb({0,0,1,1});
global u32 black_argb  = pack_argb({0,0,0,1});
global u32 white_argb  = pack_argb({1,1,1,1});

// NOTE: This is a dummy buffer, so we can use the same commands to switch to the rendered game
global String8 GAME_BUFFER_NAME = str8lit("*game*");

struct Camera
{
    v1 distance;  // NOTE: by its axis z
    union
    {
        m3x3 axes;    // transform to project the object onto the camera axes
        struct
        {
            v3 x;
            v3 y;
            v3 z;
        };
    };
    m3x3 project;
    v1   focal_length;
};

struct Screen
{
  v3 center;
  v3 x_axis;
  v3 y_axis;
};

internal void
bitmap_set(Bitmap *bitmap, i32 x, i32 y, u32 color)
{
    y = bitmap->dim.y-1 - y; // inverting y
    
    if (in_range(0, y, bitmap->dim.y) &&
        in_range(0, x, bitmap->dim.x))
    {
        i32 index = y * bitmap->dim.x + x;
        bitmap->data[index] = color;
    }
}

inline u64
get_bitmap_size(Bitmap *bitmap)
{
    return bitmap->dim.y * bitmap->pitch;
}

internal void
tr_rectangle2i(Bitmap *bitmap, rect2i rect, ARGB_Color color)
{
    v2i dim = rect2i_get_dim(rect);
    for_i32 (y, rect.min.x, dim.y)
    {
        for_i32 (x, rect.min.x, dim.x)
        {
            bitmap_set(bitmap, x, y, color);
        }
    }
}

internal void 
tr_linei(Bitmap *bitmap, 
         i32 x0, i32 y0, i32 x1, i32 y1,
         ARGB_Color color)
{
    bool steep = false; 
   
    if (absolute(x0-x1) < absolute(y0-y1))
    {// if the line is steep, we transpose the image 
        macro_swap(x0, y0); 
        macro_swap(x1, y1); 
        steep = true; 
    }
    
    if (x0 > x1)
    {// make it "left to right"
        macro_swap(x0, x1); 
        macro_swap(y0, y1); 
    }
    
    i32 dx = x1-x0;
    i32 dy = y1-y0;
    i32 derror = absolute(dy) * 2; 
    i32 error = 0;
    i32 y = y0; 
    
    for (i32 x=x0;
         x <= x1; 
         x++)
    { 
        if (steep)
            bitmap_set(bitmap, y, x, color);   // if transposed, detranspose
        else
            bitmap_set(bitmap, x, y, color);
       
        error += derror;
        if (error > dx)
        {
            if (y1 > y0) y += 1;
            else         y -= 1;
            error -= 2 * dx;
        }
    }
}

internal void
tr_line(Bitmap *bitmap, 
        f32 x0, f32 y0, f32 x1, f32 y1,
        ARGB_Color color)
{
    return tr_linei(bitmap, (i32)x0, (i32)y0, (i32)x1, (i32)y1, color);
}

inline void
tr_line(Bitmap *bitmap, v2 p0, v2 p1, ARGB_Color color)
{
    tr_line(bitmap, p0.x, p0.y, p1.x, p1.y, color);
}

internal void
tr_triangle_old_school(Bitmap *bitmap, v2 p0, v2 p1, v2 p2, ARGB_Color color) 
{
    if (p0.y > p1.y) macro_swap(p0, p1); 
    if (p0.y > p2.y) macro_swap(p0, p2); 
    if (p1.y > p2.y) macro_swap(p1, p2); 
    
    f32 dy02 = p2.y - p0.y; 
    f32 dy01 = p1.y - p0.y;
    if (dy01 < 0.0001f) return;
    
    for_i32 (y, (i32)p0.y, (i32)p1.y+1)
    {
        f32 dy0 = (f32)y - p0.y;
        f32 alpha = dy0 / dy02;
        f32 beta  = dy0 / dy01;
        f32 Ax = lerp(p0.x, alpha, p2.x);
        f32 Bx = lerp(p0.x, beta,  p1.x);
        if (Ax > Bx)  macro_swap(Ax, Bx);
        for_i32 (x, (i32)Ax, (i32)Bx+1 )
        { 
            bitmap_set(bitmap, x, y, color);
        } 
    }
    
    f32 dy12 = p2.y - p1.y;
    if (dy12 > 0.0001f)
    {
        for_i32 (y, (i32)p1.y, (i32)p2.y+1)
        { 
            f32 dy0 = (f32)y - p0.y;
            f32 dy1 = (f32)y - p1.y;
            f32 alpha = dy0 / dy02; 
            f32 beta  = dy1 / dy12; // be careful with divisions by zero 
            f32 Ax = lerp(p0.x, alpha, p2.x);
            f32 Bx = lerp(p1.x, beta,  p2.x);
            if (Ax > Bx) macro_swap(Ax, Bx); 
            for_i32 (x, (i32)Ax, (i32)Bx+1)
            { 
                bitmap_set(bitmap, x, y, color);
            } 
        }    
    }
    
    fslider( draw_outline, v1{-0.298632f} );
    if (draw_outline > 0)
    {
        tr_line(bitmap, (p1), (p2), red_argb); 
        tr_line(bitmap, (p0), (p2), green_argb); 
        tr_line(bitmap, (p0), (p1), blue_argb); 
    }
}

// TODO: idk why this returns a v3, but rn I don't care
internal v3 barycentric(v2 *p, v2 P)
{
    v2 p0 = p[0];
    v2 p1 = p[1];
    v2 p2 = p[2];
    v3 u = cross(v3{ p2.x-p0.x, p1.x-p0.x, p0.x-P.x },
                 v3{ p2.y-p0.y, p1.y-p0.y, p0.y-P.y });
    
    if ( absolute(u.z) < 0.0001f )
        return v3{-1,1,1};
    else
        return v3{1.0f - (u.x+u.y)/u.z, u.y/u.z, u.x/u.z};
}

internal void 
tr_triangle(Bitmap *bitmap, v2 *p, ARGB_Color color)
{ 
    v2i max = bitmap->dim;
    v2i bboxmin = max; 
    v2i bboxmax = {0,0}; 
    for_i32 (i,0,3)
    { 
        bboxmin.x = clamp_bot(macro_min(bboxmin.x, (i32)p[i].x), 0);
        bboxmin.y = clamp_bot(macro_min(bboxmin.y, (i32)p[i].y), 0);
        
        bboxmax.x = clamp_top(macro_max(bboxmax.x, (i32)p[i].x+1), max.x);
        bboxmax.y = clamp_top(macro_max(bboxmax.y, (i32)p[i].y+1), max.y);
    } 
    for_i32 (x, bboxmin.x, bboxmax.x)
    { 
        for_i32 (y, bboxmin.y, bboxmax.y)
        {
            v3 bc_screen  = barycentric(p, castV2(x,y));
            if (bc_screen.x >= 0 && 
                bc_screen.y >= 0 && 
                bc_screen.z >= 0)
            {
                bitmap_set(bitmap, x, y, color);
            }
        } 
    }
}

internal void
tiny_renderer_main(v2i dim, u32 *data, Model *model)
{
    Bitmap bitmap_value = {.data=data, .dim=dim, .pitch=4*dim.x};
    Bitmap *bitmap = &bitmap_value;
   
    // NOTE(kv): clear screen
    tr_rectangle2i(bitmap, rect2i{.p0={0,0}, .p1=dim}, black_argb);
    
    {// NOTE(kv): outline
        f32 X = (f32)dim.x-1;
        f32 Y = (f32)dim.y-1;
        tr_line(bitmap, 0,0, X,0, red_argb);
        tr_line(bitmap, 0,0, 0,Y, red_argb);
        tr_line(bitmap, X,0, X,Y, red_argb);
        tr_line(bitmap, 0,Y, X,Y, red_argb);
    }
   
    b32 draw_mesh = def_get_config_b32(vars_intern_lit("draw_mesh"));
    if (draw_mesh)
    {
        for (int facei=0; 
             facei < arrlen(model->faces);
             facei++) 
        { 
            i32 *face = model->faces[facei]; 
            for_i32 (verti,0,3)
            { 
                v3 v0 = model->vertices[face[verti]];
                v3 v1 = model->vertices[face[(verti+1) % 3]];
                // NOTE: the model coordinates are in clip space
                f32 x0 = unilateral(v0.x)*(f32)dim.x;
                f32 y0 = unilateral(v0.y)*(f32)dim.y;
                f32 x1 = unilateral(v1.x)*(f32)dim.x;
                f32 y1 = unilateral(v1.y)*(f32)dim.y;
                tr_line(bitmap, x0,y0, x1,y1, white_argb);
            } 
        }
    }
  
    // NOTE: triangle nonsense who gives a *
    if (0)
    {
        v2 t0[3] = {V2(10, 70),   V2(50, 160),  V2(70, 80)};
        v2 t1[3] = {V2(180, 50),  V2(150, 1),   V2(70, 180)};
        v2 t2[3] = {V2(180, 150), V2(120, 160), V2(130, 180)};
        tr_triangle(bitmap, t0, red_argb); 
        tr_triangle(bitmap, t1, white_argb); 
        tr_triangle(bitmap, t2, green_argb);
    }
   
    f32 dim_x = (f32)dim.x;
    f32 dim_y = (f32)dim.y;
   
    if (1)
    {
        v2 beg = v2{0,0};
        v2 mid = v2{0, dim_y-1};
        v2 end = V2(dim_x-1, dim_y-1);
        v1 step = {1.0f / dim_x};  // NOTE: I don't know what I'm doing!
        for (f32 t = 0;
             t <= 1.0f;
             t += step)
        {
            v2 p1 = lerp(beg, t, mid);
            v2 p2 = lerp(mid, t, end);
            v2 pos = lerp(p1, t, p2);
            bitmap_set(bitmap, (i32)pos.x, (i32)pos.y, white_argb);
        }
    }
}

internal v2
perspective_project(Camera *camera, v3 point)
{
    v3 projected = matvmul3(&camera->project, point);
    // NOTE: distance is in camera_z, because d_camera_screen is also in z, and the screen_ratio is the ratio between those two distances.
    v1 dz = camera->distance - projected.z;
    v1 near_clip = 10;
    if (dz > near_clip)
    {
        v3 result = (camera->focal_length / dz) * projected;
        return result.xy;
    }
    else
        return V2All(F32_MAX);
}

internal void
draw_cubic_bezier(App *app, Camera *camera, v3 P[4])
{
    fslider(radius_a, 23.380762);
    fslider(radius_b, 3.923108);
   
    if (0)
    {// NOTE: Control points
        for_i32 (index, 1, 3)
        {
            v2 screen_p = perspective_project(camera, P[index]);
            draw_circle(app, screen_p, radius_a, yellow_argb);
        }
    }
    
    // NOTE: Sample points
    i32 nslices = 16;
    f32 inv_nslices = 1.0f / (f32)nslices;
    for_i32 (sample_index, 0, nslices+1)
    {
        f32 t_radius = (f32)sample_index * inv_nslices;
        f32 radius = lerp(radius_a, t_radius, radius_b);
        
        f32 u = inv_nslices * (f32)sample_index;
        f32 U = 1.0f-u;
        v3 world_p = (pow(U,3)*    P[0] + 
                      3*pow(U,2)*u*P[1] +
                      3*U*pow(u,2)*P[2] +
                      pow(u,3)*    P[3]);
        //
        v2 screen_p = perspective_project(camera, world_p);
        
        v1 draw_radius;
        {// NOTE: Having to calculate the focal_length/dz twice :<
            v1 projected_z = dot(camera->axes.z, world_p);
            v1 dz = camera->distance - projected_z;
            draw_radius = absolute((camera->focal_length / dz) * radius);
        }
        
        draw_circle(app, screen_p, draw_radius, gray_argb);
    }
}

internal void
setup_camera_from_data(Camera *camera, v3 camz, v1 distance)
{
    // NOTE: "camz" is normalized.
    camera->distance = distance;
   
    camera->x = noz( v3{-camz.y, camz.x, 0} );  // NOTE: z axis' projection should point up on the screen
    if (camera->x == v3{})
    {// NOTE: Happens when we look STRAIGHT DOWN on the target.
        camera->x = noz( v3{camz.z, 0, -camz.x} );  // NOTE: we want y axis to point up in this case
    }
    camera->y = noz( cross(camz, camera->x) );
    camera->z = camz;
    
#define t camera->axes.columns
    camera->project =
    {{
            {t[0][0], t[1][0], t[2][0]},
            {t[0][1], t[1][1], t[2][1]},
            {t[0][2], t[1][2], t[2][2]},
    }};
#undef t
}

u32 data_current_version = 5;
u32 data_magic_number = *(u32 *)"kvda";

struct Bezier
{
    v3 control_points[4];
};

struct Game_Save_Old
{
    u32 magic_number;
    u32 version;
    
    v3 camera_z;
    v1 camera_distance;
    
    Bezier bezier_curves[3];
};

struct Game_Save
{
    u32 magic_number;
    u32 version;
    
    v3 camera_z;  // @Old
    v1 camera_distance;
    
    Bezier bezier_curves[3];
    
    v1 camera_phi;
    v1 camera_theta;
};

global b32 user_requested_game_save = false;

internal b32
save_game(App *app, String8 save_dir, String8 save_path, Game_Save *save)
{
    Scratch_Block scratch(app);
    local_persist b32 has_done_backup = false;
    String8 backup_dir = pjoin(scratch, save_dir, "backups");
    
    b32 ok = true;
    if (!has_done_backup)
    {
        // NOTE: backup game if is_first_write_since_launched
        time_t rawtime;
        time(&rawtime);
        struct tm *timeinfo = localtime(&rawtime);
        char time_string[256];
        size_t strftime_result = strftime(time_string, sizeof(time_string), "%d_%m_%Y_%H_%M_%S", timeinfo);
        if (strftime_result == 0)
        {
            print_message(app, "strftime failed... go figure that out!\n");
            ok = false;
        }
        else
        {
            String8 backup_path = push_stringf(scratch, "%.*s/data_%s.kv", string_expand(backup_dir), time_string);
            ok = move_file(save_path, backup_path);
           
            if (ok) has_done_backup = true;
        }
        
        if (ok)
        {// NOTE: cycle out old backup files
            File_List backup_files = system_get_file_list(scratch, backup_dir);
            if (backup_files.count > 100)
            {
                u64 oldest_mtime = U64_MAX;
                String8 file_to_delete = {};
                File_Info **opl = backup_files.infos + backup_files.count;
                for (File_Info **backup = backup_files.infos;
                     backup < opl;
                     backup++)
                {
                    File_Attributes attr = (*backup)->attributes;
                    if (attr.last_write_time < oldest_mtime)
                    {
                        oldest_mtime   = attr.last_write_time;
                        file_to_delete = pjoin(scratch, backup_dir, (*backup)->filename);
                    }
                }
                b32 delete_ok = remove_file(scratch, file_to_delete);
                if (delete_ok) printf_message(app, "INFO: deleted backup file %.*s because it's too old", string_expand(file_to_delete));
                else           printf_message(app, "ERROR: failed to delete backup file %.*s", string_expand(file_to_delete));
            }
        }
    }
    
    if (ok)
    {// note: save the file
        ok = write_entire_file(scratch, save_path, save, sizeof(*save));
        if (!ok) printf_message(app, "Failed to write to file %.*s", string_expand(save_path));
    }
    
    return ok;
}

internal b32
is_key_newly_pressed(Key_Mod active_modifiers, Key_Mod modifiers, Key_Code keycode)
{
    if (active_modifiers == modifiers)
    {
        return (key_is_down(keycode) &&
                key_state_changes(keycode) > 0);
    }
    else return false;
}

// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
internal void
game_update_and_render(App *app, View_ID view, v1 dt)
{
    // NOTE: Setup sane math coordinate system.
    draw_set_y_up(app);
    rect2 clip = draw_get_clip(app);
    v2 clip_dim      = rect_dim(clip);
    v2 clip_half_dim = 0.5f * clip_dim;
    v2 layout_center = clip.min + clip_half_dim;
    draw_set_offset(app, layout_center);
    
    for_i32 (keycode, 1, KeyCode_COUNT)
    {// NOTE: Enable animation if any key is down
        if ( key_is_down(keycode) )
        {
            animate_in_n_milliseconds(app, 0);
            break;
        }
    }
    
    Scratch_Block scratch(app);
   
    Key_Mod active_modifiers;
    {// NOTE: Input shenanigans
        Input_Modifier_Set set = system_get_keyboard_modifiers(scratch);
        active_modifiers = pack_modifiers(set.mods, set.count);
    }
    
    v1 U = 1e3;  // render scale multiplier (@Cleanup push this scale down to the renderer)
    
    local_persist Game_Save save = {};
    
    // TODO(kv): Save these in the game state!!!
    String8 binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
    String8 save_dir  = pjoin(scratch, binary_dir, "data");
    String8 save_path = pjoin(scratch, save_dir, "data.kv");
    
    local_persist b32 inited = false;
    if ( !inited )
    {// NOTE: Initialization
        inited = true;
        String8 read_string = read_entire_file(scratch, save_path);
        Game_Save *read = (Game_Save *)read_string.str;
        if (read->magic_number == data_magic_number)
        {
            if (read->version == data_current_version)
            {
                save = *read;
            }
            else if ( read->version == (data_current_version-1) )
            {
                print_message(app, "Game data load: Converting old version to new version!\n");
                Game_Save_Old *old = (Game_Save_Old *)read;
                // NOTE(kv): Conversion code!
                save = *(Game_Save *)old;
                
                save.version = data_current_version;
                // NOTE(kv): Write back the converted file.
                save_game(app, save_dir, save_path, &save);
            }
            else print_message(app, "Game data load: Unknown version\n");
        }
        else print_message(app, "Game data load: Wrong magic number!\n");
    }
    
    if (user_requested_game_save)
    {
        save_game(app, save_dir, save_path, &save);
        user_requested_game_save = false;
    }
    
    Camera camera_value = {};
    Camera *camera = &camera_value;
    {// NOTE: Camera handling
        camera->focal_length = 2000.f;
        
        if (view_is_active(app, view) && 
            (active_modifiers & KeyMod_Ctl))
        {// NOTE: Input handling
            v3 delta = dt * fui_direction_from_key_states().xyz;
            
            v1 phi   = save.camera_phi   + 0.1*delta.y;
            v1 theta = save.camera_theta + 0.1*delta.x;
            
            save.camera_phi       = clamp_between(-0.25f, phi, 0.25f);
            save.camera_theta     = cycle01(theta);
            save.camera_distance += delta.z;
            
            DEBUG_VALUE(save.camera_phi);
            DEBUG_VALUE(save.camera_theta);
        }
        
        v3 camz;
        camz.z = sin_turn(save.camera_phi);
        v1 cos_phi = cos_turn(save.camera_phi);
        camz.x = cos_phi*cos_turn(save.camera_theta);
        camz.y = cos_phi*sin_turn(save.camera_theta);
        v1 cam_distance = U * save.camera_distance;
        
        setup_camera_from_data(camera, camz, cam_distance);
        
        DEBUG_VALUE(camera->distance);
        DEBUG_VALUE(camera->z);
    }
    
    { // push coordinate system
        v2 pO_screen, px_screen, py_screen, pz_screen;
        {
            v3 pO = v3{0,0,0};
            v3 px = U * v3{1,0,0};
            v3 py = U * v3{0,1,0};
            v3 pz = U * v3{0,0,1};
            //
            pO_screen = perspective_project(camera, pO);
            px_screen = perspective_project(camera, px);
            py_screen = perspective_project(camera, py);
            pz_screen = perspective_project(camera, pz);
        }
        
        v2 O = {};
        f32 line_thickness = 8.0f;
        draw_line(app, O+pO_screen, O+px_screen, line_thickness, pack_argb({.5,  0,  0, 1}));
        draw_line(app, O+pO_screen, O+py_screen, line_thickness, pack_argb({ 0, .5,  0, 1}));
        draw_line(app, O+pO_screen, O+pz_screen, line_thickness, pack_argb({ 0,  .5, 1, 1}));
    }
   
    const u32 bezier_count = 3;
    {// NOTE: bezier curve experiment!
        local_persist u32 hot_item    = 0;
        local_persist b32 is_editing  = false;
        
        if (view_is_active(app, view))
        {
            if ( is_key_newly_pressed(active_modifiers, 0, KeyCode_E) )
            {
                is_editing = !is_editing;
            }
            
            if (!is_editing)
            {
                if ( is_key_newly_pressed(active_modifiers, 0, KeyCode_L) )
                {
                    hot_item = (hot_item + 1) % bezier_count;
                }
                if ( is_key_newly_pressed(active_modifiers, 0, KeyCode_H) )
                {
                    if (hot_item == 0) hot_item = bezier_count;
                    hot_item--;
                }
            }
        }
        
        for_u32 (curve_index, 0, bezier_count)
        {
            if (is_editing && hot_item == curve_index)
            {
                v3 direction = fui_direction_from_key_states().xyz;
                v3 delta = matvmul3(&camera->axes, direction) * dt;
                
                local_persist i32 active_cp_index = 0;
#define Down(N) global_key_states[KeyCode_##N] != 0
                if (Down(0)) active_cp_index = 0;
                if (Down(1)) active_cp_index = 1;
                if (Down(2)) active_cp_index = 2;
                if (Down(3)) active_cp_index = 3;
#undef Down
                save.bezier_curves[curve_index].control_points[active_cp_index] += delta;
            }
           
            v3 control_points[4];
            {
                block_copy_array(control_points, save.bezier_curves[curve_index].control_points);
                f32 pos_unit = 500;
                for_i32 ( index, 0, alen(control_points) )
                {
                    control_points[index] *= pos_unit;
                }
            }
            
            // NOTE: Draw
            draw_cubic_bezier(app, camera, control_points);
        }
        
        {// NOTE: UI situation
            v2 dim = V2(100,100);
            v2 at  = V2(-clip_half_dim.x, clip_half_dim.y-dim.y);
            // NOTE: The active item will be highlighted
            for_u32 (curve_index, 0, bezier_count)
            {
                rect2 rect = rect2_min_dim(at, dim);
                u32 color = gray_argb;
                if (curve_index == hot_item)
                {
                    color = (is_editing) ? red_argb : white_argb;
                }
                draw_rect_outline(app, rect, 4.0f, color);
                at.y -= dim.y;
            }
        }
    }
     
    fslider(software_rendering_slider, 0.0f);
    if (software_rendering_slider > 0)
    {// NOTE: software rendering experiments
        v3i dim = {768, 768, 1};
        local_persist b32 initial = true;
        local_persist u32 *data = 0;
        local_persist Texture_ID game_texture = {};
        Model model = {};
        b32 draw_mesh = def_get_config_b32(vars_intern_lit("draw_mesh"));
        if (initial)
        {
            initial = false;
            if (draw_mesh)
            {
                char *filename = "C:/Users/vodan/4ed/4coder-non-source/tiny_renderer/diablo3_pose.obj";
                model = new_model(filename);
            }
            // create game texture
            game_texture = graphics_get_texture(dim, TextureKind_ARGB);
            graphics_set_game_texture(game_texture);
            // allocate memory
            umm size = 4 * dim.x * dim.y * dim.z;
            data = (u32 *)malloc(size);
            block_zero(data, size);
        }
        
        tiny_renderer_main(dim.xy, data, &model);
        graphics_fill_texture(TextureKind_ARGB, game_texture, v3i{}, dim, data);
        
        v2 position = clip.min + v2{10,10};
        fslider( scale, 0.338275f );
        v2 draw_dim = scale * castV2(dim.x, dim.y);
        draw_textured_rect(app, rect2_min_dim(position, draw_dim));
    }
    
    draw_set_y_down(app);
    draw_set_offset(app, v2{});
    
    block_zero_array(global_game_key_state_changes);
}
