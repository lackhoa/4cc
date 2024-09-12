
    if (1)
    {// test srgb
        // background
        draw_rect(app, rect2_min_dim(vec2(0,0), vec2(400,400)), 0xff000000);
        // 50%-blended white
        draw_rect(app, rect2_min_dim(vec2(0,0), vec2(200,200)), 0x80ffffff);
        // midpoint gray in srgb value (which isn't even the correct blending)
        // draw_rect(app, rect2_min_dim(vec2(200,0), vec2(200,200)), 0xff808080);
        // linear blend
        draw_rect(app, rect2_min_dim(vec2(0,200), vec2(200,200)), 0xffbababa);
        
        //c 10 + 11*16
        
        // defcolor_base
        //draw_rect(app, rect2_min_dim(vec2(200,200), vec2(200,200)), 0xff777700);
        
        // a*x / (1-a)*y
        // (x^2.2) / (1-a)*(y^2.2))^0.4545
        
        // x/(foreground) should be 1, y/(background) should be 0
        // blended color in linear space is: 0.501961
        // blended color in srgb space is:   0.731062 -> 186 -> 0xBA
        // blended color in clip space 
        
        // if done wrong: 0.5 -> 0x80
        //c 0.5*100
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
        
        v2 position = v2{10,10};
        fslider( scale, 0.338275f );
        v2 draw_dim = scale * castvec2(dim.x, dim.y);
        draw_textured_rect(app, rect2_min_dim(position, draw_dim));
    }


            if(0)
            {// NOTE: Experimenting with double-tap
                u32 init  =0;
                u32 down0 =1;
                u32 up0   =2;
                u32 down1 =3;
                v1 TAP_INTERVAL = 0.25f;
                u32 left  = 1;
                u32 right = 2;
                u32 up    = 3;
                u32 down  = 4;
                
                local_persist u32 tap_state = init;
                local_persist v1 state_age = 0.f;
                local_persist u32 direction = 0;
                u32 new_state;  // NOTE: Let the compiler force us to init the variable
                if ((state_age > TAP_INTERVAL) && (tap_state != init))
                {
                    new_state = init;
                }
                else
                {
                    u32 new_direction = 0;
#define DOWN(code) is_key_down(input, KeyMod_Ctl, KeyCode_##code)
                    //
                    if ( DOWN(H) ) { new_direction = left;  }
                    if ( DOWN(L) ) { new_direction = right; }
                    if ( DOWN(K) ) { new_direction = up;    }
                    if ( DOWN(J) ) { new_direction = down;  }
#undef DOWN
                    if ( new_direction != 0 )
                    {// note: new direction
                        if (direction == new_direction)
                        {
                            if      (tap_state == init) { new_state = down0; }
                            else if (tap_state == up0)  { new_state = down1; }
                            else                        { new_state = tap_state; }
                        }
                        else
                        {
                            new_state = down0; 
                        }
                        direction = new_direction;
                    }
                    else
                    {// note: is up
                        if      (tap_state == init)  { new_state = init; }
                        else if (tap_state == up0)   { new_state = tap_state; }
                        else if (tap_state == down0) { new_state = up0; }
                        else
                        {// NOTE: Action!
                            kv_assert(tap_state == down1);
                            v1 theta = save->camera_theta;
                            v1 phi   = save->camera_phi;
                            if (direction == left)
                            {
                                save->camera_theta = floorv1(theta*4.f) * .25f;
                            }
                            else if (direction == right)
                            {
                                save->camera_theta = ceilv1(theta*4.f) * .25f;
                            }
                            else if (direction == up)
                            {
                                save->camera_phi = (phi < 0.f) ? 0.f : +.25f;
                            }
                            else if (direction == down)
                            {
                                save->camera_phi = (phi > 0.f) ? 0.f : -.25f;
                            }
                            state->camera_animation_state = 
                            {
                                true,
                                dt,
                                phi,
                                theta,
                                save->camera_phi,
                                save->camera_theta,
                            };
                            new_state = init;
                        }
                    }
                }
                
                state_age = (new_state == tap_state ? 
                             state_age + raw_dt : 
                             0.f);
                tap_state = new_state;
                if (tap_state == init)
                {
                    direction = 0;
                }
            }

function void
clipboard_collection_render(Application_Links *app, Frame_Info frame_info, View_ID view)
{
    Scratch_Block scratch(app);
    Rect_f32 region = draw_background_and_margin(app, view);
    Vec2_f32 mid_p = (region.p1 + region.p0)*0.5f;
    
    Fancy_Block message = {};
    Fancy_Line *line = push_fancy_line(scratch, &message);
    push_fancy_string(scratch, line, fcolor_id(defcolor_pop2),
                      string_u8_litexpr("Collecting all clipboard events "));
    push_fancy_string(scratch, line, fcolor_id(defcolor_pop1),
                      string_u8_litexpr("press [escape] to stop"));
    
    for (Node_String_Const_u8 *node = clipboard_collection_list.first;
         node != 0;
         node = node->next){
        line = push_fancy_line(scratch, &message);
        push_fancy_string(scratch, line, fcolor_id(defcolor_text_default), node->string);
    }
    
    Face_ID face_id = get_face_id(app, 0);
    Vec2_f32 dim = get_fancy_block_dim(app, face_id, &message);
    Vec2_f32 half_dim = dim*0.5f;
    draw_fancy_block(app, face_id, fcolor_zero(), &message, mid_p - half_dim);
}

CUSTOM_UI_COMMAND_SIG(begin_clipboard_collection_mode)
CUSTOM_DOC("Allows the user to copy multiple strings from other applications before switching to 4coder and pasting them all.")
{
    local_persist b32 in_clipboard_collection_mode = false;
    if (!in_clipboard_collection_mode){
        in_clipboard_collection_mode = true;
        system_set_clipboard_catch_all(true);
        
        Scratch_Block scratch(app);
        block_zero_struct(&clipboard_collection_list);
        
        View_ID view = get_this_ctx_view(app, Access_Always);
        View_Context ctx = view_current_context(app, view);
        ctx.render_caller = clipboard_collection_render;
        ctx.hides_buffer = true;
        View_Context_Block ctx_block(app, view, &ctx);
        
        for (;;){
            User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
            if (in.abort){
                break;
            }
            if (in.event.kind == InputEventKind_KeyStroke && in.event.key.code == Key_Code_Escape){
                break;
            }
            if (in.event.kind == InputEventKind_Core &&
                in.event.core.code == CoreCode_NewClipboardContents){
                String_Const_u8 stable_clip = clipboard_post_internal_only(0, in.event.core.string);
                string_list_push(scratch, &clipboard_collection_list, stable_clip);
            }
        }
        
        block_zero_struct(&clipboard_collection_list);
        
        system_set_clipboard_catch_all(false);
        in_clipboard_collection_mode = false;
    }
}

#if 0
    b32 editing_active = input.game_active && widget_state->is_editing;
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
#endif
   
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
    
    if (input.game_active)
    {
        if ( is_key_newly_pressed_no_mod(input, Key_Code_E) )
        {
            widget_state->is_editing = !widget_state->is_editing;
        }
        
        if (!widget_state->is_editing)
        {// NOTE: Change hot item
            Widget *parent = widget_state->hot_item->parent;
            Widget *last_child = parent->children + parent->children_count-1;
            if ( is_key_newly_pressed_no_mod(input, Key_Code_L) )
            {
                widget_state->hot_item++;
                if (widget_state->hot_item > last_child)
                {
                    widget_state->hot_item = parent->children;
                }
            }
            if ( is_key_newly_pressed_no_mod(input, Key_Code_H) )
            {
                widget_state->hot_item--;
                if (widget_state->hot_item < parent->children)
                {
                    widget_state->hot_item = last_child;
                }
            }
        }
    }

#if 0
    {// NOTE: Drawing our widgets
        v2 dim = vec2(100,100);
        v2 at  = vec2(-clip_half_dim.x, clip_half_dim.y-dim.y);
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
                    init_control_points[i][j] = U*0.4f*vec3((v1)i, (v1)j, 0);
                }
            }
        }
        
        local_persist u32 active_i = 0;
        local_persist u32 active_j = 0;
        
        b32 editing_surface = (editing_active &&
                               widget_state->hot_item == state->surface_widget);
        if (editing_surface)
        {// NOTE: pick control points
            if (is_key_newly_pressed(active_mods,0,Key_Code_Tab))
            {
                active_j++;
                if (active_j > 3)
                {
                    active_j = 0;
                    active_i++;
                    if (active_i > 3) active_i = 0;
                }
            }
            else if (is_key_newly_pressed(active_mods,Key_Mod_Sft,Key_Code_Tab))
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


        if (0)
        {
            for_i32 (iphi, -nsegment/4, nsegment/4)
            {// NOTE: Draw quads on the surface of the sphere
                for_i32 (itheta, 0, nsegment)
                {
#define SPHERE(ITHETA, IPHI) perspective_project(camera, point_on_sphere_grid(nsegment, U, ITHETA, IPHI))
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
        
internal void
draw_line_inner(App *app, v3 *points, v1 thickness, ARGB_Color color, v1 interval)
{
    macro_clamp_bot(interval, 1.f);
    v1 half_thickness = clamp_bot(0.5f*thickness, 1.0f);
    Render_Target *target = draw_get_target(app);
    b32 steep = false;
    // NOTE: If z (i.e "depth") is linear then this the interpolation works.
    v1 x0 = points[0].x; v1 y0 = points[0].y; v1 z0 = points[0].z; 
    v1 x1 = points[1].x; v1 y1 = points[1].y; v1 z1 = points[1].z;
    
    if (absolute(x0-x1) < 
        absolute(y0-y1))
    {// if the line is steep, we transpose the image 
        macro_swap(x0, y0);
        macro_swap(x1, y1);
        steep = true;
    }
    
    if (x0 > x1)
    {// make it "left to right"
        macro_swap(x0, x1); 
        macro_swap(y0, y1); 
        macro_swap(z0, z1);
    }
    
    if ((x1-x0) > 0.0001f)
    {
        v1 dy = (y1-y0) / (x1-x0);
        v1 dz = (z1-z0) / (x1-x0);
        
        // NOTE: Clipping
        v1 xstart = x0;
        v1 xend   = x1;
        {
            v1 xbot;
            v1 xtop;
            if (steep)
            {
                xbot = (target->clip_box.y0 - target->offset.y);
                xtop = (target->clip_box.y1 - target->offset.y);
            }
            else
            {
                xbot = (target->clip_box.x0 - target->offset.x);
                xtop = (target->clip_box.x1 - target->offset.x);
            }
            macro_clamp_bot(xstart, xbot);
            macro_clamp_top(xend,   xtop);
        }
        
        for (v1 x = xstart; 
             x <= xend;
             x += interval)
        {
            v1 y = y0 + dy*(x-x0);
            v1 z = z0 + dz*(x-x0);
            v2 center = (steep ?
                         v2{y,x} : 
                         v2{x,y});
            rect2 square = rect2_center_radius(center, vec2_all(half_thickness));
            draw_rect_to_target(target, square, half_thickness, color, z);
        }
    }
}

                if (group->is_perspective)
                {// NOTE: Someday when we draw lots of triangles, we may need this, that day is not today!
                   
                    m4x4 camera_project =
                    {
                        {
                            vec4(camera->px,0),
                            vec4(camera->py,0),
                            vec4(camera->pz,0),
                            vec4(0,0,0,1),
                        }
                    };
                    
                    m4x4 z_to_depth =
                    {
                        {
                           vec4(1,0,0,0),
                           vec4(0,1,0,0),
                           vec4(0,0,-1,camera->distance),
                           vec4(0,0,0,1),
                        } 
                    };
                    
                    v1 focal = camera->focal_length;
                    v1 n  = 10.0f;         // NOTE: in depth
                    v1 f  = 100.0f * 1E3;  // NOTE: in depth
                   
                    m4x4 perspective_project =
                    {
                        {
                            focal, 0,     0,            0,
                            0,     focal, 0,            0,
                            0,     0,     (n+f)/(f-n),  -2*f*n/(f-n),
   0,     0,     1,            0,
  },
 };
 
 m4x4 camera_project_and_z_to_depth = matmul4(&z_to_depth, &camera_project);
 m4x4 camera_project_and_z_to_depth_and_perspective = matmul4(&perspective_project, &camera_project_and_z_to_depth);
 view_m = matmul4(&view_m, &camera_project_and_z_to_depth_and_perspective);
 
 if (0)
 {
  v1 U = 1e3;
  v4 O = v4{0,0,0,1};
  v4 O1 = camera_project * O;
  v4 O2 = z_to_depth * O1;
  v4 O3 = perspective_project * O2;
  O3 /= O3.w;
  
  v4 point = U * v4{0,0,1,1};
  v4 point1 = camera_project * point;
  v4 point2 = z_to_depth * point1;
  v4 point3 = perspective_project * point2;
  point3 /= point3.w;
  
  breakhere;
 }
 
 
 
#if 1
 struct bucket_array_bucket
 {
  bucket_array_bucket *next;
  bucket_array_bucket *prev;
  i32 count;
  
  u8  pad[4];
  u8  items[0];
 };
 // NOTE(kv): Probably wouldn't wanna use this type with simd anyway
 static_assert((offsetof(bucket_array_bucket, items) & 7) == 0);
 
 struct bucket_array_void
 {
  Arena *arena;
  bucket_array_bucket *first_bucket;
  bucket_array_bucket *last_bucket;
  i32 bucket_count;
  i32 bucket_cap;
  i32 item_size;
  //-
  
  inline i32 item_count()
  {
   i32 result = 0;
   if (bucket_count > 1) {
    result += (bucket_count-1)*bucket_cap;
   }
   result += last_bucket->count;
   return result;
  }
  
  inline i32 item_cap() {
   return bucket_count * bucket_cap;
  }
  
  void set_count(i32 new_count)
  {
   kv_assert(new_count >= 0);
   i32 cur_count = item_count();
   i32 delta = new_count - cur_count;
   if ( new_count < cur_count )
   {// NOTE(kv): Popping items
    i32 size_to_pop = -delta;
    for_i32(auto bucket = last_bucket;
            bucket;
            bucket = bucket->prev)
    {
     if (bucket->count < size_to_pop) {
      size_to_pop -= bucket->count;
      bucket->count = 0;
     } else {
      bucket->count -= size_to_pop;
      size_to_pop = 0;  // @decor
      break;
     }
    }
    kv_assert(size_to_pop == 0);
   }
   else if (new_count > cur_count)
   {// NOTE(kv): Expanding
    i32 cur_cap = get_cap();
    if (new_count <= cur_cap) {
     // NOTE(kv): Lucky, we're within capacity
     last_bucket->count += delta;
     kv_assert(last_bucket->count <= last_bucket->cap);
    } else {
     // NOTE(kv): Need a new bucket
     delta -= last_bucket->cap - last_bucket->count;
     last_bucket->count = last_bucket->cap;
     {
      i32 new_bucket_cap = clamp_min(delta, 2*last_bucket->cap);
      u8 *new_bucket_memory = push_size(arena, (new_bucket_cap*item_size+
                                                sizeof(bucket_array_bucket)));
      auto new_bucket = (bucket_array_bucket *)new_bucket_memory;
      *new_bucket = {
       .prev  = last_bucket,
       .count = delta,
       .cap   = new_bucket_cap,
      };
      last_bucket->next = new_bucket;
     }
    }
   }
  }
  
  inline void pop() { set_count(count-1); }
  
  void push(T const& item)
  {
   i32 new_count = get_count()+1;
   set_count(new_count);
   last_bucket->items[last_bucket->count] = item;
  }
  
  inline T& operator[](i32 index)
  {
   kv_assert(index >= 0 && index < count());
   i1 index_of_bucket = index / bucket_size;
   i1 index_in_bucket = index % bucket_size;
   auto *bucket = first_bucket;
   for_i1(i, 0, index_of_bucket) {
    bucket = bucket->next;
   }
   return bucket[index_in_bucket];
  }
 };
 //
 template <class T>
  function void
  init(bucket_array<T> &array, Arena *arena, i1 bucket_size)
 {
  // NOTE: Must have at least one bucket!
  array.arena = arena;
  array.bucket_size = bucket_size;
  array.first_bucket = array.last_bucket = &array.new_bucket();
 }
#endif
//