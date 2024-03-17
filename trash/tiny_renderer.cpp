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

force_inline u64
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
    
    fuivar( draw_outline, v1{-0.298632f} );
    if (draw_outline > 0)
    {
        tr_line(bitmap, (p1), (p2), argb_red); 
        tr_line(bitmap, (p0), (p2), argb_green); 
        tr_line(bitmap, (p0), (p1), argb_blue); 
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
        bboxmin.x = clamp_min(macro_min(bboxmin.x, (i32)p[i].x), 0);
        bboxmin.y = clamp_min(macro_min(bboxmin.y, (i32)p[i].y), 0);
        
        bboxmax.x = clamp_max(macro_max(bboxmax.x, (i32)p[i].x+1), max.x);
        bboxmax.y = clamp_max(macro_max(bboxmax.y, (i32)p[i].y+1), max.y);
    } 
    for_i32 (x, bboxmin.x, bboxmax.x)
    { 
        for_i32 (y, bboxmin.y, bboxmax.y)
        {
            v3 bc_screen  = barycentric(p, castvec2(x,y));
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
    tr_rectangle2i(bitmap, rect2i{.p0={0,0}, .p1=dim}, argb_black);
    
    {// NOTE(kv): outline
        f32 X = (f32)dim.x-1;
        f32 Y = (f32)dim.y-1;
        tr_line(bitmap, 0,0, X,0, argb_red);
        tr_line(bitmap, 0,0, 0,Y, argb_red);
        tr_line(bitmap, X,0, X,Y, argb_red);
        tr_line(bitmap, 0,Y, X,Y, argb_red);
    }
   
    f32 dim_x = (f32)dim.x;
    f32 dim_y = (f32)dim.y;
   
    if (1)
    {
        v2 beg = v2{0,0};
        v2 mid = v2{0, dim_y-1};
        v2 end = vec2(dim_x-1, dim_y-1);
        v1 step = {1.0f / dim_x};  // NOTE: I don't know what I'm doing!
        for (f32 t = 0;
             t <= 1.0f;
             t += step)
        {
            v2 p1 = lerp(beg, t, mid);
            v2 p2 = lerp(mid, t, end);
            v2 pos = lerp(p1, t, p2);
            bitmap_set(bitmap, (i32)pos.x, (i32)pos.y, argb_white);
        }
    }
}
