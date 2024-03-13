#define KV_IMPLEMENTATION
#include "kv.h"
#define AD_IS_COMPILING_GAME 1
#define API_USER_SIDE
#include "4coder_game_shared.h"
#include "4ed_fui_user.h"
#include "4coder_kv_debug.cpp"
#include "4coder_fancy.cpp"
#include "4coder_app_links_allocator.cpp"
#include "4ed_render_target.cpp"

#include "time.h"

/*
  NOTE(kv): OLD Rules for the renderer:

  - Textures and bitmaps are srgb premultiplied-alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
  - Packed colors are rgba in memory order (i.e abgr in u32 register order)
  pma = Pre-multiplied alpha
 */

global const u32 data_current_version = 6;
global const u32 data_magic_number = *(u32 *)"kvda";

#define X(N) internal N##_return N(N##_params);
    // note: forawrd declare
    X_GAME_API_FUNCTIONS(X)
    //
#undef X

DLL_EXPORT game_api_export_return 
game_api_export(game_api_export_params)
{
#define X(N) api->N = N;
    //
    X_GAME_API_FUNCTIONS(X)
    //
#undef X
}

struct Bezier
{
    v3 control_points[4];
};

struct Widget
{
    // Widget_ID id;
    String name;
    
    Widget *parent;
    
    u32     children_count;
    Widget *children;
};

struct Widget_State
{
    Widget root;
    Widget *hot_item;
    b32 is_editing;
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
    
    v3 bezier_surface[4][4];
    
    v3 camera_pivot;  // NOTE: It's a v3 in case we wanna track objects, not just pan, then we'd need to change the z too
};

struct Game_State
{
    Arena permanent_arena;
    //Ed_API ed_api;
   
    Game_Save save;
    b32 has_done_backup;
    String save_dir;
    String save_path;
    
    Widget_State widget_state;
    Widget *surface_widget;
    Widget *curve_widgets;
};

struct Game_Save_Old
{
    u32 magic_number;
    u32 version;
    
    v3 camera_z;  // @Old
    v1 camera_distance;
    
    Bezier bezier_curves[3];
    
    v1 camera_phi;
    v1 camera_theta;
    
    v3 bezier_surface[4][4];
};

typedef u32 Widget_ID;


// TODO: @Cleanup: Also with the fcolor
global v4  v4_yellow   = {.5, .5, 0, 1.0};
global u32 argb_yellow = pack_argb(v4_yellow);
global u32 argb_red    = pack_argb({.5,0,0,1});
global u32 argb_green  = pack_argb({0,.5,0,1});
global u32 argb_blue   = pack_argb({0,.5,1,1});
global v4  v4_black    = {0,0,0,1};
global u32 argb_black  = pack_argb(v4_black);
global v4  v4_white    = {1,1,1,1};
global u32 argb_white  = pack_argb(v4_white);
global u32 argb_marble = 0xff616066; // Original 0xffA9A6B7, which is too bright!

force_inline ARGB_Color
argb_gray(v1 value)
{
    return pack_argb(v4{value,value,value,1});
}

force_inline v4
v4_gray(v1 value)
{
    return v4{value,value,value,1};
}

struct Screen
{
    v3 center;
    v3 x_axis;
    v3 y_axis;
};

internal v1
camera_relative_depth(Camera *camera, v3 point)
{
    v1 projected_z = dot(camera->pz, point) - camera->pivot.z;
    // NOTE: Camera distance is in the direction camera Z
    v1 depth = camera->distance - projected_z;
    return depth;
}

internal v3
perspective_project(Camera *camera, v3 point)
{
    v3 projected = matvmul3(&camera->project, point) - camera->pivot;
    // NOTE: Camera distance is in the direction camera Z
    v1 depth = camera->distance - projected.z;
    v1 n = 10;         // NOTE: near clip
    v1 f = 100 * 1E3;  // NOTE: far clip
    if ( n <= depth && depth <= f )
    {
        v2 xy = (camera->focal_length / depth) * projected.xy;
        v1 z  = (n+f - (2*f*n / depth)) / (f-n);
        return V3(xy,z);
    }
    else
        return V3All(F32_MAX);
}

internal v1
cubic_bernstein(u32 index, v1 u)
{
    v1 factor = (index == 1 || index == 2) ? 3 : 1;
    v1 result = factor * pow(u,index) * pow(1-u, 3-index);
    return result;
}

internal void
draw_sampled_bezier(App *app, Camera *camera, v3 P[4], ARGB_Color color)
{
    fslider(radius_a, 9.766055);
    fslider(radius_b, 15.380762);
    
    i32 nslices = 16;
    v1 inv_nslices = 1.0f / (v1)nslices;
    for_i32 (sample_index, 0, nslices+1)
    {
        v1 t_radius = (v1)sample_index * inv_nslices;
        v1 radius = lerp(radius_a,
                         4.f*(-t_radius*t_radius + t_radius),
                         radius_b);
        
        v1 u = inv_nslices * (v1)sample_index;
        v1 U = 1.0f-u;
        v3 world_p = (1*cubed(U)*P[0] + 
                      3*(u)*squared(U)*P[1] +
                      3*squared(u)*(U)*P[2] +
                      1*cubed(u)*P[3]);
        //
        v3 screen_p = perspective_project(camera, world_p);
        
        v1 draw_radius;
        {// NOTE: Our fancyness adjusts the disk radius... not sure if I want this?
            v1 depth = camera_relative_depth(camera, world_p);
            draw_radius = absolute((camera->focal_length / depth) * radius);
        }
        
        draw_disk(app, screen_p, draw_radius, color);
    }
}

internal void
draw_double_bezier(App *app, Camera *camera, v3 P[4], v3 Q[4], ARGB_Color color)
{
    i32 nslices = 16;
    v1 inv_nslices = 1.0f / (v1)nslices;
    v3 previous_screenP;
    v3 previous_screenQ;
    for_i32 (sample_index, 0, nslices+1)
    {
        v1 u = inv_nslices * (v1)sample_index;
        v1 U = 1.0f-u;
        v3 worldP = (1*cubed(U)*P[0] + 
                     3*(u)*squared(U)*P[1] +
                     3*squared(u)*(U)*P[2] +
                     1*cubed(u)*P[3]);
        v3 worldQ = (1*cubed(U)*Q[0] + 
                     3*(u)*squared(U)*Q[1] +
                     3*squared(u)*(U)*Q[2] +
                     1*cubed(u)*Q[3]);
        //
        v3 screenP = perspective_project(camera, worldP);
        v3 screenQ = perspective_project(camera, worldQ);
        if (sample_index != 0)
        {
            draw_quad(app, previous_screenP, previous_screenQ, screenP, screenQ, color);
        }
        previous_screenP = screenP;
        previous_screenQ = screenQ;
    }
}

internal void
draw_bezier_surface(App *app, Camera *camera, v3 P[4][4], 
                    u32 active_i, u32 active_j)
{
    if (0)
    {// NOTE: draw control points
        for_u32 (i,0,4)
        {
            for_u32 (j,0,4)
            {
                v3 screen_p = perspective_project(camera, P[i][j]);
                draw_circle(app, screen_p.xy, 6.0f, argb_gray(.5), screen_p.z);
            }
        }
    }
    
    u32 nslices = 16;
    v1 inv_nslices = 1.0f / (f32)nslices;
    for_u32 (u_index, 0, nslices+1)
    {
        for_u32 (v_index, 0, nslices+1)
        {
            v1 u = inv_nslices * (v1)u_index;
            v1 v = inv_nslices * (v1)v_index;
            v3 world_p = {};
            for_u32 (i,0,4)
            {
                for_u32 (j,0,4)
                {
                    world_p += (cubic_bernstein(i,u) * cubic_bernstein(j,v) *
                                P[i][j]);
                }
            }
            v3 screen_p = perspective_project(camera, world_p);
            
            v1 radius = 12.0f;
            v1 draw_radius;
            {// NOTE: calculate draw radius @Slow
                v1 projected_z = camera_relative_depth(camera, world_p);
                v1 dz = camera->distance - projected_z;
                draw_radius = absolute((camera->focal_length / dz) * radius);
            }
        
            v1 lightness = 0.0f;
            {
                v1 quadrance = (squared((v1)active_i - 3*u) +
                                squared((v1)active_j - 3*v));
                if (quadrance < 1.0f)
                {
                    lightness = 1.0f - quadrance;  // so it's kinda like Lambert's light law
                }
            }
            v4 color = lerp(v4_gray(.5), lightness, v4_white);
            draw_circle(app, screen_p.xy, draw_radius, pack_argb(color), screen_p.z);
        }
    }
}

internal b32
save_game(App *app, Game_State *state)
{
    Scratch_Block scratch(app);
    String backup_dir = pjoin(scratch, state->save_dir, "backups");
    
    b32 ok = true;
    if (!state->has_done_backup)
    {
        // NOTE: backup game if is_first_write_since_launched
        time_t rawtime;
        time(&rawtime);
        struct tm *timeinfo = localtime(&rawtime);
        char time_string[256];
        size_t strftime_result = strftime(time_string, sizeof(time_string), "%d_%m_%Y_%H_%M_%S", timeinfo);
        if (strftime_result == 0)
        {
            print_message(app, str8lit("strftime failed... go figure that out!\n"));
            ok = false;
        }
        else
        {
            String backup_path = push_stringf(scratch, "%.*s/data_%s.kv", string_expand(backup_dir), time_string);
            ok = move_file(state->save_path, backup_path);
            if (ok) state->has_done_backup = true;
        }
        
        if (ok)
        {// NOTE: cycle out old backup files
            File_List backup_files = system_get_file_list(scratch, backup_dir);
            if (backup_files.count > 100)
            {
                u64 oldest_mtime = U64_MAX;
                String file_to_delete = {};
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
        ok = write_entire_file(scratch, state->save_path, &state->save, sizeof(state->save));
        if (ok)
            vim_set_bottom_text(str8lit("Saved game state!"));
        else
            printf_message(app, "Failed to write to file %.*s", string_expand(state->save_path));
    }
    
    return ok;
}

internal b32
is_key_newly_pressed(Game_Input input, Key_Mod modifiers, Key_Code keycode)
{
    if (input.active_mods == modifiers)
    {
        return (input.key_states       [keycode] &&
                input.key_state_changes[keycode] > 0);
    }
    else return false;
}

global const v1 widget_margin = 5.0f;

internal rect2 draw_single_widget(App *app, Widget_State *state, Widget *tree, v2 top_left);

internal rect2
draw_sibling_widgets(App *app, Widget_State *state, Widget *siblings, u32 sibling_count, v2 top_left)
{
    v2 min = top_left;
    v2 max = top_left;
    for_u32 (index,0,sibling_count)
    {
        Widget *widget = siblings + index;
        rect2 rect = draw_single_widget(app, state, widget, min);
        
        max.x = macro_max(rect.max.x, max.x);
        min.y = rect.min.y - widget_margin;
    }
    return rect2_min_max(min, max);
}

force_inline rect2 
draw_widget_children(App *app, Widget_State *state, Widget *parent, v2 top_left)
{
    return draw_sibling_widgets(app, state, parent->children, parent->children_count, top_left);
}

internal rect2
draw_single_widget(App *app, Widget_State *state, Widget *widget, v2 top_left)
{
    Scratch_Block scratch(app);
   
#if 0
    Face_ID face = get_face_id(app,0);
    Face_Metrics face_metrics = get_face_metrics(app, face);
    
    Fancy_Line line_value = {}; 
    Fancy_Line *line = &line_value;
    push_fancy_string(scratch, line, f_white, widget->name);
    // 
    v2 name_dim = V2(get_fancy_line_width(app, face, line),
                     face_metrics.line_height);
    // 
    draw_fancy_line(app, face, fcolor_zero(), line, widget_min);
#else
    v2 name_dim = V2(50.0f, 50.0f);
    v2 widget_min = V2(top_left.x, top_left.y-name_dim.y);
    draw_rect2(app, rect2_min_dim(widget_min, name_dim), argb_gray(.5));
#endif
   
    fslider(widget_indentation, v1{10.0f});
    v2 children_top_left = V2(widget_min.x + widget_indentation,
                              widget_min.y);
    rect2 children = draw_widget_children(app, state, widget, children_top_left);
    //
    v2 whole_max = V2(macro_max(widget_min.x + name_dim.x, children.max.x),
                      widget_min.y + name_dim.y + widget_margin);
    //
    widget_min.y = children.min.y - widget_margin;
   
    rect2 whole_box = rect2_min_max(widget_min, whole_max);
    if (widget == state->hot_item)
    {
        u32 color = argb_yellow;
        if (state->is_editing)  color = argb_red;
        v1 thickness = 2.0f;
        draw_rect_outline2(app, whole_box, thickness, color);
    }
    
    return whole_box;
}


inline v3
point_on_sphere(v1 radius, v1 theta, v1 phi)
{
    v2 xy = cos_turn(phi) * arm2(theta);
    return radius * V3(xy, sin_turn(phi));
}

internal v3
point_on_sphere_grid(i32 nsegment, v1 radius, i32 itheta, i32 iphi)
{
    v1 segment = 1/(v1)nsegment;
    v1 theta = segment*(v1)itheta;
    v1 phi   = segment*(v1)iphi;
    return point_on_sphere(radius, theta, phi);
}

force_inline void
draw_dense_line(App *app, v3 p0, v3 p1, v3 half_thickness, u32 color)
{
    draw_quad(app, 
              p0-half_thickness,
              p0+half_thickness,
              p1-half_thickness,
              p1+half_thickness,
              color);
}

#if 0
internal void
ed_api_import(Ed_API *api)
{
#define X(N) N = api->N;
    // 
    X_ED_API_FUNCTIONS(X)
    //
#undef X
}
#endif

internal game_init_return
game_init(game_init_params)
{
    //ed_api_import(ed_api);
    
    Game_State *state = push_struct(bootstrap_arena, Game_State);
    state->permanent_arena = *bootstrap_arena;
    Arena *permanent_arena = &state->permanent_arena;
    //state->ed_api = *ed_api;
        
    {// NOTE: Save data business
        Scratch_Block scratch(app);
        String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
        state->save_dir  = pjoin(permanent_arena, binary_dir, "data");
        state->save_path = pjoin(permanent_arena, state->save_dir, "data.kv");
        
        String read_string = read_entire_file(scratch, state->save_path);
        Game_Save *read = (Game_Save *)read_string.str;
        if (read->magic_number == data_magic_number)
        {
            if (read->version == data_current_version)
            {
                state->save = *read;
            }
            else if ( read->version == (data_current_version-1) )
            {
                print_message(app, strlit("Game data load: Converting old version to new version!\n"));
                Game_Save_Old *old = (Game_Save_Old *)read;
                // NOTE(kv): Conversion code!
                state->save = *(Game_Save *)old;
                
                state->save.version = data_current_version;
                // NOTE(kv): Write back the converted file.
                save_game(app, state);
            }
            else print_message(app, strlit("Game data load: Unknown version\n"));
        }
        else print_message(app, strlit("Game data load: Wrong magic number!\n"));
    }
    
    {// NOTE: widget init
        Widget_State *widget_state = &state->widget_state;
        // TODO: We should make this a parsed data structure if we decide to use this system
        u32 WIDGET_COUNT = 4;
        
        Widget *widgets = push_array(permanent_arena, Widget, WIDGET_COUNT);
        
        Widget *root = &widget_state->root;
        *root = {str8lit("root"),0, WIDGET_COUNT,widgets};
        
        widget_state->hot_item = root->children;
        
        // NOTE: surface
        Widget *surface = widgets + 0;
        *surface        = {str8lit("surface"),root, 0,0};
        state->surface_widget = surface;
        
        // NOTE: Bezier curve
        Widget *curves = widgets + 1;
        for_u32 (curve_index,0,3)
        {
            curves[curve_index] = {str8lit("curve"),root, 0,0};
        }
        state->curve_widgets = curves;
    }
    
    return state;
}

force_inline v4
direction_from_input(Game_Input input, Key_Mod wanted_mods)
{
    return fui_direction_from_key_states(input.active_mods, input.key_states, wanted_mods);
}

// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
game_update_and_render_return
game_update_and_render(game_update_and_render_params)
{
    b32 view_active = view_is_active(app, view);
    
    if (view_active)
    {// NOTE: Enable animation if ANY key is down
        for_i32 (keycode, 1, KeyCode_COUNT)
        {
            if ( input.key_states[keycode] )
            {
                animate_next_frame(app);
                break;
            }
        }
    }
  
    Arena *permanent_arena     = &state->permanent_arena;
    Widget_State *widget_state = &state->widget_state;
    
    Scratch_Block scratch(app);
    u32 paint_color = argb_gray(.3f);
    
    if (view_active)
    {
        if ( is_key_newly_pressed(input, KeyMod_Ctl, KeyCode_Tab) )
        {
            change_active_primary_panel(app);
        }
    }
    
    v1 U = 1e3;  // render scale multiplier (@Cleanup push this scale down to the renderer)
    
    Game_Save *save = &state->save;
    if ( is_key_newly_pressed(input, 0, KeyCode_Return) )
    {
        save_game(app, state);
    }
    
    Camera camera_value = {};
    Camera *camera = &camera_value;
    {// NOTE: Camera handling
        camera->focal_length = 2000.f;
        
        if (view_active)
        {// NOTE: Camera rotation
            v3 delta_pivot = U * dt * direction_from_input(input, KeyMod_Alt).xyz;
            v3 delta_angle = dt * direction_from_input(input, KeyMod_Ctl).xyz;
            
            save->camera_pivot += delta_pivot;
            
            v1 phi   = save->camera_phi   + 0.1*delta_angle.y;
            v1 theta = save->camera_theta + 0.1*delta_angle.x;
            
            save->camera_phi       = clamp_between(-0.25f, phi, 0.25f);
            save->camera_theta     = cycle01(theta);
            save->camera_distance += delta_angle.z;
        }
        
        v3 camz;
        camz.y = sin_turn(save->camera_phi);
        v1 cos_phi = cos_turn(save->camera_phi);
        v1 z_point = cos_turn(save->camera_theta);
        v1 x_point = sin_turn(save->camera_theta);
        camz.z = cos_phi * z_point;
        camz.x = cos_phi * x_point;
        
        camera->distance = U * save->camera_distance;
        
        camera->px = v3{z_point, 0, -x_point};
        camera->py = noz( cross(camz, camera->px) );
        camera->pz = camz;
        camera->pivot = save->camera_pivot;
        
        DEBUG_VALUE(camera->pivot);
    }
   
    Render_Config old_render_config = draw_get_target(app)->render_config;
    v2 clip_radius;
    {
        // NOTE: Setup sane math coordinate system.
        rect2 clip_box = draw_get_clip(app);
        {// NOTE: draw background
            draw_rect2(app, clip_box, argb_marble);
        }
        v2 clip_dim = rect_dim(clip_box);
        clip_radius = 0.5f * clip_dim;
        v2 layout_center = clip_box.min + clip_radius;
        Render_Config config = 
        {
            .offset             = layout_center,
            .y_is_up            = true,
            .is_perspective     = false,
            .camera             = *camera,
            .depth_test         = true,
            .linear_alpha_blend = true,
        };
        draw_configure(app, &config);
    }
   
    if(0)
    {// NOTE: Draw coordinate axes
        v3 pO, px, py, pz;
        {
            v3 world_pO = v3{0,0,0};
            v3 world_px = U * v3{1,0,0};
            v3 world_py = U * v3{0,1,0};
            v3 world_pz = U * v3{0,0,1};
            //
            pO = perspective_project(camera, world_pO);
            px = perspective_project(camera, world_px);
            py = perspective_project(camera, world_py);
            pz = perspective_project(camera, world_pz);
        }
        
        v1 thickness = 8.0f;
        draw_line(app, pO, px, thickness, argb_red);
        draw_line(app, pO, py, thickness, argb_green);
        draw_line(app, pO, pz, thickness, argb_blue);
        
        draw_triangle(app, pO, py, pz, argb_gray(.3));
    }
    
    const u32 bezier_count = 3;
    
    if (view_active)
    {
        if ( is_key_newly_pressed(input, 0, KeyCode_E) )
        {
            widget_state->is_editing = !widget_state->is_editing;
        }
        
        if (!widget_state->is_editing)
        {// NOTE: Change hot item
            Widget *parent = widget_state->hot_item->parent;
            Widget *last_child = parent->children + parent->children_count-1;
            if ( is_key_newly_pressed(input, 0, KeyCode_L) )
            {
                widget_state->hot_item++;
                if (widget_state->hot_item > last_child)
                {
                    widget_state->hot_item = parent->children;
                }
            }
            if ( is_key_newly_pressed(input, 0, KeyCode_H) )
            {
                widget_state->hot_item--;
                if (widget_state->hot_item < parent->children)
                {
                    widget_state->hot_item = last_child;
                }
            }
        }
    }
    
    b32 editing_active = view_active && widget_state->is_editing;
    {// NOTE: bezier curve experiment!
        for_u32 (curve_index, 0, bezier_count)
        {
            Bezier *curve = save->bezier_curves + curve_index;
            if (editing_active &&
                widget_state->hot_item == state->curve_widgets + curve_index)
            {
                v3 direction = direction_from_input(input,0).xyz;
                v3 delta = matvmul3(&camera->project, direction) * dt;
                
                local_persist i32 active_cp_index = 0;
                curve->control_points[active_cp_index] += delta;
            }
           
            v3 control_points[4];
            {
                block_copy_array(control_points, curve->control_points);
                f32 pos_unit = 500;
                for_i32 ( index, 0, alen(control_points) )
                {
                    control_points[index] *= pos_unit;
                }
            }
            
            if(0)
            {
                // NOTE: Draw
                draw_sampled_bezier(app, camera, control_points, paint_color);
            }
        }
    }
    
#if 0
    {// NOTE: Drawing our widgets
        v2 dim = V2(100,100);
        v2 at  = V2(-clip_half_dim.x, clip_half_dim.y-dim.y);
        draw_widget_children(app, widget_state, &widget_state->root, at);
    } 
#endif
    
#if 0
    {// NOTE: Bezier surface experiment
        local_persist v3 init_control_points[4][4];
        if ( !state->inited )
        {
            for_u32 (i,0,4)
            {
                for_u32 (j,0,4)
                {
                    init_control_points[i][j] = U*0.4f*V3((v1)i, (v1)j, 0);
                }
            }
        }
        
        local_persist u32 active_i = 0;
        local_persist u32 active_j = 0;
        
        b32 editing_surface = (editing_active &&
                               widget_state->hot_item == state->surface_widget);
        if (editing_surface)
        {// NOTE: pick control points
            if (is_key_newly_pressed(active_mods,0,KeyCode_Tab))
            {
                active_j++;
                if (active_j > 3)
                {
                    active_j = 0;
                    active_i++;
                    if (active_i > 3) active_i = 0;
                }
            }
            else if (is_key_newly_pressed(active_mods,KeyMod_Sft,KeyCode_Tab))
            {
                active_j--;
                if (active_j > 3)
                {
                    active_j = 3;
                    active_i--;
                    if (active_i > 3) active_i = 3;
                }
            }
        }
        
        if (editing_surface)
        {// NOTE: edit direction
            v3 direction = direction_from_input(input, 0).xyz;
            v3 delta = matvmul3(&camera->project, direction) * dt * U;
            save->bezier_surface[active_i][active_j] += delta;
        }
        
        v3 control_points[4][4];
        for_u32 (i,0,4)
        {
            for_u32 (j,0,4)
            {
                control_points[i][j] = (init_control_points[i][j] +
                                        save->bezier_surface[i][j]);
            }
        }
        draw_bezier_surface(app, camera, control_points, active_i, active_j);
    }
#endif
    
    {// NOTE: The human head: revisited!
#define PROJ(v)       perspective_project(camera, radius*(v))
#define PROJV3(x,y,z)  PROJ(V3(x,y,z))
        
        i32 nsegment = 32;
        v1 segment = 1/(v1)nsegment;
        v1 radius = U;
        if(0)
        {
            for_i32 (iphi, -nsegment/4, nsegment/4 + 1)
            {// NOTE: Draw points
                u32 nloop = nsegment;
                if (iphi == -nsegment/4 || iphi == nsegment/4)
                {
                    nloop = 1;
                }
                for_u32 (itheta, 0, nloop)
                {
                    v3 world_pos  = point_on_sphere_grid(nsegment, radius, itheta, iphi);
                    v3 screen_pos = perspective_project(camera, world_pos);
                    u32 color = argb_gray(.5);
                    draw_disk(app, screen_pos, 8.f, color);
                }
            }
        }
        
        if(0)
        {
            for_i32 (iphi, -nsegment/4, nsegment/4)
            {// NOTE: Draw quads on the surface of the sphere
                for_i32 (itheta, 0, nsegment)
                {
#define SPHERE(ITHETA, IPHI) perspective_project(camera, point_on_sphere_grid(nsegment, radius, ITHETA, IPHI))
                    //
                    v3 p0 = SPHERE(itheta+0, iphi+0);
                    v3 p1 = SPHERE(itheta+1, iphi+0);
                    v3 p2 = SPHERE(itheta+0, iphi+1);
                    v3 p3 = SPHERE(itheta+1, iphi+1);
#undef SPHERE
                    {
                        u32 color = pack_argb(v4{.5,.5,.5,.2});
                        draw_quad(app, p0, p1, p2, p3, color);
                    }
                }
            }
        }
        
        {// NOTE: outline
            u32 color = pack_argb(v4{.5,.5,.5,.2});
            v3 center_screen = perspective_project(camera, V3());
            v1 depth = camera_relative_depth(camera, center_screen);
            v1 radius_screen = absolute(camera->focal_length / depth) * radius;
            draw_circle(app, center_screen, radius_screen, color, 8.f);
        }
       
        {// note: active point
            v3 midpoint = perspective_project(camera, U*v3{0,0,1});
            draw_disk(app, midpoint, 8.0f, argb_gray(.3));
        }
        
        v1 sideX = 0.75f;
        {// NOTE: The side cross section
            // We wanna make a cut normal to the x axis, so x has to be constant
            v1 yz_radius = square_root(1-squared(sideX));
            DEBUG_VALUE(sideX);
            for (v1 t=0.0f; 
                 t < 1.0f; 
                 t += (1/128.0f))
            {
                v3 world_p;
                world_p.x = sideX;
                world_p.yz = yz_radius * arm2(t);
                v3 screenP = perspective_project(camera, radius*world_p);
                draw_disk(app, screenP, 4.f, paint_color);
            }
            
            // note: z=0 line
            draw_line(app, PROJV3(sideX,yz_radius,0), PROJV3(sideX,-yz_radius,0), 8.f, paint_color);
            // note: y=0 line
            //draw_line(app, PROJV3(x,0,yz_radius), PROJV3(sideX,0,-yz_radius), 8.f, argb_gray(.3));
            // note: Filling in the cross-section disk
            for (v1 y=-yz_radius;
                 y <= yz_radius;
                 y += (yz_radius / 4.0f))
            {
                v1 z = square_root(1-squared(sideX)-squared(y));
                v3 half_thickness = V3(0,4.f,0);
                v3 p0 = PROJV3(sideX,y,-z);
                v3 p1 = PROJV3(sideX,y,+z);
                draw_dense_line(app,p0,p1,half_thickness,paint_color);
            }
        }
        
        // NOTE: the hairline
        v1 hairY = .5f;
        v3 hairline_pos = PROJV3(0,hairY,square_root(1-squared(hairY)));
        draw_disk(app, hairline_pos, 8.f, paint_color);
        
        // NOTE: The nose line
        v1 noseY = -hairY;
        v3 nose_pos = PROJV3(0.f,noseY,1.f);
        draw_disk(app, nose_pos, 8.f, paint_color);
       
        // NOTE: The chin
        // NOTE: Edge Convention: (outer comes first, inner comes second), outer is always primary
        v1 chinY = -1.f;
        v1 chin_rx = 0.15f;
        v3 chin_right  = V3(+chin_rx,chinY,1.f);
        v3 chin_middle = V3(0,chinY-0.05f,1.0f);
        {
            v3 chin_left = chin_right;
            chin_left.x = -chin_rx;
            v3 chinT = V3(0,0.02f,0);
            v3 mid0 = PROJ(chin_middle);
            v3 mid1 = PROJ(chin_middle+chinT);
            draw_triangle(app, PROJ(chin_right), mid0, mid1, paint_color);
            draw_triangle(app, PROJ(chin_left),  mid0, mid1, paint_color);
        }
        
        // NOTE: Cheek line
        v3 cheek_lower = PROJV3(0.3f,
                                noseY,
                                1.0f);
        v3 cheek_upper = PROJV3(sideX,
                                noseY / 2.f,
                                0.8f);
        draw_dense_line(app,cheek_lower,cheek_upper,V3(4.f,0,0),paint_color);
       
        // NOTE: The line that connects the lower cheek and the chin
        draw_dense_line(app,PROJ(chin_right),cheek_lower,V3(2.f,0,0),paint_color);
       
        v1 mouthY = lerp(chinY, 0.5f, noseY);
        v1 mouth_rx = .5f * chin_rx;
        {// NOTE: The mouth line
            v3 mouth_left  = V3(-mouth_rx,mouthY,1.f);
            v3 mouth_right = mouth_left;
            mouth_right.x = +mouth_rx;
            v3 frownY0 = V3(0,0.05f,0);
            v3 frownY1 = V3(0,0.03f,0);
            v3 P[4] =
            {
                radius*(mouth_left),
                radius*(mouth_left  + frownY0),
                radius*(mouth_right + frownY0),
                radius*(mouth_right),
            };
            v3 Q[4] =
            {
                radius*(mouth_left),
                radius*(mouth_left  + frownY1),
                radius*(mouth_right + frownY1),
                radius*(mouth_right),
            };
            draw_double_bezier(app, camera, P,Q, paint_color);
        }
        
        v3 lower_jaw = V3(sideX, mouthY, 0.2f);
        {
            v3 P[4] =
            {
                radius*chin_right,
                radius*chin_right,
                radius*lower_jaw,
                radius*lower_jaw,
            };
            draw_sampled_bezier(app, camera, P, paint_color);
        }
        
        v3 ear_center = V3(sideX, -0.1f, 0.f);
        {
            v3 P[4] =
            {
                radius*lower_jaw,
                radius*lower_jaw,
                radius*ear_center,
                radius*ear_center,
            };
            draw_sampled_bezier(app, camera, P, paint_color);
        }
        
        {
            v3 chin_center = V3(0,chin_middle.y,1);
            v3 adams_apple = V3(0,chin_middle.y,0.5f); // NOTE: just the location
            v3 P[] =
            {
                radius*chin_center,
                radius*chin_center,
                radius*adams_apple,
                radius*adams_apple,
            };
            v3 dy = v3{ .y=(0.02f) };
            v3 Q[] =
            {
                radius*(chin_center+dy),
                radius*(chin_center+dy),
                radius*(adams_apple+dy),
                radius*(adams_apple+dy),
            };
            draw_double_bezier(app, camera, P,Q, paint_color);
        }
        
#undef PROJ
#undef PROJV3
    }
    
    ////////////////////////////////////////////////////////////////////
    // IMPORTANT: no trespass!
    
    draw_configure(app, &old_render_config);
}
