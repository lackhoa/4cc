#pragma once

#include "4coder_fleury_plot.h"
#include "4coder_fleury_colors.h"

function void
Plot2DBegin(Plot2DInfo *plot)
{
    Scratch_Block scratch(plot->app);
    
    Rect_f32 region = plot->region;
    Rect_f32 plot_view = plot->plot_view;
    ARGB_Color comment_color = fcolor_resolve(defcolor_comment);
    
    { // NOTE(kv): plot labels
        if(plot->title.str)
        {
            Face_Metrics metrics = get_face_metrics(plot->app, plot->title_face);
            v2 pos = V2(region.x0, region.y0 - metrics.line_height);
            draw_string(plot->app, plot->title_face, plot->title, pos, comment_color);
        }
        
        if(plot->x_axis.str)
        {
            draw_string(plot->app, plot->label_face, plot->x_axis, V2(region.x0, region.y1), comment_color);
        }
        
        if(plot->y_axis.str)
        {
            draw_string_oriented(plot->app, plot->label_face, comment_color, plot->y_axis,
                                 V2(region.x0 - 10, region.y0 + 5), 0, V2(0.f, 1.f));
        }
    }
    
    plot->last_clip = draw_set_clip(plot->app, region);
    f32 region_width = region.x1 - region.x0;
    f32 region_height = region.y1 - region.y0;
    draw_rect(plot->app, region, 4.f, fcolor_resolve(defcolor_back), 0);
    
    // NOTE(rjf): Draw grid lines.
    if(plot->mode != Plot2DMode_Histogram)
    {
        ARGB_Color grid_line_color = comment_color;
        grid_line_color &= 0x00ffffff;
        grid_line_color |= 0x91000000;
        
        float tick_increment_x = (plot_view.x1 - plot_view.x0) / 10.f + 1.f;
        float tick_increment_y = (plot_view.y1 - plot_view.y0) / 10.f + 1.f;
        
        tick_increment_x = powf(10.f, floorf(log10f(tick_increment_x)));
        tick_increment_y = powf(10.f, floorf(log10f(tick_increment_y)));
        
		if(tick_increment_x <= 0)
		{
			tick_increment_x = 1;
		}
		if(tick_increment_y <= 0)
		{
			tick_increment_y = 1;
		}
        
        // NOTE(rjf): Draw vertical lines.
        {
            for(float x = plot_view.x0 - fmodf(plot_view.x0, tick_increment_x);
                x <= plot_view.x1; x += tick_increment_x)
            {
                Rect_f32 line_rect = {};
                {
                    line_rect.x0 = region.x0 + region_width * (x - plot_view.x0) / (plot_view.x1 - plot_view.x0);
                    line_rect.y0 = region.y0;
                    line_rect.x1 = line_rect.x0+1;
                    line_rect.y1 = region.y1;
                }
                
                draw_rect(plot->app, line_rect, 1.f, grid_line_color, 0);
                
                // NOTE(rjf): Draw number label.
                {
                    float nearest_y_tick = (plot_view.y1 + plot_view.y0) / 2;
                    nearest_y_tick -= fmodf(nearest_y_tick, tick_increment_y);
                    
                    String_Const_u8 str = push_stringf(scratch, "%.*f", tick_increment_y >= 1 ? 0 : 3, x);
                    draw_string(plot->app, plot->label_face, str,
                                V2(line_rect.x0,
                                   region.y0 + region_height -
                                   region_height * (nearest_y_tick - plot_view.y0) / (plot_view.y1 - plot_view.y0)),
                                grid_line_color);
                }
            }
        }
        
        // NOTE(rjf): Draw horizontal lines.
        {
            for(float y = plot_view.y0 - fmodf(plot_view.y0, tick_increment_y);
                y <= plot_view.y1; 
                y += tick_increment_y)
            {
                Rect_f32 line_rect = {};
                {
                    line_rect.x0 = region.x0;
                    line_rect.y0 = region.y0 + region_height - region_height * (y - plot_view.y0) / (plot_view.y1 - plot_view.y0);
                    line_rect.x1 = region.x1;
                    line_rect.y1 = line_rect.y0+1;
                }
                
                draw_rect(plot->app, line_rect, 1.f, grid_line_color, 0);
                
                // NOTE(rjf): Draw number label.
                {
                    float nearest_x_tick = (plot_view.x1 + plot_view.x0) / 2;
                    nearest_x_tick -= fmodf(nearest_x_tick, tick_increment_x);
                    
                    String_Const_u8 str = push_stringf(scratch, "%.*f", tick_increment_y >= 1 ? 0 : 3, y);
                    draw_string(plot->app, plot->label_face, str,
                                V2(region.x0 + region_width * (nearest_x_tick - plot_view.x0) / (plot_view.x1 - plot_view.x0),
                                   line_rect.y0),
                                grid_line_color);
                }
            }
        }
    }
}

function void
Plot2DPoints(Plot2DInfo *plot, i32 style_flags,
             float *x_data, float *y_data, int data_count)
{
    Rect_f32 region = plot->region;
    
    f32 region_width = region.x1 - region.x0;
    f32 region_height = region.y1 - region.y0;
    
    // NOTE(rjf): Draw function samples.
    {
        Color_Array plot_cycle = finalize_color_array(fleury_color_plot_cycle);
        ARGB_Color function_color =
            plot_cycle.vals[(plot->color_cycle_position++) % plot_cycle.count];
        
        for(int i = 0; i < data_count; ++i)
        {
            f32 point_x = region_width * (x_data[i] - plot->plot_view.x0) / (plot->plot_view.x1 - plot->plot_view.x0);
            f32 point_y = region_height - region_height * (y_data[i] - plot->plot_view.y0) / (plot->plot_view.y1 - plot->plot_view.y0);
            
            if(style_flags & Plot2DStyleFlags_Lines)
            {
                Rect_f32 point =
                {
                    region.x0 + point_x - 1,
                    region.y0 + point_y - 1,
                    region.x0 + point_x + 1,
                    region.y0 + point_y + 1,
                };
                
                // TODO(rjf): Real line drawing.
                draw_rect(plot->app, point, 2.f, function_color, 0);
            }
            
            if(style_flags & Plot2DStyleFlags_Points)
            {
                Rect_f32 point =
                {
                    region.x0 + point_x - 4,
                    region.y0 + point_y - 4,
                    region.x0 + point_x + 4,
                    region.y0 + point_y + 4,
                };
                
                draw_rect(plot->app, point, 6.f, function_color, 0);
            }
        }
    }
}

function void
Plot2DHistogram(Plot2DInfo *plot, float *data, int data_count)
{
    if(plot->bins && plot->num_bins > 0)
    {
        for(int i = 0; i < data_count; ++i)
        {
            float t = (data[i] - plot->bin_data_range.min) / (plot->bin_data_range.max - plot->bin_data_range.min);
            int bin_to_go_in = (int)(cast(f32)plot->num_bins * t);
            if(bin_to_go_in >= 0 && bin_to_go_in < plot->num_bins)
            {
                ++plot->bins[bin_to_go_in + plot->current_bin_group*plot->num_bins];
            }
        }
        ++plot->current_bin_group;
    }
}

function void
Plot2DEnd(Plot2DInfo *plot)
{
    if(plot->mode == Plot2DMode_Histogram)
    {
        f32 bin_screen_width = ((plot->region.x1-plot->region.x0) / cast(f32)plot->num_bins) / cast(f32)plot->bin_group_count;
        
        for(int bin_group = 0; bin_group < plot->bin_group_count; ++bin_group)
        {
            int total_data = 0;
            
            for(int i = 0; i < plot->num_bins; ++i)
            {
                total_data += plot->bins[i + bin_group*plot->num_bins];
            }
            
            Color_Array plot_cycle = finalize_color_array(fleury_color_plot_cycle);
            ARGB_Color color =
                plot_cycle.vals[bin_group % plot_cycle.count];
            
            for(int i = 0; i < plot->num_bins; ++i)
            {
                int bin_index = i + bin_group*plot->num_bins;
                Rect_f32 bin_rect = {};
                bin_rect.x0 = plot->region.x0 + ((float)i/cast(f32)plot->num_bins)*(plot->region.x1-plot->region.x0) + bin_screen_width*cast(f32)bin_group;
                bin_rect.x1 = bin_rect.x0 + bin_screen_width;
                bin_rect.y0 = bin_rect.y1 = plot->region.y1;
                bin_rect.y0 -= ((float)plot->bins[bin_index] / cast(f32)total_data) * (plot->region.y1 - plot->region.y0);
                draw_rect(plot->app, bin_rect, 4.f, color, 0);
            }
        }
    }
  
    draw_rect_outline(plot->app, plot->region, 4.f, 3.f, fcolor_resolve(defcolor_margin_active), 0);
  
    draw_set_clip(plot->app, plot->last_clip);
}
